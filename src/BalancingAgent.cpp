/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cfloat>
#include <cmath>
#include <algorithm>

// DEBUG BEGIN
#include <unistd.h>
#include <iostream>
#include <limits.h>
//END FOR DEBUG


#include "BalancingAgent.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "CircularBuffer.hpp"

#include "Helper.hpp"
#include "config.h"

// DEBUG BEGIN
static char g_hostname[NAME_MAX] = {};
// DEBUG END


namespace geopm
{
    BalancingAgent::BalancingAgent()
        : m_platform_io(platform_io())
        , m_platform_topo(platform_topo())
        , m_convergence_guard_band(0.5)
        , m_level(-1)
        , m_num_leaf(-1)
        , m_is_converged(false)
        , m_is_sample_stable(false)
        , m_updates_per_sample(5)
        , m_samples_per_control(10)
        , m_lower_bound(m_platform_io.read_signal("POWER_PACKAGE_MIN", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        , m_upper_bound(m_platform_io.read_signal("POWER_PACKAGE_MAX", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        , m_pio_idx(M_PLAT_NUM_SIGNAL)
        , m_last_power_budget_in(NAN)
        , m_last_power_budget_out(NAN)
        , m_epoch_runtime_buf(geopm::make_unique<CircularBuffer<double> >(8)) // Magic number...
        , m_epoch_power_buf(geopm::make_unique<CircularBuffer<double> >(8)) // Magic number...
        , m_sample(M_PLAT_NUM_SIGNAL)
        , m_last_energy_status(0.0)
        , m_sample_count(0)
        , m_ascend_count(0)
        , m_ascend_period(10)
        , m_is_updated(false)
        , m_convergence_target(0.01)
        , m_num_out_of_range(0)
        , m_min_num_converged(7)
        , m_num_converged(0)
        , m_magic(3.0) // m_slope_modifier from BalancingDecider
        , m_num_sample(7) // Number of samples required to be in m_epoch_runtime_buf before balancing begins
        , m_last_epoch_runtime(0.0)
    {
// BEGIN DEBUG
if (g_hostname[0] == '\0') {
gethostname(g_hostname, NAME_MAX);
}
// END DEBUG
    }

    BalancingAgent::~BalancingAgent()
    {

    }

    void BalancingAgent::init(int level, int num_leaf)
    {
        m_level = level;
        m_num_leaf = num_leaf;
        if (level == 0) {
            init_platform_io(); // Only do this at the leaf level.
        }
    }

    void BalancingAgent::init_platform_io(void)
    {
        // Setup signals
        m_pio_idx[M_PLAT_SIGNAL_EPOCH_RUNTIME] = m_platform_io.push_signal("EPOCH_RUNTIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SIGNAL_PKG_POWER] = m_platform_io.push_signal("POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SIGNAL_DRAM_POWER] = m_platform_io.push_signal("POWER_DRAM", IPlatformTopo::M_DOMAIN_BOARD, 0);

        // Setup controls
        int pkg_pwr_domain_type = m_platform_io.control_domain_type("POWER_PACKAGE");
        if (pkg_pwr_domain_type == IPlatformTopo::M_DOMAIN_INVALID) {
            throw Exception("BalancingAgent::" + std::string(__func__) + "(): Platform does not support package power control",
                            GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
        }

        int num_pkg_pwr_domains = m_platform_topo.num_domain(pkg_pwr_domain_type);
        for(int i = 0; i < num_pkg_pwr_domains; ++i) {
            int control_idx = m_platform_io.push_control("POWER_PACKAGE", pkg_pwr_domain_type, i);
            if (control_idx < 0) {
                throw Exception("BalancingAgent::" + std::string(__func__) + "(): Failed to enable package power control"
                                " in the platform.",
                                GEOPM_ERROR_DECIDER_UNSUPPORTED, __FILE__, __LINE__);
            }
            m_control_idx.push_back(control_idx);
        }

        // Setup sample aggregation for data going up the tree
        m_agg_func.push_back(IPlatformIO::agg_max);     // EPOCH_RUNTIME
        m_agg_func.push_back(IPlatformIO::agg_average); // POWER
        m_agg_func.push_back(IPlatformIO::agg_and);     // IS_CONVERGED
    }

    bool BalancingAgent::descend(const std::vector<double> &in_policy, std::vector<std::vector<double> >&out_policy)
    {
        if (in_policy.size() > 1) {
            throw Exception("BalancingAgent::" + std::string(__func__) + "(): only one power budget was expected.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }

        int num_children = out_policy.size();
        double power_budget_in = in_policy[M_POLICY_POWER];
        m_is_updated = (m_level == 0);

        if (m_level == 0 || m_last_power_budget_in != power_budget_in) {
            if (isnan(m_last_power_budget_in)) {
                // First time down the tree, send the same budget to all children.
                std::fill(out_policy.begin(), out_policy.end(), std::vector<double>(1, power_budget_in));
                m_is_updated = true;
            }
            else {
                // Not the first descent
                double stddev_child_runtime = runtime_stddev(m_last_sample0);
                if (m_is_sample_stable && stddev_child_runtime > m_convergence_target) {
                    // All children have reported that they have
                    // converged, but the relative runtimes between the
                    // children is not small enough.

                    // Reset counter of number of samples that have
                    m_num_converged = 0;
                    std::vector<double> last_runtime0(num_children);
                    std::vector<double> last_runtime1(num_children);
                    std::vector<double> last_budget0(num_children);
                    std::vector<double> last_budget1(num_children);
                    for (int child_idx = 0; child_idx < num_children; ++child_idx) {
                        last_runtime0[child_idx] = m_last_sample0[child_idx][M_SAMPLE_EPOCH_RUNTIME];
                        last_runtime1[child_idx] = m_last_sample1[child_idx][M_SAMPLE_EPOCH_RUNTIME];
                        last_budget0[child_idx] = m_last_child_policy0[child_idx][M_POLICY_POWER];
                        last_budget1[child_idx] = m_last_child_policy1[child_idx][M_POLICY_POWER];
                    }
                    std::vector<double> budget(num_children);
                    if (std::any_of(last_budget1.begin(), last_budget1.end(), isnan)) {
                        double median_runtime = IPlatformIO::agg_median(last_runtime0);
                        for (int child_idx = 0; child_idx != num_children; ++child_idx) {
                            if (last_runtime0[child_idx] < median_runtime) {
                                budget[child_idx] = last_budget0[child_idx] - 10;
                            }
                            else if (last_runtime0[child_idx] > median_runtime) {
                                budget[child_idx] = last_budget0[child_idx] + 10;
                            }
                            else {
                                budget[child_idx] = last_budget0[child_idx];
                            }
                        }
                    }
                    else {
                        budget = split_budget(power_budget_in, m_lower_bound,
                                              last_budget0, last_budget1,
                                              last_runtime0, last_runtime1);
                    }
                    for (int idx = 0; idx != num_children; ++idx) {
                         out_policy[idx][0] = budget[idx];
                    }
                    m_is_updated = true;
                }
                // We are out of bounds increment out of range counter
                if (m_is_converged && (stddev_child_runtime > m_convergence_target)) {
                    ++m_num_out_of_range;
                    if (m_num_out_of_range >= m_min_num_converged) {
                        m_is_converged  = false;
                        m_num_converged = 0;
                        m_num_out_of_range = 0;
                    }
                }
                // We are within bounds.
                else if (!m_is_converged && (stddev_child_runtime < m_convergence_target)) {
                    m_num_out_of_range = 0;
                    ++m_num_converged;
                    if (m_num_converged >= m_min_num_converged) {
                        m_is_converged = true;
                    }
                }
            }
        }
        if (m_last_child_policy0 != out_policy) {
            m_last_power_budget_in = power_budget_in;
            m_last_child_policy1 = m_last_child_policy0;
            m_last_child_policy0 = out_policy;
            m_epoch_runtime_buf->clear();
            m_epoch_power_buf->clear();
        }
        return m_is_updated;
    }

    bool BalancingAgent::ascend(const std::vector<std::vector<double> > &in_sample, std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("BalancingAgent::" + std::string(__func__) + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        /// @todo put in init() method after we pass fan_out to that method.
        if (m_last_sample0.size() == 0) {
            m_last_sample0.resize(in_sample.size(), std::vector<double>(M_NUM_SAMPLE, NAN));
            m_last_sample1.resize(in_sample.size(), std::vector<double>(M_NUM_SAMPLE, NAN));
            m_last_child_policy0.resize(in_sample.size(), std::vector<double>(M_NUM_POLICY, NAN));
            m_last_child_policy1.resize(in_sample.size(), std::vector<double>(M_NUM_POLICY, NAN));
        }
        bool result = false;
        m_is_sample_stable = std::all_of(in_sample.begin(), in_sample.end(),
            [](const std::vector<double> &val)
            {
                return val[M_SAMPLE_IS_CONVERGED];
            });

        if (m_is_sample_stable && m_ascend_count == 0) {
            result = true;
            std::vector<double> child_sample(in_sample.size());
            for (size_t sig_idx = 0; sig_idx < out_sample.size(); ++sig_idx) {
                for (size_t child_idx = 0; child_idx < in_sample.size(); ++child_idx) {
                    child_sample[child_idx] = in_sample[child_idx][sig_idx];
                }
                out_sample[sig_idx] = m_agg_func[sig_idx](child_sample);
            }
        }
        ++m_ascend_count;
        if (m_ascend_count == m_ascend_period) {
            m_ascend_count = 0;
        }

        if (out_sample[M_SAMPLE_EPOCH_RUNTIME] == 0.0 ||
            isnan(out_sample[M_SAMPLE_EPOCH_RUNTIME]) ||
            out_sample[M_SAMPLE_EPOCH_RUNTIME] == m_last_epoch_runtime) {
            result = false;
        }
        else {
            m_last_epoch_runtime = out_sample[M_SAMPLE_EPOCH_RUNTIME];
        }
        m_last_sample1 = m_last_sample0;
        m_last_sample0 = in_sample;
        return result;
    }

    bool BalancingAgent::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("BalancingAgent::" + std::string(__func__) + "(): one control was expected.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (isnan(in_policy[M_POLICY_POWER])) {
            throw Exception("BalancingAgent::" + std::string(__func__) + "(): policy is NAN.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif

        bool result = false;
        double dram_power = m_sample[M_PLAT_SIGNAL_DRAM_POWER];
        // Check that we have enough samples (two) to measure DRAM power
        if (isnan(dram_power)) {
            dram_power = 0.0;
        }
        if (m_last_power_budget_out != in_policy[M_POLICY_POWER] || m_sample_count == 0) {
            double num_pkg = m_control_idx.size();
            double target_pkg_power = (in_policy[M_POLICY_POWER] - dram_power) / num_pkg;
// DEBUG BEGIN
if (in_policy[M_POLICY_POWER] != m_last_power_budget_out) {
std::cerr << g_hostname << ": (budget, target)=(" << in_policy[M_POLICY_POWER] << ", " << target_pkg_power << ")\n";
}
            for (auto ctl_idx : m_control_idx) {
                m_platform_io.adjust(ctl_idx, target_pkg_power);
            }
            m_last_power_budget_out = in_policy[M_POLICY_POWER];
            result = true;
        }
        m_sample_count++;
        if (m_sample_count == m_samples_per_control) {
            m_sample_count = 0;
        }
        return result;
    }

    bool BalancingAgent::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("BalancingAgent::" + std::string(__func__)  + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        for (int sample_idx = 0; sample_idx < M_PLAT_NUM_SIGNAL; ++sample_idx) {
            m_sample[sample_idx] = m_platform_io.sample(m_pio_idx[sample_idx]);
        }

        if (!std::any_of(m_sample.begin(), m_sample.end(),
            [](double val)
            {
                return isnan(val) || val == 0.0;
            })) {

            m_epoch_runtime_buf->insert(m_sample[M_SAMPLE_EPOCH_RUNTIME]);
            // Sum of all PKG and DRAM power.
            m_epoch_power_buf->insert(m_sample[M_PLAT_SIGNAL_PKG_POWER] +
                                      m_sample[M_PLAT_SIGNAL_DRAM_POWER]);

            if (m_epoch_runtime_buf->size() > m_num_sample) {
                out_sample[M_SAMPLE_EPOCH_RUNTIME] = IPlatformIO::agg_median(m_epoch_runtime_buf->make_vector());
                out_sample[M_SAMPLE_POWER] = IPlatformIO::agg_median(m_epoch_power_buf->make_vector());
                out_sample[M_SAMPLE_IS_CONVERGED] = true;
                result = true;
            }
        }

        return result;
    }

    void BalancingAgent::wait()
    {
        // Wait for updates to the energy status register
        double curr_energy_status = 0;

        for (int i = 0; i < m_updates_per_sample; ++i) {
            do  {
                curr_energy_status = m_platform_io.read_signal("ENERGY_PACKAGE", IPlatformTopo::M_DOMAIN_PACKAGE, 0);
            }
            while (m_last_energy_status == curr_energy_status);
            m_last_energy_status = curr_energy_status;
        }
    }

    std::vector<double> BalancingAgent::split_budget_helper(double avg_power_budget,
                                                            double min_power_budget,
                                                            const std::vector<double> &last_budget0,
                                                            const std::vector<double> &last_budget1,
                                                            const std::vector<double> &last_runtime0,
                                                            const std::vector<double> &last_runtime1)
    {
        /// sum(result) = avg_power_budget * num_children
        /// time[i] = time[j] for all i and j

        /// m[i] = (last_runtime1[i] - last_runtime0[i]) / (last_budget1[i] - last_budget0[i])
        /// b[i] = last_budget0[i] - last_runtime0[i] / m[i]

        /// time = m[i] * result[i] + b[i]
        /// result[i] = (time - b[i]) / m[i]
        /// avg_power_budget * num_children = sum((time - b[i]) / m[i])
        ///                                 = time * sum(1/m[i]) - sum(b[i] / m[i])
        /// time = avg_power_budget * num_children / (sum(1/m[i]) - sum(b[i] / m[i]))
        /// result[i] = ((avg_power_budget * num_children / (sum(1/m[i]) - sum(b[i] / m[i]))) - b[i]) / m[i]

        int num_children = last_budget0.size();
        std::vector<double> result(num_children);
        std::vector<double> mm(num_children);
        std::vector<double> bb(num_children);
        double inv_m_sum = 0.0;
        double ratio_sum = 0.0;
        for (int child_idx = 0; child_idx != num_children; ++child_idx) {
            mm[child_idx] = (last_runtime1[child_idx] - last_runtime0[child_idx]) /
                     (last_budget1[child_idx] - last_budget0[child_idx]);
            bb[child_idx] = last_budget0[child_idx] - last_runtime0[child_idx] / mm[child_idx];
            inv_m_sum += 1.0 / mm[child_idx];
            ratio_sum += bb[child_idx] / mm[child_idx];
        }
        for (int child_idx = 0; child_idx != num_children; ++child_idx) {
            result[child_idx] = ((avg_power_budget * num_children /
                           (inv_m_sum - ratio_sum)) - bb[child_idx]) / mm[child_idx];
std::cerr << g_hostname << ": child_idx=" << child_idx << " budget_unclipped=" << result[child_idx];
            if (result[child_idx] < min_power_budget) {
                result[child_idx] = min_power_budget;
                double pool = avg_power_budget * (num_children - child_idx) - min_power_budget;
                if (child_idx != num_children - 1) {
                    avg_power_budget = pool / (num_children - child_idx - 1);
                }
            }
std::cerr << " budget_clipped=" << result[child_idx] << "\n";
        }
        return result;
    }

    std::vector<double> BalancingAgent::split_budget(double avg_power_budget,
                                                     double min_power_budget,
                                                     const std::vector<double> &last_budget0,
                                                     const std::vector<double> &last_budget1,
                                                     const std::vector<double> &last_runtime0,
                                                     const std::vector<double> &last_runtime1)
    {
#ifdef GEOPM_DEBUG
        /// @fixme check other vector lengths
        if (last_budget0.size() != last_runtime0.size()) {
            throw Exception("BalancingAgent::split_budget(): input vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        if (avg_power_budget < min_power_budget) {
            throw Exception("BalancingAgent::split_budget(): ave_power_budget less than min_power_budget.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int num_children = last_budget0.size();
        std::vector<std::pair<double, int> > indexed_sorted_last_runtime(num_children);
        for (int idx = 0; idx != num_children; ++idx) {
            indexed_sorted_last_runtime[idx] = std::make_pair(last_runtime0[idx], idx);
        }
        std::sort(indexed_sorted_last_runtime.begin(), indexed_sorted_last_runtime.end());
        std::vector<double> sorted_last_budget0(num_children);
        std::vector<double> sorted_last_budget1(num_children);
        std::vector<double> sorted_last_runtime0(num_children);
        std::vector<double> sorted_last_runtime1(num_children);
        for (int child_idx = 0; child_idx != num_children; ++child_idx) {
            int sort_idx = indexed_sorted_last_runtime[child_idx].second;
            sorted_last_budget0[child_idx] = last_budget0[sort_idx];
            sorted_last_budget1[child_idx] = last_budget1[sort_idx];
            // note last_runtime[sort_idx] == indexed_sorted_last_runtime[child_idx].first
            sorted_last_runtime0[child_idx] = last_runtime0[sort_idx];
            sorted_last_runtime1[child_idx] = last_runtime1[sort_idx];
        }
        std::vector<double> sorted_result = split_budget_helper(avg_power_budget, min_power_budget,
                                                                sorted_last_budget0, sorted_last_budget1,
                                                                sorted_last_runtime0, sorted_last_runtime1);
        std::vector<double> result(num_children);
        for (int child_idx = 0; child_idx != num_children; ++child_idx) {
            int sort_idx = indexed_sorted_last_runtime[child_idx].second;
            result[child_idx] = sorted_result[sort_idx];
        }
        return result;
    }

    double BalancingAgent::runtime_stddev(const std::vector<std::vector<double> > &last_sample)
    {
        double result = 0.0;
        double mean = 0.0;
        int num_children = last_sample.size();
        for (int child_idx = 0; child_idx < num_children; ++child_idx) {
            double child_runtime = last_sample[child_idx][M_SAMPLE_EPOCH_RUNTIME];
            mean += child_runtime;
            result += child_runtime * child_runtime;
        }
        mean /= num_children;
        result = sqrt(result / num_children - mean * mean) / mean;
        return result;
    }

    std::vector<std::pair<std::string, std::string> > BalancingAgent::report_header(void)
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > BalancingAgent::report_node(void)
    {
        return {};
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > BalancingAgent::report_region(void)
    {
        return {};
    }

    std::vector<std::string> BalancingAgent::trace_names(void) const
    {
        std::vector<std::string> trace_names;

        trace_names.push_back("epoch_runtime");
        trace_names.push_back("power_package");
        trace_names.push_back("power_dram");
        trace_names.push_back("is_converged");
        trace_names.push_back("power_budget");

        return trace_names;
    }

    void BalancingAgent::trace_values(std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        if (values.size() != M_TRACE_NUM_SAMPLE) { // Everything sampled from the platform plus convergence (and the power budget soon...)
            throw Exception("BalancingAgent::" + std::string(__func__) + "(): values vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        values[M_TRACE_SAMPLE_EPOCH_RUNTIME] = m_sample[M_PLAT_SIGNAL_EPOCH_RUNTIME];
        values[M_TRACE_SAMPLE_PKG_POWER] = m_sample[M_PLAT_SIGNAL_PKG_POWER];
        values[M_TRACE_SAMPLE_DRAM_POWER] = m_sample[M_PLAT_SIGNAL_DRAM_POWER];
        values[M_TRACE_SAMPLE_IS_CONVERGED] = m_is_converged;
        values[M_TRACE_SAMPLE_PWR_BUDGET] = m_last_power_budget_out;
    }

    std::string BalancingAgent::plugin_name(void)
    {
        return "balancer";
    }

    std::unique_ptr<IAgent> BalancingAgent::make_plugin(void)
    {
        return geopm::make_unique<BalancingAgent>();
    }

    std::vector<std::string> BalancingAgent::policy_names(void)
    {
        return {"POWER"};
    }

    std::vector<std::string> BalancingAgent::sample_names(void)
    {
        return {"EPOCH_RUNTIME", "POWER", "IS_CONVERGED"};
    }
}

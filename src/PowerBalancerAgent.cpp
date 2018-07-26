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
#include <iostream>

#include "PowerGovernor.hpp"
#include "PowerBalancerAgent.hpp"
#include "PowerBalancer.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "CircularBuffer.hpp"

#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    static std::shared_ptr<PowerBalancerAgent::IStep> make_step(size_t step);

    PowerBalancerAgent::IRole::IRole()
        : m_is_step_complete(true)
    {
        /// everyone starts at send down limit
        step(reset());
    }

    PowerBalancerAgent::IRole::~IRole() = default;

    size_t PowerBalancerAgent::IRole::reset()
    {
        m_step_count = M_STEP_SEND_DOWN_LIMIT;
        return m_step_count;
    }

    size_t PowerBalancerAgent::IRole::step_count() const
    {
        return m_step_count;
    }

    void PowerBalancerAgent::IRole::inc_step_count()
    {
        ++m_step_count;
        m_step = make_step(this->step());
    }

    bool PowerBalancerAgent::IRole::is_step_complete() const
    {
        return m_is_step_complete;
    }

    void PowerBalancerAgent::IRole::is_step_complete(bool is_complete)
    {
        m_is_step_complete = is_complete;
    }

    size_t PowerBalancerAgent::IRole::step(void) const
    {
        return m_step_count % M_NUM_STEP;
    }

    void PowerBalancerAgent::IRole::step(size_t step)
    {
        if (m_is_step_complete && m_step_count != step) {
            if (M_STEP_SEND_DOWN_LIMIT == step) {
                reset();
            }
            else if (m_step_count + 1 == step) {
                inc_step_count();
            }
            else {
                throw Exception("PowerBalancerAgent::IRole::" + std::string(__func__) + "(): step is out of sync with current step",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            is_step_complete(false);
        }
        else if (m_step_count + 1 == step) {
            inc_step_count();
            is_step_complete(false);
        }
        else if (m_step_count != step) {
            throw Exception("PowerBalancerAgent::IRole::" + std::string(__func__) + "(): step is out of sync with current step",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    PowerBalancerAgent::LeafRole::LeafRole(IPlatformIO &platform_io, IPlatformTopo &platform_topo,
                                           std::unique_ptr<IPowerGovernor> power_governor, std::unique_ptr<IPowerBalancer> power_balancer)
        : IRole()
        , m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_pio_idx(M_PLAT_NUM_SIGNAL)
        , m_power_governor(std::move(power_governor))
        , m_power_balancer(std::move(power_balancer))
        , m_last_epoch_count(0)
        , m_runtime(0.0)
        , m_power_slack(0.0)
    {
        if (nullptr == m_power_governor) {
            m_power_governor = geopm::make_unique<PowerGovernor>(m_platform_io, m_platform_topo);
        }
        if (nullptr == m_power_balancer) {
            m_power_balancer = geopm::make_unique<PowerBalancer>();
        }
        init_platform_io();
    }

    PowerBalancerAgent::LeafRole::~LeafRole() = default;

    void PowerBalancerAgent::LeafRole::init_platform_io(void)
    {
        m_power_governor->init_platform_io();
        // Setup signals
        m_pio_idx[M_PLAT_SIGNAL_EPOCH_RUNTIME] = m_platform_io.push_signal("EPOCH_RUNTIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
        m_pio_idx[M_PLAT_SIGNAL_EPOCH_COUNT] = m_platform_io.push_signal("EPOCH_COUNT", IPlatformTopo::M_DOMAIN_BOARD, 0);
    }

    bool PowerBalancerAgent::LeafRole::descend(const std::vector<double> &in_policy,
            std::vector<std::vector<double> >&out_policy)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::LeafRole::" + std::string(__func__) + "(): was called on non-tree agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
    }

    bool PowerBalancerAgent::LeafRole::ascend(const std::vector<std::vector<double> > &in_sample,
            std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::LeafRole::" + std::string(__func__) + "(): was called on non-tree agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
    }

    bool PowerBalancerAgent::LeafRole::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("PowerBalancerAgent::LeafRole::" + std::string(__func__) + "(): policy vector incorrectly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        if (in_policy[M_POLICY_POWER_CAP] != 0.0) {
            // New power cap from resource manager, reset
            // algorithm.
            reset();
            m_power_balancer->power_cap(in_policy[M_POLICY_POWER_CAP]);
            is_step_complete(true);
        }
        else {
            // Advance a step
            step(in_policy[M_POLICY_STEP_COUNT]);
            m_step->pre_adjust(*this, in_policy);
        }

        double actual_limit = 0.0;
        bool result = false;
        // Request the power limit from the balancer
        double request_limit = m_power_balancer->power_limit();
        if (!std::isnan(request_limit) && request_limit != 0.0) {
            result = m_power_governor->adjust_platform(request_limit, actual_limit);
            if (result && actual_limit != request_limit) {
                m_step->post_adjust(*this, in_policy[M_POLICY_POWER_CAP], actual_limit);
            }
        }
        return result;
    }

    bool PowerBalancerAgent::LeafRole::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent::LeafRole::" + std::string(__func__)  + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        int epoch_count = m_platform_io.sample(m_pio_idx[M_PLAT_SIGNAL_EPOCH_COUNT]);
        // If all of the ranks have observed a new epoch then update
        // the power_balancer.
        if (epoch_count != m_last_epoch_count &&
            !is_step_complete()) {
            double epoch_runtime = m_platform_io.sample(m_pio_idx[M_PLAT_SIGNAL_EPOCH_RUNTIME]);
            m_step->post_sample(*this, epoch_runtime);
            m_last_epoch_count = epoch_count;
        }
        m_power_governor->sample_platform();
        out_sample[M_SAMPLE_STEP_COUNT] = step_count();
        out_sample[M_SAMPLE_MAX_EPOCH_RUNTIME] = m_runtime;
        out_sample[M_SAMPLE_SUM_POWER_SLACK] = m_power_slack;
        return is_step_complete();
    }

    std::vector<std::string> PowerBalancerAgent::LeafRole::trace_names(void) const
    {
        return {"epoch_runtime",
                "power_limit"};
    }

    void PowerBalancerAgent::LeafRole::trace_values(std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        if (values.size() != M_TRACE_NUM_SAMPLE) { // Everything sampled from the platform plus convergence (and the power budget soon...)
            throw Exception("PowerBalancerAgent::LeafRole::" + std::string(__func__) + "(): values vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        values[M_TRACE_SAMPLE_EPOCH_RUNTIME] = m_power_balancer->runtime_sample();
        values[M_TRACE_SAMPLE_POWER_LIMIT] = m_power_balancer->power_limit();
    }

    PowerBalancerAgent::TreeRole::TreeRole(int num_children)
        : IRole()
        , M_AGG_FUNC({
              IPlatformIO::agg_min, // M_SAMPLE_STEP_COUNT
              IPlatformIO::agg_max, // M_SAMPLE_MAX_EPOCH_RUNTIME
              IPlatformIO::agg_sum, // M_SAMPLE_SUM_POWER_SLACK
          })
        , M_NUM_CHILDREN(num_children)
        , m_policy(M_NUM_POLICY, NAN)
    {
#ifdef GEOPM_DEBUG
        if (M_AGG_FUNC.size() != M_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): aggregation function vector is not the size of the policy vector",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
    }

    PowerBalancerAgent::TreeRole::~TreeRole() = default;

    bool PowerBalancerAgent::TreeRole::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): was called on non-leaf agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
    }

    bool PowerBalancerAgent::TreeRole::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): was called on non-leaf agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
    }

    bool PowerBalancerAgent::TreeRole::descend(const std::vector<double> &in_policy,
            std::vector<std::vector<double> >&out_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY ||
            out_policy.size() != (size_t)M_NUM_CHILDREN) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        step(m_policy[M_POLICY_STEP_COUNT]);
        for (auto &po : out_policy) {
            po = in_policy;
        }
        m_policy = in_policy;
        return true;
    }

    bool PowerBalancerAgent::TreeRole::ascend(const std::vector<std::vector<double> > &in_sample,
            std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (in_sample.size() != (size_t)M_NUM_CHILDREN ||
            out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): sample vectors not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        aggregate_sample(in_sample, M_AGG_FUNC, out_sample);
        if (!is_step_complete() && out_sample[M_SAMPLE_STEP_COUNT] == step_count()) {
            // Method returns true if all children have completed the step
            // for the first time.
            result = true;
            is_step_complete(true);
        }
        return result;
    }

    std::vector<std::string> PowerBalancerAgent::TreeRole::trace_names(void) const
    {
#ifdef GEOPM_DEBUG
        throw Exception("TreeRole::" + std::string(__func__) + "(): called on non-leaf agent.",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
    }

    void PowerBalancerAgent::TreeRole::trace_values(std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        throw Exception("TreeRole::" + std::string(__func__) + "(): called on non-leaf agent.",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
    }

    PowerBalancerAgent::RootRole::RootRole(int num_node, int num_children)
        : TreeRole(num_children)
        , M_NUM_NODE(num_node)
        , m_root_cap(NAN)
    {

    }

    PowerBalancerAgent::RootRole::~RootRole() = default;


    bool PowerBalancerAgent::RootRole::ascend(const std::vector<std::vector<double> > &in_sample,
            std::vector<double> &out_sample)
    {
        if (step_count() != m_policy[M_POLICY_STEP_COUNT]) {
            throw Exception("RootRole::" + std::string(__func__) + "(): sample passed does not match current step_count.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        bool result = TreeRole::ascend(in_sample, out_sample);
        result |=  m_step->update_policy(*this, out_sample);
        m_policy[M_POLICY_STEP_COUNT] = step_count() + 1;
        return result;
    }

    bool PowerBalancerAgent::RootRole::descend(const std::vector<double> &in_policy,
            std::vector<std::vector<double> >&out_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY ||
            out_policy.size() != (size_t)M_NUM_CHILDREN) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        if (in_policy[M_POLICY_POWER_CAP] != m_root_cap) {
            m_policy[M_POLICY_POWER_CAP] = in_policy[M_POLICY_POWER_CAP];
            m_policy[M_POLICY_STEP_COUNT] = reset();
            m_policy[M_POLICY_MAX_EPOCH_RUNTIME] = 0.0;
            m_policy[M_POLICY_POWER_SLACK] = 0.0;
            m_root_cap = in_policy[M_POLICY_POWER_CAP];
        }
        else {
            step(m_policy[M_POLICY_STEP_COUNT]);
        }
        for (auto &op : out_policy) {
            op = m_policy;
        }
        return true;
    }

    class SendDownLimitStep : public PowerBalancerAgent::IStep {
        public:
            SendDownLimitStep() = default;
            ~SendDownLimitStep() = default;
            bool update_policy(PowerBalancerAgent::RootRole &role, const std::vector<double> &sample)
            {
                role.m_policy[PowerBalancerAgent::M_POLICY_POWER_CAP] = 0.0;
                return true;
            }

            void pre_adjust(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy)
            {
                role.m_power_balancer->power_cap(role.m_power_balancer->power_limit() + in_policy[PowerBalancerAgent::M_POLICY_POWER_SLACK]);
                role.is_step_complete(true);
            }

            void post_adjust(PowerBalancerAgent::LeafRole &role, double policy_limit, double actual_limit)
            {
                if (policy_limit != 0.0) {
                    std::cerr << "Warning: <geopm> PowerBalancerAgent: per node power cap of "
                        << policy_limit << " Watts could not be maintained (request=" << actual_limit << ");" << std::endl;
                }
            }

            void post_sample(PowerBalancerAgent::LeafRole &role, double epoch_runtime)
            {
            }
    };

    class MeasureRuntimeStep : public PowerBalancerAgent::IStep {
        public:
            MeasureRuntimeStep() = default;
            ~MeasureRuntimeStep() = default;
            bool update_policy(PowerBalancerAgent::RootRole &role, const std::vector<double> &sample)
            {
                role.m_policy[PowerBalancerAgent::M_POLICY_MAX_EPOCH_RUNTIME] = sample[PowerBalancerAgent::M_SAMPLE_MAX_EPOCH_RUNTIME];
                return true;
            }

            void pre_adjust(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy)
            {
            }

            void post_adjust(PowerBalancerAgent::LeafRole &role, double policy_limit, double actual_limit)
            {
                if (policy_limit != 0.0) {
                    std::cerr << "Warning: <geopm> PowerBalancerAgent: per node power cap of "
                        << policy_limit << " Watts could not be maintained (request=" << actual_limit << ");" << std::endl;
                }
            }

            void post_sample(PowerBalancerAgent::LeafRole &role, double epoch_runtime)
            {
                role.m_runtime = role.m_power_balancer->runtime_sample();
                role.is_step_complete(role.m_power_balancer->is_runtime_stable(epoch_runtime));
            }
    };

    class ReduceLimitStep : public PowerBalancerAgent::IStep {
        public:
            ReduceLimitStep() = default;
            ~ReduceLimitStep() = default;
            bool update_policy(PowerBalancerAgent::RootRole &role, const std::vector<double> &sample)
            {
                role.m_policy[PowerBalancerAgent::M_POLICY_POWER_SLACK] = sample[PowerBalancerAgent::M_SAMPLE_SUM_POWER_SLACK] / role.M_NUM_NODE;
                return true;
            }

            void pre_adjust(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy)
            {
                role.m_power_balancer->target_runtime(in_policy[PowerBalancerAgent::M_POLICY_MAX_EPOCH_RUNTIME]);
            }

            void post_adjust(PowerBalancerAgent::LeafRole &role, double policy_limit, double actual_limit)
            {
                role.m_power_balancer->achieved_limit(actual_limit);
            }

            void post_sample(PowerBalancerAgent::LeafRole &role, double epoch_runtime)
            {
                role.m_power_slack = role.m_power_balancer->power_cap() - role.m_power_balancer->power_limit();
                role.is_step_complete(role.m_power_balancer->is_target_met(epoch_runtime));
            }
    };

    static std::shared_ptr<PowerBalancerAgent::IStep> make_step(size_t step)
    {
        std::shared_ptr<PowerBalancerAgent::IStep> ret_val;
        switch(step) {
            case PowerBalancerAgent::IRole::M_STEP_SEND_DOWN_LIMIT:
                ret_val = std::make_shared<SendDownLimitStep>();
                break;
            case PowerBalancerAgent::IRole::M_STEP_MEASURE_RUNTIME:
                ret_val = std::make_shared<MeasureRuntimeStep>();
                break;
            case PowerBalancerAgent::IRole::M_STEP_REDUCE_LIMIT:
                ret_val = std::make_shared<ReduceLimitStep>();
                break;
            default:
#ifdef GEOPM_DEBUG
                throw Exception("PowerBalancer::" + std::string(__func__) + "(): invalid step.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
        }
        return ret_val;
    }

    PowerBalancerAgent::PowerBalancerAgent()
        : PowerBalancerAgent(platform_io(), platform_topo(), nullptr, nullptr)
    {

    }

    PowerBalancerAgent::PowerBalancerAgent(IPlatformIO &platform_io, IPlatformTopo &platform_topo,
                                           std::unique_ptr<IPowerGovernor> power_governor, std::unique_ptr<IPowerBalancer> power_balancer)
        : m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_role(nullptr)
        , m_power_governor(std::move(power_governor))
        , m_power_balancer(std::move(power_balancer))
        , m_last_wait{{0,0}}
        , M_WAIT_SEC(0.005)
    {
        geopm_time(&m_last_wait);
    }

    PowerBalancerAgent::~PowerBalancerAgent() = default;

    void PowerBalancerAgent::init(int level, const std::vector<int> &fan_in, bool is_root)
    {
        int num_children;
        int num_node;
        bool is_tree_root;
        is_tree_root = (level == (int)fan_in.size());
        num_children = fan_in[level - 1];
        for (auto fi : fan_in) {
            num_node *= fi;
        }
        if (is_tree_root) {
            m_role = std::make_shared<RootRole>(num_node, num_children);
        }
        else if (level == 0) {
            m_role = std::make_shared<LeafRole>(m_platform_io, m_platform_topo, std::move(m_power_governor), std::move(m_power_balancer));
        }
        else {
            m_role = std::make_shared<TreeRole>(num_children);
        }
    }

    bool PowerBalancerAgent::descend(const std::vector<double> &policy_in, std::vector<std::vector<double> > &policy_out)
    {
        return m_role->descend(policy_in, policy_out);
    }

    bool PowerBalancerAgent::ascend(const std::vector<std::vector<double> > &sample_in, std::vector<double> &sample_out)
    {
        return m_role->ascend(sample_in, sample_out);
    }

    bool PowerBalancerAgent::adjust_platform(const std::vector<double> &in_policy)
    {
        return m_role->adjust_platform(in_policy);
    }

    bool PowerBalancerAgent::sample_platform(std::vector<double> &out_sample)
    {
        return m_role->sample_platform(out_sample);
    }


    void PowerBalancerAgent::wait(void) {
        geopm_time_s current_time;
        do {
            geopm_time(&current_time);
        }
        while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
        geopm_time(&m_last_wait);
    }

    std::vector<std::pair<std::string, std::string> > PowerBalancerAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > PowerBalancerAgent::report_node(void) const
    {
        return {};
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > PowerBalancerAgent::report_region(void) const
    {
        return {};
    }

    std::vector<std::string> PowerBalancerAgent::trace_names(void) const
    {
        return m_role->trace_names();
    }

    void PowerBalancerAgent::trace_values(std::vector<double> &values)
    {
        m_role->trace_values(values);
    }

    std::string PowerBalancerAgent::plugin_name(void)
    {
        return "power_balancer";
    }

    std::unique_ptr<Agent> PowerBalancerAgent::make_plugin(void)
    {
        return geopm::make_unique<PowerBalancerAgent>();
    }

    std::vector<std::string> PowerBalancerAgent::policy_names(void)
    {
        return {"POWER_CAP",
                "STEP_COUNT",
                "MAX_EPOCH_RUNTIME",
                "POWER_SLACK"};
    }

    std::vector<std::string> PowerBalancerAgent::sample_names(void)
    {
        return {"STEP_COUNT",
                "MAX_EPOCH_RUNTIME",
                "SUM_POWER_SLACK"};
    }
}

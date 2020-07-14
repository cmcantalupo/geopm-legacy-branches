/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "PowerBalancerAgent.hpp"

#include <cfloat>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "PowerBalancer.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "Agg.hpp"
#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    PowerBalancerAgent::Role::Role()
        : M_STEP_IMP({
                std::make_shared<SendDownLimitStep>(),
                std::make_shared<MeasureRuntimeStep>(),
                std::make_shared<ReduceLimitStep>()
            })
        , m_policy(M_NUM_POLICY, NAN)
        , m_step_count(-1)
        , m_is_step_complete(false)
    {
#ifdef GEOPM_DEBUG
        if (M_STEP_IMP.size() != M_NUM_STEP) {
            throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): invalid M_STEP_IMP size",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
    }

    bool PowerBalancerAgent::Role::descend(const std::vector<double> &in_policy,
            std::vector<std::vector<double> >&out_policy)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): was called on non-tree agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
        return false;
    }

    bool PowerBalancerAgent::Role::ascend(const std::vector<std::vector<double> > &in_sample,
            std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): was called on non-tree agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
        return false;
    }

    bool PowerBalancerAgent::Role::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): was called on non-leaf agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
        return false;
    }

    bool PowerBalancerAgent::Role::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): was called on non-leaf agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
        return false;
    }

    void PowerBalancerAgent::Role::trace_values(std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        throw Exception("PowerBalancerAgent::Role::" + std::string(__func__) + "(): was called on non-leaf agent",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
#endif
    }

    int PowerBalancerAgent::Role::step(size_t step_count) const
    {
        return (step_count % M_NUM_STEP);
    }

    int PowerBalancerAgent::Role::step(void) const
    {
        return step(m_step_count);
    }

    const PowerBalancerAgent::Step& PowerBalancerAgent::Role::step_imp()
    {
        return *M_STEP_IMP[step()];
    }

    /// @todo There must be one power_balancer class per socket
    PowerBalancerAgent::LeafRole::LeafRole(PlatformIO &platform_io,
                                           const PlatformTopo &platform_topo,
                                           std::vector<std::shared_ptr<PowerBalancer> > power_balancer)
        : Role()
        , m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_num_domain(m_platform_topo.num_domain(GEOPM_DOMAIN_PACKAGE))
        , m_pio_idx(m_num_domain, std::vector<int>(M_PLAT_NUM_SIGNAL, -1))
        , m_power_balancer(power_balancer)
        , M_STABILITY_FACTOR(3.0)
        , m_package(m_num_domain, m_package_s {0, 0.0, NAN, 0.0, 0.0, false})
    {
        if (m_power_balancer.empty()) {
            double time_window = m_platform_io.read_signal("POWER_PACKAGE_TIME_WINDOW",
                                                           GEOPM_DOMAIN_BOARD, 0);
            double ctl_latency = M_STABILITY_FACTOR * time_window;
            for (int pkg_idx = 0; pkg_idx != m_num_domain; ++pkg_idx) {
                m_power_balancer.push_back(PowerBalancer::make_unique(ctl_latency));
            }
        }
        init_platform_io();
        m_is_step_complete = true;
    }

    PowerBalancerAgent::LeafRole::~LeafRole() = default;

    void PowerBalancerAgent::LeafRole::init_platform_io(void)
    {
        // Setup signals
        for (int pkg_idx = 0; pkg_idx != m_num_domain; ++pkg_idx) {
            m_pio_idx[pkg_idx][M_PLAT_SIGNAL_EPOCH_RUNTIME] =
                m_platform_io.push_signal("EPOCH_RUNTIME",
                                          GEOPM_DOMAIN_PACKAGE, pkg_idx);
            m_pio_idx[pkg_idx][M_PLAT_SIGNAL_EPOCH_COUNT] =
                m_platform_io.push_signal("EPOCH_COUNT",
                                          GEOPM_DOMAIN_PACKAGE, pkg_idx);
            m_pio_idx[pkg_idx][M_PLAT_SIGNAL_EPOCH_RUNTIME_NETWORK] =
                m_platform_io.push_signal("EPOCH_RUNTIME_NETWORK",
                                          GEOPM_DOMAIN_PACKAGE, pkg_idx);
            m_pio_idx[pkg_idx][M_PLAT_SIGNAL_EPOCH_RUNTIME_IGNORE] =
                m_platform_io.push_signal("EPOCH_RUNTIME_IGNORE",
                                          GEOPM_DOMAIN_PACKAGE, pkg_idx);
        }
    }

    bool PowerBalancerAgent::LeafRole::adjust_platform(const std::vector<double> &in_policy)
    {
        m_policy = in_policy;
        if (in_policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] != 0.0) {
            // New power cap from resource manager, reset algorithm.
            m_step_count = M_STEP_SEND_DOWN_LIMIT;
            double pkg_limit = in_policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] / m_num_domain;
            for (auto &balancer : m_power_balancer) {
                balancer->power_cap(pkg_limit);
            }
            m_is_step_complete = true;
        }
        else if (in_policy[M_POLICY_STEP_COUNT] != m_step_count) {
            // Advance a step
            ++m_step_count;
            m_is_step_complete = false;
            if (m_step_count != in_policy[M_POLICY_STEP_COUNT]) {
                throw Exception("PowerBalancerAgent::adjust_platform(): The policy step is out of sync "
                                "with the agent step or first policy received had a zero power cap.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            step_imp().enter_step(*this, in_policy);
        }

        bool result = false;
        for (int pkg_idx = 0; pkg_idx != m_num_domain; ++pkg_idx) {
            auto &balancer = m_power_balancer[pkg_idx];
            // Request the power limit from the balancer
            double request_limit = balancer->power_limit();
            if (!std::isnan(request_limit) && request_limit != 0.0) {
                /// @todo Adjust package power limit controls with platform_io
            }

            if (request_limit < m_package[pkg_idx].actual_limit) {
                m_package[pkg_idx].is_out_of_bounds = true;
            }
            if (result) {
                balancer->power_limit_adjusted(m_package[pkg_idx].actual_limit);
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
        step_imp().sample_platform(*this);
        out_sample[M_SAMPLE_STEP_COUNT] = m_step_count;
        double runtime = 0.0;
        double power_slack = 0.0;
        double power_headroom = 0.0;
        for (auto &pkg : m_package) {
            runtime = runtime > pkg.runtime ?
                      runtime : pkg.runtime;
            power_slack += pkg.power_slack;
            power_headroom += pkg.power_headroom;
        }
        out_sample[M_SAMPLE_MAX_EPOCH_RUNTIME] = runtime;
        out_sample[M_SAMPLE_SUM_POWER_SLACK] = power_slack;
        out_sample[M_SAMPLE_MIN_POWER_HEADROOM] = power_headroom;
        return m_is_step_complete;
    }

    void PowerBalancerAgent::LeafRole::trace_values(std::vector<double> &values)
    {
#ifdef GEOPM_DEBUG
        if (values.size() != M_TRACE_NUM_SAMPLE) { // Everything sampled from the platform, latest policy values, and actual power limit
            throw Exception("PowerBalancerAgent::LeafRole::" + std::string(__func__) + "(): values vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        values[M_TRACE_SAMPLE_POLICY_POWER_PACKAGE_LIMIT_TOTAL] =
            m_policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL];
        values[M_TRACE_SAMPLE_POLICY_STEP_COUNT] =
            m_policy[M_POLICY_STEP_COUNT];
        values[M_TRACE_SAMPLE_POLICY_MAX_EPOCH_RUNTIME] =
            m_policy[M_POLICY_MAX_EPOCH_RUNTIME];
        values[M_TRACE_SAMPLE_POLICY_POWER_SLACK] =
            m_policy[M_POLICY_POWER_SLACK];
        double actual_limit = 0.0;
        for (auto &pkg : m_package) {
            actual_limit += pkg.actual_limit;
        }
        values[M_TRACE_SAMPLE_ENFORCED_POWER_LIMIT] =
            actual_limit;
    }

    PowerBalancerAgent::TreeRole::TreeRole(int level, const std::vector<int> &fan_in)
        : Role()
        , M_AGG_FUNC({
              Agg::min, // M_SAMPLE_STEP_COUNT
              Agg::max, // M_SAMPLE_MAX_EPOCH_RUNTIME
              Agg::sum, // M_SAMPLE_SUM_POWER_SLACK
              Agg::min, // M_SAMPLE_MIN_POWER_HEADROOM
          })
        , M_NUM_CHILDREN(fan_in[level - 1])
    {
#ifdef GEOPM_DEBUG
        if (M_AGG_FUNC.size() != M_NUM_SAMPLE) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): aggregation function vector is not the size of the policy vector",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_is_step_complete = true;
    }

    PowerBalancerAgent::TreeRole::~TreeRole() = default;

    bool PowerBalancerAgent::TreeRole::descend(const std::vector<double> &in_policy,
                                               std::vector<std::vector<double> >&out_policy)
    {
#ifdef GEOPM_DEBUG
        if (out_policy.size() != (size_t)M_NUM_CHILDREN) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        if (m_is_step_complete &&
            in_policy[M_POLICY_STEP_COUNT] != m_step_count) {
            if (in_policy[M_POLICY_STEP_COUNT] == M_STEP_SEND_DOWN_LIMIT) {
                m_step_count = M_STEP_SEND_DOWN_LIMIT;
            }
            else if (in_policy[M_POLICY_STEP_COUNT] == m_step_count + 1) {
                ++m_step_count;
            }
            else {
                throw Exception("PowerBalancerAgent::descend(): policy is out of sync with agent step.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_is_step_complete = false;
            // Copy the input policy directly into each child's
            // policy.
            for (auto &po : out_policy) {
                po = in_policy;
            }
            m_policy = in_policy;
            result = true;
        }
        return result;
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
        Agent::aggregate_sample(in_sample, M_AGG_FUNC, out_sample);
        if (!m_is_step_complete && out_sample[M_SAMPLE_STEP_COUNT] == m_step_count) {
            // Method returns true if all children have completed the step
            // for the first time.
            result = true;
            m_is_step_complete = true;
            if (out_sample[M_SAMPLE_STEP_COUNT] != m_step_count) {
                throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) +
                                "(): sample received has true for step complete field, " +
                                "but the step_count does not match the agent's current step_count.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        return result;
    }

    static int calc_num_node(const std::vector<int> &fan_in)
    {
        int num_node = 1;
        for (auto fi : fan_in) {
            num_node *= fi;
        }
        return num_node;
    }

    PowerBalancerAgent::RootRole::RootRole(int level, const std::vector<int> &fan_in,
                                           double min_power, double max_power)
        : TreeRole(level, fan_in)
        , M_NUM_NODE(calc_num_node(fan_in))
        , m_root_cap(NAN)
        , M_MIN_PKG_POWER_SETTING(min_power)
        , M_MAX_PKG_POWER_SETTING(max_power)
    {
        m_step_count = M_STEP_SEND_DOWN_LIMIT;
        m_is_step_complete = false;
    }

    PowerBalancerAgent::RootRole::~RootRole() = default;

    bool PowerBalancerAgent::RootRole::ascend(const std::vector<std::vector<double> > &in_sample,
            std::vector<double> &out_sample)
    {
        bool result = TreeRole::ascend(in_sample, out_sample);
        if (result) {
            if (m_step_count != m_policy[M_POLICY_STEP_COUNT]) {
                throw Exception("PowerBalancerAgent::RootRole::" + std::string(__func__) +
                                "(): sample passed does not match current step_count.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            step_imp().update_policy(*this, out_sample);
            m_policy[M_POLICY_STEP_COUNT] = m_step_count + 1;
        }
        return result;
    }

    bool PowerBalancerAgent::RootRole::descend(const std::vector<double> &in_policy,
            std::vector<std::vector<double> >&out_policy)
    {
#ifdef GEOPM_DEBUG
        if (out_policy.size() != (size_t)M_NUM_CHILDREN) {
            throw Exception("PowerBalancerAgent::TreeRole::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        if (in_policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] != m_root_cap) {
            m_step_count = M_STEP_SEND_DOWN_LIMIT;
            m_policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] = in_policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL];
            m_policy[M_POLICY_STEP_COUNT] = M_STEP_SEND_DOWN_LIMIT;
            m_policy[M_POLICY_MAX_EPOCH_RUNTIME] = 0.0;
            m_policy[M_POLICY_POWER_SLACK] = 0.0;
            m_root_cap = in_policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL];
            if (m_root_cap > M_MAX_PKG_POWER_SETTING ||
                m_root_cap < M_MIN_PKG_POWER_SETTING) {
                throw Exception("PowerBalancerAgent::descend(): invalid power budget: " + std::to_string(m_root_cap),
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = true;
        }
        else if (m_step_count + 1 == m_policy[M_POLICY_STEP_COUNT]) {
            ++m_step_count;
            m_is_step_complete = false;
            result = true;
        }
        else if (m_step_count != m_policy[M_POLICY_STEP_COUNT]) {
            throw Exception("PowerBalancerAgent::descend(): updated policy is out of sync with current step",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (result) {
            for (auto &po : out_policy) {
                po = m_policy;
            }
        }
        return result;
    }

    void PowerBalancerAgent::SendDownLimitStep::update_policy(PowerBalancerAgent::RootRole &role, const std::vector<double> &sample) const
    {
        role.m_policy[PowerBalancerAgent::M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] = 0.0;
    }

    void PowerBalancerAgent::SendDownLimitStep::enter_step(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const
    {
        double slack_power = in_policy[PowerBalancerAgent::M_POLICY_POWER_SLACK] /
                             role.m_num_domain;
        for (auto &balancer : role.m_power_balancer) {
            balancer->power_cap(balancer->power_limit() + slack_power);
        }
        role.m_is_step_complete = true;
    }

    void PowerBalancerAgent::SendDownLimitStep::sample_platform(PowerBalancerAgent::LeafRole &role) const
    {
    }

    void PowerBalancerAgent::MeasureRuntimeStep::update_policy(PowerBalancerAgent::RootRole &role, const std::vector<double> &sample) const
    {
        role.m_policy[PowerBalancerAgent::M_POLICY_MAX_EPOCH_RUNTIME] = sample[PowerBalancerAgent::M_SAMPLE_MAX_EPOCH_RUNTIME];
    }

    void PowerBalancerAgent::MeasureRuntimeStep::enter_step(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const
    {
    }

    void PowerBalancerAgent::MeasureRuntimeStep::sample_platform(PowerBalancerAgent::LeafRole &role) const
    {
        const int COUNT = PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_COUNT;
        const int TOTAL = PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_RUNTIME;
        const int NETWORK = PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_RUNTIME_NETWORK;
        const int IGNORE = PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_RUNTIME_IGNORE;

        for (int pkg_idx = 0; pkg_idx < role.m_num_domain; ++pkg_idx) {
            auto &pkg = role.m_package[pkg_idx];
            pkg.runtime = 0;
            int epoch_count = role.m_platform_io.sample(role.m_pio_idx[pkg_idx][COUNT]);
            if (epoch_count != pkg.last_epoch_count &&
                !role.m_is_step_complete) {
                /// We wish to measure runtime that is a function of node
                /// local optimizations only, and therefore uncorrelated
                /// between compute nodes.
                double total = role.m_platform_io.sample(role.m_pio_idx[pkg_idx][TOTAL]);
                double network = role.m_platform_io.sample(role.m_pio_idx[pkg_idx][NETWORK]);
                double ignore = role.m_platform_io.sample(role.m_pio_idx[pkg_idx][IGNORE]);
                double balanced_epoch_runtime =  total - network - ignore;

                auto &balancer = role.m_power_balancer[pkg_idx];
                auto &pkg = role.m_package[pkg_idx];
                bool is_step_complete = balancer->is_runtime_stable(balanced_epoch_runtime);
                balancer->calculate_runtime_sample();
                double runtime = balancer->runtime_sample();
                if (pkg.runtime < runtime) {
                    pkg.runtime = runtime;
                }
                if (role.m_is_step_complete && !is_step_complete) {
                    role.m_is_step_complete = is_step_complete;
                }
            }
        }
    }

    void PowerBalancerAgent::ReduceLimitStep::update_policy(PowerBalancerAgent::RootRole &role, const std::vector<double> &sample) const
    {
        double slack = sample[PowerBalancerAgent::M_SAMPLE_SUM_POWER_SLACK] / role.M_NUM_NODE;
        double head = sample[PowerBalancerAgent::M_SAMPLE_MIN_POWER_HEADROOM];
        role.m_policy[PowerBalancerAgent::M_POLICY_POWER_SLACK] = slack < head ? slack : head;
    }

    void PowerBalancerAgent::ReduceLimitStep::enter_step(PowerBalancerAgent::LeafRole &role, const std::vector<double> &in_policy) const
    {
        double target = in_policy[PowerBalancerAgent::M_POLICY_MAX_EPOCH_RUNTIME];
        for (auto balancer : role.m_power_balancer) {
            balancer->target_runtime(target);
        }
    }

    void PowerBalancerAgent::ReduceLimitStep::sample_platform(PowerBalancerAgent::LeafRole &role) const
    {
        const int COUNT = PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_COUNT;
        const int TOTAL = PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_RUNTIME;
        const int NETWORK = PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_RUNTIME_NETWORK;
        const int IGNORE = PowerBalancerAgent::M_PLAT_SIGNAL_EPOCH_RUNTIME_IGNORE;

        for (int pkg_idx = 0; pkg_idx != role.m_num_domain; ++pkg_idx) {
            int epoch_count = role.m_platform_io.sample(role.m_pio_idx[pkg_idx][COUNT]);
            // If all of the ranks have observed a new epoch then update
            // the power_balancer.
            auto &package = role.m_package[pkg_idx]
            auto &balancer = role.m_power_balancer[pkg_idx];
            if (epoch_count != package.last_epoch_count &&
                !package.is_step_complete) {
                /// We wish to measure runtime that is a function of
                /// node local optimizations only, and therefore
                /// uncorrelated between compute nodes.
                double total = role.m_platform_io.sample(role.m_pio_idx[pkg_idx][TOTAL]);
                double network = role.m_platform_io.sample(role.m_pio_idx[pkg_idx][NETWORK]);
                double ignore = role.m_platform_io.sample(role.m_pio_idx[pkg_idx][IGNORE]);
                double balanced_epoch_runtime =  total - network - ignore;
                package.is_step_complete = package.is_out_of_bounds ||
                                           balancer->is_target_met(balanced_epoch_runtime);
                package.power_slack = role.m_power_balancer->power_slack();
                package.is_out_of_bounds = false;
                package.power_headroom = balancer->power_cap() - balancer->power_limit();
                package.last_epoch_count = epoch_count;
            }
        }
    }

    PowerBalancerAgent::PowerBalancerAgent()
        : PowerBalancerAgent(platform_io(), platform_topo(), {})
    {

    }

    PowerBalancerAgent::PowerBalancerAgent(PlatformIO &platform_io,
                                           const PlatformTopo &platform_topo,
                                           std::vector<std::shared_ptr<PowerBalancer> > power_balancer)
        : m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_role(nullptr)
        , m_power_balancer(std::move(power_balancer))
        , m_last_wait(time_zero())
        , M_WAIT_SEC(0.005)
        , m_power_tdp(NAN)
        , m_do_send_sample(false)
        , m_do_send_policy(false)
        , m_do_write_batch(false)
    {
        geopm_time(&m_last_wait);
        m_power_tdp = m_platform_io.read_signal("POWER_PACKAGE_TDP", GEOPM_DOMAIN_BOARD, 0);
    }

    PowerBalancerAgent::~PowerBalancerAgent() = default;

    void PowerBalancerAgent::init(int level, const std::vector<int> &fan_in, bool is_root)
    {
        bool is_tree_root = (level == (int)fan_in.size());
        if (level == 0) {
            m_role = std::make_shared<LeafRole>(m_platform_io,
                                                m_platform_topo,
                                                m_power_balancer);
        }
        else if (is_tree_root) {
            double min_power = m_platform_io.read_signal("POWER_PACKAGE_MIN", GEOPM_DOMAIN_PACKAGE, 0);
            double max_power = m_platform_io.read_signal("POWER_PACKAGE_MAX", GEOPM_DOMAIN_PACKAGE, 0);
            m_role = std::make_shared<RootRole>(level, fan_in, min_power, max_power);
        }
        else {
            m_role = std::make_shared<TreeRole>(level, fan_in);
        }
    }

    void PowerBalancerAgent::split_policy(const std::vector<double> &in_policy,
                                          std::vector<std::vector<double> > &out_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_do_send_policy =  m_role->descend(in_policy, out_policy);
    }

    bool PowerBalancerAgent::do_send_policy(void) const
    {
        return m_do_send_policy;
    }

    void PowerBalancerAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                              std::vector<double> &out_sample)
    {
        m_do_send_sample = m_role->ascend(in_sample, out_sample);
    }

    bool PowerBalancerAgent::do_send_sample(void) const
    {
        return m_do_send_sample;
    }

    void PowerBalancerAgent::adjust_platform(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("PowerBalancerAgent::" + std::string(__func__) + "(): policy vectors are not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_do_write_batch = m_role->adjust_platform(in_policy);
    }

    bool PowerBalancerAgent::do_write_batch(void) const
    {
        return m_do_write_batch;
    }

    void PowerBalancerAgent::sample_platform(std::vector<double> &out_sample)
    {
        m_do_send_sample = m_role->sample_platform(out_sample);
    }

    void PowerBalancerAgent::wait(void)    {
        while(geopm_time_since(&m_last_wait) < M_WAIT_SEC) {

        }
        geopm_time(&m_last_wait);
    }

    std::vector<std::pair<std::string, std::string> > PowerBalancerAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > PowerBalancerAgent::report_host(void) const
    {
        return {};
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > PowerBalancerAgent::report_region(void) const
    {
        return {};
    }

    std::vector<std::string> PowerBalancerAgent::trace_names(void) const
    {
        return {"POLICY_POWER_PACKAGE_LIMIT_TOTAL", // M_TRACE_SAMPLE_POLICY_POWER_PACKAGE_LIMIT_TOTAL
                "POLICY_STEP_COUNT",        // M_TRACE_SAMPLE_POLICY_STEP_COUNT
                "POLICY_MAX_EPOCH_RUNTIME", // M_TRACE_SAMPLE_POLICY_MAX_EPOCH_RUNTIME
                "POLICY_POWER_SLACK",       // M_TRACE_SAMPLE_POLICY_POWER_SLACK
                "ENFORCED_POWER_LIMIT",     // M_TRACE_SAMPLE_ENFORCED_POWER_LIMIT
               };
    }

    std::vector<std::function<std::string(double)> > PowerBalancerAgent::trace_formats(void) const
    {
        return {string_format_double,         // M_TRACE_SAMPLE_POLICY_POWER_PACKAGE_LIMIT_TOTAL
                format_step_count,            // M_TRACE_SAMPLE_POLICY_STEP_COUNT
                string_format_double,         // M_TRACE_SAMPLE_POLICY_MAX_EPOCH_RUNTIME
                string_format_double,         // M_TRACE_SAMPLE_POLICY_POWER_SLACK
                string_format_double,         // M_TRACE_SAMPLE_ENFORCED_POWER_LIMIT
               };
    }

    void PowerBalancerAgent::trace_values(std::vector<double> &values)
    {
        m_role->trace_values(values);
    }

    void PowerBalancerAgent::enforce_policy(const std::vector<double> &policy) const
    {
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("PowerBalancerAgent::enforce_policy(): policy vector incorrectly sized.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int control_domain = m_platform_io.control_domain_type("POWER_PACKAGE_LIMIT");
        double pkg_policy = policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] / m_platform_topo.num_domain(control_domain);
        m_platform_io.write_control("POWER_PACKAGE_LIMIT", GEOPM_DOMAIN_BOARD, 0, pkg_policy);
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
        return {"POWER_PACKAGE_LIMIT_TOTAL",
                "STEP_COUNT",
                "MAX_EPOCH_RUNTIME",
                "POWER_SLACK"};
    }

    std::vector<std::string> PowerBalancerAgent::sample_names(void)
    {
        return {"STEP_COUNT",
                "MAX_EPOCH_RUNTIME",
                "SUM_POWER_SLACK",
                "MIN_POWER_HEADROOM"};
    }

    std::string PowerBalancerAgent::format_step_count(double step)
    {
        int64_t step_count = step;
        int64_t step_type = step_count % M_NUM_STEP;
        int64_t loop_count = step_count / M_NUM_STEP;
        std::string result(std::to_string(loop_count));
        switch (step_type) {
            case M_STEP_SEND_DOWN_LIMIT:
                result += "-STEP_SEND_DOWN_LIMIT";
                break;
            case M_STEP_MEASURE_RUNTIME:
                result += "-STEP_MEASURE_RUNTIME";
                break;
            case M_STEP_REDUCE_LIMIT:
                result += "-STEP_REDUCE_LIMIT";
                break;
            default:
                throw Exception("PowerBalancerAgent::format_step_count() step count signal is negative: " + std::to_string(step),
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }
        return result;
    }

    void PowerBalancerAgent::validate_policy(std::vector<double> &policy) const
    {
        // If NAN, use default
        if (std::isnan(policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL])) {
            policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] = m_power_tdp;
        }
        if (std::isnan(policy[M_POLICY_STEP_COUNT])) {
            policy[M_POLICY_STEP_COUNT] = 0.0;
        }
        if (std::isnan(policy[M_POLICY_MAX_EPOCH_RUNTIME])) {
            policy[M_POLICY_MAX_EPOCH_RUNTIME] = 0.0;
        }
        if (std::isnan(policy[M_POLICY_POWER_SLACK])) {
            policy[M_POLICY_POWER_SLACK] = 0.0;
        }

        // Clamp to min or max; note that 0.0 is a valid power limit
        // when the step is not SEND_DOWN_LIMIT
        if (policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] != 0) {
            double min_power_setting(m_platform_io.read_signal("POWER_PACKAGE_MIN", GEOPM_DOMAIN_BOARD, 0));
            double max_power_setting(m_platform_io.read_signal("POWER_PACKAGE_MAX", GEOPM_DOMAIN_BOARD, 0));
            if (policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] < min_power_setting) {
                policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] = min_power_setting;
            }
            else if (policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] > max_power_setting) {
                policy[M_POLICY_POWER_PACKAGE_LIMIT_TOTAL] = max_power_setting;
            }
        }

        // Policy of all zero is not valid
        if (std::all_of(policy.begin(), policy.end(),
                        [] (double x) { return x == 0.0; })) {
            throw Exception("PowerBalancerAgent: invalid policy.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }
}

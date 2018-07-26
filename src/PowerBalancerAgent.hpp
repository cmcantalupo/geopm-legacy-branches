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

#ifndef BALANCINGAGENT_HPP_INCLUDE
#define BALANCINGAGENT_HPP_INCLUDE

#include <vector>
#include <iostream>

#include "geopm_time.h"
#include "Agent.hpp"

namespace geopm
{
    class IPlatformIO;
    class IPlatformTopo;
    template <class type>
    class ICircularBuffer;
    class IPowerBalancer;
    class IPowerGovernor;

    class PowerBalancerAgent : public Agent
    {
        public:
            enum m_policy_e {
                /// @brief The power cap enforced on average over all
                ///        nodes running the application.  This has
                ///        value 0.0 except in two cases.  In the
                ///        first case this is the M_SEND_DOWN_LIMIT
                ///        step at the beginning of the application
                ///        run.  This value will also be non-zero in
                ///        the case where the resource mananger has
                ///        requested a new budget for the application,
                ///        and thus, the algorithm must be restarted
                ///        at step M_SEND_DOWN_LIMIT.
                M_POLICY_POWER_CAP,
                /// @brief Step that the root is providing a policy
                ///        for.  The parent has received a sample
                ///        matching this step in the last walk up the
                ///        tree, execpt in the case where the endpoint
                ///        has recently been updated with a new
                ///        policy, in this case the step will be
                ///        M_SEND_DOWN_LIMIT and the policy indexed by
                ///        M_POLICY_POWER_CAP will have a non-zero
                ///        value.
                M_POLICY_STEP_COUNT,
                /// @brief The largest runtime reported by any leaf
                ///        agent since the last redistribution of
                ///        power.  This will have value 0.0 until all
                ///        leaf agents have reported a runtime to the
                ///        root agent.
                M_POLICY_MAX_EPOCH_RUNTIME,
                /// @brief This value is updated in step
                ///        M_STEP_ADJUST_LIMIT to the amount that each
                ///        leaf agent should increase their power
                ///        limit by calling:
                ///            power_cap(current_limit + slack)
                ///        by before starting the algorithm over again
                ///        at step M_STEP_MEASURE_RUNTIME.  For all
                ///        other steps this value is 0.0.
                M_POLICY_POWER_SLACK,
                /// @brief Number of steps in each iteration of the
                ///        balancing algorithm.
                M_NUM_POLICY,
            };

            enum m_sample_e {
                /// @brief The the step counter that is currently in
                ///        execution.  Note that the step is equal to
                ///        the step counter modulo M_NUM_STEP and is
                ///        reset each time a new power cap is provided
                ///        by sending a policy with a non-zero
                ///        M_POLICY_POWER_CAP field.
                M_SAMPLE_STEP_COUNT,
                /// @brief Maximum expected runtime for any node
                ///        below.
                M_SAMPLE_MAX_EPOCH_RUNTIME,
                /// @brief The sum of all slack power available from
                ///        children below the agent.
                M_SAMPLE_SUM_POWER_SLACK,
                /// @brief Number of elements in a sample vector.
                M_NUM_SAMPLE,
            };

            enum m_plat_signal_e {
                M_PLAT_SIGNAL_EPOCH_RUNTIME,
                M_PLAT_SIGNAL_EPOCH_COUNT,
                M_PLAT_NUM_SIGNAL,
            };
            enum m_trace_sample_e {
                M_TRACE_SAMPLE_EPOCH_RUNTIME,
                M_TRACE_SAMPLE_POWER_LIMIT,
                M_TRACE_NUM_SAMPLE,
            };

            PowerBalancerAgent(IPlatformIO &platform_io, IPlatformTopo &platform_topo,
                               std::unique_ptr<IPowerGovernor> power_governor, std::unique_ptr<IPowerBalancer> power_balancer);
            PowerBalancerAgent();
            virtual ~PowerBalancerAgent();
            void init(int level, const std::vector<int> &fan_in, bool is_level_root) override;
            bool descend(const std::vector<double> &in_policy,
                         std::vector<std::vector<double> >&out_policy) override;
            bool ascend(const std::vector<std::vector<double> > &in_sample,
                        std::vector<double> &out_sample) override;
            bool adjust_platform(const std::vector<double> &in_policy) override;
            bool sample_platform(std::vector<double> &out_sample) override;
            void wait(void) override;
            std::vector<std::pair<std::string, std::string> > report_header(void) const override;
            std::vector<std::pair<std::string, std::string> > report_node(void) const override;
            std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > report_region(void) const override;
            std::vector<std::string> trace_names(void) const override;
            void trace_values(std::vector<double> &values) override;
            static std::string plugin_name(void);
            static std::unique_ptr<Agent> make_plugin(void);
            static std::vector<std::string> policy_names(void);
            static std::vector<std::string> sample_names(void);

            class IStep;

            class IRole {
                public:
                    enum m_step_e {
                        /// @brief On first iteration send down resource
                        ///        manager average limit requested, otherwise
                        ///        send down average excess power.
                        M_STEP_SEND_DOWN_LIMIT,
                        /// @brief Measure epoch runtime several times and
                        ///        apply median filter.  Aggregate epoch
                        ///        runtime up tree by applying maximum filter
                        ///        to measured values.  Propagate down from
                        ///        root the longest recorded runtime from any
                        ///        node.
                        M_STEP_MEASURE_RUNTIME,
                        /// @brief Decrease power limit on all nodes (other
                        ///        than the slowest) until epoch runtime
                        ///        matches the slowest.  Aggregate amount
                        ///        power limit was reduced in last step up the
                        ///        tree with sum filter.  (Go to
                        ///        M_STEP_SEND_DOWN_LIMIT next).
                        M_STEP_REDUCE_LIMIT,
                        /// @brief Number of steps in process.
                        M_NUM_STEP,
                    };

                    virtual bool descend(const std::vector<double> &in_policy,
                            std::vector<std::vector<double> >&out_policy) = 0;
                    virtual bool ascend(const std::vector<std::vector<double> > &in_sample,
                            std::vector<double> &out_sample) = 0;
                    virtual bool adjust_platform(const std::vector<double> &in_policy) = 0;
                    virtual bool sample_platform(std::vector<double> &out_sample) = 0;
                    virtual std::vector<std::string> trace_names(void) const = 0;
                    virtual void trace_values(std::vector<double> &values) = 0;
                    size_t reset();
                    size_t step_count() const;
                    bool is_step_complete() const;
                    void is_step_complete(bool is_complete);
                    void step(size_t step);
                protected:
                    IRole();
                    virtual ~IRole();
                    void inc_step_count();
                    size_t step(void) const;
                    size_t m_step_count;
                    bool m_is_step_complete;
                    std::shared_ptr<IStep> m_step;
            };

            class TreeRole : public IRole {
                friend class SendDownLimitStep;
                friend class MeasureRuntimeStep;
                friend class ReduceLimitStep;
                public:
                    TreeRole(int num_children);
                    virtual ~TreeRole();
                    virtual bool descend(const std::vector<double> &in_policy,
                            std::vector<std::vector<double> >&out_policy) override;
                    virtual bool ascend(const std::vector<std::vector<double> > &in_sample,
                            std::vector<double> &out_sample) override;
                    bool adjust_platform(const std::vector<double> &in_policy) override;
                    bool sample_platform(std::vector<double> &out_sample) override;
                    std::vector<std::string> trace_names(void) const override;
                    void trace_values(std::vector<double> &values) override;
                protected:
                    const std::vector<std::function<double(const std::vector<double>&)> > M_AGG_FUNC;
                    const int M_NUM_CHILDREN;
                    std::vector<double> m_policy;
            };

            class RootRole : public TreeRole {
                friend class SendDownLimitStep;
                friend class MeasureRuntimeStep;
                friend class ReduceLimitStep;
                public:
                    RootRole(int num_node, int num_children);
                    virtual ~RootRole();
                    bool descend(const std::vector<double> &in_policy,
                            std::vector<std::vector<double> >&out_policy) override;
                    bool ascend(const std::vector<std::vector<double> > &in_sample,
                            std::vector<double> &out_sample) override;
                private:
                    const int M_NUM_NODE;
                    double m_root_cap;
            };

            class LeafRole : public IRole {
                friend class SendDownLimitStep;
                friend class MeasureRuntimeStep;
                friend class ReduceLimitStep;
                public:
                    LeafRole(IPlatformIO &platform_io, IPlatformTopo &platform_topo,
                             std::unique_ptr<IPowerGovernor> power_governor, std::unique_ptr<IPowerBalancer> power_balancer);
                    virtual ~LeafRole();
                    bool descend(const std::vector<double> &in_policy,
                            std::vector<std::vector<double> >&out_policy) override;
                    bool ascend(const std::vector<std::vector<double> > &in_sample,
                            std::vector<double> &out_sample) override;
                    bool adjust_platform(const std::vector<double> &in_policy) override;
                    bool sample_platform(std::vector<double> &out_sample) override;
                    std::vector<std::string> trace_names(void) const override;
                    void trace_values(std::vector<double> &values) override;
                private:
                    void init_platform_io(void);

                    IPlatformIO &m_platform_io;
                    IPlatformTopo &m_platform_topo;
                    std::vector<int> m_pio_idx;
                    std::unique_ptr<IPowerGovernor> m_power_governor;
                    std::unique_ptr<IPowerBalancer> m_power_balancer;
                    int m_last_epoch_count;
                    double m_runtime;
                    double m_power_slack;
            };

            class IStep {
                public:
                    IStep() = default;
                    virtual ~IStep() = default;
                    virtual bool update_policy(RootRole &role, const std::vector<double> &sample) = 0;
                    virtual void pre_adjust(LeafRole &role, const std::vector<double> &in_policy) = 0;
                    virtual void post_adjust(LeafRole &role, double policy_limit, double actual_limit) = 0;
                    virtual void post_sample(LeafRole &role, double epoch_runtime) = 0;
            };

        protected:
            IPlatformIO &m_platform_io;
            IPlatformTopo &m_platform_topo;
            std::shared_ptr<IRole> m_role;
            std::unique_ptr<IPowerGovernor> m_power_governor;   /// temporary ownership, moved to IRole on init
            std::unique_ptr<IPowerBalancer> m_power_balancer;   /// temporary ownership, moved to IRole on init
            struct geopm_time_s m_last_wait;
            const double M_WAIT_SEC;
    };
}

#endif

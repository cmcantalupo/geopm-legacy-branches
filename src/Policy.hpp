/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#ifndef POLICY_HPP_INCLUDE
#define POLICY_HPP_INCLUDE

#include <vector>
#include <map>
#include <float.h>

#include "geopm_message.h"
#include "PolicyFlags.hpp"

namespace geopm
{

    class RegionPolicy;

    class Policy
    {
        public:
            Policy(int num_domain);
            virtual ~Policy();
            int num_domain(void);
            void region_id(std::vector<uint64_t> &region_id);
            void update(uint64_t region_id, int domain_idx, double target);
            void update(uint64_t region_id, const std::vector<double> &target);
            void mode(int new_mode);
            void policy_flags(long int new_flags);
            void target(uint64_t region_id, std::vector<double> &target);
            void target(uint64_t region_id, int domain, double &target);
            /// @brief Get the policy power mode
            /// @return geopm_policy_mode_e power mode
            int mode(void) const;
            /// @brief Get the policy frequency
            /// @return frequency in MHz
            int frequency_mhz(void) const;
            /// @brief Get the policy TDP percentage
            /// @return TDP (thermal design power) percentage between 0-100
            int tdp_percent(void) const;
            /// @brief Get the policy affinity. This is the cores that we
            /// will dynamically control. One of
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT or
            /// GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT.
            /// @return enum power affinity
            int affinity(void) const;
            /// @brief Get the policy power goal, One of
            /// GEOPM_FLAGS_GOAL_CPU_EFFICIENCY,
            /// GEOPM_FLAGS_GOAL_NETWORK_EFFICIENCY, or
            /// GEOPM_FLAGS_GOAL_MEMORY_EFFICIENCY
            /// @return enum power goal
            int goal(void) const;
            /// @brief Get the number of 'big' cores
            /// @return number of cores where we will run
            ///         unconstrained power.
            int num_max_perf(void) const;
            void target_updated(uint64_t region_id, std::map<int, double> &target); // map from domain index to updated target value
            void target_valid(uint64_t region_id, std::map<int, double> &target);
            void policy_message(uint64_t region_id,
                                const struct geopm_policy_message_s &parent_msg,
                                std::vector<struct geopm_policy_message_s> &child_msg);
            /// @brief Set the convergence state.
            /// Called by the decision algorithm when it has determined
            /// whether or not the power policy enforcement has converged
            /// to an acceptance state.
            void is_converged(uint64_t region_id, bool converged_state);
            /// @brief Have we converged for this region.
            /// Set by he decision algorithm when it has determined
            /// that the power policy enforcement has converged to an
            /// acceptance state.
            /// @return true if converged else false.
            bool is_converged(uint64_t region_id);
        protected:
            PolicyFlags m_policy_flags;
            RegionPolicy *region_policy(uint64_t region_id);
            int m_num_domain;
            int m_mode;
            int m_num_sample;
            std::map<uint64_t, RegionPolicy *> m_region_policy;
    };
}

#endif

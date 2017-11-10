/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#ifndef ADAPTIVE_FREQ_REGION_HPP_INCLUDE
#define ADAPTIVE_FREQ_REGION_HPP_INCLUDE

#include "Region.hpp"

#include <vector>

namespace geopm
{

    /// @brief Holds the performance history of a Region.
    class AdaptiveFreqRegion
    {
        public:
            AdaptiveFreqRegion(geopm::IRegion *region, double freq_min,
                               double freq_max, double freq_step, int num_domain);
            virtual ~AdaptiveFreqRegion() = default;
            double freq(void) const;
            void update_entry(void);
            void update_exit(void);
        private:
            // Used to determine whether performance degraded or not.
            // Higher is better.
            virtual double perf_metric();
            virtual double energy_metric();
            geopm::IRegion *m_region;
            const size_t M_NUM_FREQ;
            size_t m_curr_idx;
            double m_target = 0.0;
            const double M_TARGET_RATIO = 0.10;  // up to 10% degradation allowed
            const size_t M_MIN_BASE_SAMPLE = 4;

            size_t m_num_increase = 0;
            const size_t M_MAX_INCREASE = 4;
            bool m_is_learning = true;

            std::vector<double> m_allowed_freq;
            std::vector<double> m_perf_max;
            std::vector<double> m_energy_min;
            std::vector<size_t> m_num_sample;
            struct geopm_time_s m_start_time;
            double m_start_energy = 0.0;
            int m_num_domain;
    };

} // namespace geopm

#endif

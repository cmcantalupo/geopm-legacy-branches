/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include "config.h"

#include <utility>
#include "geopm/Exception.hpp"
#include "LevelZero.hpp"

namespace geopm
{
    class LevelZeroNull : public LevelZero
    {
        public:
            LevelZeroNull() = default;
            virtual ~LevelZeroNull() = default;
            int num_accelerator(void) const override
            {
                return 0;
            }
            int num_accelerator(int domain) const override
            {
                return 0;
            }
            int frequency_domain_count(unsigned int l0_device_idx,
                                       int domain) const override
            {
                return 0;
            }
            double frequency_status(unsigned int l0_device_idx,
                                       int l0_domain, int l0_domain_idx) const override
            {
                return 0;
            }
            double frequency_min(unsigned int l0_device_idx, int l0_domain,
                                 int l0_domain_idx) const override
            {
                return 0;
            }
            double frequency_max(unsigned int l0_device_idx, int l0_domain,
                                 int l0_domain_idx) const override {
                return 0;
            }
            std::pair<double, double> frequency_range(unsigned int l0_device_idx,
                                                      int l0_domain,
                                                      int l0_domain_idx) const override
            {
                return std::make_pair<double, double>(0, 0);
            }
            int engine_domain_count(unsigned int l0_device_idx, int domain) const override
            {
                return 0;
            }
            std::pair<uint64_t, uint64_t> active_time_pair(unsigned int l0_device_idx,
                                                           int l0_domain, int l0_domain_idx) const override
            {
                return std::make_pair<double, double>(0, 0);
            }
            uint64_t active_time(unsigned int l0_device_idx, int l0_domain,
                                 int l0_domain_idx) const override
            {
                return 0;
            }
            uint64_t active_time_timestamp(unsigned int l0_device_idx,
                                          int l0_domain, int l0_domain_idx) const override
            {
                return 0;
            }
            std::pair<uint64_t, uint64_t> energy_pair(unsigned int l0_device_idx) const override
            {
                return std::make_pair<uint64_t, uint64_t>(0, 0);
            }
            uint64_t energy(unsigned int l0_device_idx) const override
            {
                return 0;
            }
            uint64_t energy_timestamp(unsigned int l0_device_idx) const override
            {
                return 0;
            }
            int32_t power_limit_tdp(unsigned int l0_device_idx) const override
            {
                return 0;
            }
            int32_t power_limit_min(unsigned int l0_device_idx) const override
            {
                return 0;
            }
            int32_t power_limit_max(unsigned int l0_device_idx) const override
            {
                return 0;
            }
            void frequency_control(unsigned int l0_device_idx, int l0_domain,
                                   int l0_domain_idx, double range_min,
                                   double range_max) const override
            {

            }
    };

    const LevelZero &levelzero()
    {
        throw Exception("LevelZeroThrow::" + std::string(__func__) +
                        ": GEOPM configured without Level Zero library support.  Please configure with --enable-levelzero",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        static LevelZeroNull instance;
        return instance;
    }
}

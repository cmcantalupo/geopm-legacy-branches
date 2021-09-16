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
#include <string>

#include "geopm/Exception.hpp"
#include "NVMLDevicePool.hpp"

namespace geopm
{
    class NVMLDevicePoolNull : public NVMLDevicePool
    {
        public:
            NVMLDevicePoolNull() = default;
            ~NVMLDevicePoolNull() = default;
            virtual int num_accelerator(void) const override {return -1;}
            virtual cpu_set_t *cpu_affinity_ideal_mask(int accel_idx) const override {return nullptr;}
            virtual uint64_t frequency_status_sm(int accel_idx) const override {return 0;}
            virtual uint64_t utilization(int accel_idx) const override {return 0;}
            virtual uint64_t power(int accel_idx) const override {return 0;}
            virtual uint64_t power_limit(int accel_idx) const override {return 0;}
            virtual uint64_t frequency_status_mem(int accel_idx) const override {return 0;}
            virtual uint64_t throttle_reasons(int accel_idx) const override {return 0;}
            virtual uint64_t temperature(int accel_idx) const override {return 0;}
            virtual uint64_t energy(int accel_idx) const override {return 0;}
            virtual uint64_t performance_state(int accel_idx) const override {return 0;}
            virtual uint64_t throughput_rx_pcie(int accel_idx) const override {return 0;}
            virtual uint64_t throughput_tx_pcie(int accel_idx) const override {return 0;}
            virtual uint64_t utilization_mem(int accel_idx) const override {return 0;}
            virtual std::vector<int> active_process_list(int accel_idx) const override {std::vector<int> result; return result;}

            virtual void frequency_control_sm(int accel_idx, int min_freq, int max_freq) const override {}
            virtual void frequency_reset_control(int accel_idx) const override {}
            virtual void power_control(int accel_idx, int setting) const override {}

    };

    const NVMLDevicePool &nvml_device_pool(const int num_cpu)
    {
        throw Exception("NVMLDevicePoolThrow::" + std::string(__func__) +
                        ": GEOPM configured without nvml library support.  Please configure with --enable-nvml",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        static NVMLDevicePoolNull instance;
        return instance;
    }

}

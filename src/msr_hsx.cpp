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

#include "MSR.hpp"
#include "PlatformTopo.hpp"
#include "config.h"

namespace geopm
{
    const MSR *msr_hsx(size_t &num_msr)
    {
        static const MSR instance[] = {
            MSR("TIME_STAMP_COUNTER", 0x10,
                {{"TIMESTAMP_COUNT", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 64,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}},
                {}),
            MSR("PERF_STATUS", 0x198,
                {{"FREQ", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 16,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}}},
                {}),
            MSR("PERF_CTL", 0x199,
                {},
                {{"FREQ", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 16,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                 {"ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 33,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}}),
            MSR("THERM_STATUS", 0x19C,
                {{"DIGITAL_READOUT", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 23,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_CELSIUS,
                      .scalar    = 1.0}},
                {"VALID", (struct IMSR::m_encode_s) {
                      .begin_bit = 31,
                      .end_bit   = 32,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}},
                {}),
            MSR("MISC_ENABLE", 0x1A0,
                {},
                {{"ENHANCED_SPEEDSTEP_TECH_ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 17,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                {"TURBO_MODE_DISABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 38,
                      .end_bit   = 39,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}}),
            MSR("TEMPERATURE_TARGET", 0x1A2,
                {{"DIGITAL_READOUT", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 24,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_CELSIUS,
                      .scalar    = 1.0}}},
                {}),
            MSR("TURBO_RATIO_LIMIT", 0x1AD,
                {},
                {{"MAX_RATIO_LIMIT_1CORE", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 8,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_2CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 16,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_3CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 24,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_4CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 24,
                      .end_bit   = 32,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_5CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 40,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_6CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 40,
                      .end_bit   = 48,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_7CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 48,
                      .end_bit   = 56,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_8CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 56,
                      .end_bit   = 64,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}}}),
            MSR("TURBO_RATIO_LIMIT1", 0x1AE,
                {},
                {{"MAX_RATIO_LIMIT_9CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 8,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_10CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 16,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_11CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 24,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_12CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 24,
                      .end_bit   = 32,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_13CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 40,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_14CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 40,
                      .end_bit   = 48,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_15CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 48,
                      .end_bit   = 56,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_16CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 56,
                      .end_bit   = 64,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}}}),
            MSR("TURBO_RATIO_LIMIT2", 0x1AF,
                {},
                {{"MAX_RATIO_LIMIT_17CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 8,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"MAX_RATIO_LIMIT_18CORES", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 16,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                {"PCU_SEMAPHORE", (struct IMSR::m_encode_s) {
                      .begin_bit = 63,
                      .end_bit   = 64,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}}),
            MSR("PACKAGE_THERM_STATUS", 0x1B1,
                {{"DIGITAL_READOUT", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 23,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_CELSIUS,
                      .scalar    = 1.0}},
                {"VALID", (struct IMSR::m_encode_s) {
                      .begin_bit = 31,
                      .end_bit   = 32,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}},
                {}),
            MSR("PERF_FIXED_CTR0", 0x309,
                {{"INST_RETIRED_ANY", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 64,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}},
                {}),
            MSR("PERF_FIXED_CTR1", 0x30A,
                {{"CPU_CLK_UNHALTED_THREAD", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 64,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}},
                {}),
            MSR("PERF_FIXED_CTR2", 0x30B,
                {{"CPU_CLK_UNHALTED_REF_TSC", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 64,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}},
                {}),
            MSR("PERF_FIXED_CTR_CTRL", 0x38D,
                {},
                {{"EN0_CTR", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 2,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN0_BOTH_THREAD", (struct IMSR::m_encode_s) {
                      .begin_bit = 2,
                      .end_bit   = 3,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN0_PMI", (struct IMSR::m_encode_s) {
                      .begin_bit = 3,
                      .end_bit   = 4,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN1_CTR", (struct IMSR::m_encode_s) {
                      .begin_bit = 4,
                      .end_bit   = 6,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN1_BOTH_THREAD", (struct IMSR::m_encode_s) {
                      .begin_bit = 6,
                      .end_bit   = 7,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN1_PMI", (struct IMSR::m_encode_s) {
                      .begin_bit = 7,
                      .end_bit   = 8,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN2_CTR", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 10,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN2_BOTH_THREAD", (struct IMSR::m_encode_s) {
                      .begin_bit = 10,
                      .end_bit   = 11,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN2_PMI", (struct IMSR::m_encode_s) {
                      .begin_bit = 11,
                      .end_bit   = 12,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}}),
            MSR("PERF_GLOBAL_CTRL", 0x38F,
                {},
                {{"EN_PMC0", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 1,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN_PMC1", (struct IMSR::m_encode_s) {
                      .begin_bit = 1,
                      .end_bit   = 2,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN_PMC2", (struct IMSR::m_encode_s) {
                      .begin_bit = 2,
                      .end_bit   = 3,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN_PMC3", (struct IMSR::m_encode_s) {
                      .begin_bit = 3,
                      .end_bit   = 4,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN_FIXED_CTR0", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 33,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN_FIXED_CTR1", (struct IMSR::m_encode_s) {
                      .begin_bit = 33,
                      .end_bit   = 34,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"EN_FIXED_CTR2", (struct IMSR::m_encode_s) {
                      .begin_bit = 34,
                      .end_bit   = 35,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}}),
            MSR("PERF_GLOBAL_OVF_CTRL", 0x390,
                {},
                {{"CLEAR_OVF_PMC0", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 1,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"CLEAR_OVF_PMC1", (struct IMSR::m_encode_s) {
                      .begin_bit = 1,
                      .end_bit   = 2,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"CLEAR_OVF_PMC2", (struct IMSR::m_encode_s) {
                      .begin_bit = 2,
                      .end_bit   = 3,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"CLEAR_OVF_PMC3", (struct IMSR::m_encode_s) {
                      .begin_bit = 3,
                      .end_bit   = 4,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"CLEAR_OVF_FIXED_CTR0", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 33,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"CLEAR_OVF_FIXED_CTR1", (struct IMSR::m_encode_s) {
                      .begin_bit = 33,
                      .end_bit   = 34,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"CLEAR_OVF_FIXED_CTR2", (struct IMSR::m_encode_s) {
                      .begin_bit = 34,
                      .end_bit   = 35,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}}),
            MSR("PKG_RAPL_UNIT", 0x606,
                {{"POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 4,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_LOG_HALF,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 8.0}}, // Signal is 1.0 because the units should be 0.125 Watts
                 {"ENERGY", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 13,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_LOG_HALF,
                      .units     = IMSR::M_UNITS_JOULES,
                      .scalar    = 1.6384e4}}, // Signal is 1.0 because the units should be 6.103515625e-05 Joules.
                 {"TIME", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 20,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_LOG_HALF,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 1.024e3}}}, // Signal is 1.0 because the units should be 9.765625e-04 seconds.
                {}),
            MSR("PKG_POWER_LIMIT", 0x610,
                {},
                {{"SOFT_POWER_LIMIT", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 15,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"SOFT_LIMIT_ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 15,
                      .end_bit   = 16,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"SOFT_CLAMP_ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 17,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"SOFT_TIME_WINDOW", (struct IMSR::m_encode_s) {
                      .begin_bit = 17,
                      .end_bit   = 24,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_7_BIT_FLOAT,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 9.765625e-04}},
                 {"HARD_POWER_LIMIT", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 47,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"HARD_LIMIT_ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 47,
                      .end_bit   = 48,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"HARD_CLAMP_ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 48,
                      .end_bit   = 49,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"HARD_TIME_WINDOW", (struct IMSR::m_encode_s) {
                      .begin_bit = 49,
                      .end_bit   = 56,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_7_BIT_FLOAT,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 9.765625e-04}},
                 {"LOCK", (struct IMSR::m_encode_s) {
                      .begin_bit = 63,
                      .end_bit   = 64,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}}),
            MSR("PKG_ENERGY_STATUS", 0x611,
                {{"ENERGY", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 32,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_JOULES,
                      .scalar    = 1.525878906e-05}}},
                {}),
            MSR("PKG_POWER_INFO", 0x614,
                {{"THERMAL_SPEC_POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 15,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"MIN_POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 31,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"MAX_POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 47,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"MAX_TIME_WINDOW", (struct IMSR::m_encode_s) {
                      .begin_bit = 48,
                      .end_bit   = 55,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_7_BIT_FLOAT,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 9.765625e-04}}},
                {}),
            MSR("DRAM_POWER_LIMIT", 0x618,
                {},
                {{"POWER_LIMIT", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 15,
                      .domain    = IPlatformTopo::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"ENABLE", (struct IMSR::m_encode_s) {
                      .begin_bit = 15,
                      .end_bit   = 16,
                      .domain    = IPlatformTopo::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}},
                 {"TIME_WINDOW", (struct IMSR::m_encode_s) {
                      .begin_bit = 17,
                      .end_bit   = 24,
                      .domain    = IPlatformTopo::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_7_BIT_FLOAT,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 9.765625e-04}},
                 {"LOCK", (struct IMSR::m_encode_s) {
                      .begin_bit = 31,
                      .end_bit   = 32,
                      .domain    = IPlatformTopo::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1}}}),
            MSR("DRAM_ENERGY_STATUS", 0x619,
                {{"ENERGY", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 32,
                      .domain    = IPlatformTopo::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_JOULES,
                      .scalar    = 1.525878906e-05}}},
                {}),
            MSR("DRAM_PERF_STATUS", 0x61B,
                {{"THROTTLE_TIME", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 32,
                      .domain    = IPlatformTopo::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 9.765625e-04}}},
                {}),
            MSR("DRAM_POWER_INFO", 0x61C,
                {{"THERMAL_SPEC_POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 15,
                      .domain    = IPlatformTopo::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"MIN_POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 16,
                      .end_bit   = 31,
                      .domain    = IPlatformTopo::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"MAX_POWER", (struct IMSR::m_encode_s) {
                      .begin_bit = 32,
                      .end_bit   = 47,
                      .domain    = IPlatformTopo::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_WATTS,
                      .scalar    = 1.25e-1}},
                 {"MAX_TIME_WINDOW", (struct IMSR::m_encode_s) {
                      .begin_bit = 48,
                      .end_bit   = 55,
                      .domain    = IPlatformTopo::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_7_BIT_FLOAT,
                      .units     = IMSR::M_UNITS_SECONDS,
                      .scalar    = 9.765625e-04}},
                 {"LOCK", (struct IMSR::m_encode_s) {
                      .begin_bit = 48,
                      .end_bit   = 55,
                      .domain    = IPlatformTopo::M_DOMAIN_BOARD_MEMORY,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}},
                {}),
            MSR("PLATFORM_INFO", 0xCE,
                {{"MAX_NON_TURBO_RATIO", (struct IMSR::m_encode_s) {
                      .begin_bit = 8,
                      .end_bit   = 16,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}},
                 {"PROGRAMMABLE_RATIO_LIMITS_TURBO_MODE", (struct IMSR::m_encode_s) {
                      .begin_bit = 28,
                      .end_bit   = 29,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1}},
                 {"PROGRAMMABLE_TDP_LIMITS_TURBO_MODE", (struct IMSR::m_encode_s) {
                      .begin_bit = 29,
                      .end_bit   = 30,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1}},
                 {"PROGRAMMABLE_TCC_ACTIVATION_OFFSET", (struct IMSR::m_encode_s) {
                      .begin_bit = 30,
                      .end_bit   = 31,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1}},
                 {"MAX_EFFICIENCY_RATIO", (struct IMSR::m_encode_s) {
                      .begin_bit = 40,
                      .end_bit   = 48,
                      .domain    = IPlatformTopo::M_DOMAIN_PACKAGE,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_HZ,
                      .scalar    = 1e8}}},
                {}),
            MSR("MPERF", 0xE7,
                {{"MCNT", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 64,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}},
                {}),
            MSR("APERF", 0xE8,
                {{"ACNT", (struct IMSR::m_encode_s) {
                      .begin_bit = 0,
                      .end_bit   = 64,
                      .domain    = IPlatformTopo::M_DOMAIN_CPU,
                      .function  = IMSR::M_FUNCTION_SCALE,
                      .units     = IMSR::M_UNITS_NONE,
                      .scalar    = 1.0}}},
                {}),
        };
        num_msr = sizeof(instance) / sizeof(MSR);
        return instance;
    }
}

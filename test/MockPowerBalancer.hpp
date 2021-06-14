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

#ifndef MOCKPOWERBALANCER_HPP_INCLUDE
#define MOCKPOWERBALANCER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "PowerBalancer.hpp"

class MockPowerBalancer : public geopm::PowerBalancer
{
    public:
        MOCK_METHOD(void, power_cap, (double cap), (override));
        MOCK_METHOD(double, power_cap, (), (const, override));
        MOCK_METHOD(double, power_limit, (), (const, override));
        MOCK_METHOD(void, power_limit_adjusted, (double limit), (override));
        MOCK_METHOD(bool, is_runtime_stable, (double measured_runtime), (override));
        MOCK_METHOD(double, runtime_sample, (), (const, override));
        MOCK_METHOD(void, calculate_runtime_sample, (), (override));
        MOCK_METHOD(void, target_runtime, (double largest_runtime), (override));
        MOCK_METHOD(bool, is_target_met, (double measured_runtime), (override));
        MOCK_METHOD(void, achieved_limit, (double achieved), (override));
        MOCK_METHOD(double, power_slack, (), (override));
};

#endif

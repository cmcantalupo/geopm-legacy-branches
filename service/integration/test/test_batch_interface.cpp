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
#include <iostream>
#include <unistd.h>
#include "geopm/PlatformIO.hpp"
#include "geopm_topo.h"

void run(void)
{
    geopm::PlatformIO &pio = geopm::platform_io();
    std::vector<int> signal_handles;
    signal_handles.push_back(pio.push_signal("SERVICE::TIME", GEOPM_DOMAIN_BOARD, 0));
    signal_handles.push_back(pio.push_signal("SERVICE::ENERGY_PACKAGE", GEOPM_DOMAIN_PACKAGE, 0));
    signal_handles.push_back(pio.push_signal("SERVICE::ENERGY_PACKAGE", GEOPM_DOMAIN_PACKAGE, 1));
    signal_handles.push_back(pio.push_signal("SERVICE::POWER_PACKAGE", GEOPM_DOMAIN_PACKAGE, 0));
    signal_handles.push_back(pio.push_signal("SERVICE::POWER_PACKAGE", GEOPM_DOMAIN_PACKAGE, 1));
    for (int idx = 0; idx < 10; ++idx) {
        pio.read_batch();
        for (const auto &handle : signal_handles) {
            std::cerr << pio.sample(handle) << " ";
        }
        std::cerr << "\n";
        sleep(1);
    }
}


int main (int argc, char **argv)
{
    run();
    return 0;
}

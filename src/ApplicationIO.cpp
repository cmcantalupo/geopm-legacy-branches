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

#include <utility>

#include "ApplicationIO.hpp"
#include "PlatformIO.hpp"
#include "ProfileSampler.hpp"
#include "SampleRegulator.hpp"
#include "RuntimeRegulator.hpp"
#include "ProfileIOSample.hpp"
#include "ProfileIOGroup.hpp"
#include "Helper.hpp"
#include "config.h"

#ifdef GEOPM_HAS_XMMINTRIN
#include <xmmintrin.h>
#endif

namespace geopm
{
    constexpr size_t ApplicationIO::M_SHMEM_REGION_SIZE;

    ApplicationIO::ApplicationIO(const std::string &shm_key)
        : ApplicationIO(shm_key, geopm::make_unique<ProfileSampler>(M_SHMEM_REGION_SIZE))
    {

    }

    ApplicationIO::ApplicationIO(const std::string &shm_key,
                                 std::unique_ptr<IProfileSampler> sampler)
        : m_sampler(std::move(sampler))
        , m_sample_regulator(nullptr)
        , m_do_shutdown(false)
        , m_is_connected(false)
        , m_rank_per_node(-1)
        , m_app_start_time{{0, 0}}
    {

    }

    ApplicationIO::~ApplicationIO()
    {

    }

    void ApplicationIO::connect(void)
    {
        if (!m_is_connected) {
            m_sampler->initialize();
            geopm_time(&m_app_start_time);
            m_rank_per_node = m_sampler->rank_per_node();
            m_prof_sample.resize(m_sampler->capacity());
            std::vector<int> cpu_rank = m_sampler->cpu_rank();
            m_profile_io_sample = std::make_shared<ProfileIOSample>(cpu_rank);
            platform_io().register_iogroup(geopm::make_unique<ProfileIOGroup>(m_profile_io_sample));
            m_is_connected = true;
        }
    }

    bool ApplicationIO::do_shutdown(void) const
    {
        return m_sampler->do_shutdown();
    }

    std::string ApplicationIO::report_name(void) const
    {
        return m_sampler->report_name();
    }

    std::string ApplicationIO::profile_name(void) const
    {
        return m_sampler->profile_name();
    }

    std::set<std::string> ApplicationIO::region_name_set(void) const
    {
        return m_sampler->name_set();
    }

    double ApplicationIO::total_region_runtime(uint64_t region_id) const
    {
        return m_profile_io_sample->total_region_runtime(region_id);
    }

    double ApplicationIO::total_region_mpi_runtime(uint64_t region_id) const
    {
        return m_profile_io_sample->total_region_mpi_time(region_id);
    }

    double ApplicationIO::total_epoch_runtime(void) const
    {
        return m_profile_io_sample->total_epoch_runtime();
    }

    double ApplicationIO::total_app_runtime(void) const
    {
        geopm_time_s curr_time{{0, 0}};
        geopm_time(&curr_time);
        return geopm_time_diff(m_app_start_time, curr_time);
    }

    double ApplicationIO::total_app_mpi_runtime(void) const
    {
        return m_profile_io_sample->total_app_mpi_runtime();
    }

    int ApplicationIO::total_count(uint64_t region_id) const
    {
        return m_profile_io_sample->total_count(region_id);
    }

    void ApplicationIO::update(std::shared_ptr<IComm> comm)
    {
        size_t length = 0;
        m_sampler->sample(m_prof_sample, length, comm);
        m_profile_io_sample->update(m_prof_sample.cbegin(), m_prof_sample.cbegin() + length);
    }
}

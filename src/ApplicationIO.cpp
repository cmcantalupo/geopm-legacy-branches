/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include "ApplicationIO.hpp"

#include <utility>

#include "Exception.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "ProfileSampler.hpp"
#include "ProfileEventBuffer.hpp"
#include "ProfileThread.hpp"
#include "Helper.hpp"
#include "config.h"

#ifdef GEOPM_HAS_XMMINTRIN
#include <xmmintrin.h>
#endif

namespace geopm
{
    constexpr size_t ApplicationIOImp::M_SHMEM_REGION_SIZE;

    ApplicationIOImp::ApplicationIOImp(const std::string &shm_key)
        : ApplicationIOImp(shm_key,
                           geopm::make_unique<ProfileSamplerImp>(M_SHMEM_REGION_SIZE),
                           platform_io(), platform_topo(), profile_event_buffer())
    {

    }

    ApplicationIOImp::ApplicationIOImp(const std::string &shm_key,
                                 std::unique_ptr<ProfileSampler> sampler,
                                 PlatformIO &platform_io,
                                 const PlatformTopo &platform_topo,
                                 ProfileEventBuffer &profile_event_buffer)
        : m_sampler(std::move(sampler))
        , m_platform_io(platform_io)
        , m_platform_topo(platform_topo)
        , m_profile_event_buffer(profile_event_buffer)
        , m_thread_progress(m_platform_topo.num_domain(GEOPM_DOMAIN_CPU))
        , m_is_connected(false)
        , m_rank_per_node(-1)
        , m_start_energy_pkg(NAN)
        , m_start_energy_dram(NAN)
    {
    }

    ApplicationIOImp::~ApplicationIOImp()
    {

    }

    void ApplicationIOImp::connect(void)
    {
        if (!m_is_connected) {
            m_sampler->initialize();
            m_rank_per_node = m_sampler->rank_per_node();
            m_prof_sample.resize(m_sampler->capacity());
            m_profile_event_buffer.cpu_rank(m_sampler->cpu_rank());
            m_is_connected = true;
            m_start_energy_pkg = current_energy_pkg();
            m_start_energy_dram = current_energy_dram();
        }
    }

    void ApplicationIOImp::controller_ready(void)
    {
        m_sampler->controller_ready();
    }

    bool ApplicationIOImp::do_shutdown(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = m_sampler->do_shutdown();
        return result;
    }

    std::string ApplicationIOImp::report_name(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_sampler->report_name();
    }

    std::string ApplicationIOImp::profile_name(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_sampler->profile_name();
    }

    std::set<std::string> ApplicationIOImp::region_name_set(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return m_sampler->name_set();
    }

    double ApplicationIOImp::total_region_runtime(uint64_t region_id) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double result = 0.0;
        throw Exception("ApplicationIO switch to user of ProfileEventBuffer incomplete",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return result;
    }

    double ApplicationIOImp::total_region_runtime_mpi(uint64_t region_id) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double result = 0.0;
        throw Exception("ApplicationIO switch to user of ProfileEventBuffer incomplete",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return result;
    }

    double ApplicationIOImp::total_epoch_runtime(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        throw Exception("ApplicationIO switch to user of ProfileEventBuffer incomplete",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double ApplicationIOImp::total_epoch_runtime_network(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        throw Exception("ApplicationIO switch to user of ProfileEventBuffer incomplete",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double ApplicationIOImp::total_epoch_energy_pkg(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        throw Exception("ApplicationIO switch to user of ProfileEventBuffer incomplete",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double ApplicationIOImp::total_epoch_energy_dram(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        throw Exception("ApplicationIO switch to user of ProfileEventBuffer incomplete",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double ApplicationIOImp::total_app_runtime(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double result = 0.0;
        throw Exception("ApplicationIO switch to user of ProfileEventBuffer incomplete",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return result;
    }

    double ApplicationIOImp::current_energy_pkg(void) const
    {
        double energy = 0.0;
        int num_package = m_platform_topo.num_domain(GEOPM_DOMAIN_PACKAGE);
        for (int pkg = 0; pkg < num_package; ++pkg) {
            energy += m_platform_io.read_signal("ENERGY_PACKAGE", GEOPM_DOMAIN_PACKAGE, pkg);
        }
        return energy;
   }

    double ApplicationIOImp::current_energy_dram(void) const
    {
        double energy = 0.0;
        int num_dram = m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD_MEMORY);
        for (int dram = 0; dram < num_dram; ++dram) {
            energy += m_platform_io.read_signal("ENERGY_DRAM", GEOPM_DOMAIN_BOARD_MEMORY, dram);
        }
        return energy;
    }

    double ApplicationIOImp::total_app_energy_pkg(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return current_energy_pkg() - m_start_energy_pkg;
    }

    double ApplicationIOImp::total_app_energy_dram(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return current_energy_dram() - m_start_energy_dram;
    }

    double ApplicationIOImp::total_app_runtime_mpi(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        throw Exception("ApplicationIO switch to user of ProfileEventBuffer incomplete",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double ApplicationIOImp::total_app_runtime_ignore(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        throw Exception("ApplicationIO switch to user of ProfileEventBuffer incomplete",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    int ApplicationIOImp::total_epoch_count(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        throw Exception("ApplicationIO switch to user of ProfileEventBuffer incomplete",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double ApplicationIOImp::total_epoch_runtime_ignore(void) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        throw Exception("ApplicationIO switch to user of ProfileEventBuffer incomplete",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    int ApplicationIOImp::total_count(uint64_t region_id) const
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double result = 0.0;
        throw Exception("ApplicationIO switch to user of ProfileEventBuffer incomplete",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return result;
    }

    void ApplicationIOImp::update(std::shared_ptr<Comm> comm)
    {
#ifdef GEOPM_DEBUG
        if (!m_is_connected) {
            throw Exception("ApplicationIOImp::" + std::string(__func__) +
                            " called before connect().",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        size_t length = 0;
        m_sampler->sample(m_prof_sample, length, comm);
        for (auto it = m_prof_sample.cbegin();
             it != m_prof_sample.cbegin() + length;
             ++it) {
            m_profile_event_buffer.insert(it->second);
        }
        m_sampler->tprof_table()->dump(m_thread_progress);
        m_profile_event_buffer.thread_progress(m_thread_progress);
    }

    void ApplicationIOImp::abort(void)
    {
        m_sampler->abort();
    }
}

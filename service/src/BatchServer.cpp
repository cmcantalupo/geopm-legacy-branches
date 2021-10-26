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
#include "BatchServer.hpp"

#include <cstdlib>
#include <cerrno>
#include <string>
#include <sstream>
#include <signal.h>
#include <unistd.h>
#include <wait.h>

#include "geopm/Exception.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/SharedMemory.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "BatchStatus.hpp"
#include "geopm_debug.hpp"


namespace geopm
{
    std::unique_ptr<BatchServer>
    BatchServer::make_unique(int client_pid,
                             const std::vector<geopm_request_s> &signal_config,
                             const std::vector<geopm_request_s> &control_config)
    {
        return geopm::make_unique<BatchServerImp>(client_pid, signal_config,
                                                  control_config);
    }
    BatchServerImp::BatchServerImp(int client_pid,
                                   const std::vector<geopm_request_s> &signal_config,
                                   const std::vector<geopm_request_s> &control_config)
        : BatchServerImp(client_pid, signal_config, control_config,
                         platform_io(), nullptr, nullptr, nullptr)
    {

    }

    BatchServerImp::BatchServerImp(int client_pid,
                                   const std::vector<geopm_request_s> &signal_config,
                                   const std::vector<geopm_request_s> &control_config,
                                   PlatformIO &pio,
                                   std::shared_ptr<BatchStatus> batch_status,
                                   std::shared_ptr<SharedMemory> signal_shmem,
                                   std::shared_ptr<SharedMemory> control_shmem)
        : m_client_pid(client_pid)
        , m_signal_config(signal_config)
        , m_control_config(control_config)
        , m_pio(pio)
        , m_signal_shmem(signal_shmem)
        , m_control_shmem(control_shmem)
        , m_batch_status(batch_status)
        , m_server_key(std::to_string(m_client_pid))
        , m_server_pid(0)
        , m_is_active(false)
    {
        bool is_test = (m_batch_status != nullptr);
        if (!is_test) {
            m_batch_status = BatchStatus::make_unique_server(m_client_pid, m_server_key);
        }
        else {
            GEOPM_DEBUG_ASSERT(m_signal_shmem != nullptr &&
                               m_control_shmem != nullptr,
                               "BatchServer: Called test constructor with null values");
        }
        if (!is_test) {
            // This is not a unit test, so actually do the fork()
            int parent_pid = getpid();
            int pipe_fd[2];
            int err = pipe(pipe_fd);
            check_return(err, "pipe(2)");
            int forked_pid = fork();
            check_return(forked_pid, "fork(2)");
            if (forked_pid == 0) {
                err = close(pipe_fd[0]);
                check_return(err, "close(2)");
                create_shmem();
                char msg = M_MESSAGE_CONTINUE;
                err = write(pipe_fd[1], &msg, 1);
                check_return(err, "write(2)");
                err = close(pipe_fd[1]);
                check_return(err, "close(2)");
                run_batch(parent_pid);
                exit(0);
            }
            err = close(pipe_fd[1]);
            check_return(err, "close(2)");
            char msg = '\0';
            err = read(pipe_fd[0], &msg, 1);
            check_return(err, "read(2)");
            if (msg != M_MESSAGE_CONTINUE) {
                throw Exception("BatchServerImp: Recievied unexpected message from batch server at startup: \"" +
                                std::to_string(msg) + "\"", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            err = close(pipe_fd[0]);
            check_return(err, "close(2)");
            m_server_pid = forked_pid;
        }
        m_is_active = true;
    }

    BatchServerImp::~BatchServerImp()
    {
        stop_batch();
    }

    int BatchServerImp::server_pid(void) const
    {
        return m_server_pid;
    }

    std::string BatchServerImp::server_key(void) const
    {
        return m_server_key;
    }

    void BatchServerImp::stop_batch(void)
    {
        if (m_is_active) {
            m_batch_status->send_message(M_MESSAGE_QUIT);
            m_batch_status->receive_message(M_MESSAGE_CONTINUE);
            m_is_active = false;
        }
    }

    void BatchServerImp::run_batch(int parent_pid)
    {

        push_requests();
        m_batch_status->send_message(M_MESSAGE_CONTINUE);
        // Start event loop
        bool do_cont = true;
        while (do_cont) {
            int message = m_batch_status->receive_message();
            switch (message) {
                case M_MESSAGE_READ:
                    read_and_update();
                    break;
                case M_MESSAGE_WRITE:
                    update_and_write();
                    break;
                case M_MESSAGE_QUIT:
                    do_cont = false;
                    break;
                default:
                    throw Exception("BatchServerImp::run_batch(): Recieved unknown response from client: " +
                                    std::to_string(message), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                    break;
            }
            m_batch_status->send_message(M_MESSAGE_CONTINUE);
        }
    }

    bool BatchServerImp::is_active(void) const
    {
        return m_is_active;
    }

    void BatchServerImp::push_requests(void)
    {
        for (const auto &req : m_signal_config) {
            m_signal_idx.push_back(
                m_pio.push_signal(req.name, req.domain, req.domain_idx));
        }
        for (const auto &req : m_control_config) {
            m_control_idx.push_back(
                m_pio.push_control(req.name, req.domain, req.domain_idx));
        }
    }

    void BatchServerImp::read_and_update(void)
    {
        if (m_signal_config.size() == 0) {
            return;
        }

        m_pio.read_batch();
        auto lock = m_signal_shmem->get_scoped_lock();
        double *buffer = (double *)m_signal_shmem->pointer();
        int buffer_idx = 0;
        for (const auto &idx : m_signal_idx) {
            buffer[buffer_idx] = m_pio.sample(idx);
            ++buffer_idx;
        }
    }

    void BatchServerImp::update_and_write(void)
    {
        if (m_control_config.size() == 0) {
            return;
        }

        auto lock = m_control_shmem->get_scoped_lock();
        double *buffer = (double *)m_control_shmem->pointer();
        int buffer_idx = 0;
        for (const auto &idx : m_control_idx) {
            m_pio.adjust(idx, buffer[buffer_idx]);
            ++buffer_idx;
        }
        m_pio.write_batch();
    }


    void BatchServerImp::create_shmem(void)
    {
        // Create shared memory regions
        size_t signal_size = m_signal_config.size() * sizeof(double);
        size_t control_size = m_control_config.size() * sizeof(double);
        std::string shmem_prefix = "/geopm-service-" + m_server_key;
        int uid = pid_to_uid(m_client_pid);
        int gid = pid_to_gid(m_client_pid);
        if (signal_size != 0) {
            m_signal_shmem = SharedMemory::make_unique_owner_secure(
                shmem_prefix + "-signals", signal_size);
            // Requires a chown if server is different user than client
            m_signal_shmem->chown(uid, gid);
        }
        if (control_size != 0) {
            m_control_shmem = SharedMemory::make_unique_owner_secure(
                shmem_prefix + "-controls", control_size);
            // Requires a chown if server is different user than client
            m_control_shmem->chown(uid, gid);
        }
    }

    void BatchServerImp::check_return(int ret, const std::string &func_name)
    {
        if (ret == -1) {
            throw Exception("BatchServerImp: System call failed: " + func_name,
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }
}

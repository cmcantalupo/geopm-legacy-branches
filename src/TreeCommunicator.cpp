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

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stddef.h>
#include <errno.h>
#include <pthread.h>
#include <algorithm>
#include <fstream>
#include <system_error>

#include "Exception.hpp"
#include "TreeCommunicator.hpp"
#include "GlobalPolicy.hpp"
#include "SharedMemory.hpp"
#include "geopm_message.h"
#include "geopm_env.h"
#include "config.h"

extern "C"
{
    static int geopm_comm_split_imp(MPI_Comm comm, const char *tag, int *num_node, MPI_Comm *split_comm, int *is_ctl_comm);

    int geopm_comm_split_ppn1(MPI_Comm comm, const char *tag, MPI_Comm *ppn1_comm)
    {
        int num_node = 0;
        int is_shm_root = 0;
        int err = geopm_comm_split_imp(comm, tag, &num_node, ppn1_comm, &is_shm_root);
        if (!err && !is_shm_root) {
            err = MPI_Comm_free(ppn1_comm);
            *ppn1_comm = MPI_COMM_NULL;
        }
        return err;
    }

    int geopm_comm_split_shared(MPI_Comm comm, const char *tag, MPI_Comm *split_comm)
    {
        int err = 0;
        struct stat stat_struct;
        try {
            std::string shmem_key(geopm_env_shmkey());
            shmem_key += "-comm-split-" + std::string(tag);
            std::string shmem_path("/dev/shm" + shmem_key);
            geopm::SharedMemory *shmem = NULL;
            geopm::SharedMemoryUser *shmem_user = NULL;
            int rank, color = -1;

            MPI_Comm_rank(comm, &rank);
            // remove shared memory file if one already exists
            (void)unlink(shmem_path.c_str());
            MPI_Barrier(comm);
            err = stat(shmem_path.c_str(), &stat_struct);
            if (!err || (err && errno != ENOENT)) {
                throw geopm::Exception("geopm_comm_split_shared(): " +
                                       std::string(shmem_key) +
                                       " already exists and cannot be deleted.",
                                       GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            MPI_Barrier(comm);
            try {
                shmem = new geopm::SharedMemory(shmem_key, sizeof(int));
            }
            catch (geopm::Exception ex) {
                if (ex.err_value() != EEXIST) {
                    throw ex;
                }
            }
            if (!shmem) {
                shmem_user = new geopm::SharedMemoryUser(shmem_key, 1);
            }
            else {
                color = rank;
                *((int*)(shmem->pointer())) = color;
            }
            MPI_Barrier(comm);
            if (shmem_user) {
                color = *((int*)(shmem_user->pointer()));
            }
            err = MPI_Comm_split(comm, color, rank, split_comm);
            delete shmem;
            delete shmem_user;
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_comm_split(MPI_Comm comm, const char *tag, MPI_Comm *split_comm, int *is_ctl_comm)
    {
        int num_node = 0;
        return geopm_comm_split_imp(comm, tag, &num_node, split_comm, is_ctl_comm);
    }

    static int geopm_comm_split_imp(MPI_Comm comm, const char *tag, int *num_node, MPI_Comm *split_comm, int *is_shm_root)
    {
        int err, comm_size, comm_rank, shm_rank;
        MPI_Comm shm_comm = MPI_COMM_NULL, tmp_comm = MPI_COMM_NULL;
        MPI_Comm *split_comm_ptr;

        *is_shm_root = 0;

        if (split_comm) {
            split_comm_ptr = split_comm;
        }
        else {
            split_comm_ptr = &tmp_comm;
        }

        err = MPI_Comm_size(comm, &comm_size);
        if (!err) {
            err = MPI_Comm_rank(comm, &comm_rank);
        }
        if (!err) {
            err = geopm_comm_split_shared(comm, tag, &shm_comm);
        }
        if (!err) {
            err = MPI_Comm_rank(shm_comm, &shm_rank);
        }
        if (!err) {
            if (!shm_rank) {
                *is_shm_root = 1;
            }
            else {
                *is_shm_root = 0;
            }
            err = MPI_Comm_split(comm, *is_shm_root, comm_rank, split_comm_ptr);
        }
        if (!err) {
            if (*is_shm_root == 1) {
                err = MPI_Comm_size(*split_comm_ptr, num_node);
            }
        }
        if (!err) {
            err = MPI_Bcast(num_node, 1, MPI_INT, 0, shm_comm);
        }
        if (shm_comm != MPI_COMM_NULL) {
            MPI_Comm_free(&shm_comm);
        }
        if (!split_comm) {
            MPI_Comm_free(split_comm_ptr);
        }
        return err;
    }
}

namespace geopm
{
    //////////////////////
    // Static constants //
    //////////////////////

    enum mpi_tag_e {
        GEOPM_SAMPLE_TAG,
        GEOPM_POLICY_TAG,
    };

    ///////////////////////////////
    // Helper for MPI exceptions //
    ///////////////////////////////
    void check_mpi(int err)
    {
        if (err) {
            char error_str[MPI_MAX_ERROR_STRING];
            int name_max = MPI_MAX_ERROR_STRING;
            MPI_Error_string(err, error_str, &name_max);
            throw Exception("MPI Error: " + std::string(error_str), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    /////////////////////////////////
    // Internal class declarations //
    /////////////////////////////////

    /// @brief TreeCommunicatorLevel class encapsulates communication functionality on
    /// a per-level basis.
    class TreeCommunicatorLevel
    {
        public:
            TreeCommunicatorLevel(MPI_Comm comm);
            ~TreeCommunicatorLevel();
            /// Check sample mailbox for each child and if all are full copy
            /// them into sample and post a new MPI_Get(), otherwise throw
            /// geopm::Exception with err_value() of
            /// GEOPM_ERROR_SAMPLE_INCOMPLETE
            void get_sample(std::vector<struct geopm_sample_message_s> &sample);
            /// Check policy mailbox and if full copy to m_policy and post a
            /// new MPI_Get(). If mailbox is empty set policy to last known
            /// policy.  If mailbox is empty and no policy has been set throw
            /// a geopm::Exception with err_value() of
            /// GEOPM_ERROR_POLICY_UNKNOWN.
            void get_policy(struct geopm_policy_message_s &policy);
            /// Send sample via MPI_Put() to root of level, skip if no
            /// recieve has been posted.
            void send_sample(const struct geopm_sample_message_s &sample);
            /// Send policy via MPI_Put() to all children, skip if no
            /// recieve has been posted.
            void send_policy(const std::vector<struct geopm_policy_message_s> &policy);
            /// Returns the level rank of the calling process.
            int level_rank(void);
            size_t overhead_send(void);
        protected:
            void create_window(void);
            void destroy_window(void);
            MPI_Comm m_comm;
            int m_size;
            int m_rank;
            struct geopm_sample_message_s *m_sample_mailbox;
            volatile struct geopm_policy_message_s m_policy_mailbox;
            MPI_Win m_sample_window;
            MPI_Win m_policy_window;
            size_t m_overhead_send;
            std::vector<struct geopm_policy_message_s> m_last_policy;
    };

    ///////////////////////////////////
    // TreeCommunicator public API's //
    ///////////////////////////////////

    TreeCommunicator::TreeCommunicator(const std::vector<int> &fan_out, GlobalPolicy *global_policy, const MPI_Comm &comm)
        : m_num_node(0)
        , m_fan_out(fan_out)
        , m_comm(fan_out.size())
        , m_global_policy(global_policy)
        , m_level(fan_out.size())
    {
        comm_create(comm);
        level_create();
        check_mpi(MPI_Comm_size(comm, &m_num_node));
        PMPI_Barrier(comm);
    }

    TreeCommunicator::~TreeCommunicator()
    {
        level_destroy();
        comm_destroy();
    }

    int TreeCommunicator::num_level(void) const
    {
        return m_num_level;
    }

    int TreeCommunicator::root_level(void) const
    {
        return m_fan_out.size();
    }

    int TreeCommunicator::level_rank(int level) const
    {
        return m_level[level]->level_rank();
    }

    int TreeCommunicator::level_size(int level) const
    {
        int result = 1;
        if (level < (int)m_fan_out.size()) {
            result = *(m_fan_out.end() - level - 1);
        }
        return result;
    }

    void TreeCommunicator::send_sample(int level, const struct geopm_sample_message_s &sample)
    {
        if (level < 0 || level >= num_level() || level == root_level()) {
            throw Exception("TreeCommunicator::send_sample()", GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        m_level[level]->send_sample(sample);
    }

    void TreeCommunicator::send_policy(int level, const std::vector<struct geopm_policy_message_s> &policy)
    {
        if (level < 0 || level >= num_level() || level == root_level()) {
            throw Exception("TreeCommunicator::send_policy()", GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        else {
            m_level[level]->send_policy(policy);
        }
    }

    void TreeCommunicator::get_sample(int level, std::vector<struct geopm_sample_message_s> &sample)
    {
        if (level <= 0 || level >= num_level()) {
            throw Exception("TreeCommunicator::get_sample()", GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        m_level[level - 1]->get_sample(sample);
    }

    void TreeCommunicator::get_policy(int level, struct geopm_policy_message_s &policy)
    {
        if (level < 0 || level >= num_level()) {
            throw Exception("TreeCommunicator::get_policy()", GEOPM_ERROR_LEVEL_RANGE, __FILE__, __LINE__);
        }
        if (level == root_level()) {
            m_global_policy->policy_message(policy);
            if (policy.power_budget > 0) {
                policy.power_budget *= m_num_node;
            }
        }
        else {
            m_level[level]->get_policy(policy);
        }
    }

    size_t TreeCommunicator::overhead_send(void)
    {
        size_t result = 0;
        for (auto it = m_level.begin(); it != m_level.end(); ++it) {
            result += (*it)->overhead_send();
        }
        return result;
    }


    //////////////////////////////////////
    // TreeCommunicator protected API's //
    //////////////////////////////////////

    void TreeCommunicator::comm_create(const MPI_Comm &comm)
    {
        int num_dim = m_fan_out.size();
        int color, key;
        MPI_Comm comm_cart;
        std::vector<int> flags(num_dim, 0);
        std::vector<int> coords(num_dim, 0);
        std::vector<int> parent_coords(num_dim, 0);
        int rank_cart;

        check_mpi(MPI_Cart_create(comm, num_dim, m_fan_out.data(), flags.data(), 1, &comm_cart));
        check_mpi(MPI_Comm_rank(comm_cart, &rank_cart));
        check_mpi(MPI_Cart_coords(comm_cart, rank_cart, num_dim, coords.data()));
        parent_coords = coords;

        bool is_all_zero = true;
        for (int i = 0; i != num_dim; ++i) {
            if (is_all_zero) {
                parent_coords[num_dim - 1 - i] = 0;
                MPI_Cart_rank(comm_cart, parent_coords.data(), &color);
                key = rank_cart;
            }
            else {
                color = MPI_UNDEFINED;
                key = 0;
            }

            check_mpi(MPI_Comm_split(comm_cart, color, key, &(m_comm[i])));

            if (coords[num_dim - 1 - i] != 0) {
               is_all_zero = false;
            }
        }
        check_mpi(MPI_Comm_free(&comm_cart));

        m_num_level = 0;
        for (auto comm_it = m_comm.begin();
             comm_it != m_comm.end() && *comm_it != MPI_COMM_NULL;
             ++comm_it) {
            m_num_level++;
        }

        m_comm.resize(m_num_level);

        if (m_global_policy) {
            m_num_level++;
        }

        if (rank_cart == 0 && m_global_policy == NULL) {
            throw Exception("process at root of tree communicator has not mapped the control file",
                            GEOPM_ERROR_CTL_COMM, __FILE__, __LINE__);
        }
        if (rank_cart != 0 && m_global_policy != NULL) {
            throw Exception("process not at root of tree communicator has mapped the control file",
                            GEOPM_ERROR_CTL_COMM, __FILE__, __LINE__);
        }
    }

    void TreeCommunicator::level_create(void)
    {
        if (num_level() == root_level() + 1) {
            m_level.resize(root_level());
        }
        else {
            m_level.resize(num_level());
        }

        auto comm_it = m_comm.begin();
        for (auto level_it = m_level.begin();
             level_it != m_level.end();
             ++level_it, ++comm_it) {
            *level_it = new TreeCommunicatorLevel(*comm_it);
        }
    }

    void TreeCommunicator::level_destroy(void)
    {
        for (auto level_it = m_level.rbegin();
             level_it != m_level.rend();
             ++level_it) {
            delete *level_it;
        }
    }

    void TreeCommunicator::comm_destroy(void)
    {
        for (auto comm_it = m_comm.begin(); comm_it != m_comm.end(); ++comm_it) {
            MPI_Comm_free(&(*comm_it));
        }
    }

    /////////////////////////////////
    // TreeCommunicatorLevel API's //
    /////////////////////////////////

    TreeCommunicatorLevel::TreeCommunicatorLevel(MPI_Comm comm)
        : m_comm(comm)
        , m_size(0)
        , m_rank(0)
        , m_sample_mailbox(NULL)
        , m_policy_mailbox(GEOPM_POLICY_UNKNOWN)
        , m_sample_window(MPI_WIN_NULL)
        , m_policy_window(MPI_WIN_NULL)
        , m_overhead_send(0)
    {
        check_mpi(MPI_Comm_size(m_comm, &m_size));
        check_mpi(MPI_Comm_rank(m_comm, &m_rank));
        if (!m_rank) {
            m_last_policy.resize(m_size, GEOPM_POLICY_UNKNOWN);
        }
        create_window();
    }

    TreeCommunicatorLevel::~TreeCommunicatorLevel()
    {
        destroy_window();
    }

    void TreeCommunicatorLevel::get_sample(std::vector<struct geopm_sample_message_s> &sample)
    {
        if (m_rank != 0) {
            throw Exception(std::string(__func__) + ": Only zero rank of the level can call sample",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        if (sample.size() < (size_t)m_size) {
            throw Exception(std::string(__func__) + ": Input sample vector too small",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        bool is_complete = true;
        check_mpi(MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, m_sample_window));
        for (int i = 0; is_complete && i < m_size; ++i) {
            if (m_sample_mailbox[i].region_id == 0) {
                is_complete = false;
            }
        }
        check_mpi(MPI_Win_unlock(0, m_sample_window));

        if (!is_complete) {
            throw Exception(__func__, GEOPM_ERROR_SAMPLE_INCOMPLETE, __FILE__, __LINE__);
        }

        check_mpi(MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, m_sample_window));
        std::copy(m_sample_mailbox, m_sample_mailbox + m_size, sample.begin());
        std::fill(m_sample_mailbox, m_sample_mailbox + m_size, GEOPM_SAMPLE_INVALID);
        check_mpi(MPI_Win_unlock(0, m_sample_window));
    }

    void TreeCommunicatorLevel::get_policy(struct geopm_policy_message_s &policy)
    {
        if (m_rank) {
            check_mpi(MPI_Win_lock(MPI_LOCK_SHARED, m_rank, 0, m_policy_window));
            policy = *((struct geopm_policy_message_s*)(&m_policy_mailbox));
            check_mpi(MPI_Win_unlock(m_rank, m_policy_window));
        }
        else {
            policy = *((struct geopm_policy_message_s *)(&m_policy_mailbox));
        }

        if (geopm_is_policy_equal(&policy, &GEOPM_POLICY_UNKNOWN)) {
            throw Exception("TreeCommunicatorLevel::get_policy",
                            GEOPM_ERROR_POLICY_UNKNOWN, __FILE__, __LINE__);
        }
    }

    void TreeCommunicatorLevel::send_sample(const struct geopm_sample_message_s &sample)
    {
        size_t msg_size = sizeof(struct geopm_sample_message_s);
        if (m_rank) {
            check_mpi(MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, m_sample_window));
#ifdef GEOPM_ENABLE_MPI3
            check_mpi(MPI_Put(&sample, msg_size, MPI_BYTE, 0, m_rank * msg_size,
                              msg_size, MPI_BYTE, m_sample_window));
#else
            check_mpi(MPI_Put((void *)&sample, msg_size, MPI_BYTE, 0, m_rank * msg_size,
                              msg_size, MPI_BYTE, m_sample_window));
#endif
            check_mpi(MPI_Win_unlock(0, m_sample_window));
            m_overhead_send += sizeof(struct geopm_sample_message_s);
        }
        else {
            *m_sample_mailbox = sample;
        }
    }

    void TreeCommunicatorLevel::send_policy(const std::vector<struct geopm_policy_message_s> &policy)
    {
        if (m_rank != 0) {
            throw Exception("Called send_policy() from rank not at root of level",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }

        *((struct geopm_policy_message_s *)(&m_policy_mailbox)) = policy[0];
        m_last_policy[0] = policy[0];
        size_t msg_size = sizeof(struct geopm_policy_message_s);
        int child_rank = 1;
        auto last_it = m_last_policy.begin() + 1;
        for (auto this_it = policy.begin() + 1;
             this_it != policy.end();
             ++this_it, ++last_it, ++child_rank) {
            if (!geopm_is_policy_equal(&(*this_it), &(*last_it))) {
                check_mpi(MPI_Win_lock(MPI_LOCK_EXCLUSIVE, child_rank, 0, m_policy_window));
#ifdef GEOPM_ENABLE_MPI3
                check_mpi(MPI_Put(&(*this_it), msg_size, MPI_BYTE, child_rank, 0,
                                  msg_size, MPI_BYTE, m_policy_window));
#else
                check_mpi(MPI_Put((void *)&(*this_it), msg_size, MPI_BYTE, child_rank, 0,
                                  msg_size, MPI_BYTE, m_policy_window));
#endif
                check_mpi(MPI_Win_unlock(child_rank, m_policy_window));
                *last_it = *this_it;
            }
        }
    }

    size_t TreeCommunicatorLevel::overhead_send(void)
    {
        return m_overhead_send;
    }

    int TreeCommunicatorLevel::level_rank(void)
    {
        return m_rank;
    }

    void TreeCommunicatorLevel::create_window(void)
    {
        // Create policy window
        size_t msg_size = sizeof(struct geopm_policy_message_s);
        check_mpi(MPI_Win_create((void *)(&m_policy_mailbox), msg_size, 1,
                                 MPI_INFO_NULL, m_comm, &m_policy_window));
        // Create sample window
        if (!m_rank) {
            size_t msg_size = sizeof(struct geopm_sample_message_s);
            check_mpi(MPI_Alloc_mem(m_size * msg_size, MPI_INFO_NULL, &m_sample_mailbox));
            std::fill(m_sample_mailbox, m_sample_mailbox + m_size, GEOPM_SAMPLE_INVALID);
            check_mpi(MPI_Win_create((void *)m_sample_mailbox, m_size * msg_size,
                                     1, MPI_INFO_NULL, m_comm, &m_sample_window));
        }
        else {
            check_mpi(MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, m_comm, &m_sample_window));
        }
    }

    void TreeCommunicatorLevel::destroy_window(void)
    {
        PMPI_Barrier(m_comm);
        // Destroy sample window
        check_mpi(MPI_Win_free(&m_sample_window));
        if (m_sample_mailbox) {
            MPI_Free_mem(m_sample_mailbox);
        }
        // Destroy policy window
        check_mpi(MPI_Win_free(&m_policy_window));
    }

    SingleTreeCommunicator::SingleTreeCommunicator(GlobalPolicy *global_policy)
        : m_policy(global_policy)
        , m_sample(GEOPM_SAMPLE_INVALID)
    {

    }

    SingleTreeCommunicator::~SingleTreeCommunicator()
    {

    }

    int SingleTreeCommunicator::num_level(void) const
    {
        return 1;
    }

    int SingleTreeCommunicator::root_level(void) const
    {
        return 0;
    }

    int SingleTreeCommunicator::level_rank(int level) const
    {
        return 0;
    }

    int SingleTreeCommunicator::level_size(int level) const
    {
        return 1;
    }

    void SingleTreeCommunicator::send_sample(int level, const struct geopm_sample_message_s &sample)
    {
        m_sample = sample;
    }

    void SingleTreeCommunicator::send_policy(int level, const std::vector<struct geopm_policy_message_s> &policy)
    {

    }

    void SingleTreeCommunicator::get_sample(int level, std::vector<struct geopm_sample_message_s> &sample)
    {
        sample[0] = m_sample;
    }

    void SingleTreeCommunicator::get_policy(int level, struct geopm_policy_message_s &policy)
    {
        m_policy->policy_message(policy);
    }

    size_t SingleTreeCommunicator::overhead_send(void)
    {
        return 0;
    }

}

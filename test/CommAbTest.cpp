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

#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

typedef int MPI_Op;
#define MPI_MAX     (MPI_Op)(0x58000001)
#define MPI_LAND    (MPI_Op)(0x58000005)
typedef int MPI_Comm;
typedef int MPI_Datatype;
#define MPI_COMM_WORLD ((MPI_Comm)0x44000000)
#define MPI_COMM_NULL      ((MPI_Comm)0x04000000)
#define MPI_LOCK_EXCLUSIVE  234
#define MPI_LOCK_SHARED     235
#define MPI_CHAR           ((MPI_Datatype)0x4c000101)
#define MPI_BYTE           ((MPI_Datatype)0x4c00010d)
#define MPI_INT            ((MPI_Datatype)0x4c000405)
#define MPI_DOUBLE         ((MPI_Datatype)0x4c00080b)
typedef long MPI_Aint;
typedef int MPI_Info;
#define MPI_INFO_NULL         ((MPI_Info)0x1c000000)
typedef int MPI_Win;
#define MPI_WIN_NULL ((MPI_Win)0x20000000)
#define MPI_MAX_ERROR_STRING   512

extern "C"
{
    std::vector<void *> g_params;
    std::vector<size_t> g_sizes;

    void reset()
    {
        for(auto it = g_params.begin(); it != g_params.end(); ++it) {
            if (*it) {
                free(*it);
                *it = NULL;
                //g_params.erase(it);
            }
        }
        g_params.clear();
        g_sizes.clear();
    }

    int mock_error_string(int param0, char *param1, int *param2)
    {
        *param2 = 0;
        return 0;
    }

    #define MPI_Error_string(p0, p1, p2) mock_error_string(p0, p1, p2)

    int mock_comm_dup(MPI_Comm param0, MPI_Comm *param1)
    {
        if (g_params.size() < 2) return 0;  //comm free test does not properly set these up
        memcpy(g_params[0], &param0, g_sizes[0]);
        memcpy(g_params[1], param1, g_sizes[1]);
        return 0;
    }

    #define MPI_Comm_dup(p0, p1) mock_comm_dup(p0, p1)
    #define PMPI_Comm_dup(p0, p1) mock_comm_dup(p0, p1)

    int mock_cart_create(MPI_Comm param0, int param1, const int param2[], const int param3[], int param4,
            MPI_Comm *param5)
    {
        if (g_params.size() < 6) return 0;          //cart rank and coord tests do not properly set up these params
        memcpy(g_params[0], &param0, g_sizes[0]);
        memcpy(g_params[1], &param1, g_sizes[1]);
        memcpy(g_params[2], param2, g_sizes[2]);
        memcpy(g_params[3], param3, g_sizes[3]);
        memcpy(g_params[4], &param4, g_sizes[4]);
        size_t tmp = (size_t) param5;
        memcpy(g_params[5], &tmp, g_sizes[5]);
        return 0;
    }

    #define MPI_Cart_create(p0, p1, p2, p3, p4, p5) mock_cart_create(p0, p1, p2, p3, p4, p5)
    #define PMPI_Cart_create(p0, p1, p2, p3, p4, p5) mock_cart_create(p0, p1, p2, p3, p4, p5)

    int mock_cart_rank(MPI_Comm param0, int param1[], int *param2)
    {
        memcpy(g_params[0], &param0, g_sizes[0]);
        memcpy(g_params[1], param1, g_sizes[1]);
        //param2 is output, stored on stack beneath API
        return 0;
    }

    #define MPI_Cart_rank(p0, p1, p2) mock_cart_rank(p0, p1, p2)
    #define PMPI_Cart_rank(p0, p1, p2) mock_cart_rank(p0, p1, p2)

    int mock_dims_create(int param0, int param1, int param2[])
    {
        memcpy(g_params[0], &param0, g_sizes[0]);
        memcpy(g_params[1], &param1, g_sizes[1]);
        memcpy(g_params[2], param2, g_sizes[2]);
        return 0;
    }

    #define MPI_Dims_create(p0, p1, p2) mock_dims_create(p0, p1, p2)
    #define PMPI_Dims_create(p0, p1, p2) mock_dims_create(p0, p1, p2)

    int mock_alloc_mem(MPI_Aint param0, MPI_Info param1, void **param2)
    {
        memcpy(g_params[0], &param0, g_sizes[0]);
        memcpy(g_params[1], &param1, g_sizes[1]);
        memcpy(g_params[2], param2, g_sizes[2]);
        return 0;
    }

    #define MPI_Alloc_mem(p0, p1, p2) mock_alloc_mem(p0, p1, p2)
    #define PMPI_Alloc_mem(p0, p1, p2) mock_alloc_mem(p0, p1, p2)

    int mock_free_mem(void *param0)
    {
        size_t tmp = (size_t) param0;
        memcpy(g_params[0], &tmp, g_sizes[0]);
        return 0;
    }

    #define MPI_Free_mem(p0) mock_free_mem(p0)
    #define PMPI_Free_mem(p0) mock_free_mem(p0)

    int mock_cart_coords(MPI_Comm param0, int param1, int param2, int param3[])
    {
        memcpy(g_params[0], &param0, g_sizes[0]);
        memcpy(g_params[1], &param1, g_sizes[1]);
        memcpy(g_params[2], &param2, g_sizes[2]);
        size_t tmp = (size_t) param3;
        memcpy(g_params[3], &tmp, g_sizes[3]);
        return 0;
    }

    #define MPI_Cart_coords(p0, p1, p2, p3) mock_cart_coords(p0, p1, p2, p3)
    #define PMPI_Cart_coords(p0, p1, p2, p3) mock_cart_coords(p0, p1, p2, p3)

    int mock_reduce(void *param0, void *param1, int param2, MPI_Datatype param3, MPI_Op param4, int param5, MPI_Comm param6)
    {
        size_t tmp;
        tmp = (size_t) param0;
        memcpy(g_params[0], &tmp, g_sizes[0]);
        tmp = (size_t) param1;
        memcpy(g_params[1], &tmp, g_sizes[1]);
        memcpy(g_params[2], &param2, g_sizes[2]);
        memcpy(g_params[3], &param3, g_sizes[3]);
        memcpy(g_params[4], &param4, g_sizes[4]);
        memcpy(g_params[5], &param5, g_sizes[5]);
        memcpy(g_params[6], &param6, g_sizes[6]);
        return 0;
    }

    #define MPI_Reduce(p0, p1, p2, p3, p4, p5, p6) mock_reduce(p0, p1, p2, p3, p4, p5, p6)
    #define PMPI_Reduce(p0, p1, p2, p3, p4, p5, p6) mock_reduce(p0, p1, p2, p3, p4, p5, p6)

    int mock_allreduce(const void *param0, void *param1, int param2, MPI_Datatype param3, MPI_Op param4, MPI_Comm param5)
    {
        // TODO, create test fixture.  abandoned for now because MPIComm.test takes away so much...
        size_t tmp;
        tmp = (size_t) param0;
        memcpy(g_params[0], &tmp, g_sizes[0]);
        tmp = (size_t) param1;
        memcpy(g_params[1], &tmp, g_sizes[1]);
        memcpy(g_params[2], &param2, g_sizes[2]);
        memcpy(g_params[3], &param3, g_sizes[3]);
        memcpy(g_params[4], &param4, g_sizes[4]);
        memcpy(g_params[5], &param5, g_sizes[5]);
        return 0;
    }

    #define MPI_Allreduce(p0, p1, p2, p3, p4, p5) mock_allreduce(p0, p1, p2, p3, p4, p5)
    #define PMPI_Allreduce(p0, p1, p2, p3, p4, p5) mock_allreduce(p0, p1, p2, p3, p4, p5)

    int mock_gather(const void *param0, int param1, MPI_Datatype param2, void *param3, int param4, MPI_Datatype param5, int param6, MPI_Comm param7)
    {
        size_t tmp;
        tmp = (size_t) param0;
        memcpy(g_params[0], &tmp, g_sizes[0]);
        memcpy(g_params[1], &param1, g_sizes[1]);
        memcpy(g_params[2], &param2, g_sizes[2]);
        tmp = (size_t) param3;
        memcpy(g_params[3], &tmp, g_sizes[3]);
        memcpy(g_params[4], &param4, g_sizes[4]);
        memcpy(g_params[5], &param5, g_sizes[5]);
        memcpy(g_params[6], &param6, g_sizes[6]);
        memcpy(g_params[7], &param7, g_sizes[7]);
        return 0;
    }

    #define MPI_Gather(p0, p1, p2, p3, p4, p5, p6, p7) mock_gather(p0, p1, p2, p3, p4, p5, p6, p7)
    #define PMPI_Gather(p0, p1, p2, p3, p4, p5, p6, p7) mock_gather(p0, p1, p2, p3, p4, p5, p6, p7)

    int mock_gatherv(const void *param0, int param1, MPI_Datatype param2, void *param3, const int *param4, const int *param5, MPI_Datatype param6, int param7, MPI_Comm param8)
    {
        size_t tmp;
        tmp = (size_t) param0;
        memcpy(g_params[0], &tmp, g_sizes[0]);
        memcpy(g_params[1], &param1, g_sizes[1]);
        memcpy(g_params[2], &param2, g_sizes[2]);
        tmp = (size_t) param3;
        memcpy(g_params[3], &tmp, g_sizes[3]);
        tmp = (size_t) param4;
        memcpy(g_params[4], &tmp, g_sizes[4]);
        tmp = (size_t) param5;
        memcpy(g_params[5], &tmp, g_sizes[5]);
        memcpy(g_params[6], &param6, g_sizes[6]);
        memcpy(g_params[7], &param7, g_sizes[7]);
        memcpy(g_params[8], &param8, g_sizes[8]);
        return 0;
    }

    #define MPI_Gatherv(p0, p1, p2, p3, p4, p5, p6, p7, p8) mock_gatherv(p0, p1, p2, p3, p4, p5, p6, p7, p8)
    #define PMPI_Gatherv(p0, p1, p2, p3, p4, p5, p6, p7, p8) mock_gatherv(p0, p1, p2, p3, p4, p5, p6, p7, p8)

    int mock_win_create(void *param0, MPI_Aint param1, int param2, MPI_Info param3, MPI_Comm param4, MPI_Win *param5)
    {
        size_t tmp;
        tmp = (size_t) param0;
        memcpy(g_params[0], &tmp, g_sizes[0]);
        memcpy(g_params[1], &param1, g_sizes[1]);
        memcpy(g_params[2], &param2, g_sizes[2]);
        memcpy(g_params[3], &param3, g_sizes[3]);
        memcpy(g_params[4], &param4, g_sizes[4]);
        tmp = (size_t) param5;
        memcpy(g_params[5], &tmp, g_sizes[5]);
        return 0;
    }

    #define MPI_Win_create(p0, p1, p2, p3, p4, p5) mock_win_create(p0, p1, p2, p3, p4, p5)
    #define PMPI_Win_create(p0, p1, p2, p3, p4, p5) mock_win_create(p0, p1, p2, p3, p4, p5)

    int mock_win_free(MPI_Win *param0)
    {
        size_t tmp = (size_t) param0;
        memcpy(g_params[0], &tmp, g_sizes[0]);
        return 0;
    }

    #define MPI_Win_free(p0) mock_win_free(p0)
    #define PMPI_Win_free(p0) mock_win_free(p0)

    int mock_win_lock(int param0, int param1, int param2, MPI_Win param3)
    {
        memcpy(g_params[0], &param0, g_sizes[0]);
        memcpy(g_params[1], &param1, g_sizes[1]);
        memcpy(g_params[2], &param2, g_sizes[2]);
        memcpy(g_params[3], &param3, g_sizes[3]);
        return 0;
    }

    #define MPI_Win_lock(p0, p1, p2, p3) mock_win_lock(p0, p1, p2, p3)
    #define PMPI_Win_lock(p0, p1, p2, p3) mock_win_lock(p0, p1, p2, p3)

    int mock_win_unlock(int param0, MPI_Win param1)
    {
        memcpy(g_params[0], &param0, g_sizes[0]);
        memcpy(g_params[1], &param1, g_sizes[1]);
        return 0;
    }

    #define MPI_Win_unlock(p0, p1) mock_win_unlock(p0, p1)
    #define PMPI_Win_unlock(p0, p1) mock_win_unlock(p0, p1)

    int mock_put(const void *param0, int param1, MPI_Datatype param2, int param3, MPI_Aint param4,
            int param5, MPI_Datatype param6, MPI_Win param7)
    {
        size_t tmp = (size_t) param0;
        memcpy(g_params[0], &tmp, g_sizes[0]);
        memcpy(g_params[1], &param1, g_sizes[1]);
        memcpy(g_params[2], &param2, g_sizes[2]);
        memcpy(g_params[3], &param3, g_sizes[3]);
        memcpy(g_params[4], &param4, g_sizes[4]);
        memcpy(g_params[5], &param5, g_sizes[5]);
        memcpy(g_params[6], &param6, g_sizes[6]);
        memcpy(g_params[7], &param7, g_sizes[7]);
        return 0;
    }

    #define MPI_Put(p0, p1, p2, p3, p4, p5, p6, p7) mock_put(p0, p1, p2, p3, p4, p5, p6, p7)
    #define PMPI_Put(p0, p1, p2, p3, p4, p5, p6, p7) mock_put(p0, p1, p2, p3, p4, p5, p6, p7)

    int mock_rank(MPI_Comm param0, int *param1)
    {
        memcpy(g_params[0], &param0, g_sizes[0]);
        memcpy(g_params[1], param1, g_sizes[1]);
        return 0;
    }

    #define MPI_Comm_rank(p0, p1) mock_rank(p0, p1)
    #define PMPI_Comm_rank(p0, p1) mock_rank(p0, p1)

    int mock_free(MPI_Comm *param0)
    {
        size_t tmp = (size_t) param0;
        memcpy(g_params[0], &tmp, g_sizes[0]);
        return 0;
    }

    #define MPI_Comm_free(p0) mock_free(p0)
    #define PMPI_Comm_free(p0) mock_free(p0)

    int mock_barrier(MPI_Comm param0)
    {
        memcpy(g_params[0], &param0, g_sizes[0]);
        return 0;
    }

    #define MPI_Barrier(p0) mock_barrier(p0)
    #define PMPI_Barrier(p0) mock_barrier(p0)

    int mock_comm_split(MPI_Comm param0, int param1, int param2, MPI_Comm *param3)
    {
        memcpy(g_params[0], &param0, g_sizes[0]);
        memcpy(g_params[1], &param1, g_sizes[1]);
        memcpy(g_params[2], &param2, g_sizes[2]);
        size_t tmp = (size_t) param3;
        memcpy(g_params[3], &tmp, g_sizes[3]);
        return 0;
    }

    #define MPI_Comm_split(p0, p1, p2, p3) mock_comm_split(p0, p1, p2, p3)
    #define PMPI_Comm_split(p0, p1, p2, p3) mock_comm_split(p0, p1, p2, p3)

    int mock_comm_size(MPI_Comm param0, int *param1)
    {
        return 0;
    }
    
    #define MPI_Comm_size(p0, p1) mock_comm_size(p0, p1)
    #define PMPI_Comm_size(p0, p1) mock_comm_size(p0, p1)

    int mock_bcast(void *param0, int param1, MPI_Datatype param2, int param3, MPI_Comm param4)
    {
        memcpy(g_params[0], param0, g_sizes[0]);
        memcpy(g_params[1], &param1, g_sizes[1]);
        memcpy(g_params[2], &param2, g_sizes[2]);
        memcpy(g_params[3], &param3, g_sizes[3]);
        memcpy(g_params[4], &param4, g_sizes[4]);
        return 0;
    }


    #define MPI_Bcast(p0, p1, p2, p3, p4) mock_bcast(p0, p1, p2, p3, p4)
    #define PMPI_Bcast(p0, p1, p2, p3, p4) mock_bcast(p0, p1, p2, p3, p4)
}

#include "gtest/gtest.h"
#define GEOPM_TEST
#include "MPIComm.cpp"

namespace geopm
{
class MPICommTestHelper : public MPIComm
{
    public:
    MPICommTestHelper()
        : MPIComm()
    {}
    MPICommTestHelper(const MPIComm *in_comm)
        : MPIComm(in_comm)
    {}
    MPICommTestHelper(const MPICommTestHelper *in_comm, int color, int key)
        : MPIComm((const MPIComm *) in_comm, color, key)
    {}
    MPICommTestHelper(const MPIComm *in_comm, std::vector<int> dimension, std::vector<int> periods, bool is_reorder)
        : MPIComm(in_comm, dimension, periods, is_reorder)
    { m_comm = MPI_COMM_WORLD; } // so is_valid() calls work as expected
    MPI_Comm * get_comm_ref() { return &m_comm; }
    MPI_Win * get_win_ref(size_t win_handle) { return &((CommWindow *) win_handle)->m_window; }
};

class CommAbTest: public :: testing :: Test
{
    protected:
        std::vector<void *> m_params;
        void SetUp();
        void TearDown();
        void check_params();
};

void CommAbTest::SetUp()
{
    reset();
}

void CommAbTest::TearDown()
{
    reset();
}

void CommAbTest::check_params()
{
    ASSERT_EQ(g_params.size(), m_params.size()) <<
        "Parameter checking failed at vector size comparison.";
    for (size_t x = 0; x < g_params.size(); x++) {
        int res = memcmp(g_params[x], m_params[x], g_sizes[x]);
        if (res) {
            printf("x is %lu", x);
        }
        EXPECT_EQ(0, res) <<
            "Parameter checking failed at parameter " << x << ".";
    }
}

// TODO remove void * in m_params.push_back calls... not needed...
// TODO do not reused size_t tmp in test fixtures, explicitly enumerate (tmp1, 2, etc.), why isn't gather failing?
TEST_F(CommAbTest, mpi_comm_rank)
{
    MPICommTestHelper tmp_comm;//no param constructor uses MPI_COMM_WORLD, others will dup causing failure
    int test_rank = 0; // interally MPIComm.rank init's tmp var to 0 which is passed to [P]MPI_Comm_rank

    g_sizes.push_back(sizeof(MPI_Comm));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(test_rank));
    g_params.push_back(malloc(g_sizes[1]));

    m_params.push_back(tmp_comm.get_comm_ref());
    m_params.push_back(&test_rank);
    
    tmp_comm.rank();

    check_params();
}

TEST_F(CommAbTest, mpi_reduce)
{
    MPICommTestHelper tmp_comm;
    size_t tmp;
    void *send = NULL;
    void *recv = NULL;
    size_t count = 1;
    MPI_Datatype dt = MPI_DOUBLE; // used beneath API
    MPI_Op op = MPI_MAX; // used beneath API
    int root = 0;

    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[1]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[2]));
    g_sizes.push_back(sizeof(MPI_Datatype));
    g_params.push_back(malloc(g_sizes[3]));
    g_sizes.push_back(sizeof(MPI_Op));
    g_params.push_back(malloc(g_sizes[4]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[5]));
    g_sizes.push_back(sizeof(MPI_Comm));
    g_params.push_back(malloc(g_sizes[6]));

    tmp = (size_t) send;
    m_params.push_back(&tmp);
    tmp = (size_t) recv;
    m_params.push_back(&tmp);
    m_params.push_back(&count);
    m_params.push_back(&dt);
    m_params.push_back(&op);
    m_params.push_back(&root);
    m_params.push_back(tmp_comm.get_comm_ref());
    
    tmp_comm.reduce_sum((double *) send, (double *) recv, count, root);

    check_params();
}

TEST_F(CommAbTest, mpi_gather)
{
    MPICommTestHelper tmp_comm;
    size_t tmp;
    void *send = NULL;
    void *recv = NULL;
    size_t count = 1;
    MPI_Datatype dt = MPI_INT; // used beneath API
    int root = 0;

    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[1]));
    g_sizes.push_back(sizeof(MPI_Datatype));
    g_params.push_back(malloc(g_sizes[2]));
    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[3]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[4]));
    g_sizes.push_back(sizeof(MPI_Datatype));
    g_params.push_back(malloc(g_sizes[5]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[6]));
    g_sizes.push_back(sizeof(MPI_Comm));
    g_params.push_back(malloc(g_sizes[7]));

    tmp = (size_t) send;
    m_params.push_back(&tmp);
    m_params.push_back(&count);
    m_params.push_back(&dt);
    tmp = (size_t) recv;
    m_params.push_back(&tmp);
    m_params.push_back(&count);
    m_params.push_back(&dt);
    m_params.push_back(&root);
    m_params.push_back(tmp_comm.get_comm_ref());
    
    tmp_comm.gather(send, count, recv, count, root);

    check_params();
}

/*
// TODO
TEST_F(CommAbTest, mpi_gatherv)
{
    MPICommTestHelper tmp_comm;
    size_t tmp;
    void *send = NULL;
    void *recv = NULL;
    size_t count = 1;
    MPI_Datatype dt = MPI_BYTE; // used beneath API
    int root = 0;

    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[1]));
    g_sizes.push_back(sizeof(MPI_Datatype));
    g_params.push_back(malloc(g_sizes[2]));
    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[3]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[4]));
    g_sizes.push_back(sizeof(MPI_Datatype));
    g_params.push_back(malloc(g_sizes[5]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[6]));
    g_sizes.push_back(sizeof(MPI_Comm));
    g_params.push_back(malloc(g_sizes[7]));

    tmp = (size_t) send;
    m_params.push_back(&tmp);
    m_params.push_back(&count);
    m_params.push_back(&dt);
    tmp = (size_t) recv;
    m_params.push_back(&tmp);
    m_params.push_back(&count);
    m_params.push_back(&dt);
    m_params.push_back(&root);
    m_params.push_back(tmp_comm.get_comm_ref());
    
    tmp_comm.gatherv(send, count, recv, rsizes, offsets, root);

    check_params();
}
*/

TEST_F(CommAbTest, mpi_broadcast)
{
    size_t val = 0xDEADBEEF;
    int size = sizeof(val);
    MPI_Datatype dt = MPI_BYTE;  // used internally in MPIComm.broadcast
    int root_rank = 0;

    g_sizes.push_back(size);
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(size));
    g_params.push_back(malloc(g_sizes[1]));
    g_sizes.push_back(sizeof(dt));
    g_params.push_back(malloc(g_sizes[2]));
    g_sizes.push_back(sizeof(root_rank));
    g_params.push_back(malloc(g_sizes[3]));
    g_sizes.push_back(sizeof(MPI_Comm));
    g_params.push_back(malloc(g_sizes[4]));
    
    MPICommTestHelper tmp_comm;

    m_params.push_back(&val);
    m_params.push_back(&size);
    m_params.push_back(&dt);
    m_params.push_back(&root_rank);
    m_params.push_back(tmp_comm.get_comm_ref());

    tmp_comm.broadcast(&val, size, root_rank);

    check_params();
}

TEST_F(CommAbTest, mpi_cart_create)
{
    MPICommTestHelper old_comm;
    int dims = 2;
    std::vector<int> vdims(dims, 16);
    std::vector<int> vpers(dims, 8);
    int reorder = 1;

    g_sizes.push_back(sizeof(MPI_Comm));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(dims));
    g_params.push_back(malloc(g_sizes[1]));
    g_sizes.push_back(sizeof(int) * dims);
    g_params.push_back(malloc(g_sizes[2]));
    g_sizes.push_back(sizeof(int) * dims);
    g_params.push_back(malloc(g_sizes[3]));
    g_sizes.push_back(sizeof(reorder));
    g_params.push_back(malloc(g_sizes[4]));
    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[5]));

    MPICommTestHelper tmp_comm(&old_comm, vdims, vpers, reorder);

    m_params.push_back(old_comm.get_comm_ref());
    m_params.push_back(&dims);
    m_params.push_back(vdims.data());
    m_params.push_back(vpers.data());
    m_params.push_back(&reorder);
    size_t tmp = (size_t) tmp_comm.get_comm_ref();
    m_params.push_back(&tmp);
    check_params();
}

TEST_F(CommAbTest, mpi_cart_rank)
{
    MPIComm old_comm;
    int dims = 2;
    std::vector<int> vdims(dims, 16);
    std::vector<int> vpers(dims, 8);
    std::vector<int> vcoords(dims, 4);
    int reorder = 1;

    g_sizes.push_back(sizeof(MPI_Comm));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(int) * dims);
    g_params.push_back(malloc(g_sizes[1]));

    MPICommTestHelper tmp_comm(&old_comm, vdims, vpers, reorder);

    m_params.push_back(tmp_comm.get_comm_ref());
    m_params.push_back(vcoords.data());

    tmp_comm.cart_rank(vcoords);

    check_params();
}

TEST_F(CommAbTest, mpi_cart_coord)
{
    MPIComm old_comm;
    int dims = 2;
    int rank = 0;
    std::vector<int> vdims(dims, 16);
    std::vector<int> vpers(dims, 8);
    std::vector<int> vcoords(dims, 4);
    int reorder = 1;

    MPICommTestHelper tmp_comm(&old_comm, vdims, vpers, reorder);

    g_sizes.push_back(sizeof(MPI_Comm));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[1]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[2]));
    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[3]));

    m_params.push_back(tmp_comm.get_comm_ref());
    m_params.push_back(&rank);
    m_params.push_back(&dims);
    size_t tmp = (size_t) vcoords.data();
    m_params.push_back(&tmp);

    tmp_comm.coordinate(rank, vcoords);

    check_params();
}

TEST_F(CommAbTest, mpi_dims_create)
{
    MPIComm comm;
    int nnodes = 9;
    int dims = 2;
    std::vector<int> vdims(dims, 16);

    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[1]));
    g_sizes.push_back(sizeof(int) * dims);
    g_params.push_back(malloc(g_sizes[2]));

    m_params.push_back(&nnodes);
    m_params.push_back(&dims);
    m_params.push_back(vdims.data());

    comm.dimension_create(nnodes, vdims);

    check_params();
}

TEST_F(CommAbTest, mpi_alloc_mem)
{
    MPIComm comm;
    MPI_Aint size = 16;
    MPI_Info info = MPI_INFO_NULL;
    void *base = NULL;

    g_sizes.push_back(sizeof(MPI_Aint));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(MPI_Info));
    g_params.push_back(malloc(g_sizes[1]));
    g_sizes.push_back(sizeof(void *));
    g_params.push_back(malloc(g_sizes[2]));

    m_params.push_back(&size);
    m_params.push_back(&info);
    m_params.push_back(&base);

    comm.alloc_mem((size_t) size, &base);

    check_params();
}

TEST_F(CommAbTest, mpi_free_mem)
{
    MPIComm comm;
    void *base = NULL;

    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[0]));

    m_params.push_back(&base);

    comm.free_mem(base);

    check_params();
}

TEST_F(CommAbTest, mpi_comm_dup)
{
    MPICommTestHelper old_comm;

    g_sizes.push_back(sizeof(MPI_Comm));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(MPI_Comm*));
    g_params.push_back(malloc(g_sizes[1]));

    MPICommTestHelper tmp_comm(&old_comm);

    m_params.push_back(old_comm.get_comm_ref());
    m_params.push_back(tmp_comm.get_comm_ref());
    check_params();
}

TEST_F(CommAbTest, mpi_comm_free)
{
    MPICommTestHelper old_comm;

    MPICommTestHelper *tmp_comm = new MPICommTestHelper(&old_comm);

    g_sizes.push_back(sizeof(MPI_Comm *));
    g_params.push_back(malloc(g_sizes[0]));

    size_t tmp = (size_t) tmp_comm->get_comm_ref();
    m_params.push_back(&tmp);

    delete tmp_comm;

    check_params();
}

TEST_F(CommAbTest, mpi_barrier)
{
    MPICommTestHelper comm;

    g_sizes.push_back(sizeof(MPI_Comm));
    g_params.push_back(malloc(g_sizes[0]));

    m_params.push_back(comm.get_comm_ref());

    comm.barrier();

    check_params();
}

TEST_F(CommAbTest, mpi_comm_split)
{
    MPICommTestHelper comm;

    g_sizes.push_back(sizeof(MPI_Comm));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[1]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[2]));
    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[3]));


    int color = 128;
    int key = 256;
    MPICommTestHelper test_comm(&comm, color, key);
    m_params.push_back(comm.get_comm_ref());
    m_params.push_back(&color);
    m_params.push_back(&key);
    size_t tmp = (size_t) test_comm.get_comm_ref();
    m_params.push_back(&tmp);

    check_params();
}

TEST_F(CommAbTest, mpi_win_ops)
{
    MPICommTestHelper tmp_comm;

    // win create
    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(MPI_Aint));
    g_params.push_back(malloc(g_sizes[1]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[2]));
    g_sizes.push_back(sizeof(MPI_Info));
    g_params.push_back(malloc(g_sizes[3]));
    g_sizes.push_back(sizeof(MPI_Comm));
    g_params.push_back(malloc(g_sizes[4]));
    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[5]));

    int input  = 0;
    size_t tmp = (size_t) &input;
    size_t input_size = sizeof(input);
    MPI_Info info = MPI_INFO_NULL; // used beneath API
    MPI_Aint disp = 1; // used beneath API
    size_t win_handle = tmp_comm.create_window(input_size, &input);

    m_params.push_back(&tmp);
    m_params.push_back(&input_size);
    m_params.push_back(&disp);
    m_params.push_back(&info);
    m_params.push_back(tmp_comm.get_comm_ref());
    size_t tmp2 = (size_t) tmp_comm.get_win_ref(win_handle);
    m_params.push_back(&tmp2);
    
    check_params();
    reset();
    m_params.clear();

    int rank = 0;
    int ex;
    for (int exclusive = 0; exclusive < 2; exclusive++) {
        for (int assert = 0; assert < 2; assert++) {
            g_sizes.push_back(sizeof(int));
            g_params.push_back(malloc(g_sizes[0]));
            g_sizes.push_back(sizeof(int));
            g_params.push_back(malloc(g_sizes[1]));
            g_sizes.push_back(sizeof(int));
            g_params.push_back(malloc(g_sizes[2]));
            g_sizes.push_back(sizeof(MPI_Win));
            g_params.push_back(malloc(g_sizes[3]));

            tmp_comm.lock_window(win_handle, exclusive, rank, assert);

            ex = exclusive ? MPI_LOCK_EXCLUSIVE : MPI_LOCK_SHARED;
            m_params.push_back(&ex);
            m_params.push_back(&rank);
            m_params.push_back(&assert);
            m_params.push_back((void *) tmp2);

            check_params();
            reset();
            m_params.clear();
        }
    }

    // put
    MPI_Datatype dt = MPI_BYTE;  // used beneath API
    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[1]));
    g_sizes.push_back(sizeof(dt));
    g_params.push_back(malloc(g_sizes[2]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[3]));
    g_sizes.push_back(sizeof(MPI_Aint));
    g_params.push_back(malloc(g_sizes[4]));
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[5]));
    g_sizes.push_back(sizeof(dt));
    g_params.push_back(malloc(g_sizes[6]));
    g_sizes.push_back(sizeof(MPI_Win));
    g_params.push_back(malloc(g_sizes[7]));

    tmp_comm.window_put(&input, input_size, rank, disp, win_handle);

    m_params.push_back(&tmp);
    m_params.push_back(&input_size);
    m_params.push_back(&dt);
    m_params.push_back(&rank);
    m_params.push_back(&disp);
    m_params.push_back(&input_size);
    m_params.push_back(&dt);
    m_params.push_back(tmp_comm.get_win_ref(win_handle));

    check_params();
    reset();
    m_params.clear();

    // unlock
    g_sizes.push_back(sizeof(int));
    g_params.push_back(malloc(g_sizes[0]));
    g_sizes.push_back(sizeof(MPI_Win));
    g_params.push_back(malloc(g_sizes[3]));

    m_params.push_back(&rank);
    m_params.push_back((void *) tmp2);

    tmp_comm.unlock_window(win_handle, rank);

    check_params();
    reset();
    m_params.clear();

    // win destroy
    g_sizes.push_back(sizeof(size_t));
    g_params.push_back(malloc(g_sizes[0]));

    m_params.push_back(&tmp2);

    tmp_comm.destroy_window(win_handle);

    check_params(); }
}


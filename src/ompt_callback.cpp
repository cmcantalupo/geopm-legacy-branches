/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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
#include "OMPT.hpp"
#include "geopm.h"
#include <omp-tools.h>
#include <iostream>


extern "C"
{

    static void on_ompt_event_parallel_begin(ompt_data_t *encountering_task_data,
                                             const ompt_frame_t *encountering_task_frame,
                                             ompt_data_t *parallel_data,
                                             unsigned int requested_parallelism,
                                             int flags,
                                             const void *parallel_function)
    {
        geopm::OMPT::ompt().region_enter(parallel_function);
    }


    static void on_ompt_event_parallel_end(ompt_data_t *parallel_data,
                                           ompt_data_t *encountering_task_data,
                                           int flags,
                                           const void *parallel_function)
    {
        geopm::OMPT::ompt().region_exit(parallel_function);
    }

    static void on_ompt_event_work(ompt_work_t wstype,
                                   ompt_scope_endpoint_t endpoint,
                                   ompt_data_t *parallel_data,
                                   ompt_data_t *task_data,
                                   uint64_t count,
                                   const void *parallel_function)
    {
std::cerr << "geopm_tprof_init(" << count << ")\n";
        geopm_tprof_init(count);
    }

    static void on_ompt_event_dispatch(ompt_data_t *parallel_data,
                                       ompt_data_t *task_data,
                                       ompt_dispatch_t kind,
                                       ompt_data_t instance)
    {
std::cerr << "geopm_tprof_post()\n";
        geopm_tprof_post();
    }

    int ompt_initialize(ompt_function_lookup_t lookup,
                        int initial_device_num,
                        ompt_data_t *tool_data)
    {
        if (geopm::OMPT::ompt().is_enabled()) {
            ompt_set_callback_t ompt_set_callback = (ompt_set_callback_t) lookup("ompt_set_callback");
            ompt_set_callback(ompt_callback_parallel_begin, (ompt_callback_t) &on_ompt_event_parallel_begin);
            ompt_set_callback(ompt_callback_parallel_end, (ompt_callback_t) &on_ompt_event_parallel_end);
            ompt_set_callback(ompt_callback_work, (ompt_callback_t) &on_ompt_event_work);
            ompt_set_callback(ompt_callback_dispatch, (ompt_callback_t) &on_ompt_event_dispatch);
        }
        // OpenMP 5.0 standard says return non-zero on success!?!?!
        return 1;
    }

    void ompt_finalize(ompt_data_t *data)
    {

    }

    ompt_start_tool_result_t *ompt_start_tool(unsigned int omp_version, const char *runtime_version)
    {
        static ompt_start_tool_result_t ompt_start_tool_result = {&ompt_initialize,
                                                                  &ompt_finalize,
                                                                  {}};
        return &ompt_start_tool_result;
    }
}

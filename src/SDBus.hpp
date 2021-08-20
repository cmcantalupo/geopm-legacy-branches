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

#ifndef SDBUS_HPP_INCLUDE
#define SDBUS_HPP_INCLUDE


namespace geopm
{
    class SDBus
    {
        public:
            SDBus() = default;
            virtual ~SDBus() = default;
            static std::unique_ptr<SDBus> make_unique(void);
            virtual std::shared_ptr<SDBusMessage> call(
                std::shared_ptr<SDBusMessage> message,
                double timeout) = 0;
            virtual std::shared_ptr<SDBusMessage> call_method(
                const std::string &destination,
                const std::string &path,
                const std::string &interface,
                const std::string &member,
                std::shared_ptr<SDBusMessage> message) = 0;
            virtual std::shared_ptr<SDBusMessage> call_method(
                const std::string &destination,
                const std::string &path,
                const std::string &interface,
                const std::string &member,
                std::shared_ptr<SDBusMessage> message,
                const std::string &arg0,
                int arg1,
                int arg2) = 0;
            virtual std::shared_ptr<SDBusMessage> call_method(
                const std::string &destination,
                const std::string &path,
                const std::string &interface,
                const std::string &member,
                std::shared_ptr<SDBusMessage> message,
                const std::string &arg0,
                int arg1,
                int arg2,
                double arg3) = 0;
            virtual std::shared_ptr<SDBusMessage> make_call_message(
                const std::string &destination,
                const std::string &path,
                const std::string &interface,
                const std::string &member) = 0;
    }

    class SDBusMessage
    {
        public:
            SDBusMessage() = default;
            virtual ~SDBusMessage() = default;
            virtual void enter_container(
                char type,
                const std::string &contents) = 0;
            virtual void exit_container(void) = 0;
            virtual std::string read_string(void) = 0;
            virtual double read_double(void) = 0;
            virtual int read_integer(void) = 0;
            virtual void append_strings(
                const std::vector<std::string> > write_values) = 0;
    }
}

#endif

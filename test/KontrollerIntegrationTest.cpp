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

#include <vector>
#include <memory>
#include <iostream>
#include <sstream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Kontroller.hpp"
#include "Agent.hpp"
#include "MonitorAgent.hpp"
#include "ApplicationIO.hpp"

#include "MockPlatformTopo.hpp"
#include "MockPlatformIO.hpp"
#include "MockComm.hpp"
//#include "MockTreeComm.hpp"
#include "Helper.hpp"

// for mocks
#include "TreeComm.hpp"
#include "Reporter.hpp"
#include "Tracer.hpp"
#include "PlatformIO.hpp"
#include "Comm.hpp"
#include "geopm_message.h"
#include "geopm_hash.h"

using geopm::Kontroller;
using geopm::IPlatformIO;
using geopm::IPlatformTopo;
using geopm::ApplicationIO;
using geopm::IComm;
using testing::NiceMock;
using testing::_;
using testing::Return;
using testing::AtLeast;

class MockTreeComm : public geopm::ITreeComm
{
    public:
        MOCK_CONST_METHOD0(num_level_controlled,
                           int(void));
        MOCK_CONST_METHOD0(root_level,
                           int(void));
        MOCK_CONST_METHOD1(level_rank,
                           int(int level));
        /// @todo fix this with ctor
        int level_size(int level) const override
        {
            return 1;
        }
        void send_up(int level, const std::vector<double> &sample) override
        {
            std::cout << "tree send up to level " << level+1 << std::endl;
            m_data_sent_up[level+1] = sample;
        }
        void send_down(int level, const std::vector<std::vector<double> > &policy) override
        {
            std::cout << "tree send down to level " << level-1 << std::endl;
            m_data_sent_down[level-1] = policy[0]; /// @todo slightly wrong
        }
        bool receive_up(int level, std::vector<std::vector<double> > &sample)
        {
            std::cout << "tree recv up level " << level << std::endl;
            for (auto &vec : sample) {
                vec = m_data_sent_up.at(level);
            }
            return true;
        }
        bool receive_down(int level, std::vector<double> &policy)
        {
            std::cout << "tree recv down level " << level << std::endl;
            policy = m_data_sent_down.at(level);
            return true;
        }
        MOCK_CONST_METHOD0(overhead_send,
                     size_t(void));
        MOCK_METHOD1(broadcast_string,
                     void(const std::string &str));
        MOCK_METHOD0(broadcast_string,
                     std::string(void));

    private:
        // map from level -> last sent data
        std::map<int, std::vector<double> > m_data_sent_up;
        std::map<int, std::vector<double> > m_data_sent_down;

};

class MockReporter : public geopm::IReporter
{
    public:
        void generate(const std::string &agent_name,
                      const std::string &agent_report_header,
                      const std::string &agent_node_report,
                      const std::map<uint64_t, std::string> &agent_region_report,
                      const geopm::IApplicationIO &application_io,
                      std::shared_ptr<geopm::IComm> comm) override
        {
            std::ostringstream oss;
            oss << "----" << std::endl;
            oss << "my report" << std::endl;
            oss << "profile: " << std::endl;
            oss << "agent: " << agent_name << std::endl;
            oss << "  " << agent_report_header << std::endl;
            oss << "node" << std::endl;
            oss << agent_node_report << std::endl;
            for (auto region_name : application_io.region_name_set()) {
                uint64_t region_id = geopm_crc32_str(0, region_name.c_str());
                oss << region_name << " (" << region_id << ")" << std::endl;
                if (agent_region_report.find(region_id) != agent_region_report.end()) {
                    oss << "  " << agent_region_report.at(region_id) << std::endl;
                }
            }
            oss << "----" << std::endl;
            std::cout << oss.str();
        }

};

class MockTracer : public geopm::ITracer
{
    public:
        void update(const std::vector <struct geopm_telemetry_message_s> &telemetry) override {}
        void update(const struct geopm_policy_message_s &policy) override {}
        void columns(const std::vector<geopm::IPlatformIO::m_request_s> &cols) override {}
        void update(bool is_epoch) override {};
        void flush(void) override
        {
            std::cout << "flush trace" << std::endl;
            std::cout << "----" << std::endl;
            std::cout << stream.str() << std::endl;
            std::cout << "----" << std::endl;
        }
    private:
        std::ostringstream stream;
};

class SpyAgent : public geopm::IAgent
{
    public:
        void init(int level) override
        {
            m_level = level;
        }
        void descend(const std::vector<double> &in_policy,
                     std::vector<std::vector<double> >&out_policy) override
        {
            std::cout << "Agent " << m_level << " descend()" << std::endl;
        }
        void ascend(const std::vector<std::vector<double> > &in_signal,
                    std::vector<double> &out_signal) override
        {
            std::cout << "Agent " << m_level << " ascend()" << std::endl;
        }
        void adjust_platform(const std::vector<double> &in_policy) override
        {
            std::cout << "Agent " << m_level << " adjust_platform(): ";
            for (auto pol : in_policy) {
                std::cout << pol << " ";
            }
            std::cout << std::endl;
        }
        void sample_platform(std::vector<double> &out_sample) override
        {
            std::cout << "Agent " << m_level << " sample_platform()" << std::endl;
            // Pretend to read samples
            out_sample[0] = 4.0;
            out_sample[1] = 6.0;
            out_sample[2] = 8.0;
        }
        void wait(void) override
        {
            std::cout << "Agent " << m_level << " wait()" << std::endl;
        }
        std::vector<std::string> policy_names(void)
        {
            std::cout << "Agent " << m_level << " policy_names()" << std::endl;
            return {"POL1", "POL2"};
        }
        std::vector<std::string> sample_names(void)
        {
            std::cout << "Agent " << m_level << " sample_names()" << std::endl;
            return {"SIG1", "SIG2", "SIG3"};
        }
        std::string report_header(void)
        {
            //std::cout << "Agent " << m_level << " report_header()" << std::endl;
            return "agent header";
        }
        std::string report_node(void)
        {
            //std::cout << "Agent " << m_level << " report_node()" << std::endl;
            return "agent node report";
        }
        std::map<uint64_t, std::string> report_region(void)
        {
            std::cout << "Agent " << m_level << " report_region()" << std::endl;
            return {};
        }
        std::vector<geopm::IPlatformIO::m_request_s> trace_columns(void)
        {
            std::cout << "Agent " << m_level << " trace_columns()" << std::endl;
            return {};
        }
    private:
        int m_level = -1;
};

class KontrollerTestMockPlatformIO : public MockPlatformIO
{
    public:
        KontrollerTestMockPlatformIO()
        {
            ON_CALL(*this, agg_function(_))
                .WillByDefault(Return(IPlatformIO::agg_sum));

            // any other "unsupported" signals
            ON_CALL(*this, push_signal(_, _, _))
                .WillByDefault(Return(-1));
            ON_CALL(*this, sample(-1))
                .WillByDefault(Return(NAN));
        }
        void add_supported_signal(IPlatformIO::m_request_s signal, double default_value)
        {
            ON_CALL(*this, push_signal(signal.name, signal.domain_type, signal.domain_idx))
                .WillByDefault(Return(m_index));
            ON_CALL(*this, sample(m_index))
                .WillByDefault(Return(default_value));
            ON_CALL(*this, read_signal(signal.name, signal.domain_type, signal.domain_idx))
                .WillByDefault(Return(default_value));
            ++m_index;
        }

    private:
        int m_index = 0;
};



class KontrollerIntegrationTest : public ::testing::Test
{
    protected:
        void SetUp();

        NiceMock<MockPlatformTopo> m_topo;
        //NiceMock<MockPlatformIO> m_platform_io;
        KontrollerTestMockPlatformIO m_platform_io;
        std::string m_agent_name = "temp";
        int m_num_send_up = 4;//3;
        int m_num_send_down = 2;
        int m_num_level_ctl = 2;
        int m_root_level = 1;
        std::shared_ptr<MockComm> m_comm;
        std::vector<std::unique_ptr<geopm::IAgent> > m_level_agent;
        std::map<std::string, double> m_manager_policy;

        int m_num_step = 3;
};

void KontrollerIntegrationTest::SetUp()
{
    m_comm = std::make_shared<MockComm>();
    // static policy agent signals
    m_platform_io.add_supported_signal({"TIME", IPlatformTopo::M_DOMAIN_BOARD, 0}, 99);
    m_platform_io.add_supported_signal({"POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0}, 4545);
    m_platform_io.add_supported_signal({"FREQUENCY", IPlatformTopo::M_DOMAIN_BOARD, 0}, 333);
    m_platform_io.add_supported_signal({"REGION_PROGRESS", IPlatformTopo::M_DOMAIN_BOARD, 0}, 0.5);

    // toggle to suppress warnings; this is not the correct set of expect call for this test
#if 1
    EXPECT_CALL(m_platform_io, push_signal(_, _, _)).Times(AtLeast(0));
    EXPECT_CALL(m_platform_io, sample(_)).Times(AtLeast(0));
    EXPECT_CALL(m_platform_io, read_signal(_, _, _)).Times(AtLeast(0));
    EXPECT_CALL(m_platform_io, agg_function(_)).Times(AtLeast(0));
    EXPECT_CALL(m_platform_io, push_control(_, _, _)).Times(AtLeast(0));
    EXPECT_CALL(m_platform_io, adjust(_, _)).Times(AtLeast(0));
#endif

    for (int level = 0; level < m_num_level_ctl; ++level) {
        auto tmp = new geopm::MonitorAgent(m_platform_io, m_topo);
        //auto tmp = new SpyAgent();
        tmp->init(level);
        m_level_agent.emplace_back(tmp);
    }

    m_manager_policy = {{"FREQUENCY", 2.3e9}, {"POWER", 222}};
}

TEST_F(KontrollerIntegrationTest, main)
{
    auto m_application_io = std::make_shared<ApplicationIO>("/test_shm");
    Kontroller kontroller(m_comm, m_topo, m_platform_io,
                          m_agent_name, m_num_send_down, m_num_send_up,
                          geopm::make_unique<MockTreeComm>(),
                          m_num_level_ctl, m_root_level,
                          m_application_io,
                          geopm::make_unique<NiceMock<MockReporter> >(),
                          geopm::make_unique<NiceMock<MockTracer> >(),
                          std::move(m_level_agent),
                          m_manager_policy);

    EXPECT_CALL(m_platform_io, read_batch()).Times(m_num_step);
    EXPECT_CALL(m_platform_io, write_batch()).Times(m_num_step);

    kontroller.setup_trace();
    for (int step = 0; step < m_num_step; ++step) {
        kontroller.step();
    }

    kontroller.generate();

}

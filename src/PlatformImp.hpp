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

#ifndef PLATFORMIMP_HPP_INCLUDE
#define PLATFORMIMP_HPP_INCLUDE

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

#include <sys/types.h>
#include <stdint.h>
#include <vector>
#include <set>
#include <map>
#include <utility>
#include <string>

#include "PlatformTopology.hpp"

namespace geopm
{

#ifndef X86_IOC_MSR_BATCH
#define  X86_IOC_MSR_BATCH _IOWR('c', 0xA2, m_msr_batch_array)
#endif

    struct geopm_signal_descriptor {
        int device_type;
        int device_index;
        int signal_type;
        double value;
    };

    /* Platform IDs
    ((family << 8) + model)
    0x62A - Sandy Bridge
    0x62D - Sandy Bridge E
    0x63A - Ivy Bridge
    0x63E - Ivy Bridge E
    0x63C - Haswell
    0x645 - Haswell
    0x646 - Haswell
    0x63f - Haswell E
    */
    /// @brief This class provides an abstraction of specific functionality
    /// and attributes of different hardware implementations. It holds
    /// the platform topology of the underlying hardware as well as
    /// address offsets of Model Specific Registers.
    class PlatformImp
    {
        public:
            /// @brief default PlatformImp constructor
            PlatformImp();
            PlatformImp(int num_energy_signal, int num_counter_signal,
                        const std::map<int, double> &control_latency,
                        const std::map<std::string, std::pair<off_t, unsigned long> > *msr_map);
            PlatformImp(const PlatformImp &other);
            /// @brief default PlatformImp destructor
            virtual ~PlatformImp();

            ////////////////////////////////////////////////////////////////////
            //                     Topology Information                       //
            ////////////////////////////////////////////////////////////////////
            virtual int num_energy_signal(void) const;
            /// @brief Retrieve the number of per-cpu signals
            /// @return number of per-cpu signals.
            virtual int num_counter_signal(void) const;
            virtual int num_control_domain(int control_type) const;
            virtual int num_counter_domain(int counter_type) const;
            virtual double control_latency_ms(int control_type) const;
            /// @brief Gives the domain type for specified control type.
            /// @return The domain type.
            virtual int domain_type(int control_type) const;
            /// @brief Return the TDP of a single package.
            double package_tdp(void) const;
            ////////////////////////////////////////////////////////////////////
            //                     MSR read/write support                     //
            ////////////////////////////////////////////////////////////////////
            /// @brief Write a value to a Model Specific Register.
            /// @param [in] device_type enum device type can be
            ///        one of GEOPM_DOMAIN_PACKAGE, GEOPM_DOMAIN_CPU,
            ///        GEOPM_DOMAIN_TILE, or GEOPM_DOMAIN_BOARD_MEMORY.
            /// @param [in] device_index Numbered index of the specified type.
            /// @param [in] msr_name String name of the requested MSR.
            /// @param [in] value Value to write to the specified MSR.
            void msr_write(int device_type, int device_index, const std::string &msr_name, uint64_t value);
            /// @brief Read a value from a Model Specific Register.
            /// @param [in] device_type enum device type can be
            ///        one of GEOPM_DOMAIN_PACKAGE, GEOPM_DOMAIN_CPU,
            ///        GEOPM_DOMAIN_TILE, or GEOPM_DOMAIN_BOARD_MEMORY.
            /// @param [in] device_index Numbered index of the specified type.
            /// @param [in] msr_name String name of the requested MSR.
            /// @return Value read from the specified MSR.
            uint64_t msr_read(int device_type, int device_index, const std::string &msr_name);
            /// @brief Output a MSR whitelist for use with the Linux MSR driver.
            /// @param [in] file_desc File descriptor for output.
            void whitelist(FILE* file_desc);
            /// @brief Initialize the topology and hardware counters.
            virtual void initialize(void);
            /// @brief Write to a file the current state of RAPL, per-CPU counters,
            /// and free running counters.
            /// @param [in] path The path of the file to write.
            virtual void save_msr_state(const char *path);
            /// @brief Read in MSR state for RAPL, per-CPU counters,
            /// and free running counters and set them to that
            /// state.
            /// @param [in] path The path of the file to read in.
            void restore_msr_state(const char *path);
            /// @brief Revert the MSR values to their initial state.
            void revert_msr_state(void);
            /// @brief Return if sample is updated.
            virtual bool is_updated(void);

            ////////////////////////////////////////////////////////////////////
            //              Platform dependent implementations                //
            ////////////////////////////////////////////////////////////////////
            /// @brief Does this PlatformImp support a specific platform.
            /// @param [in] platform_id Platform identifier specific to the
            ///        underlying hardware. On x86 platforms this can be obtained by
            ///        the cpuid instruction.
            /// @return true if this PlatformImp supports platform_id,
            ///         else false.
            virtual bool is_model_supported(int platform_id) = 0;
            /// @brief Retrieve the string name of the underlying platform.
            /// @return Underlying platform name.
            virtual std::string platform_name(void) = 0;
            /// @brief Read and transform a signal.
            /// Read a signal value from the hw platform and do any transformation
            /// needed to convert it to the expected output format and units.
            /// @param [in] device_type enum device type can be
            ///        one of GEOPM_DOMAIN_PACKAGE, GEOPM_DOMAIN_CPU,
            ///        GEOPM_DOMAIN_TILE, or GEOPM_DOMAIN_BOARD_MEMORY.
            /// @param [in] device_index Numbered index of the specified type.
            /// @param [in] signal_type enum signal type of geopm_telemetry_type_e.
            ///        The signal typr to return.
            /// @return The read and transformed value.
            virtual double read_signal(int device_type, int device_index, int signal_type) = 0;
            /// @brief Batch read multiple signal values.
            /// @param [in/out] signal_desc A vector of descriptors for each read operation.
            /// @param [in] is_changed Has the data in the signal_desc changed since the last call?
            ///             This enables the method to reuse the already created data structures
            ///             if the same data is being requested repeatedly. This must be set to
            ///             false the first time this method is called.
            virtual void batch_read_signal(std::vector<struct geopm_signal_descriptor> &signal_desc, bool is_changed) = 0;
            /// @brief Transform and write a value to a hw platform control.
            /// Transform a given a control value from the given format to the format
            /// and units expected by the hw platform. Write the transformed value to
            /// a controll on the hw platform.
            /// @param [in] device_type enum device type can be
            ///        one of GEOPM_DOMAIN_PACKAGE, GEOPM_DOMAIN_CPU,
            ///        GEOPM_DOMAIN_TILE, or GEOPM_DOMAIN_BOARD_MEMORY.
            /// @param [in] device_index Numbered index of the specified type.
            /// @param [in] signal_type enum signal type of geopm_telemetry_type_e.
            ///        The control type to write to.
            /// @param [in] value The value to be transformed and written.
            virtual void write_control(int device_type, int device_index, int signal_type, double value) = 0;
            /// @brief Reset MSRs to a default state.
            virtual void msr_reset(void) = 0;
            /// @brief Return the upper and lower bounds for each control.
            ///
            /// For a RAPL platform this would be the package power limit,
            /// as well as the frequency p-state bounds.
            ///
            /// @param [out] map from control domain to the lower and upper
            ///        control bounds for each control.
            virtual void bound(std::map<int, std::pair<double, double> > &bound) = 0;
            /// @brief Return the frequency limit where throttling occurs.
            ///
            /// @return frequency limit where anything <= is considered throttling.
            virtual double throttle_limit_mhz(void) const = 0;
            virtual int control_domain(int domain_type) const = 0;
            virtual int counter_domain(int domain_type) const = 0;
            virtual void create_domain_maps(std::set<int> &domain, std::map<int, std::map<int, std::set<int> > > &domain_map) = 0;
            /// @brief Return the path used for the MSR default save file.
            std::string msr_save_file_path(void);

        protected:

            ////////////////////////////////////////////////////////////////////
            //                     MSR read/write support                     //
            ////////////////////////////////////////////////////////////////////
            /// @brief Write a value to a Model Specific Register.
            /// @param [in] device_type enum device type can be
            ///        one of GEOPM_DOMAIN_PACKAGE, GEOPM_DOMAIN_CPU,
            ///        GEOPM_DOMAIN_TILE, or GEOPM_DOMAIN_BOARD_MEMORY.
            /// @param [in] device_index Numbered index of the specified type.
            /// @param [in] msr_offset Address offset of the requested MSR.
            /// @param [in] msr_mask Write mask of the specified MSR.
            /// @param [in] value Value to write to the specified MSR.
            void msr_write(int device_type, int device_index, off_t msr_offset,
                           unsigned long msr_mask, uint64_t value);
            /// @brief Read a value from a Model Specific Register.
            /// @param [in] device_type enum device type can be
            ///        one of GEOPM_DOMAIN_PACKAGE, GEOPM_DOMAIN_CPU,
            ///        GEOPM_DOMAIN_TILE, or GEOPM_DOMAIN_BOARD_MEMORY.
            /// @param [in] device_index Numbered index of the specified type.
            /// @param [in] msr_offset Address offset of the requested MSR.
            /// @return Value read from the specified MSR.
            uint64_t msr_read(int device_type, int device_index, off_t msr_offset);
            /// @brief Batch read values from multiple Model Specific Registers.
            void batch_msr_read(void);
            /// @brief Retrieve the address offset of a Model Specific Register.
            /// @param [in] msr_name String name of the requested MSR.
            /// @return Address offset of the requested MSR.
            virtual off_t msr_offset(std::string msr_name);
            /// @brief Retrieve the write mask of a Model Specific Register.
            /// @param [in] msr_name String name of the requested MSR.
            /// @return Write mask of the requested MSR.
            unsigned long msr_mask(std::string msr_name);
            /// @brief Set the path to the MSR special file. In Linux this path
            /// is /dev/msr/cpu_num.
            /// @param [in] cpu_num Logical cpu number to set the path for.
            virtual void msr_path(int cpu_num);
            /// @brief Open a MSR special file.
            /// @param [in] cpu Number of logical cpu to open.
            void msr_open(int cpu);
            /// @brief Close a MSR special file.
            /// @param [in] cpu Number of logical cpu to close.
            void msr_close(int cpu);
            /// @brief Look up topology information to set member variables.
            virtual void parse_hw_topology(void);
            /// @brief Opens the per cpu special files, initializes the MSR offset
            /// map, initialize RAPL, CBO and fixed counter MSRs.
            virtual void msr_initialize() = 0;
            int num_domain(int domain_type) const;
            /// @brief Handles the overflow of fixed size counters.
            /// @param [in] signal_idx The index into the overflow offset vector
            ///        for this counter.
            /// @param [in] The size of the counter.
            /// @param [in] The value read from the counter.
            /// @return The value corrected for overflow.
            double msr_overflow(int signal_idx, uint32_t msr_size, uint64_t value);

            struct m_msr_batch_op {
                uint16_t cpu;      /// @brief In: CPU to execute {rd/wr}msr ins.
                uint16_t isrdmsr;  /// @brief In: 0=wrmsr, non-zero=rdmsr
                int32_t err;
                uint32_t msr;      /// @brief In: MSR Address to perform op
                uint64_t msrdata;  /// @brief In/Out: Input/Result to/from operation
                uint64_t wmask;    /// @brief Out: Write mask applied to wrmsr
            };

            struct m_msr_batch_array {
                uint32_t numops;            /// @brief In: # of operations in ops array
                struct m_msr_batch_op *ops;   /// @brief In: Array[numops] of operations
            };

            struct m_msr_signal_entry {
                off_t offset;
                uint64_t write_mask;
                int lshift_mod;
                int rshift_mod;
                uint64_t mask_mod;
                double multiply_mod;
            }

            /// @brief Holds the underlying hardware topology.
            PlatformTopology m_topology;
            /// @brief Holds the file descriptors for the per-cpu special files.
            std::vector<int> m_cpu_file_desc;
            /// @brief Map of MSR string name to address offset and write mask.
            /// This is a map is keyed by a string of the MSR's name and maps a pair
            /// which contain the MSR's offset (first) and write mask (second).
            const std::map<std::string, struct m_msr_signal_entry> *m_msr_signal_map_ptr;
            const std::map<std::string, std::pair<off_t, uint64_t> > *m_msr_control_map_ptr;
            /// @brief Number of logical CPUs.
            int m_num_logical_cpu;
            /// @brief Number of hardware CPUs.
            int m_num_hw_cpu;
            /// @brief Number of logical cpus per hardware core.
            int m_num_cpu_per_core;
            /// @brief Number of tiles.
            int m_num_tile;
            /// @brief Number of tiles.
            int m_num_tile_group;
            /// @brief Number of packages.
            int m_num_package;
            int m_num_core_per_tile;
            /// @brief File path to MSR special files.
            char m_msr_path[NAME_MAX];
            /// @brief The number of signals per package.
            int m_num_energy_signal;
            /// @brief The number of signals per CPU.
            int m_num_counter_signal;
            /// @brief Map from control type to control latency in miliseconds
            std::map<int, double> m_control_latency_ms;
            /// @brief TDP value for package (CPU) power read from RAPL.
            double m_tdp_pkg_watts;
            /// @brief The last values read from all counters.
            std::vector<uint64_t> m_msr_value_last;
            /// @brief The current aggregated overflow for all the counters.
            std::vector<double> m_msr_overflow_offset;
            int m_msr_batch_desc;
            bool m_is_batch_enabled;
            struct m_msr_batch_array m_batch;
            uint64_t m_trigger_offset;
            uint64_t m_trigger_value;

        private:
            void build_msr_save_string(std::ofstream &save_file, int device_type, int device_index, std::string name);
            std::string m_msr_save_file_path;
            bool m_is_initialized;

            ///Constants
            const std::string M_MSR_SAVE_FILE_PATH;
    };
}

#endif

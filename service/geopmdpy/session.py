#!/usr/bin/env python3

#  Copyright (c) 2015 - 2021, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

"""Implementation for the geopmsession command line tool

"""

import sys
import os
import time
import math
from argparse import ArgumentParser
from dasbus.connection import SystemMessageBus
from dasbus.error import DBusError
from . import topo
from . import pio


class Session:
    """Object responsible for creating a GEOPM service session

    This object's run() method is the main entry point for
    geopmsession command line tool.  The inputs to run() are derived
    from the command line options provided by the user.  The Session
    object uses the Requests object defined below to parse the input
    request buffer from the user.

    """
    def __init__(self, geopm_proxy):
        """Constructor for Session class

        Args:
            geopm_proxy (dasbus.client.proxy.InterfaceProxy): The
                dasbus proxy for the GEOPM D-Bus interface.

        Raises:
            RuntimeError: The geopm systemd service is not running

        """
        try:
            geopm_proxy.PlatformOpenSession
        except DBusError as ee:
            if 'io.github.geopm was not provided' in str(ee):
                err_msg = """The geopm systemd service is not enabled.
    Install geopm service and run 'systemctl start geopm'"""
                raise RuntimeError(err_msg) from ee
            else:
                raise ee
        self._geopm_proxy = geopm_proxy

    def read_signals(self, requests):
        """Read requested signals from GEOPM DBus interface

        All of the signals parsed from the input stream of requests
        are read and the values returned.

        Args:
            requests (Requests): Request object parsed from user input.

        Returns:
            list(float): Read signal values: one for each element of
                the list of parsed requests.

        """
        result = []
        for rr in requests:
            result.append(self._geopm_proxy.PlatformReadSignal(*rr))
        return result

    def format_signals(self, signals, signal_format):
        """Format a list of signal values for printing

        Args:
            signals (list(float)): Values to be printed

            signal_format (list(int)): The geopm::format_string_e enum
                                       value describing the formating
                                       for each signal provided.

        Returns:
            str: Ready-to-print line of formatted values

        """
        if len(signals) != len(signal_format):
            raise RuntimeError(
                'Number of signal values does not match the number of requests')
        result = [pio.format_signal(ss, ff) for (ss, ff) in
                  zip(signals, signal_format)]
        return '{}\n'.format(','.join(result))

    def num_sample(self, duration, period):
        """Get required number of times to read samples

        The user specifies a duration and period, this method
        interprets how many samples should be read to satisfy the
        request.

        Args:
            duration (float): The user specified length of time for
                              the samples to span in units of seconds.

            period (float): The user specified period between samples
                            in units of seconds.

        Returns:
            int: The required number of samples

        """
        if period == 0:
            result = 1
        else:
            result = int(duration / period)  + 1
        return result

    def run_read(self, duration, period, requests, out_stream):
        """Run a read mode session

        Use the GEOPM DBus interface to periodically read the
        requested signals. A line of text will be printed to the
        output stream for each period of time.  The line will contain
        a comma separated list of the read values, one for each
        request.

        Prior to calling run_read() the parse_read_requests() method
        must be called.  This will create the list of requests based
        on the input stream from the user.

        Args:
            duration (float): The user specified length of time for
                              the samples to span in units of seconds.

            period (float): The user specified period between samples
                            in units of seconds.
            requests (Requests): Request object parsed from user input.

            out_stream (file): Object with write() method where output
                               will be printed (typically sys.stdout).

        """
        self._geopm_proxy.PlatformOpenSession()
        num_sample = self.num_sample(duration, period)

        for sample_idx in TimedLoop(num_sample, period):
            signals = self.read_signals(requests)
            line = self.format_signals(signals, requests.get_formats())
            out_stream.write(line)
        self._geopm_proxy.PlatformCloseSession()

    def run_write(self, duration, requests):
        """Run a write mode session

        Use the GEOPM DBus interface to write the requested set of
        controls.  The contol settings will be held for the specified
        duration of time.  The call is blocking for the requested
        duration of time while the session is held open.

        Args:
            duration (float): Length of time to hold the requested
                              control settings in units of seconds.

            requests (Requests): Request object parsed from user input.

        """
        self._geopm_proxy.PlatformOpenSession()
        for rr in requests:
            self._geopm_proxy.PlatformWriteControl(*rr)
        time.sleep(duration)
        self._geopm_proxy.PlatformCloseSession()

    def check_read_args(self, run_time, period):
        """Check that the run time and period are valid for a read session

        Args:
            run_time (float):

        Raises:
            RuntimeError: The period is greater than the total time or
                          either is negative.

        """
        if period > run_time:
            raise RuntimeError('Specified a period that is greater than the total run time')
        if period < 0.0 or run_time < 0.0:
            raise RuntimeError('Specified a negative run time or period')

    def check_write_args(self, run_time, period):
        """Check that the run time and period are valid for a write session

        Raises:
            RuntimeError: The period is non-zero or total time is negative.

        """
        if run_time <= 0.0:
            raise RuntimeError('When opening a write mode session, a time greater than zero must be specified')
        if period != 0.0:
            raise RuntimeError('Cannot specify period with write mode session')

    def run(self, is_write, run_time, period, request_stream, out_stream):
        """"Create a GEOPM session with values parsed from the command line

        The implementation for the geopmsession command line tool.
        The inputs to this method are derived from the parsed command
        line provided by the user.

        Args:

            is_write (bool): True for write mode session requests, and
                             False for read mode session requests.

            run_time (float): Time duration of the session in seconds.


            period (float): Time interval for each line of output for
                            a read session.  Value must be zero for a
                            write mode session.

            request_stream (file): Input from user describing the
                                   requests to read or write values.

            out_stream (file): Stream where output from a read mode
                               session will be printed.  This
                               parameter is not used for a write mode
                               session.

        """
        requests = Requests(self._geopm_proxy, request_stream, is_write)
        if is_write:
            self.check_write_args(run_time, period)
            self.run_write(run_time, requests)
        else:
            self.check_read_args(run_time, period)
            self.run_read(run_time, period, requests, out_stream)


class TimedLoop:
    """Object that can be iterated over to run a timed loop

    Use in a for loop to execute a fixed number of timed delays.  The
    overhead time for executing what is inside of the loop is
    accounted for.  Calls to time.sleep() are made to delay until the
    targeted end time for each iteration.

    Example:

        >>> time_zero = time.time()
        >>> for loop_idx in TimedLoop(10, 0.1):
        ...     print('{}: {}'.format(loop_idx, time.time() - time_zero))
        ...
        0: 0.0008680820465087891
        1: 0.10126090049743652
        2: 0.20174455642700195
        3: 0.30123186111450195
        4: 0.4010961055755615
        5: 0.5020360946655273
        6: 0.6011238098144531
        7: 0.7011349201202393
        8: 0.8020164966583252
        9: 0.9015650749206543
        10: 1.0021190643310547

    """

    def __init__(self, num_period, period):
        """Constructor for timed loop object

        The number of loops executed is one greater than the number of
        time intervals requested, and that the first iteration is not
        delayed.  The total amount of time spanned by the loop is the
        product of the two input parameters.

        Args:

            num_period (int): Number of time periods spanned by the
                              loop.

            period (float): Target interval for the loop execution in
                            units of seconds.

        """

        self._period = period
        # Add one to ensure:
        #     total_time = num_loop * period
        # because we do not delay the start iteration
        self._num_loop = num_period + 1

    def __iter__(self):
        """Set up a timed loop

        Iteration method for timed loop.  This iterator can be used in
        a for statement to execute the loop periodically.


        """
        self._loop_idx = 0
        self._target_time = time.time()
        return self

    def __next__(self):
        """Sleep until next targeted time for loop and update counter

        """
        result = self._loop_idx
        if self._loop_idx == self._num_loop:
            raise StopIteration
        if self._loop_idx != 0:
            sleep_time = self._target_time - time.time()
            if sleep_time > 0:
                time.sleep(sleep_time)
        self._target_time += self._period
        self._loop_idx += 1
        return result


class Requests:
    """Object derived from user input that provides request information

    The geopmsession command line tool parses requests for reading or
    writing from standard input.  The Requests object holds the logic
    for parsing the input stream upon construction.  The resulting
    object may be iterated upon to retrieve the requested
    signals/controls that the user would like to read/write and the
    enum used to format signal values into strings (for read mode
    sessions).

    """
    def __init__(self, geopm_proxy, request_stream, is_write):
        """Constructor for Requests object

        Args:
            geopm_proxy (dasbus.client.proxy.InterfaceProxy): The
                dasbus proxy for the GEOPM D-Bus interface.

            request_stream (file): Input from user describing the
                                   requests to read or write values.

            is_write (bool): True for write mode session requests, and
                             False for read mode session requests.

        """
        self._geopm_proxy = geopm_proxy
        if is_write:
            self._requests = self.parse_write_requests(request_stream)
            self._formats = None
        else:
            self._requests = self.parse_read_requests(request_stream)
            self._formats = self.query_formats(self.get_names())

    def __iter__(self):
        """Iteration initialization for retrieving requests

        """
        self._curr_idx = 0
        return self

    def __next__(self):
        """Iteration steping function for retrieving requests

        """
        if self._curr_idx < len(self._requests):
            result = self._requests[self._curr_idx]
            self._curr_idx += 1
        else:
            raise StopIteration
        return result

    def get_formats(self):
        """Get formatting enum values for the parsed read requests

        Returns:
            list(int): The geopm::string_format_e enum value for each
                       read request.  None is returned for a Requests
                       object created in write mode.

        """
        return self._formats

    def get_names(self):
        """Get the signal or control names from each request

        Returns:
            list(string): The name of the signal or control associated
                           with each user request.

        """
        return [rr[0] for rr in self._requests]

    def iterate_stream(self, request_stream):
        """Iterated over a stream of requests

        This is a generator function that will filter out comment
        lines and trailing white space from the input stream.  It can
        be used to iterate over a request stream from the user for
        either a read or write mode session.

        Args:
            request_stream (file): Stream containing requests from
            user.

        Returns:
            generator: Iterate over filtered lines of the
                       request_stream

        """
        for line in request_stream:
            line = line.strip()
            if line == '':
                break
            if not line.startswith('#'):
                yield line

    def parse_read_requests(self, request_stream):
        """Parse input stream and return list of read requests

        Parse a user supplied stream into a list of tuples
        representing read requests.  The tuples are of the form
        (signal_name, domain_type, domain_idx) and are parsed one from
        each line of the stream.  Each of the values in the stream is
        seperated by white space.

        Each signal_name should match one of the signal names provided
        by the service.  The domain_type is specified as the name
        string, i.e one of the following strings: "board", "package",
        "core", "cpu", "board_memory", "package_memory", "board_nic",
        "package_nic", "board_accelerator", "package_accelerator".
        The domain index is a positive integer indexing the specific
        domain.

        Args:
            request_stream (file): Input stream to parse for read
                                   requests

        Returns:
            list((str, int, int)): List of request tuples. Each
                                   request comprises a signal name,
                                   domain type, and domain index.

        Raises:
            RuntimeError: Line from stream does not split into three
                          words and is also not a comment or empty
                          line.

        """
        requests = []
        for line in self.iterate_stream(request_stream):
            words = line.split()
            if len(words) != 3:
                raise RuntimeError('Read request must be three words: "{}"'.format(line))
            try:
                signal_name = words[0]
                domain_type = topo.domain_type(words[1])
                domain_idx = int(words[2])
            except (RuntimeError, ValueError):
                raise RuntimeError('Unable to convert values into a read request: {}'.format(line))
            requests.append((signal_name, domain_type, domain_idx))
        return requests

    def parse_write_requests(self, request_stream):
        """Parse input stream and return list of write requests

        Parse a user supplied stream into a list of tuples
        representing write requests.  The tuples are of the form
        (control_name, domain_type, domain_idx, setting) and are parsed
        one from each line of the stream.  Each of the values in the
        stream is seperated by white space.

        Each control_name should match one of the control names
        provided by the service.  The domain_type is specified as the
        name string, i.e one of the following strings: "board",
        "package", "core", "cpu", "board_memory", "package_memory",
        "board_nic", "package_nic", "board_accelerator",
        "package_accelerator".  The domain index is a positive integer
        indexing the specific domain.  The setting is the requested
        control value.

        Args:
            request_stream (file): Input stream to parse for write
                                   requests

        Returns:
            list((str, int, int, float)): List of request tuples. Each
                request comprises a signal name, domain type, domain
                index, and setting value.

        Raises:
            RuntimeError: Line from stream does not split into three
                          words and is also not a comment or empty
                          line.

        """
        requests = []
        for line in self.iterate_stream(request_stream):
            words = line.split()
            if len(words) != 4:
                raise RuntimeError('Invalid command for writing: "{}"'.format(line))
            control_name = words[0]
            domain_type = topo.domain_type(words[1])
            domain_idx = int(words[2])
            setting = float(words[3])
            requests.append((control_name, domain_type, domain_idx, setting))
        return requests

    def query_formats(self, signal_names):
        """Call the GEOPM DBus API to get the format type for each signal name

        Returns a list of geopm::string_format_e integers that determine
        how each signal value is formatted as a string.

        Args:
            signal_names (list(str)): List of signal names to query.

        Returns:
            list(int): List of geopm::string_format_e integers, one
                       for each signal name in input list.

        """
        return [info[4] for info in
                self._geopm_proxy.PlatformGetSignalInfo(signal_names)]


def main():
    """Command line interface for the geopm service read/write features.
    This command can be used to read signals and write controls by
    opening a session with the geopm service.

    """
    err = 0
    parser = ArgumentParser(description=main.__doc__)
    parser.add_argument('-w', '--write', dest='write', action='store_true', default=False,
                        help='Open a write mode session to adjust control values.')
    parser.add_argument('-t', '--time', dest='time', type=float, default=0.0,
                        help='Total run time of the session to be openend in seconds')
    parser.add_argument('-p', '--period', dest='period', type=float, default = 0.0,
                        help='When used with a read mode session reads all values out periodically with the specified period in seconds')
    args = parser.parse_args()
    try:
        sess = Session(SystemMessageBus().get_proxy('io.github.geopm',
                                                    '/io/github/geopm'))
        sess.run(args.write, args.time, args.period, sys.stdin, sys.stdout)
    except RuntimeError as ee:
        if 'GEOPM_DEBUG' in os.environ:
            # Do not handle exception if GEOPM_DEBUG is set
            raise ee
        sys.stderr.write('Error: {}\n\n'.format(ee))
        err = -1
    return err

if __name__ == '__main__':
    exit(main())

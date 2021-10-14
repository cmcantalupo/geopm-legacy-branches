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

class MLFilter:
    def __init__(self, model_name):
        """Create a filter that will manipulate signals read from geopmdpy.pio
        and produce a time series of data that can be input to a
        machine learning framework.

        """
        pass

    def get_signal_requests(self):
        """Get the (name, domain, domain_idx) tuples for the input signals as
        defined by geopmdpy.pio.  These signals will be provided to
        update() periodically.

        Returns:
            list((str, str, int))

        """
        pass

    def get_filtered_names(self):
        """Return a name for each value in the list returned by sample().

        This is a brief name for each column of the filtered data.

        Returns:
            list(str)

        """
        pass

    def get_filtered_descriptions(self):
        """Return a description for each value in the list returned by
        sample()

        This is a longer description for each column of the fitlered
        data.

        Returns:
            list(str)

        """
        pass

    def update(self, signals):
        """Called periodically with the values read from geomdpy.pio based on
        the requests defined by get_signal_requests().

        """
        pass

    def sample(self):
        """Return the most recent interpreted value"""
        pass

    def reset(self):
        """Reset the filter state to remove any update information"""
        pass

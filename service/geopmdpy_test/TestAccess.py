#!/usr/bin/env python3
#
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


from unittest import mock
from unittest import TestCase
from unittest import main
from mock import create_autospec

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy.access import Access

class TestAccess(TestCase):
    def setUp(self):
        self._geopm_proxy = mock.MagicMock()
        self._access = Access(self._geopm_proxy)

    def test_default_run(self):
        """Test default inputs to run()

        Test that the run method of Access when called with default
        arguments prints default access list.

        """
        signals_expect = ['TIME',
                          'ALL_ACCESS',
                          'SIGNAL']
        controls_expect = ['MAX_LIMIT',
                           'WRITABLE',
                           'CONTROL']
        return_value = (signals_expect,
                        controls_expect)
        self._geopm_proxy.PlatformGetGroupAccess = mock.Mock(return_value=return_value)
        actual_result = self._access.run(False, False, False, '')
        self._geopm_proxy.PlatformGetGroupAccess.assert_called_with('')
        expected_result = '\n'.join(signals_expect)
        self.assertEqual(expected_result, actual_result)


if __name__ == '__main__':
    main()

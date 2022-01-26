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

import unittest
from unittest import mock
import os
import stat
import tempfile

from geopmdpy.varrun import ActiveSessions

class TestActiveSessions(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestActiveSessions'
        self._TEMP_DIR = tempfile.TemporaryDirectory(self._test_name)

    def tearDown(self):
        self._TEMP_DIR.cleanup()

    def check_dir_perms(self, path):
        st = os.stat(path)
        self.assertEqual(0o700, stat.S_IMODE(st.st_mode))

    def test_default_creation(self):
        """Test default creation of an ActiveSessions object

        Test creates an ActiveSessions object when the geopm-service
        directory is not present.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        act_sess = ActiveSessions(sess_path)
        self.check_dir_perms(sess_path)

    def test_default_creation_negative(self):
        """Test default creation of an ActiveSessions object

        Test creates an ActiveSessions object when the geopm-service
        directory is present with wrong permissions.

        """
        sess_path = f'{self._TEMP_DIR.name}/geopm-service'
        os.umask(0o000)
        os.mkdir(sess_path, mode=0o755)
        with mock.patch('sys.stderr.write', return_value=None) as mock_err:
            act_sess = ActiveSessions(sess_path)
            mock_err.assert_called_once_with(f'Warning: <geopm> {sess_path} has wrong permissions, reseting to 0o700, current value: 0o755')
        self.check_dir_perms(sess_path)


if __name__ == '__main__':
    unittest.main()

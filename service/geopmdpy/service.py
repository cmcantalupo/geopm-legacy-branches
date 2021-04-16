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

"""Module containing the implementations of the D-Bus interfaces
exposed by geopmd."""

import os
import pwd
import grp
import json
from . import pio
from . import topo
from dasbus.connection import SystemMessageBus


def signal_info(name,
                description,
                domain,
                aggregation,
                string_format,
                behavior):
    # TODO: type checking
    return (name,
            description,
            domain,
            aggregation,
            string_format,
            behavior)

def control_info(name,
                 description,
                 domain):
    # TODO: type checking
    return (name,
            description,
            domain)

class PlatformService(object):
    def __init__(self, pio=pio, config_path='/etc/geopm-service', var_path='/var/run/geopm-service'):
        self._pio = pio
        self._CONFIG_PATH = config_path
        self._VAR_PATH = var_path
        self._DEFAULT_ACCESS = '0.DEFAULT_ACCESS'
        self._active_pid = None

    def get_group_access(self, group):
        group = self._validate_group(group)
        group_dir = os.path.join(self._CONFIG_PATH, group)
        if os.path.isdir(group_dir):
            path = os.path.join(group_dir, 'allowed_signals')
            signals = self._read_allowed(path)
            path = os.path.join(group_dir, 'allowed_controls')
            controls = self._read_allowed(path)
        else:
            signals = []
            controls = []
        return signals, controls

    def set_group_access(self, group, allowed_signals, allowed_controls):
        group = self._validate_group(group)
        group_dir = os.path.join(self._CONFIG_PATH, group)
        os.makedirs(group_dir, exist_ok=True)
        path = os.path.join(group_dir, 'allowed_signals')
        self._write_allowed(path, allowed_signals)
        path = os.path.join(group_dir, 'allowed_controls')
        self._write_allowed(path, allowed_controls)

    def get_user_access(self, user):
        if user == 'root':
            return self.get_all_access()
        user_gid = pwd.getpwnam(user).pw_gid
        all_gid = os.getgrouplist(user, user_gid)
        all_groups = [grp.getgrgid(gid).gr_name for gid in all_gid]
        signal_set = set()
        control_set = set()
        print("DEBUG: <geopm> user = {}".format(user))
        print("DEBUG: <geopm> all_groups = {}".format(all_groups))
        for group in all_groups:
            signals, controls = self.get_group_access(group)
            signal_set.update(signals)
            control_set.update(controls)
        signals = sorted(signal_set)
        controls = sorted(control_set)
        return signals, controls

    def get_all_access(self):
        return self._pio.signal_names(), self._pio.control_names()

    def get_signal_info(self, signal_names):
        raise NotImplementedError('PlatformService: Implementation incomplete')
        return infos

    def get_control_info(self, control_names):
        raise NotImplementedError('PlatformService: Implementation incomplete')
        return infos

    def open_session(self, client_pid, signal_config, control_config, interval,  protocol):
        if self._active_pid is not None:
            raise RuntimeError('The geopm service already has a connected client')
        self._active_pid = client_pid

        loop_pid, start_sec, start_nsec, key = self._pio.open_session(client_pid, signal_config, control_config, interval, protocol)

        makedirs(self._VAR_PATH, exist_ok=True)
        session_file = os.path.join(self._VAR_PATH, 'session-{}'.format(key))
        session_data = {'client_pid': client_pid,
                        'signal_config': signal_config,
                        'control_config': control_config,
                        'interval': interval,
                        'protocol': protocol,
                        'loop_pid': loop_pid,
                        'start_sec': start_sec,
                        'start_nsec': start_nsec,
                        'key': key}
        with open(session_file, 'w') as fid:
            json.dump(session_data, fid)
        return loop_pid, start_sec, start_nsec, key

    def close_session(self, client_pid, key):
        if self._active_pid is not None:
            if client_pid != self.active_pid:
                raise RuntimeError('The currently active geopm session was opened by a different process')
            self._active_pid = None
            raise NotImplementedError('PlatformService: Implementation incomplete')

    def _read_allowed(self, path):
        try:
            with open(path) as fid:
                result = [line.strip() for line in fid.readlines() if line.strip()]
        except FileNotFoundError:
            result = []
        return result

    def _write_allowed(self, path, allowed):
        allowed.append('')
        with open(path, 'w') as fid:
            fid.write('\n'.join(allowed))

    def _validate_group(self, group):
        if group is None or group == '':
            group = self._DEFAULT_ACCESS
        else:
            group = str(group)
            if group[0].isdigit():
                raise RuntimeError('Linux group name cannot begin with a digit: group = "{}"'.format(group))
        return group


class TopoService(object):
    def __init__(self, topo=topo):
        self._topo = topo

    def get_cache(self):
        self._topo.create_cache()
        with open('/tmp/geopm-topo-cache') as fid:
            result = fid.read()
        return result


class GEOPMService(object):
    __dbus_xml__ = """
    <node>
        <interface name="io.github.geopm">
            <method name="TopoGetCache">
                <arg direction="out" name="result" type="s" />
            </method>
            <method name="PlatformGetGroupAccess">
                <arg direction="in" name="group" type="s" />
                <arg direction="out" name="access_lists" type="(asas)" />
            </method>
            <method name="PlatformSetGroupAccess">
                <arg direction="in" name="group" type="s" />
                <arg direction="in" name="allowed_signals" type="as" />
                <arg direction="in" name="allowed_controls" type="as" />
            </method>
            <method name="PlatformGetUserAccess">
                <arg direction="out" name="access_lists" type="(asas)" />
            </method>
            <method name="PlatformGetAllAccess">
                <arg direction="out" name="access_lists" type="(asas)" />
            </method>
            <method name="PlatformGetSignalInfo">
                <arg direction="in" name="signal_names" type="as" />
                <arg direction="out" name="info" type="a(ssiiii)" />
            </method>
            <method name="PlatformGetControlInfo">
                <arg direction="in" name="control_names" type="as" />
                <arg direction="out" name="info" type="a(ssi)" />
            </method>
            <method name="PlatformOpenSession">
                <arg direction="in" name="signal_names" type="as" />
                <arg direction="in" name="control_names" type="as" />
                <arg direction="in" name="interval" type="d" />
                <arg direction="in" name="protocol" type="i" />
                <arg direction="out" name="session" type="(ixxs)" />
            </method>
            <method name="PlatformCloseSession">
                <arg direction="in" name="key" type="s" />
            </method>
        </interface>
    </node>
    """
    def __init__(self, topo=TopoService(),
                 platform=PlatformService()):
        self._topo = topo
        self._platform = platform
        self._active_pid = None

    def TopoGetCache(self):
        return self._topo.get_cache()

    def PlatformGetGroupAccess(self, group):
        return self._platform.get_group_access(group)

    def PlatformSetGroupAccess(self, group, allowed_signals, allowed_controls):
        self._platform.set_group_access(group, allowed_signals, allowed_controls)

    def PlatformGetUserAccess(self):
        return self._platform.get_user_access(self._get_user())

    def PlatformGetAllAccess(self):
        return self._platform.get_all_access()

    def PlatformGetSignalInfo(self, signal_names):
        return self._platform.get_signal_info(signal_names)

    def PlatformGetControlInfo(self, control_names):
        return self._platform.get_control_info(control_names)

    def PlatformOpenSession(self, signal_names, control_names, interval, protocol):
        return self._platform.open_session(self._get_pid(), signal_names, control_names, interval, protocol)

    def PlatformCloseSession(self, key):
        self._platform.close_session(self._get_pid(), key)

    def _get_user(self):
        bus = SystemMessageBus()
        dbus_proxy = bus.get_proxy('org.freedesktop.DBus',
                                   '/org/freedesktop/DBus')
        unique_name = bus.connection.get_unique_name()
        print("DEBUG: <geopm> unique_name = {}".format(unique_name))
        uid = dbus_proxy.GetConnectionUnixUser(unique_name)
        user = pwd.getpwuid(uid).pw_name
        print("DEBUG: <geopm> user = {}".format(user))
        return user

    def _get_pid(self):
        bus = SystemMessageBus()
        dbus_proxy = bus.get_proxy('org.freedesktop.DBus',
                                   '/org/freedesktop/DBus')
        unique_name = bus.connection.get_unique_name()
        pid = dbus_proxy.GetConnectionUnixProcessID(unique_name)
        return pid

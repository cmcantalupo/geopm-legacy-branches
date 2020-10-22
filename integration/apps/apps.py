#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

import os
import stat
import textwrap


class AppConf(object):
    '''An interface that encapsulates all details needed to run an
    application including: setup and cleanup commands; executable path
    and arguments; number of ranks and threads per node; and
    information on the performance of the application (figure of
    merit).  When deriving a new class based on AppConf, only those
    methods that are different from the default implementation below
    need to be redefined.

    The information contained in AppConf is passed to the launcher by
    the launch helper methods in
    integration/experiment/launch_util.py; these helper methods are
    used by all run scripts in integration/experiments.  The sequence
    used in each trial at launch time is as follows:

    get_rank_per_node() and get_cpu_per_rank() for geopmlaunch
    trial_setup()
    get_bash_setup_commands()
    get_bash_exec_path() and get_bash_exec_args()
    get_bash_cleanup_commands()
    parse_fom()
    trial_teardown()

    As part of each experiment trial, a bash script is generated by
    make_bash() to handle the setup and cleanup of the environment for
    each trial, constructing the application command line, and
    redirection of standard out.  This script will be launched and run
    on the compute nodes of the job.

    There are three methods that enable management of files for running
    experiments: trial_setup(), trial_teardown(), and
    experiment_teardown().  These are used to prepare files for
    application runs and clean up files produced by the application
    runs.

    The bash script produced will look similar to the following:

        #!/bin/bash
        # setup steps
        ulimit -s unlimited
        # run commands
        /full/path/to/myapplication.x --arg1=5 2>&1 | tee -a experiment_trial_0.log
        # clean up steps
        cp app.out experiment_trial_0.out

    '''

    @classmethod
    def name():
        ''' A short application name used to label output files.

            Returns:
               str: Name of the application.
        '''
        return "APP"

    def __init__(self, exec_path, exec_args=[]):
        ''' Default constructor for simple apps with no custom behavior or
            requirements.

            Args:
                exec_path (str): Full path to the executable.
                exec_args (list of str, optional): List of application arguments.
        '''
        self._exec_path = exec_path
        self._exec_args = exec_args

    def set_run_id(self, run_id):
        ''' Saves internally the unique id for the current trial.  This id
            can be used to name app output files.

            Args:
                run_id (str): A unique string used to label output for
                              the current experiment trial.

            Returns:
                None
        '''
        self._run_id = run_id

    def get_run_id(self):
        ''' Return the saved value for the unique id for the current trial.

            Returns:
                str: A unique string used to label output for the current
                     experiment trial.
        '''
        return self._run_id

    def get_rank_per_node(self):
        ''' Number of ranks per node required by the application.

            Returns:
                int: number of ranks per node to pass to launch.
        '''
        return 1

    def get_cpu_per_rank(self):
        ''' Hardware threads per rank required by the application.
            If None is returned, the launcher will select the number
            of threads to use based on the number of cores on the
            platform.

            Returns:
                int or None: number of OMP threads to set during launch.
        '''
        return None


    def trial_setup(self, run_id, output_dir):
        '''Create files to be used during a launch of a step in an experiment.
           This method may create symbolic links, copy files into the
           output directory, or execute commands that create input
           data for the application.  This step will be called
           repeatedly, once for each trial executed by an
           experiment. Some of these files may be cleaned up after
           each trial with the trial_teardown() method.  Some files
           may be left by the trial_teardown() method for use by
           subsequent trials.  For this reason it is advised to check
           for file existence prior to creation if a previous trial
           may have left a valid file for future use.  These files
           that persist between trials can be removed by the
           experiment_teardown() method.

           Args:
               run_id (str): A unique string used to label output for
                             the current experiment trial.

               output_dir (str): Path to the working directory where
                                 the trial will be executed.  The
                                 value of this parameter will be the
                                 same for all calls made to this
                                 method during an experiment.

        '''
        pass

    def trial_teardown(self, run_id, output_dir):
        '''Delete temporary files or symbolic links created as part of an
           experiment trial that are not needed by subsequent trials.
           This step will be called repeatedly, once for each trial
           executed by an experiment.  This method may also rename
           output files to contain the run_id so that they are not
           overwritten by subsequent trials.

           Args:
               run_id (str): A unique string used to label output for
                             the current experiment trial.

               output_dir (str): Path to the working directory where
                                 the trial was be executed.  The value
                                 of this parameter will be the same
                                 for all calls made to this method
                                 during an experiment.

        '''
        pass

    def experiment_teardown(self, output_dir):
        '''Delete all files or symbolic links created by an experiment after
           all trials have completed.  This is for the removal of
           files and symbolic links that are not cleaned up by
           trial_teardown().

           Args:
               output_dir (str): Path to the working directory where
                                 the trial was be executed.  The value
                                 of this parameter will be the same
                                 for all calls made to this method
                                 during an experiment.

        '''
        pass

    def get_bash_setup_commands(self):
        ''' Any actions to be added to the bash script to run prior to one
            iteration of the application.  This method should not have
            any side effects.  It must return a string containing
            valid Bash commands to be run at launch time (or empty
            string).

            Returns:
                str: a substring of the bash script that will be run before
                     the application during one trial of the experiment.
        '''
        return ''

    def get_bash_exec_path(self):
        ''' Full path to the application executable to be used in the launch
            script.

            Returns:
                str: application executable path.

        '''
        return self._exec_path

    def get_bash_exec_args(self):
        ''' Command line arguments to the application as a string or
            list of strings to be joined using " ".

            Returns:
                str or list of str: list of arguments to be passed to
                                    the application command

        '''
        return self._exec_args

    def get_custom_geopm_args(self):
        ''' Additional geopmlaunch arguments required for the app, such as
            --geopm-ompt-disable.

            Returns:
                list of str: list of geopmlaunch command line arguments.
        '''
        return []

    def get_bash_cleanup_commands(self):
        ''' Any steps to be added to the bash script to run after one
            iteration of the application.  This method should not have
            side effects.  It must always return a string containing
            valid bash commands to be run at launch time (or empty
            string).

            Returns:
                str: a substring of the bash script that will be run after
                     the application during one trial of the experiment.

        '''
        return ''

    def parse_fom(self, log_path):
        ''' Method to parse output from the given log to determine the
            performance (figure of merit) for one experiment trial.
            Returns the figure of merit as a string containing a
            single number, which will be appended to the report.  Returns None
            if there is no figure of merit for the application; in such cases
            inverse runtime will be used.

            Args:
                log_path (str): path to the log file containing the
                                standard output from the previous run.

            Return:
                float or None: application figure of merit for the run.
        '''
        return None


def make_bash(app_conf, run_id, log_file):
    ''' Produces a run script for one experiment trial using the get_bash_*
        methods of an AppConf object.

        Args:
            run_id (str): A unique string used to label output for
                          the current experiment trial.
            log_path (str): path to the log file containing the
                            standard output from the previous run.

        Return:
            str: path to the generated script
    '''

    app_conf.set_run_id(run_id)

    app_params = app_conf.get_bash_exec_args()
    if type(app_params) is list:
        app_params = ' '.join(app_params)

    script = '''#!/bin/bash\n'''
    script += textwrap.dedent('''\
        {setup}
        {app_exec} {app_params} 2>&1 | tee -a {log_file}
        {cleanup}
    '''.format(setup=app_conf.get_bash_setup_commands(),
               app_exec=app_conf.get_bash_exec_path(),
               app_params=app_params,
               log_file=log_file,
               cleanup=app_conf.get_bash_cleanup_commands()))
    bash_file = '{}.sh'.format(app_conf.name())
    with open(bash_file, 'w') as ofile:
        ofile.write(script)
    # read, write, execute for owner
    os.chmod(bash_file, stat.S_IRWXU)
    return bash_file

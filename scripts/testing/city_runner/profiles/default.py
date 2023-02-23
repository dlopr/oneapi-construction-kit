# Copyright (C) Codeplay Software Limited. All Rights Reserved.

import os
import sys
from city_runner.profile import Profile
from city_runner.cts import CTSTestRun


def create_profile():
    """ Create a new instance of a profile. This is usually called once."""
    return DefaultProfile()


class DefaultProfile(Profile):
    """ This profile executes tests locally. """
    def create_run(self, test, worker_state=None):
        """ Create a new test run from a test description.  """
        return DefaultTestRun(self, test)


class DefaultTestRun(CTSTestRun):
    """ Represent the execution of a particular test. """
    def __init__(self, profile, test):
        super(DefaultTestRun, self).__init__(profile, test)

    def execute(self):
        """ Execute the test and wait for completion. """
        exe = self.test.executable
        working_dir = self.profile.args.binary_path

        # Technically this if statement is redundant, the 'else' branch should
        # be correct in all cases.  However, we had an issue with
        # os.path.realpath on Cygwin python where it was merging Unix style and
        # Windows style paths illegally.  Given that when using Cygwin we were
        # always using absolute paths we just skip os.path.realpath for those
        # cases, as the function (should be) a no-op.  Thus relative paths on
        # Cygwin are known to be broken.  Note: os.path.isabs is also "broken"
        # on Cygwin, hence the explicit string check.
        if working_dir[1:3] == ":\\":
            exe_path = os.path.join(working_dir, exe.path)
        else:
            exe_path = os.path.join(os.path.realpath(working_dir), exe.path)

        if not sys.platform.startswith('win'):
            # Add -b/--binary-dir to PATH when not executing on Windows. This
            # is useful when testing binary or spir-v offline compilation mode
            # in the OpenCL-CTS.
            paths = os.environ['PATH'].split(':')
            if working_dir not in paths:
                paths.insert(0, working_dir)
                os.environ['PATH'] = ':'.join(paths)

        # Add user-defined environment variables.
        env = {}
        for key, value in self.profile.args.env_vars:
            env[key] = value

        # Parse the list of library paths from the environment.
        env_var_name = None
        env_var_separator = None
        lib_dirs = []
        if sys.platform.startswith("win"):
            env_var_name = "PATH"
            env_var_separator = ";"
        else:
            env_var_name = "LD_LIBRARY_PATH"
            env_var_separator = ":"
        if env_var_name:
            try:
                orig_path = os.environ[env_var_name]
                if orig_path:
                    lib_dirs.extend(orig_path.split(env_var_separator))
            except KeyError:
                pass

        # Add user-defined library paths, making them absolute in the process.
        for user_path in self.profile.args.lib_paths:
            lib_dirs.append(os.path.realpath(user_path))
        if lib_dirs:
            env[env_var_name] = env_var_separator.join(lib_dirs)

        self.create_process(exe_path, self.test.arguments, working_dir, env)
        self.analyze_process_output()

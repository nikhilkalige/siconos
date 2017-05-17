#!/usr/bin/env python

import sys
import os
from socket import gethostname

from tasks import default, known_tasks, database
from subprocess import check_output, check_call

from getopt import gnu_getopt, GetoptError


def usage():
    print("""
    {0} [-v] [--tasks=<task1>,<task2>,...] \
        [--list-tasks] \
        [--root-dir=...] \
        [--targets=...] \
        [--run]\
    [--brutal-docker-clean]
    """.format(sys.argv[0]))


try:
    opts, args = gnu_getopt(sys.argv[1:], 'v',
                            ['run', 'tasks=', 'list-tasks', 'targets=',
                             'root-dir=',
                             'print=', 'brutal-docker-clean'])

except GetoptError as err:
    sys.stderr.write(str(err))
    usage()
    exit(2)

tasks = None
targets_override = None
verbose = False
run = False
return_code = 0
print_mode = False

for o, a in opts:
    if o in ('--run',):
        run = True

    if o in ('-v',):
        verbose = True

    if o in ('--root-dir',):
        root_dir = os.path.abspath(a)

    if o in ('--tasks',):
        import tasks
        tasks = [getattr(tasks, s) for s in a.split(',')]

    if o in ('--list-tasks',):
        import tasks
        from machinery.ci_task import CiTask
        for s in dir(tasks):
            if isinstance(getattr(tasks, s), CiTask):
                print(s)
                if verbose:
                    t = getattr(tasks, s)
                    t.display()

    if o in ('--print',):
        print_mode = True
        output_mode_str = a

    if o in ('--targets',):
        targets_override = a.split(',')


hostname = gethostname().split('.')[0]

if tasks is None:

    if hostname in known_tasks:
        tasks = known_tasks[hostname]
    else:
        tasks = [default]

if print_mode:

    from machinery.mksenv import print_commands, OutputMode, output_mode_spec

    for task in tasks:
        distrib, distrib_version = task._distrib.split(':')
        print_commands(specfilename=database,
                       distrib=distrib,
                       distrib_version=distrib_version,
                       pkgs=task._pkgs,
                       output_mode=output_mode_spec[output_mode_str],
                       split=False)
if run:

    for task in tasks:
        try:

            return_code += task.run(root_dir,
                                    targets_override=targets_override)

        except Exception as e:
            sys.stderr.write(str(e))

    for task in tasks:
        try:

            task.clean()

        except Exception as e:
            sys.stderr.write(str(e))

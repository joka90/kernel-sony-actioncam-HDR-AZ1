#!/bin/sh

# Exit immediately on errors.
set -e

chmod_x () {
    for f in "$@"; do
	test -f "$f" && chmod ug+x "$f"
    done
    return 0 # Discard result of last test, which may fail.
}

# For kprobe tests.
chmod_x samples/kprobes/user-tests/*.sh

# For SS related works.
chmod_x setup-*
chmod_x scripts/setup_common.py
chmod_x scripts/config-modifiers/*

# For git to properly work with .pc/ files.
chmod -R ug+rw .pc/

# This script itself
chmod_x scripts/fix-filemodes.sh

# Kernel header scripts
chmod_x scripts/*.sh

# setconfig scripts
chmod_x scripts/setconfig.py

# function duration dump
chmod_x scripts/fdd

# migration trace
chmod_x scripts/migrate_extract
chmod_x scripts/migrate_stats

# Kinspect scripts
chmod_x scripts/kinspect/splitter.rb scripts/kinspect/kbootchart.rb
chmod_x scripts/kinspect/dummy_kinspect.rb
chmod_x scripts/kinspect/viewer/httpserver.rb
chmod_x scripts/kinspect/viewer/html/kipct.cgi

# bootchart scripts
chmod_x scripts/bootchart/bootchart.pl scripts/bootchart/script/bootchartd

# stack size scripts
chmod_x tools/stack_size/checkstack-all.pl
chmod_x tools/stack_size/check_static_stack_usage.sh
chmod_x tools/stack_size/columnize.py
chmod_x tools/stack_size/joinlines.py
chmod_x tools/stack_size/stack_size

# Looks at the current index and checks to see if merges or updates
# are needed by checking stat() information.
# This is needed for errornous cg-commit log listing all files
# modified by this file.
#
# git-update-index returns non-zero (indicating some files are changed).
# Ignore it by `|| true', so that the entire script exits with zero.
git update-index --refresh > /dev/null || true

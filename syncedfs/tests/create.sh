#!/bin/bash

cd /home/$(whoami)/syncedfs/primary/virtual

# GENERIC_OPERATION__OPERATION_TYPE__CREATE:
touch test.txt

# GENERIC_OPERATION__OPERATION_TYPE__MKNOD:
mkfifo test.pipe

# GENERIC_OPERATION__OPERATION_TYPE__MKDIR:
mkdir test
touch test/testdir.txt

# GENERIC_OPERATION__OPERATION_TYPE__SYMLINK:
ln -s test.txt test.symlink

# GENERIC_OPERATION__OPERATION_TYPE__LINK:
ln test.txt test.link

# GENERIC_OPERATION__OPERATION_TYPE__WRITE:
date > test.txt
date > test2.txt

# GENERIC_OPERATION__OPERATION_TYPE__TRUNCATE:
truncate -s 2 test2.txt

# GENERIC_OPERATION__OPERATION_TYPE__CHMOD:
chmod 600 test2.txt

# GENERIC_OPERATION__OPERATION_TYPE__CHOWN:
#cannot test at the moment

# GENERIC_OPERATION__OPERATION_TYPE__RENAME:
mv test2.txt test3.txt

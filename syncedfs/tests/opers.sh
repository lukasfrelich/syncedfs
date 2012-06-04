#!/bin/bash

cd /home/$(whoami)/syncedfs/primary/virtual

#        case GENERIC_OPERATION__OPERATION_TYPE__CREATE:
touch test.txt
touch test2.txt
#        case GENERIC_OPERATION__OPERATION_TYPE__MKNOD:
mkfifo test.pipe
#        case GENERIC_OPERATION__OPERATION_TYPE__MKDIR:
mkdir test
#        case GENERIC_OPERATION__OPERATION_TYPE__SYMLINK:
ln -s test.txt test.symlink
#        case GENERIC_OPERATION__OPERATION_TYPE__LINK:
ln test.txt test.link
#        case GENERIC_OPERATION__OPERATION_TYPE__WRITE:
date > test.txt
date > test2.txt
#        case GENERIC_OPERATION__OPERATION_TYPE__UNLINK:
rm test.symlink
rm test.txt
rm test.link
rm test.pipe
#        case GENERIC_OPERATION__OPERATION_TYPE__RMDIR:
rmdir test
#        case GENERIC_OPERATION__OPERATION_TYPE__TRUNCATE:
truncate -s 2 test2.txt
#        case GENERIC_OPERATION__OPERATION_TYPE__CHMOD:
chmod 600 test2.txt
#        case GENERIC_OPERATION__OPERATION_TYPE__CHOWN:
#cannot test at the moment
#        case GENERIC_OPERATION__OPERATION_TYPE__RENAME:
mv test2.txt test3.txt

#!/usr/bin/python

import os, sys
import re
import time, signal
import ctypes, os
import glob
import cPickle as pickle
import timeit

# clock_gettime is not directly available
# source: http://stackoverflow.com/questions/1205722/how-do-i-get-monotonic-time-durations-in-python
CLOCK_MONOTONIC = 1 # see <linux/time.h>

class timespec(ctypes.Structure):
    _fields_ = [
        ('tv_sec', ctypes.c_long),
        ('tv_nsec', ctypes.c_long)
    ]

librt = ctypes.CDLL('librt.so.1', use_errno=True)
clock_gettime = librt.clock_gettime
clock_gettime.argtypes = [ctypes.c_int, ctypes.POINTER(timespec)]

def monotonic_time():
    t = timespec()
    if clock_gettime(CLOCK_MONOTONIC, ctypes.pointer(t)) != 0:
        errno_ = ctypes.get_errno()
        raise OSError(errno_, os.strerror(errno_))
    return t.tv_sec + t.tv_nsec * 1e-9

block_devices = []
executables = []
eth_devices = []

stats = []  # all gathered statistics

def start_test(testcmd):
    try:
        pid = os.fork();
    except OSError, e:
        raise RuntimeError('Fork failed: %s [%d]' % (e.strerror, e.errno))
    if pid != 0:
        try:
            os.system(testcmd)
        except Exception:
            os._exit(255)


def start_measurements():
    signal.signal(signal.SIGALRM, gather_stats)
    signal.signal(signal.SIGINT, end_measurements)
    signal.setitimer(signal.ITIMER_REAL, 0.1, 0.1)


def end_measurements(signum, frame):
    signal.setitimer(signal.ITIMER_REAL, 0, 0)

    # dump stats
    filename = 'stats.log'

    if os.path.exists(filename):
        # if filename exists, create new file 'stats.log.x', where x is the
        # lowest unused number
        pattern = re.compile('\.([0-9]+)$')
        maxnum = 0
        for path in glob.glob(filename + '*'):
            number = 0
            if (pattern.search(path)):
                number = int(pattern.search(path).group(1))

            if number > maxnum:
                maxnum = number
        filename += '.' + str(maxnum + 1);

    # if saving fails, try home directory
    try:
        pickle.dump(stats, open(filename, 'wb'))
    except Exception:
        filename = os.path.expanduser('~/stats.log')
        pickle.dump(stats, open(filename, 'wb'))
    sys.exit()


def gather_stats(signum, frame):
    stats.append([monotonic_time(), block_dev_io_stats(), process_io_stats(),
                  net_stats()])


def block_dev_io_stats():
    filtered_devs = {}
    with open('/proc/diskstats', 'r') as f:
        for line in f:
            fields = line.split()
            if filter_equal(fields[2], block_devices):
                filtered_devs[fields[2]] = [long(e) for e in fields[3:]]

    return filtered_devs


def process_io_stats():
    filtered_procs = {}
    for path in glob.glob('/proc/[1-9]*'):
        try:
            f = open(path + '/cmdline', 'r')
            filename = (f.read().split('\x00'))[0]
            f.close()
            if filter_endswith(filename, executables):
                iostats = [path[6:]]    # pid

                f = open(path + '/io', 'r')
                for line in f:
                    if len(line) < 1:
                        continue
                    iostats.append(long((line.split())[1]))
                f.close()

                # if we get multiple processes with same name, sum-up the stats
                if filename in filtered_procs:
                    oldiostats = filtered_procs[filename]
                    for i in range(1, 7):
                        iostats[i] += oldiostats[i]
                filtered_procs[filename] = iostats
        except IOError:    # process might have disappear in the meantime
            pass

    return filtered_procs


def net_stats():
    filtered_devs = {}
    with open('/proc/net/dev', 'r') as f:
        for line in f:
            fields = line.split()
            if filter_startswith(fields[0], eth_devices):
                # fields[:-1] because, there is always trailing :
                filtered_devs[fields[0][:-1]] = [long(e) for e in fields[1:]]

    return filtered_devs


def filter_startswith(line, keywords):
    for key in keywords:
        if line.startswith(key):
            return True
    return False


def filter_endswith(line, keywords):
    for key in keywords:
        if line.endswith(key):
            return True
    return False


def filter_equal(line, keywords):
    for key in keywords:
        if key == line:
            return True
    return False


def usage():
    print('Usage: measure.py test-command devs procs eths')
    print('       test-command will not by interpreted by this script')
    print('       comma-separated list or - for no stats')
    sys.exit(1)

if __name__ == '__main__':
#	t = timeit.Timer("gather_stats(None, None)",
#        "from __main__ import gather_stats")
#	print t.timeit(10)

    if (len(sys.argv) < 5):
        usage()

    entities_lists = sys.argv[2:]

    def parse_csv(csv):
        if csv != '-':
            return csv.split(',')
        else:
            return []

    # get command-line options translated to numbers
    block_devices = parse_csv(entities_lists[0])
    executables = parse_csv(entities_lists[1])
    eth_devices = parse_csv(entities_lists[2])

    testcmd = sys.argv[1]
    start_measurements()
    while True:
        time.sleep(5);
    start_test()

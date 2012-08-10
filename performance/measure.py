#!/usr/bin/python

import os, sys
import re
import time, signal
import ctypes, os
import glob
import cPickle as pickle
import subprocess
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


filename = ''

block_devices = []
executables = []
eth_devices = []

stats = []  # all gathered statistics

# for automated measurements end
eth_name = 'eth2'           # eth device, which we will monitor to see when the
                            # transfer has finished
#process_started = False
process = None
blacklisted_process = ''
process_finished = False

traffic_threshold = 4096
traffic_baseline = -1
number_measurements = 0


def start_test(cmd):
    global process
    print cmd
    process = subprocess.Popen(cmd, shell=True)
    print 'process spawned'

def start_measurements(cmd):
    signal.signal(signal.SIGALRM, gather_stats)
    signal.signal(signal.SIGINT, end_measurements)
    signal.setitimer(signal.ITIMER_REAL, 0.1, 0.1)
    time.sleep(1)   # wait for first measurement
    start_test(cmd)


def end_measurements(signum, frame):
    signal.setitimer(signal.ITIMER_REAL, 0, 0)

    global filename
    if not filename:
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
    block_stat = block_dev_io_stats()
    proc_stat = process_io_stats()
    net_stat = net_stats()
    stats.append([monotonic_time(), block_stat, proc_stat, net_stat])

    global process_finished
    global number_measurements
    global process
    if not process_finished and process is not None:
        if process.poll() is not None:
            process_finished = True
            print 'process finished'

    # if the process had finished, start traffic monitoring
    if process_finished:
        #print 'waiting for traffic'

        if blacklisted_process not in proc_stat:
            if check_network_traffic(net_stat[eth_name]):
                end_measurements(None, None)
        else:
            #print 'blacklisted process still in memory'
            number_measurements = 0

def check_network_traffic(stats):
    global traffic_baseline
    global number_measurements
    curr_traffic = stats[8] # transmitted bytes
    # first time only assign
    if traffic_baseline == -1:
        traffic_baseline = curr_traffic
        #print 'baseline: ' + str(traffic_baseline)
        return False
    else:
        number_measurements += 1
        if number_measurements >= 50:   # 5 seconds
            if (curr_traffic - traffic_baseline) < traffic_threshold:
                #print 'exit, traffic diff: ' + str(curr_traffic - traffic_baseline)
                return True
            else:
                # reset baseline and measurements counter
                traffic_baseline = curr_traffic
                number_measurements = 0
                return False


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
    print('Usage: measure.py [output-file] test-cmd blacklist-process devs procs eths')
    print('       test-cmd will be executed by shell')
    print('       blacklist-process measurement will not finish if this process is in the memory, - for none')
    print('       comma-separated list or - for no stats')
    sys.exit(1)

if __name__ == '__main__':
#   measurements performance, on average one measurement takes 0.01s
#	t = timeit.Timer("gather_stats(None, None)",
#        "from __main__ import gather_stats")
#	print t.timeit(10)

    if len(sys.argv) < 6:
        usage()

    # output file was defined
    if len(sys.argv) == 7:
        filename = sys.argv[1]
        cmd = sys.argv[2]
        blacklisted_process = sys.argv[3]
        entities_lists = sys.argv[4:]
    else:
        cmd = sys.argv[1]
        blacklisted_process = sys.argv[2]
        entities_lists = sys.argv[3:]

    def parse_csv(csv):
        if csv != '-':
            return csv.split(',')
        else:
            return []

    # get command-line options translated to numbers
    block_devices = parse_csv(entities_lists[0])
    executables = parse_csv(entities_lists[1])
    eth_devices = parse_csv(entities_lists[2])

    start_measurements(cmd)
    while True:
        time.sleep(5)
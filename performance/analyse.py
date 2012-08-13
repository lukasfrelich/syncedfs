#!/usr/bin/python
import os
import re
import sys
import cPickle as pickle
import glob

# reversed dictionaries (for column names in output)
a_dev_stats_rev = {'0': 'rcompleted', '1': 'rmerged', '2': 'rsectors', '3': 'rtime',
             '4': 'wcompleted', '5': 'wmerged', '6': 'wsectors',
             '7': 'wtime', '8': 'inprogress',
             '9': 'iotime', '10': 'iowtime'}

# Achtung: 0 is pid
a_proc_stats_rev = {'1': 'rchar', '2': 'wchar', '3': 'syscr', '4': 'syscw',
              '5': 'rbytes', '6': 'wbytes', '7': 'cancelled_write_bytes'}

a_eth_stats_rev = {'0': 'rbytes', '1': 'rpackets',  # important
             '2': 'rerrs', '3': 'rdrop', '4': 'rfifo', '5': 'rframe',
             '6': 'rcompressed', '7': 'rmulticast',
             '8': 'tbytes', '9': 'tpackets',  # important
             '10': 'terrs', '11': 'tdrop', '12': 'tfifo', '13': 'tcolls',
             '14': 'tcarrier', '15': 'tcompressed'}

a_dev_stats = {'rcompleted': '0', 'rmerged': '1', 'rsectors': '2', 'rtime': '3',
               'wcompleted': '4', 'wmerged': '5', 'wsectors': '6',
               'wtime': '7', 'inprogress': '8',
               'iotime': '9', 'iowtime': '10'}
#a_dev_stats = {'rcompleted': [0, False, 1], 'rmerged': [1, False, 1],
#               'rsectors': [2, False, 512], 'rtime': [3, False, 1],
#               'wcompleted': [4, False, 1], 'wmerged': [5, False, 1],
#               'wsectors': [6, False, 512], 'wtime': [7, False, 1],
#               'inprogress': [8, False, 1], 'iotime': [9, False, 1],
#               'iowtime': [10, False, 1]}

# Achtung: 0 is pid
a_proc_stats = {'rchar': '1', 'wchar': '2', 'syscr': '3', 'syscw': '4',
              'rbytes': '5', 'wbytes': '6', 'cancelled_write_bytes': '7'}

a_eth_stats = {'rbytes': '0', 'rpackets': '1',  # important
             'rerrs': '2', 'rdrop': '3', 'rfifo': '4', 'rframe': '5',
             'rcompressed': '6', 'rmulticast': '7',
             'tbytes': '8', 'tpackets': '9',  # important
             'terrs': '10', 'tdrop': '11', 'tfifo': '12', 'tcolls': '13',
             'tcarrier': '14', 'tcompressed': '15'}

dev_stats = []
devs = []
proc_stats = []
procs = []
eth_stats = []
eths = []

def print_computed_stats(computed_stats):
    def strip_paths(paths):
        return [os.path.split(path)[1] for path in paths]

    def get_header_part(entities, stats, stats_rev_dict):
        header_part = ''
        for entity in entities:
            for stat in stats:
                if stat[1]: # if absolute
                    header_part += entity + '-a-' + stats_rev_dict[str(stat[0])] + '/' + str(stat[2]) + ','
                else:
                    header_part += entity + '-' + stats_rev_dict[str(stat[0])] + '/' + str(stat[2]) + ','
                    #header_part += entity + '-' + stats_rev_dict[str(stat[0])] + ','

        return header_part

    # print header
    header = 'time,'
    header += get_header_part(devs, dev_stats, a_dev_stats_rev)
    header += get_header_part(strip_paths(procs), proc_stats, a_proc_stats_rev)
    header += get_header_part(eths, eth_stats, a_eth_stats_rev)
    print(header[:-1])

    def get_stats_part(stats):
        stats_part = ''
        for dev in stats:
            for stat in dev:
                stats_part += str(round(stat, 2)) + ','

        return stats_part

    # print records
    for rec in computed_stats:
        line = str(round(rec[0], 2)) + ',' # time
        for i in range(1, len(rec)):
            line += get_stats_part(rec[i])

        print(line[:-1])


def analyse(stats):
    #print(len(stats))
    sdevs = set()
    sprocs = set()
    seths = set()

    # get all values first, need mainly for processes (might not be present
    # already in the first record)
    for rec in stats:
        sdevs.update(rec[1].keys())
        sprocs.update(rec[2].keys())
        seths.update(rec[3].keys())

    global devs
    global procs
    global eths
    devs = list(sdevs)
    procs = list(sprocs)
    eths = list(seths)

    def initialize_base_dict(x, y):
        out = dict()
        for i in range(x):
            out[i] = dict()
            for j in range(y):
                out[i][j] = 0
        return out

    base_dev = initialize_base_dict(len(devs), len(dev_stats))
    base_proc = initialize_base_dict(len(procs), len(proc_stats))
    base_eth = initialize_base_dict(len(eths), len(eth_stats))

    # get start time
    if len(stats) > 0:
        last_time = stats[0][0]     # for time_span
        first_time = stats[0][0]    # for out_rec

    computed_stats = []

    for rec in stats:
        time_span = rec[0] - last_time
        last_time = rec[0]

        out_rec = [last_time - first_time]
        if len(dev_stats) > 0:
            out_rec.append(process_record(rec[1], devs, dev_stats, base_dev, time_span))
        if len(proc_stats) > 0:
            out_rec.append(process_record(rec[2], procs, proc_stats, base_proc, time_span))
        if len(eth_stats) > 0:
            out_rec.append(process_record(rec[3], eths, eth_stats, base_eth, time_span))

        computed_stats.append(out_rec)

    return computed_stats

def process_record(record_dict, entities, stats, base, time_span):
    out = []
    i = 0
    for entity in entities:     # sda1, sda2 or eth0, eth2...
        j = 0
        out_sub = []
        for stat in stats:      # wsectors, wtime  or rbytes, wbytes
            # if last time span was 0 second, it must have been the first one
            # only set the base
            if time_span == 0:
                try:
                    base[i][j] = record_dict[entity][stat[0]] / stat[2]
                except KeyError:
                    pass
                out_sub.append(0)
            else:
                try:
                    if stat[1] == True:
                        # absolute value
                        out_sub.append((record_dict[entity][stat[0]] / stat[2] - base[i][j]))
                    else:
                        # otherwise try to compute the rate
                        out_sub.append((record_dict[entity][stat[0]] / stat[2] - base[i][j]) / time_span)
                        base[i][j] = record_dict[entity][stat[0]] / stat[2]
                except KeyError:
                    # stat is always there, only entity might be missing
                    if stat[1] == True:
                        # output -1 as result, to indicate, that the entity was missing
                        out_sub.append(-1)
                        # but don't reset base
                    else:
                        # assign -1 as result, to indicate, that the entity was missing
                        out_sub.append(-1)
                        # and reset base
                        base[i][j] = 0
            j += 1
        i += 1
        out.append(out_sub)

    return out

#def process_record_dummy(record_dict, entities, stats):
#    out = []
#    for entity in entities:     # sda1, sda2 or eth0, eth2...
#        out_sub = []
#        for stat in stats:      # wsectors, wtime  or rbytes, wbytes
#                try:
#                    out_sub.append(record_dict[entity][stat])
#                except Exception:
#                    out_sub.append(-1)
#        out.append(out_sub)
#
#    return out

def usage():
    def print_keys(spaces, dict):
        # hard-coded to three columns
        keys = sorted(dict.keys())
        if (len(keys) % 3) != 0:
            for i in range(3 - (len(keys) % 3)):
                keys.append('')
        for i in range(0, len(keys), 3):
            print(' ' * spaces +
                  '{0} {1} {2}'.format(keys[i], keys[i + 1], keys[i + 2]))
        print('')

    print('Usage: analyse.py [log-file] ' +
          'dev-stats-list proc-stats-list eth-stats-list')

    print('       log-file: if not specified' +
          'last log file will be sought for')
    print('       comma-separated list or - for no stats')

    print('       dev-stats-list:')
    print_keys(23, a_dev_stats)
    print('       proc-stats-list:')
    print_keys(24, a_proc_stats)
    print('       eth-stats-list:')
    print_keys(23, a_eth_stats)

    sys.exit(1)

if __name__ == '__main__':
    # check number of command-line parameters, get log name
    if len(sys.argv) == 5:
        stat_lists = sys.argv[2:]
        filename = sys.argv[1]
    elif len(sys.argv) == 4:
        stat_lists = sys.argv[1:]
        filename = 'stats.log'
        # if filename exists, create new file 'stats.log.x', where x is the
        # lowest unused number
        pattern = re.compile('\.([0-9]+)$')
        maxnum = 0
        for path in glob.glob(filename + '*'):
            number = 0
            if pattern.search(path):
                number = int(pattern.search(path).group(1))

            if number > maxnum:
                maxnum = number

        if maxnum > 0:
            filename += '.' + str(maxnum)
    else:
        usage()

    def parse_csv(csv):
        if csv != '-':
            return csv.split(',')
        else:
            return []

    def parse_stat(stat, dict):
        # name: [index, absolute, divisor]
        absolute = False
        divisor = 1
        #out = []
        parts = stat.split('-')
        if len(parts) == 1:
            # we got just index
            index = parts[0]
        elif len(parts) == 2:
            # we got index and absolute flag or divisor
            if parts[0] == 'a':
                absolute = True
                index = parts[1]
            else:
                index = parts[0]
                divisor = int(parts[1])
        elif len(parts) == 3:
            # we got both absolute flag and divisor
            absolute = True
            index = parts[1]
            divisor = int(parts[2])

        return [int(dict[index]), absolute, divisor]

    # get command-line options translated to numbers
    dev_stats = [parse_stat(stat, a_dev_stats) for stat in parse_csv(stat_lists[0])]
    proc_stats = [parse_stat(stat, a_proc_stats) for stat in parse_csv(stat_lists[1])]
    eth_stats = [parse_stat(stat, a_eth_stats) for stat in parse_csv(stat_lists[2])]

    stats = pickle.load(open(filename, 'rb'))

    print_computed_stats(analyse(stats));

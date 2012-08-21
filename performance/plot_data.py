#!/usr//bin/python

import sys

search_drbd = ['fswrite64-wchar', 'truncate-wchar', 'cmp-rbytes', 'eth2-tbytes', 'sda4-wsectors', 'drbd1-wsectors']
search_gluster = ['fswrite64-wchar', 'truncate-wchar', 'cmp-rbytes', 'rsync-rbytes', 'eth2-tbytes', 'sda4-wsectors']
search_syncedfs = ['fswrite64-wchar', 'truncate-wchar', 'cmp-rbytes', 'syncedfs-daemon-rbytes', 'eth2-tbytes', 'sda4-wsectors']

def output_average(lines):
    # list of csv values
    if len(lines) == 0:
        return
    a = None
    for line in lines:
        values = line.split(',')
        if a == None:
            a = [0] * len(values)

        i = 0
        for val in values:
            a[i] = a[i] + float(val)
            i = i + 1

    out = ''
    for i in range(len(a)):
        out = out + str(a[i]/len(lines)) + ','
    
    print(out[:-1])

def get_column_indexes(header, search_list):
    a = [0] # always add time column
    columns = header.split(',')
    for elem in columns:
        for s in search_list:
            if elem.find(s) != -1:
                a.append(columns.index(elem))
                break
    return a

def usage():
    print('Usage: plot_data.py input-file output-file')
    print('       will be create two files: output-file.data and output-file.plot')
    sys.exit(1)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        usage()

    lines = open(sys.argv[1]).read().splitlines()
    header = lines[0]
    
    # search for desired columns
    if sys.argv[1].startswith('drbd'):
        searched_columns = get_column_indexes(header, search_drbd)
    elif sys.argv[1].startswith('gluster'):
        searched_columns = get_column_indexes(header, search_gluster)
    elif sys.argv[1].startswith('syncedfs'):
        searched_columns = get_column_indexes(header, search_syncedfs)

    
    with open(sys.argv[2] + '.data', mode='w') as data:
        column_names = [''] # dummy element at beginning (gnuplot starts with index 1)
        for line in lines:
            out = ''
            columns = line.split(',')

            if line.startswith('time'):  # skip first line
                for i in searched_columns:
                    column = columns[i]
                    if column.find('/') != -1:
                        column_names.append(column[:column.find('/')])
                    else:
                        column_names.append(column)
                continue

            for i in searched_columns:
                out = out + columns[i] + ' '
            
            data.write(out + '\n')
        print column_names

    with open(sys.argv[2] + '.plot', mode='w') as plot:
        #plot.write('set terminal svg size 1280, 700 dynamic enhanced fname \'TheSans\' fsize 14 mousing name \"' + sys.argv[2].replace('-', '_') + '\" butt solid\n')
        plot.write('set terminal pdf size 13.7cm,7.7cm font \'TheSans,10\'\n')
        plot.write('set output \"' + sys.argv[2] + '.pdf\"\n')
        plot.write('set xlabel "time [seconds]"\n')
        plot.write('set ylabel "data rate [MB/s]"\n')
        plot_str = 'plot '
        for i in range(2, len(column_names)):
            plot_str = plot_str + '\"' + sys.argv[2] + '.data\" using 1:' + str(i) + ' title \"' + column_names[i] + '\" with lines, '

        plot.write(plot_str[:-2] + '\n')

    # png version
    #with open(sys.argv[2] + '.plot', mode='w') as plot:
    #   plot.write('set terminal png size 1280, 700\n')
    #    plot.write('set output \"' + sys.argv[2] + '.png\"\n')
    #    plot.write('set xlabel "time [seconds]"\n')
    #    plot.write('set ylabel "data rate [MB/s]"\n')
    #    plot_str = 'plot '
    #    for i in range(2, len(column_names)):
    #        plot_str = plot_str + '\"' + sys.argv[2] + '.data\" using 1:' + str(i) + ' title \"' + column_names[i] + '\" with lines, '

    #    plot.write(plot_str[:-2] + '\n')

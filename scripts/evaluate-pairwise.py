#!/usr/bin/env python3
import sys
sys.path.append('model/Simulator')
sys.path.append('model/Simulator/machinedb')

import os
import gzip
import tools
import argparse
import fileinput
import re
import time
from queue import Queue

from machineinfo import machines, commands

BASENAME_DEFAULT='pairwise'
BASENAME=None

BATCHSIZE=8 # The batchsize to use for evaluation

MACHINEDIR='./'

def parse_title(title):
    """Parse the title of the measurement

    @return Tuple (what, batch, src, dest), (None, None, None, None) in case of failure.
    """
    tmp = title.split('-')
    if len(tmp)!=4:
        return (None, None, None, None)
    return (tmp[0], int(tmp[1]), int(tmp[2]), int(tmp[3]))


def rm(s):
    """Can't believe I have to implement this myself"""
    try:
        os.remove(s)
    except OSError:
        pass

def parse_sk_m(line):
    """
    Parse sk_m measurements.

    This is a line having this format:
    $ sk_m_print(0,send-1-0-1) idx= 4 tscdiff= 42
    @return None, in case the lines is not an sk_m, a tuple (core, title, idx, tscdiff) otherwise

    """

    if line.startswith('sk_m_print'):
        (desc, _, idx, _, tscdiff) = line.split()
        desc = desc.replace('sk_m_print(', '')
        desc = desc[:-1]
        (core, title) = desc.split(',')
        return (int(core), title, int(idx), int(tscdiff))

    else:
        return None

def add_final(core, title, l, final):
    # receive-8-0-1
    global BATCHSIZE

    (what, batch, src, dest) = parse_title(title)

    if what == 'rtt' and int(batch) == 1 :
        final[(core, title)] =  tools.statistics_cropped(l, .1)

    if what == 'receive' and int(batch) == BATCHSIZE :
        l_normalized = [ x/float(1) for x in l]
        final[(core, title)] =  tools.statistics_cropped(l_normalized, .1)

    if what == 'send' and int(batch) == BATCHSIZE:
        # Normalize results depending on the batch size
        l_normalized = [ x/float(BATCHSIZE) for x in l]
        final[(core, title)] =  tools.statistics_cropped(l_normalized, .1)

def main(m, infile=sys.stdin):
    num = 0

    global BATCHSIZE

    f1 = MACHINEDIR + m + '/%s_send' % BASENAME
    f2 = MACHINEDIR + m + '/%s_receive' % BASENAME
    f3 = MACHINEDIR + m + '/%s_rtt' % BASENAME

    for mode in ['send', 'receive', 'rtt']:

        rm(MACHINEDIR + m + '/%s_%s.pdf' % (BASENAME, mode))
        rm(MACHINEDIR + m + '/%s_%s_reordered.pdf' % (BASENAME, mode))


    d = {}
    final = {}

    num_lines = 0
    start = time.time()

    q = Queue()

    num_cores = 0
    step_size = 1

    for l in infile:
        if len(l)<1:
            break

        num_lines += 1
        if (num_lines % 100000) == 0:
            print('   Read %dk lines' % (num_lines/1000))

        if l.startswith("NUM_CORES=") :
            tmp = l.split("=");
            num_cores = int(tmp[1])
            print("   NUM_CORES=" + str(num_cores))
            continue

        if l.startswith("STEP_SIZE=") :
            tmp = l.split("=");
            step_size = int(tmp[1])
            print("   STEP_SIZE=" + str(step_size))
            continue

        if l.startswith("NUM_MSG=") :
            tmp = l.split("=")
            if len(tmp) == 2 :
                BATCHSIZE = int(tmp[1])
                print("   BATCHSIZE=" + str(BATCHSIZE))
                continue

        # sk_m_print(0,send-1-0-1) idx= 1 tscdiff= 389
        o = parse_sk_m(l)
        if not o :
            # print 'Could not parse line', l
            continue

        (core, title, idx, tscdiff) = o

        # Only add if measurement is for the requested BATCH
        what, batch, src, dest = parse_title(title)

        if what == 'rtt' and batch != 1 :
            continue

        if what == 'send' and batch != BATCHSIZE:
            continue

        if what == 'receive' and batch != BATCHSIZE:
            continue

        if not (core,title) in d:
            d[(core,title)] = [ tscdiff ]
            q.put((core,title))
        else:
            d[(core,title)].append(tscdiff)

#        if len(d)>100:
            # Removing half the last 100 entries from the queue -- SK: WTF?
#            for i in range(50):
#                key = q.get()
#                (core, title) = key
#                l = d[key]
#                add_final(core, title, l, final)
#                del d[key]

    # Drain the rest of the queue
    for ((core, title), l) in list(d.items()):
        add_final(core, title, l, final)

    print('   Reading input file done, %d seconds' % (time.time()-start))

    print('   Generating', f1, f2)

    fsend = open(f1, 'w')
    freceive = open(f2, 'w')
    frtt = open(f3, 'w')

    num_snd = 0
    num_rcv = 0
    num_rtt = 0
    do_print = False

    num_entries = (num_cores) * (num_cores - 1) * 3 / step_size

    if len(final) != (num_entries) :
        print("   WARNING: dict size is: %d, expected %d" % (len(final), num_entries))

    print('   Number of entries in dict: %d' % len(final))

    #   3         send4-3-2          240.76          236.00           22.10 (90 values)
    for ((core, title), val) in list(final.items()):
        (mean, err, median, _, _) = val
        _tmp = title.split('-')

        (sender, receiver) = int(_tmp[2]), int(_tmp[3])
        if _tmp[0] == 'send' and int(_tmp[1]) == BATCHSIZE:
            fsend.write('%d %d %f %f\n' % (sender, receiver, mean, err))
            num_snd += 1
        if _tmp[0] == 'receive' and int(_tmp[1]) == BATCHSIZE:
            freceive.write('%d %d %f %f\n' % (sender, receiver, mean, err))
            num_rcv += 1

        if _tmp[0] == 'rtt' and int(_tmp[1]) == 1:
            frtt.write('%d %d %f %f\n' % (sender, receiver, mean, err))
            num_rtt += 1

        if (num_snd+num_rcv)%50 == 0 and do_print:
            print('   Found snd=%d/rcv=%d/rtt=%d entries' % (num_snd, num_rcv, num_rtt))
        else:
            do_print = False

    print('   Reading input file done snd=%d/rcv=%d/rtt=%d, %d seconds' % \
        (num_snd, num_rcv, num_rtt, time.time()-start))

    fsend.close()
    freceive.close()

if __name__ == "__main__":

#    import debug

    parser = argparse.ArgumentParser(description=(
        'Parses the output of the pairwise microbenchmark and'
        'creates a summary as output, which can be used by the'
        'Simulator and the plot script (./generate-pairwise)'
    ))
    parser.add_argument('--machine', default=None)
    parser.add_argument('--basename', default=BASENAME_DEFAULT,
                        help="Default: %s, use pairwise-nsend for nsend benchmark" % BASENAME_DEFAULT)
    args = parser.parse_args()

    BASENAME = args.basename

    main(args.machine)

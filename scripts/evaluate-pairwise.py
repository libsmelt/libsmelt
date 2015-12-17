#!/usr/bin/env python

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')

import tools
import argparse
import fileinput
import re

MACHINEDIR=os.getenv('HOME') + '/projects/netos-machine-hardware-information/'

def rm(s):
    """Can't believe I have to implement this myself"""
    try:
        os.remove(s)
    except OSError:
        pass

def main(m):
    num = 0

    f1 = MACHINEDIR + m + '/pairwise_send'
    f2 = MACHINEDIR + m + '/pairwise_receive'

    print 'Generating', f1, f2
    
    fsend = open(f1, 'w')
    freceive = open(f2, 'w')

    rm(MACHINEDIR + m + '/pairwise_send.pdf')
    rm(MACHINEDIR + m + '/pairwise_send_reordered.pdf')

    rm(MACHINEDIR + m + '/pairwise_receive.pdf')
    rm(MACHINEDIR + m + '/pairwise_receive_reordered.pdf')
    
    for line in fileinput.input('-'):
        m = re.match('\s+\d+\s+([a-z]+-\d+-\d+)\s+([0-9.]+)\s+([0-9.]+)\s+([0-9.]+)', line)
        if m:
            (title, mean, median, err) = (m.group(1), float(m.group(2)),
                                          float(m.group(3)), float(m.group(4)))
            _tmp = title.split('-')
            (sender, receiver) = int(_tmp[1]), int(_tmp[2])
            if _tmp[0] == 'send':
                fsend.write('%d %d %f %f\n' % (sender, receiver, mean, err))
            if _tmp[0] == 'receive':
                freceive.write('%d %d %f %f\n' % (sender, receiver, mean, err))
            num += 1

    print num

    fsend.close()
    freceive.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Help')
    parser.add_argument('machine')
    args = parser.parse_args()
    
    main(args.machine)

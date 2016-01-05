#!/usr/bin/env python

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')

import tools
import fileinput
import re
import argparse

def tsc():
    stsc = []
    rtsc = { i:[] for i in range(100) }

    for l in fileinput.input('-'):
        m = re.match('.*receive time (\d+) (\d+)', l)
        if m:
            # add to dict
            rtsc[int(m.group(1))].append(int(m.group(2)))
        m = re.match('.*send time \d+ (\d+)', l)
        if m:
            print l
            stsc.append(int(m.group(1)))

#    rtsc = [ (c, v) for (c,v) in rtsc.items() if len(v)>0 ]
    for (c, v) in rtsc.items():
        if len(v)<1:
            continue
        print "CPU", c
        print stsc
        print v
        v_diff = [ f-s for (f,s) in zip(v, stsc) ]
        print v_diff
        return

def barrier():
    r = []
    for l in fileinput.input('-'):
        m = re.match('.*time for barrier is: (\d+)', l)
        if m:
            r.append(int(m.group(1)))
            print r

    (mean, stderr, _, _, _) = tools.statistics(r)
    print 'Barrier', mean, stderr


def skm(crop):
    res = tools.parse_sk_m_input(sys.stdin).items()
    res = sorted(res)
    if len(res) < 1:
        return 
    maxlen = max([ len(title) for ((core, title), values) in res]) + 5
    print 'maxlen is', maxlen

    for ((core, title), values) in res:
        (mean, stderr, median, _, _) = tools.statistics_cropped(values, crop)
        print "%3d %*s %15.2f %15.2f %15.2f (%d values)" % \
            (core, maxlen, title, mean, median, stderr, len(values))

def main():
    parser = argparse.ArgumentParser(description='Evaluate sk_m measurements')
    parser.add_argument('--crop', type=float, default=.1,
                        help="How many values to crop from the result [0..1.0] (default 0.1)")
    args = parser.parse_args()
    skm(args.crop)

if __name__ == "__main__":
    main()

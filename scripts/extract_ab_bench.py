#!/usr/bin/env python

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')

from tools import statistics_cropped, parse_sk_m_input
import argparse
import re
import fileinput

sim = {}
lookup = {
    'binarytree': 'bintree'
    }

def parse_simulator_output(s, output=True):
    """Parse Simulator output from given stream <s>

    """

    curr_top = None
    for l in fileinput.input(s):

        # Simulating machine [sgs-r815-03] with topology [adaptivetree]
        m = re.match('Simulating machine \[(\S+)\] with topology \[(\S+)\]', l)
        if m:
            curr_top = m.group(2)

        # time, time (no ab)
        m = re.match('Cost for tree is: (\d+) \((\d+)\), last node is (\d+)', l)
        if m:
            sim[curr_top] = (int(m.group(2)), int(m.group(3)))
            curr_top = None

    if output:
        print sim

    return sim



def parse_log(s=sys.stdin, output=True):
    if output:
        print 'Parsing raw output from stdin for sk_m data'

    d = parse_sk_m_input(s)

    all_res = []

    for t in [ 'ab', 'reduction', 'barriers' ]:

        if output:
            print
            print '------------------------------'
            print t
            print '------------------------------'
            print

        res = {}

        for ((core, title), values) in d.items():

            # XXX store with respect to topo
            if title.startswith(t+'_'):
                (mean, stderr, vmin, vmax, median) = statistics_cropped(values)
                topo = title.replace(t+'_', '')
                if not topo in res:
                    res[topo] = []
                res[topo].append((core, median, stderr))


        res_t = []

        # Output
        for (topo, data) in res.items():
            data = sorted(data, key=lambda x: x[1], reverse=True)
            (c, m, e) =  data[0]

            # Simulator prediction
            pred = 0
            pred_ln = -1
            if t == 'ab':
                key = lookup[topo] if topo in lookup else topo
                (pred, pred_ln) = sim[key]

            res_t.append((topo, m, e, pred))

            if output:
                print '%-20s %5d %2d %8.2f     %3d %5d %6.2f' % \
                    (topo, m, c, e, pred_ln, pred, float(pred)/m )

        all_res.append((t, res_t))

    return all_res


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Help')
    parser.add_argument('sim', help='File with Simulator output')
    args = parser.parse_args()

    parse_simulator_output(args.sim)
    parse_log()

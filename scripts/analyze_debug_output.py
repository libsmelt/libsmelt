#!/usr/bin/env python

import sys
import os
sys.path.append(os.getenv('HOME') + '/bin/')

import tools
import re
import fileinput

d_send = {}
d_recv = {}
d_bc_send = {}

def output_ints_as_range(ints):

    ints = sorted(ints)

    f_prt_range = lambda x,y: '%s-%s' % (c_start, c_curr)

    c_start = None
    c_curr = None
    for c in ints:

        if c_start == None:

            c_start = c

        elif c != c_curr + 1 and c_curr != None:

            print(f_prt_range(c_start, c_curr), end=' ')
            c_start = c

        c_curr = c

    print(f_prt_range(c_start, c_curr), end=' ')


def increment_counter(d, fst, snd=None):

    if snd == None:
        if not fst in d:
            d[fst] = 0

        d[fst] += 1
        return


    if not fst in d:
        d[fst] = {}

    if not snd in d[fst]:
        d[fst][snd] = 0

    d[fst][snd] += 1

def main():

    d_bc_send_last = 0
    children = []

    for line in fileinput.input('-'):
        line = line.rstrip()

        # smlt: Node  0 sending to  0/ 8
        m = re.match('smlt: Node  (\d+) sending to  ([0-9 ]+)/([0-9 ]+)', line)
        if m:

            sndr = int(m.group(1))
            recv = int(m.group(3))

            increment_counter(d_send, sndr, recv)
            continue

        # smlt: Node 20: broadcast recv f
        m = re.match('smlt: Node (\d+): broadcast recv from parent chan', line)
        if m:

            recv = int(m.group(1))

            increment_counter(d_recv, recv)
            continue


        # That's just an index
        m = re.match('smlt: Node (\d+): broadcast send to child\[(\d+)\]', line)
        if m:

            sndr = int(m.group(1))
            recv = int(m.group(2))

            if not recv == d_bc_send_last:
                print('Warning: sending to child index %d, previous %d' %\
                    (recv, d_bc_send_last))

            d_bc_send_last += 1

            continue



        # Check children
        m = re.match('smlt: Child of (\d+) is (\d+) at pos (\d+)', line)
        if m:
            children.append(int(m.group(2)))


        m = re.match('smlt: smlt_node_start with', line)
        if m:
            continue


        print('Line not parsed: [%s]' % line)

    # Verify result
    # --------------------------------------------------
    output_ints_as_range(children)

    # --------------------------------------------------
    print(d_send)
    for sender, data in list(d_send.items()):

        for receiver, ctr in list(data.items()):

            if not receiver in d_recv or d_recv[receiver] != ctr:

                print('Receiver %d has receiver %d, but %d where sent' % \
                    (receiver, d_recv[receiver] if receiver in d_recv else -1, ctr))

    for i in range(64):
        if not i in list(d_send.items())[0][1]:
            print('Nothing sent to node %d' % i)

    for sender, data in list(d_bc_send.items()):
        for receiver, ctr in list(data.items()):
            if not receiver in d_send[sender]:
                print('Node %d has executed bc send, but not send' % \
                    (receiver))



if __name__ == "__main__":
    main()

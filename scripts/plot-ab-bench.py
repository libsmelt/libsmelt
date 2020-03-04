#!/usr/bin/python3

import matplotlib
matplotlib.use('Agg')

import sys
import os
import config

import numpy
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages

import plotsetup
import json
import gzip
import numpy as np

#import topology_parser
import helpers

from extract_ab_bench import parse_log, parse_metadata
    

#from tableau20 import colors, tab_cmap, tab3_cmap

PPT = True if os.environ.get('PPT') else False
SHOW = False if os.environ.get('HIDE') else True

PAPER = True if os.environ.get('PAPER') else False
PRESENTATION = PPT

import config
prop, prop_small = config.get_font(paper=PAPER, ppt=PPT)
matplotlib.rcParams['figure.figsize'] = 8, 5

MAX_Z_DIFF=.15 # Maximum range for colorbar (1.0 - MAX_Z_DIFF, 1.0 + MAX_Z_DIFF)
EXPORT_LATEX=True

def machine_name_translation(_in):

    print('Looking up', _in, '-->', end=' ')
    #val = generatelatex.get_machine_info(_in)
    return _in
    print(val)
    assert val
    print(val['shortname'])
    return val['shortname']


SHOW_ADAPTIVE=None # given as argument

ymax_d = {
    'sgs-r820-01': 70
}

MDIR='./measurements/'

def configure_plot(ax):
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)
    ax.get_xaxis().tick_bottom()
    ax.get_yaxis().tick_left()

label_lookup = {
    'ab': 'a. broadcast',
    'reduction': 'reduction',
    'barriers': 'barrier',
    'agreement': '2PC'
} if not PAPER and not PPT else {
    'ab': 'bcast',
    'reduction': 'red',
    'barriers': 'barrier',
    'agreement': '2PC'
}

def print_res(plotdata, machine, metadata):

    global arg
    global bl_comp


    for t in [ 'ab', 'reduction', 'barriers', 'agreement' ]:

        if arg.algorithm and arg.algorithm!=t:
            continue

        print()
        print('------------------------------')
        print(t)
        print('------------------------------')
        print()

        if not isinstance(plotdata, dict):
            plotdata = { a: { title: (v, e, 0) for (title, v, e, _) in x } for (a, x) in plotdata }

        val = list(plotdata[t].items())
        val = sorted(val, key=lambda x: x[1][0])

        best_other = ('n.a.', sys.maxsize)
        (baseline, _, _) = plotdata[t][config.get_adaptive_tree_variant(machine, metadata)]
        baseline = float(baseline)

        for topo, (m, e, pred) in val:

            if config.is_topo_ignore(topo, machine, metadata):
                continue

            fac = m/baseline

            if not 'adaptivetree' in topo and m<best_other[1]:
                best_other = (topo, min(best_other[1], m))

            color = helpers.bcolors.OKGREEN if fac>=1.01 else \
                    (helpers.bcolors.FAIL if fac<=0.99 else '')

            s = '%-30s %5d %8.2f %s %8.2f %s' % \
                (topo, m, e, color, fac, helpers.bcolors.ENDC)

            if arg.highlight and topo == arg.highlight:
                print(helpers.bcolors.BOLD + s + helpers.bcolors.ENDC)
            else:
                print(s)

        print('Best other: %30s %8.2f %8.2f' % \
            (best_other[0], best_other[1], best_other[1]/baseline))

        bl_comp.append((best_other[1]/baseline, best_other[1], best_other[0], t))


def run_sim(machine, topo):
    """Execute the Simulator for the given topology

    @param machine The machine to simulatr
    @param topo The topology to Simulator

    @return The cost of executing an atomic broadcast on that machine.
    """
    import subprocess
    import re

    cmd = [ './simulator.py',  machine, topo ]
    res = None

    try:
        print('Executing', ' '.join(cmd))
        for l in subprocess.check_output(cmd, stderr=subprocess.STDOUT, cwd="/home/skaestle/projects/Simulator/").split('\n'):

            m = re.match('Cost atomic broadcast for tree is: (\d+) \(\d+\), last node is \d+', l)
            if m:
                res = int(m.group(1))

    except subprocess.CalledProcessError:
        helpers.warn(('Simulator failed for machine %s topo %s ' % (machine, topo)))
        raise

    return res



def plot_heatmap(store):
    """Plot a heat map comparing the adaptive tree variations.

    @param store: A dictionary mapping machine name -> mbench results
    """
    topos = set([v for _, v in list([ v for _, v in list(store.items()) ][0].items()) ][0].keys())
    topos = [ t for t in topos if 'adaptive' in t ]

    machines = list(store.keys())
    algo = 'ab'
    normalize = 'adaptivetree-nomm-shuffle-sort'

    topos = [ t for t in topos if not t == normalize ]


    # Assign a x-axis position for each machine, based on sorted num_cores array
    num_cores_used = { m: c for (m, c) in list(num_cores.items()) if m in machines }
    num_cores_ordered = { machine: idx for ((machine, numcores), idx) in \
                          zip(sorted(list(num_cores_used.items()), key=lambda x: x[1]), \
                              list(range(len(num_cores_used)))) }

    # make these smaller to increase the resolution
    dx, dy = 1.0, 1.0 # 0.15, 0.05

    # generate grid for the x & y bounds
    Y, X = np.mgrid[slice(0, len(topos)+1, dy),
                    slice(0, len(machines)+1, dx)]

    z = np.zeros((len(topos), len(machines)), dtype=np.float)

    l_machines = [ None for i in range(len(machines)) ]

    for m in machines:

        assert m in num_cores_ordered
        _x = num_cores_ordered[m]

        l_machines[_x] = machine_name_translation(m)

        for i,t in enumerate(topos):
            v, _, _ = store[m][algo][t]
            ref, _, _ = store[m][algo][normalize] # should be smaller
            z[i,_x] = v/ref # should be > ref

    # Maximum distance from 1.0
    z_min, z_max = np.min(z), np.max(z)
    z_min = abs(1-z_min)
    z_max = abs(1-z_max)
    z_dist = max(z_min, z_max)

    z_dist = MAX_Z_DIFF

    f = 'ab-bench-adaptive-heatmap'
    with PdfPages(f + '.pdf') as pdf:

        fig, ax = plt.subplots()

        ## Add measurements to heatmap
        for x in range(0,len(machines)):
            for y in range(0, len(topos)):
                color = 'black'
                plt.text(x + 0.5, y + 0.5, '%.2f' % (z[y][x]),
                         horizontalalignment='center',
                         verticalalignment='center',
                         color=color,
                         fontproperties=prop)


        plt.pcolor(X, Y, z, cmap=tab_cmap, vmin=1-z_dist, vmax=1+z_dist)
        plt.colorbar()

        ax.set_xticks([ i + 0.5 for i in range(len(machines))])
        ax.set_xticklabels(l_machines)

        ax.set_yticks([ i + 0.5 for i in range(len(topos))])
        ax.set_yticklabels(topos)

        ax.set_xlim(xmin=0,xmax=len(machines))
        ax.set_ylim(ymin=0,ymax=len(topos))

        fig.autofmt_xdate() # rotate x-labels
        plt.tight_layout() # re-create bounding-box

        fout = f + '.png'
        plt.savefig(fout)
        pdf.savefig(bbox_inches='tight')


def ab_comparison(plotdata, metadata):
    """Compare adaptive tree configurations

    """
    assert len(list(metadata.items())) > 0

    f_name = lambda x: x.replace('adaptivetree', 'AT').replace('-shuffle-sort', 'optimized')
    m_name = lambda x: x.rstrip('0123456789') if not x.startswith('sgs') else x

    t_labels = [ 'ATopt', 'AT' ]
    f = { t: open(f_name(t), 'w') for t in t_labels }

    m_ids = [ '\\machine{%s}' % m_name(x) for x in list(plotdata.keys())]
    f_labels = open('at-labels', 'w')
    f_labels.write('\\newcommand{\\atcomplabels}{%s}\n' % ','.join(m_ids))
    f_labels.write('\\newcommand{\\atcompticks}{%s}\n' % ','.join(str(i) for i in range(len(list(plotdata.keys())))))
    f_labels.close()

    res = { m: [ 0 for x in range(len(t_labels)) ] for m in list(plotdata.keys()) }

    for t in t_labels:
        f[t].write('x y e\n')

    for i, (m, data) in enumerate(plotdata.items()):

        if m == 'phi':
            continue

        topos = [ config.get_adaptive_tree_variant(m, metadata),
                  config.get_adaptive_tree_variant(m, metadata).replace('-shuffle-sort', '') ]

        data = data['ab']
        for j, (t, t_name) in enumerate(zip(topos, t_labels)):
            mean, stderr, _ = data[t]
            f[t_name].write('%d %f %f\n' % (i, mean/1000., stderr/1000.))
            res[m][j] = mean/res[m][0] if j > 0 else mean

    for t in t_labels:
        f[t].close()

    # Statistics per machine
    for m in list(plotdata.keys()):
        for i, t in enumerate(topos):
            if i==0:
                continue
            print('%20s %30s %8.2f' % (m, t, res[m][i]))

    # Global statistics
    for idx, t in enumerate(topos):
        if idx == 0:
            continue
        l = [ val for m, d in list(res.items()) for i, val in enumerate(d) if i == idx ]
        print('%30s %8.2f' % (t, float(sum(l))/len(l)))

        l.remove(max(l))
        print('%30s %8.2f' % (t, float(sum(l))/len(l)))



# def ab_sim_precision(plotdata, metadata):
#     """Output a heatmap showing the precision of the Simulator compared to
#     execution on real hardware.

#     """

#     # Read previous simulator results from file
#     _json = 'sim_results.json'
#     try:
#         global arg
#         if arg.force:
#             raise Exception('Ignoring json file - force reload')
#         with open(_json, 'r') as f:
#             sim_cache = json.loads(f.read())
#             helpers.warn("Reading cached Simulator results from %s" % _json)
#             f.close()
#     except:
#         sim_cache = {}


#     #   import pdb; pdb.set_trace()
#     translate_m_name = lambda x: x if not 'adaptive' in x else 'AT optmized'

#     # Re-add "adaptivetree-nomm-shuffle-sort"
#     topos = [ 'adaptivetree-shuffle-sort',
#               'bintree',
#               'fibonacci',
# #              'sequential',
#               'badtree',
#               'cluster',
#               'mst'
#           ]

#     machines = list(plotdata.keys())

#     m_inst = { s: topology_parser.parse_machine_db(s) for s in machines }

#     # Number of sockets, cpu's and physical cores per socket for sorting
#     m_sorted = { s: (m['sockets'],
#                      m['numcpus'],
#                      m['cores'],
#                      m['threads'])  for s, m in list(m_inst.items()) }

#     if 'phi' in m_sorted:
#         m_sorted['phi'] = (1, 60, 60)

#     # Short names for machines
#     translate = { s: topology_parser.generate_short_name(m) for s, m in list(m_inst.items()) }
#     for t in translate:
#         assert t != None

#     # Assign a x-axis position for each machine, based on core configuration
#     num_cores_ordered = { machine: idx for ((machine, numcores), idx) in \
#                           zip(sorted(list(m_sorted.items()), key=lambda x: (x[1][3], x[1][1], x[1][0], x[1][2])), \
#                               list(range(len(m_sorted)))) }


#     # make these smaller to increase the resolution
#     dx, dy = 1.0, 1.0 # 0.15, 0.05

#     # generate grid for the x & y bounds
#     Y, X = np.mgrid[slice(0, len(topos)+1, dy),
#                     slice(0, len(machines)+1, dx)]

#     z = np.zeros((len(topos), len(machines)), dtype=np.float)

#     l_machines = [ None for i in range(len(machines)) ]

#     ## < Prepare heatmap end

#     for (m, data) in list(plotdata.items()):

#         data = data['ab']

#         # Determine x-position of that machine (depends on #cores)
#         x = num_cores_ordered[m]

#         # Set x-label
#         l_machines[x] = machine_name_translation(m)

#         for y, t in enumerate(topos):

#             # Execute Simulator prediction
#             # ------------------------------
#             key = '%s/%s' % (m, t)
#             if key in sim_cache:
#                 sim_prediction = sim_cache[key]
#             else:
#                 sim_prediction = run_sim(m, t)
#                 sim_cache[key] = sim_prediction

#             # Extract hardware result
#             # ------------------------------

#             # adaptive tree
#             if 'adaptive' in t:
#                 val, err, _ = data[config.get_adaptive_tree_variant(m, metadata)]

#             else:
#                 t_ = t if not config.is_topo_ignore(t, m, metadata) else t + '_naive'
#                 assert not config.is_topo_ignore(t_, m, metadata)

#                 val, err, _ = data[t]

#             fact = float(sim_prediction)/val
#             print('PRED %20s %20s hw %9.1f sim %7.2f putting at %2d %d as %9.2f' % \
#                 (m, t, val, sim_prediction, y, x, fact))

#             z[y, x] = fact


#     ## Store Simulator cache
#     f = open(_json, 'w')
#     json.dump(sim_cache, f)
#     f.close()

#     ## Generate heatmap
#     f = 'sim-precision'
#     with PdfPages(f + '.pdf') as pdf:

#         fig, ax = plt.subplots()

#         ## Add measurements to heatmap
#         for x in range(0,len(machines)):
#             for y in range(0, len(topos)):
#                 color = 'black'
#                 plt.text(x + 0.5, y + 0.5, '%.2f' % (z[y][x]),
#                          horizontalalignment='center',
#                          verticalalignment='center',
#                          color=color,
#                          fontproperties=prop)


#         MAX_Z_DIFF = 0.5
#         plt.pcolor(X, Y, z, cmap=tab3_cmap, vmin=1-MAX_Z_DIFF, vmax=1+0.5*MAX_Z_DIFF)
#         plt.colorbar()

#         ax.set_xticks([ i + 0.5 for i in range(len(machines))])
#         ax.set_xticklabels(l_machines)

#         ax.set_yticks([ i + 0.5 for i in range(len(topos))])
#         ax.set_yticklabels([ translate_m_name(t) for t in topos ])

#         ax.set_xlim(xmin=0,xmax=len(machines))
#         ax.set_ylim(ymin=0,ymax=len(topos))

#         fig.autofmt_xdate() # rotate x-labels
#         plt.tight_layout() # re-create bounding-box

#         print('Writing output to ', f)
#         pdf.savefig(bbox_inches='tight')

#     ## Output some statistics
#     all_factors = []
#     factors_no_smt = []
#     all_topos = { t: [] for t in topos }

#     for (m, data) in list(plotdata.items()):

#         # Determine x-position of that machine (depends on #cores)
#         x = num_cores_ordered[m]

#         machine_factors = []
#         print('%s -> %s' % (m, m_inst[m]['threads']))
#         smt = (int(m_inst[m]['threads']) > 1)

#         for y, t in enumerate(topos):

#             rel_error = abs(1-z[y, x])
#             machine_factors.append(rel_error)
#             all_factors.append(rel_error)
#             all_topos[t].append(rel_error)

#             if not smt:
#                 factors_no_smt.append(rel_error)

#         print('Machine %20s avg %10.2f min %10.2f max %10.2f' % \
#             (m, float(sum(machine_factors))/len(machine_factors),
#              min(machine_factors), max(machine_factors)))

#     for t in topos:
#         print('Topology %20s avg %10.2f min %10.2f max %10.2f' % \
#             (t, float(sum(all_topos[t]))/len(all_topos[t]),
#              min(all_topos[t]), max(all_topos[t])))


#     print('all avg %f min %f max %f' % \
#         (float(sum(all_factors))/len(all_factors),
#          min(all_factors), max(all_factors)))
#     print('no smt avg %f min %f max %f' % \
#         (float(sum(factors_no_smt))/len(factors_no_smt),
#          min(factors_no_smt), max(factors_no_smt)))

#     exit (0)

def export_csv(plotdata, machine):

    exp_data = {}
    exp_err = {}

    topos = set([ topo for _, l in list(plotdata.items()) \
                  for topo, _ in list(l.items()) if not 'naive' in topo ])
    print('Topos are', topos)
    exit (0)

#
# Barchart
#
def multi_bar_chart(plotdata, machine, metadata=None):

    """Plot ab-bench

    @param topos Restrict the plot to these topologies. Set None to
    display default topologies.

    """

    output_for_poster = False
   # print('Plotting data', len(plotdata), plotdata)

    import sys
    import os

    print(plotdata)

    # Parse topos from measurment file
    m_topos = set([ topo for _, l in list(plotdata.items()) \
                  for topo, _ in list(l.items()) ])

    bars = [ t for t in m_topos if not config.is_topo_ignore(t, machine, metadata) if not 'adaptive' in t ]
    if len(bars) < 7:
        helpers.warn(('Enough bars?'))

    helpers.info(('Using bars %s' % str(bars)))

    N = len(plotdata)
    print(N)
    ind = numpy.arange(N)

    _width = len(bars)+1 if SHOW_ADAPTIVE==True else len(bars)
    width = 1./(_width + 1)

    # Plot the datall
    f = 'measurements/%s/ab-bench' % machine

    if SHOW_ADAPTIVE:
        bars += [ config.get_adaptive_tree_variant(machine, metadata) ]

    # Change file-name
    if not SHOW_ADAPTIVE:
        f += '_no_adaptive'
    if PAPER:
        f += '.PAPER'
    if not SHOW:
        f += '_empty'

    order_labels = { 'ab': 0,
                     'reduction': 1,
                     'barriers': 2,
                     'agreement': 3
    }

    with PdfPages('%s.pdf' % f) as pdf:

        fig, ax = plt.subplots()
        _labels = [ (l, order_labels[l]) for (l, _) in list(plotdata.items()) ]
        labels = [ label_lookup.get(l, l) for (l, _) in sorted(_labels, key=lambda x: x[1]) ]

        colors = plt.get_cmap('PuOr', 9)
        # colors = brewer2mpl.get_map('PuOr', 'diverging', 9).mpl_colors
        hs = [ '.', '/', '//', None,  '\\', '\\\\', '*', None, 'o', 
              '.', '/', '//', None,  '\\', '\\\\', '*', None, 'o' ]

        legends = []
        n = 0 #if SHOW_ADAPTIVE else 1

        _vmax = 0

        # One bar per algorithm
        exp_dat = { b: [] for b in bars }
        exp_err = { b: [] for b in bars }
        exp_label = { b: [] for b in bars }
        for b in bars:

            v = [ 0 for l in labels ]
            yerr = [ 0 for l in labels ]

            for (algo, l) in list(plotdata.items()):
                for topo, (vmean, vstderr, est) in list(l.items()):
                    if (topo==b):
                        idx = order_labels[algo]
                        _vmax = max(_vmax, vmean/1000.)

                        if SHOW:
                            v[idx] = vmean/1000.
                            yerr[idx] = vstderr/1000.

                        else:
                            v[idx] = 0
                            yerr[idx] = 0

            if len(v) == N and len(yerr) == N:
                r = ax.bar(ind+(n+0.5)*width, v, width, \
                           color=colors(n), hatch=hs[n], yerr=yerr, \
                           error_kw=dict(ecolor='gray'))

                bar_label = 'AT' if 'adaptive' in b else b
                bar_label = bar_label.replace('-naive', '')

                legends.append((r, bar_label))
                n+=1

                exp_dat[b] = v
                exp_err[b] = yerr
                exp_label[b] = bar_label

                if EXPORT_LATEX:
                    _f = f + ('_%s.dat' % bar_label)
                    print('Exporting LaTeX to', _f)
                    idx = 0
                    with open(_f, 'w') as fl:
                        fl.write('x y e\n')
                        for (v,e) in zip(v, yerr):
                            fl.write('%d %f %f\n' % (idx, v, e))
                            idx += 1
                        fl.close()
            else:
                print('NOT Adding bar - does not have the right dimension ', \
                    v, yerr)


        # Export CSV file
        _f = f + '.csv'
        print('Writing CSV data to %s' % _f)
        with open(_f, 'w') as fc:

            # Header
            fc.write('algorithm')
            for b in bars:
                b_ = exp_label[b]
                fc.write(',' + b_ + ',' + b_ + '_stderr')
            fc.write('\n')

            # data
            for a, i in list(order_labels.items()):
                fc.write('%s' % a)
                for b in bars:
                    try:
                        val = exp_dat[b][i]
                        err = exp_err[b][i]
                    except IndexError:
                        val = -1
                        err = 0
                    fc.write(',%f,%f' % (val, err))
                fc.write('\n')

            fc.close()

        if not output_for_poster:
            if not PAPER and not PPT:
                ax.set_xlabel('Tree topology ' + machine, fontproperties=prop)
            ax.set_ylabel('Execution time [x1000 cycles]', fontproperties=prop)


        ax.set_xticks(ind + n/2.0*width)

        ax.set_ylim(ymin=0, ymax=_vmax*1.08)
        if machine in ymax_d:
            ax.set_ylim(ymax=ymax_d[machine])
            ax.text(3.625, 66, "93.5", ha='center', rotation=90,
                    fontproperties=prop)

        ylticks = [int(i) for i in ax.get_yticks()]
        ax.set_yticklabels(list(map(str, ylticks)), fontproperties=prop)

        ax.set_xticklabels(labels, fontproperties=prop)

        configure_plot(ax)
        lgnd_title = topology_parser.generate_short_name(topology_parser.parse_machine_db(machine), longer=True) if PPT else None
        lgnd_boxes, lgnd_labels = list(zip(*legends))
        ax.legend(lgnd_boxes, lgnd_labels, title=lgnd_title,
                  loc=2, ncol=2, borderaxespad=0.,
                  bbox_to_anchor=(0.0, 1.01, .75, .102), prop=prop)

        ax.set_axisbelow(True)
        ax.yaxis.grid(linestyle='-', linewidth=.5, color='.8')

        if output_for_poster:
            print('Saving picture to', f)
            plt.savefig(f + '.png', dpi=1200)
        elif PRESENTATION:
            print('Saving picture to', f)
            plt.savefig(f + '.png', dpi=400, bbox_inches='tight')
        else:
            print('Saving picture to', f)
            pdf.savefig(bbox_inches='tight')


def generate_machine(m, store, _metadata):

    # Parse ab-bench results
    # --------------------------------------------------

    _raw = MDIR + '%s/ab-bench.last.gz' % m
    _json = _raw + '.json'

    print("parsing %s" % _raw)

    # Read meta-data
    meta = {}
    try:
        f = gzip.open(_raw)
        parse_metadata(f, meta_data=meta)
        f.close()
    except IOError:
        raise
    except:
        raise
    _metadata[m] =  meta


    # Try reading from Json file
    try:
        global arg
        if arg.force:
            raise Exception('Ignoring json file - force reload')
        with open(_json, 'r') as f:
            _mbench = json.loads(f.read())
            mbench = { a: { title: (v, e, 0) for (title, v, e, _) in x } for (a, x) in _mbench }
            print('json summary exists, reading from there .. ')
            f.close()
    except:
        print('json summary does not exist, generating .. ')

        mbench = parse_log(gzip.open(_raw), True)

        f = open(_json, 'w')
        json.dump(mbench, f)
        f.close()

        # Re-read from json to guarantee same format
        with open(_json, 'r') as f:
            _mbench = json.loads(f.read())
            mbench = { a: { title: (v, e, 0) for (title, v, e, _) in x } for (a, x) in _mbench }
            f.close()

    store[m] = mbench

    if arg.export:
        print('Exporting data .. ')
        export_csv(mbench, m)

    # Done

    print('Printing result .. ')
    print_res(mbench, m, metadata)

    print('Plotting bar char')
    multi_bar_chart(mbench, m, metadata)


global arg

import argparse
parser = argparse.ArgumentParser()
parser.add_argument('--machines', help="Koma separated list of machines")
parser.add_argument('--showadaptive', dest='showadaptive', action='store_false')
parser.add_argument('--highlight', help='Which measurement should be highlighted?')
parser.add_argument('--algorithm', help='Algorithm to evaluate. Default: all')
parser.add_argument('--plot', dest='plot', action='store_true')
parser.add_argument('--export', dest='export', action='store_true')
parser.add_argument('--heatmap', dest='heatmap', action='store_true')
parser.add_argument('--ab-comparison', dest='ab_comparison', action='store_true')
parser.add_argument('--sim', dest='sim', action='store_true')
parser.add_argument('-f', dest='force', action='store_true')
parser.set_defaults(showadaptive=True, force=False, plot=False, heatmap=False,
                    ab_comparison=False, sim=False, export=False)
arg = parser.parse_args()

machines = config.machines \
           if not arg.machines else arg.machines.split(',')

SHOW_ADAPTIVE = arg.showadaptive

# Comparison with baselines
global bl_comp
bl_comp = []

# Store all results
store = {}
metadata = {}

for m in machines:

    print()
    print(helpers.bcolors.HEADER + helpers.bcolors.UNDERLINE + \
        m + helpers.bcolors.ENDC)
    print()

    done = False

    try:
        generate_machine(m, store, metadata)
        assert len(metadata) > 0
        done = True
    except IOError as e:
        print('IOError - probably measurement file does not exist')
    except Exception as e:
        print('Failed for machine %s' % m)
        raise

    if not done:
        print(helpers.bcolors.FAIL + 'Failed for machine ' + helpers.bcolors.UNDERLINE\
            + m + helpers.bcolors.ENDC)


print('All read, ', len(metadata))

if arg.plot and arg.ab_comparison:
    print('Executing ab comparison')
    ab_comparison(store, metadata)
    exit(0)

elif arg.plot and arg.sim:
    print('Generating Simulator evaluation')
    if 'phi' in store:
        del store['phi']
    ab_sim_precision(store, metadata)
    exit(0)


# Print statistics
# --------------------------------------------------

select_algo = arg.algorithm if arg.algorithm else 'ab'

best_other = {}
bl_ab = [ (a, b, c, topo) for (a, b, c, topo) in bl_comp if topo == select_algo ]
avg_rel_slowdown = 0.0
for (rel_slowdown, _, name, _) in bl_ab:
    best_other[name] = best_other.get(name, 0) + 1
    avg_rel_slowdown += rel_slowdown

print()
print(helpers.bcolors.HEADER + helpers.bcolors.UNDERLINE + \
    'STATISTICS' + helpers.bcolors.ENDC)
print()


if len(bl_ab)>0:
    print('Average relative slowdown of next best %8.2f' % (avg_rel_slowdown/float(len(bl_ab))))
    print(best_other)


if arg.heatmap:
    print('Generating heatmap')
    plot_heatmap(store)


exit(0)

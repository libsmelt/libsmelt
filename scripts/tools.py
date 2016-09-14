#!/usr/bin/env python

import sys
import re
import os
import subprocess
import numpy
import fileinput
from datetime import *

## NOTE: We want this file with only dependencies on standart-library things
## Dependencies to own files are an absolut no-go

def parse_sk_m(line):
    """
    Parse sk_m measurements.

    This is a line having this format:
    $ sk_m_print(6,shm_fifo) idx= 249 tscdiff= 1969

    @return None, in case the lines is not an sk_m, a tuple (core, title, idx, tscdiff) otherwise

    """

    # Replace () pairs in label
    m = re.match('sk_m_print\((\d+),(.*)\)\s+idx=\s*(\d+)\s+tscdiff=\s*(\d+)', line)
    if m:
        core = int(m.group(1))
        title = m.group(2)
        idx = int(m.group(3))
        tscdiff = int(m.group(4))
        return (core, title, idx, tscdiff)

    else:
        return None

def parse_sk_m_input(stream=sys.stdin):
    """Read all lines from given handle and execute parse_sk_m on all of
    them. The stream from which to read data is given as argument
    stream. The default is to read from stdin.

    @return: A dict (core, title) -> [values .. ]

    """
    d = {}

    for l in stream:
        if len(l)<1:
            break
        
        o = parse_sk_m(l)
        if not o:
            continue
        (core, title, idx, tscdiff) = o
        if not (core,title) in d:
            # Create a new array
            d[(core,title)] = []

        d[(core,title)].append(tscdiff)

    return d


def statistics_cropped(l, r=.1):
    """Print statistics for the given list of integers

    @param r Crop ratio. .1 corresponds to dropping the 10% highest
    values. Note that cropping elements from a list is an expensive
    operationg for large lists.

    @return A tuple (mean, stderr, min, max, median)
    """
    if not isinstance(l, list) or len(l)<1:
        return None

    if r>0:
        # Crop list
        # --------------------------------------------------

        crop = int(len(l)*r)
        assert crop<len(l)

        # We have to remove the <crop> highest values
        import heapq
        highest = heapq.nlargest(crop, l)

        assert len(highest) == crop

        for h in highest:
            l.remove(h)


    return statistics(l)


def statistics(l):
    """
    Print statistics for the given list of integers
    @return A tuple (mean, stderr, median, min, max)
    """

    # given argument is ALREADY an numpy array
    if isinstance(l, numpy.ndarray):
        nums = l

    # given argument is no list type
    elif not isinstance(l, list) or len(l)<1:
        return None

    # given argument is list and can be converted to numpy.array
    else:
        nums = numpy.array(l)

    m = nums.mean(axis=0)
    median = numpy.median(nums, axis=0)
    d = nums.std(axis=0)

    return (m, d, median, nums.min(), nums.max())


def latex_header(f, args=[]):
    header = (
        "\\documentclass[a4wide]{article}\n"
        "\\usepackage{url,color,xspace,verbatim,subfig,ctable,multirow,listings}\n"
        "\\usepackage[utf8]{inputenc}\n"
        "\\usepackage[T1]{fontenc}\n"
        "\\usepackage{txfonts}\n"
        "\\usepackage{rotating}\n"
        "\\usepackage{paralist}\n"
        "\\usepackage{subfig}\n"
        "\\usepackage{graphics}\n"
        "\\usepackage{enumitem}\n"
        "\\usepackage{times}\n"
        "\\usepackage{amssymb}\n"
        "\\usepackage[colorlinks=true]{hyperref}\n"
        "\\usepackage[ruled,vlined]{algorithm2e}\n"
        "\n"
        "\\graphicspath{{figs/}}\n"
        "\\urlstyle{sf}\n"
        "\n"
        "\\usepackage{tikz}\n"
        "\\usepackage{pgfplots}\n"
        "\\usetikzlibrary{shapes,positioning,calc,snakes,arrows,shapes,fit,backgrounds}\n"
        "\n"
        "%s\n"
        "\\begin{document}\n"
        "\n"
        ) % '\n'.join(args)
    f.write(header)


def _pgf_footer(f):
    s = ("  \\end{tikzpicture}\n"
         "\\end{figure}\n")
    f.write(s)


def _pgf_header(f, caption='TODO', label='TODO'):
    s = (("\\begin{figure}\n"
          "  \\caption{%s}\n"
          "  \\label{%s}\n"
          "  \\begin{tikzpicture}[scale=.75]\n")
         % (caption, label))
    f.write(s)


def latex_footer(f):
    footer = (
        "\n"
        "\\end{document}\n"
        )
    f.write(footer)


def _pgf_plot_header(f, plotname, caption, xlabel, ylabel, attr=[], desc='...'):
    label = "pgfplot:%s" % plotname
    s = (("Figure~\\ref{%s} shows %s\n"
          "\\pgfplotsset{width=\linewidth}\n") % (label, desc))
    if xlabel:
        attr.append('xlabel={%s}' % xlabel)
    if ylabel:
        attr.append('ylabel={%s}' % ylabel)
    t = ("    \\begin{axis}[%s]\n") % (','.join(attr))
    f.write(s)
    _pgf_header(f, caption, label)
    f.write(t)


def _pgf_plot_footer(f):
    f.write("    \\end{axis}\n")
    _pgf_footer(f)


def do_pgf_3d_plot(f, datafile, caption='', xlabel=None, ylabel=None, zlabel=None):
    """
    Generate PGF plot code for the given data
    @param f File to write the code to
    @param data Data points to print as list of tuples (x, y, err)
    """
    attr = ['scaled z ticks=false',
            'z tick label style={/pgf/number format/fixed}']
    if zlabel:
        attr.append('zlabel={%s}' % zlabel)
    now = datetime.today()
    plotname = "%02d%02d%02d" % (now.year, now.month, now.day)
    _pgf_plot_header(f, plotname, caption, xlabel, ylabel, attr)
    f.write(("    \\addplot3[surf] file {%s};\n") % datafile)
    _pgf_plot_footer(f)


def run_pdflatex(fname, openFile=True):
    d = os.path.dirname(fname)
    if d == '':
        d = '.'
    print 'run_pdflatex in %s' % d
    if subprocess.call(['pdflatex',
                     '-output-directory', d,
                     '-interaction', 'nonstopmode', '-shell-escape',
                        fname], cwd=d) == 0:
        if openFile:
            subprocess.call(['okular', fname.replace('.tex', '.pdf')])


def dump_stdin_no_colors():
    import fileinput
    for line in fileinput.input('-'):
        print remove_ascii(line)

def remove_ascii(line):        
    ansi_escape = re.compile(r'\x1b[^m]*m')
    return ansi_escape.sub('', line.rstrip())

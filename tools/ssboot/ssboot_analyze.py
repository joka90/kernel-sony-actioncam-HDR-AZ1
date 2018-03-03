#! /usr/bin/env python
#
# Sony CONFIDENTIAL
#
# Copyright 2012 Sony Corporation
#
# DO NOT COPY AND/OR REDISTRIBUTE WITHOUT PERMISSION.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

import yaml
import sys
import xlwt
# import xlrd to get column name and cell name
import xlrd

#============================ parse functions =================================
from yaml import load, dump
try:
    from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper

def value_inc(dic, key):
    dic.setdefault(key, 0)
    dic[key] += 1

def parse_line(line, dpids, dfiles,
               pid_pcount, pid_vcount, file_pcount, type_pcount,
               type_pid_pvcount, pid_file_pcount):

    data = yaml.load(line, Loader=Loader)
    page_type = data['type']
    value_inc(type_pcount, page_type)

    if page_type == 'pgcache':
        fname = data['file']
        value_inc(file_pcount, fname)
        dfiles.add(fname)

    if 'vma' in data:
        distinct_pids = set()
        for vma in data['vma']:
            pids = vma['pid']
            distinct_pids.update(set(pids))
            for pid in pids:
                value_inc(pid_vcount, pid)
                value_inc(type_pid_pvcount['vma'][page_type], pid)
        dpids.update(distinct_pids)

        for pid in distinct_pids:
            value_inc(pid_pcount, pid)
            value_inc(type_pid_pvcount['page'][page_type], pid)

            if page_type == 'pgcache':
                fname = data['file']
                pid_file_pcount.setdefault(pid, {fname: 0})
                value_inc(pid_file_pcount[pid], fname)

    else:
        #these pages are not mapped
        pid = '-1(kernel-or-not-mapped)'
        value_inc(pid_pcount, pid)
        value_inc(pid_vcount, pid)
        value_inc(type_pid_pvcount['vma'][page_type], pid)
        value_inc(type_pid_pvcount['page'][page_type], pid)
        if page_type == 'pgcache':
            fname = data['file']
            value_inc(pid_file_pcount[pid], fname)

#============================== main ==========================================
#--------------------------- option parse -------------------------------------
from optparse import OptionParser

usage = "usage: %prog -i input [-o output]"
parser = OptionParser(usage = usage)
parser.add_option("-i", "-f", "--input", "--file", dest = "input_file",
                  help = "Input filename")
parser.add_option("-o", "-x", "--output", "--xls",
                  dest = "output_file", default = "output.xls",
                  help = "Output xls filename [Default: output.xls]")
parser.add_option("-v", "--verbose", action = "store_true", dest = "verbose",
                  default = False, help = "Be verbose [Default: off]")

(options, args) = parser.parse_args()

if not options.input_file:
    parser.print_help()
    sys.exit()
else:
    stream = file(options.input_file, 'r')

xlsfile = options.output_file
verbose = options.verbose

if verbose:
    def vprint(*args):
        for arg in args:
            print arg,
        print
else:
    vprint = lambda *a: None

vprint("Parsing", options.input_file + "...")

#------------------------------ init ------------------------------------------
pid_pcount = {'-1(kernel-or-not-mapped)': 0}
pid_vcount = {'-1(kernel-or-not-mapped)': 0}
file_pcount = {'libsonyc.so': 0}
type_pcount = {'kernel': 0}
type_pid_pvcount = {
    'vma': {
        'pgcache': {'-1(kernel-or-not-mapped)': 0},
        'anon': {'-1(kernel-or-not-mapped)': 0},
        'kernel': {'-1(kernel-or-not-mapped)': 0}
        },
    'page': {
        'pgcache': {'-1(kernel-or-not-mapped)': 0},
        'anon': {'-1(kernel-or-not-mapped)': 0},
        'kernel': {'-1(kernel-or-not-mapped)': 0}
        }
    }
pid_file_pcount = {
    '-1(kernel-or-not-mapped)': {'libsonyc.so': 0}}

dpids = set({'-1(kernel-or-not-mapped)'})
dfiles = set({'libsonyc.so'})

#------------------------------ parse -----------------------------------------
for line in stream.readlines():
    parse_line(line, dpids, dfiles,
               pid_pcount, pid_vcount, file_pcount, type_pcount,
               type_pid_pvcount, pid_file_pcount)

#------------------------------ output ----------------------------------------

dpids_list = list(dpids)
dfiles_list = list(dfiles)
dpids_list.sort()
dfiles_list.sort()

vprint('Creating xls file ' + xlsfile)
percentstyle = xlwt.easyxf(num_format_str = '0.000%')
xbook = xlwt.Workbook()

process_sheet = xbook.add_sheet('process')
vprint('pid\tmapped_page_count\tpage_count')
process_sheet.write(0, 0, 'pid')
process_sheet.write(0, 1, 'mapped_page_count')
process_sheet.write(0, 2, 'page_count')
process_sheet.write(0, 3, 'mapped_page%')
process_sheet.write(0, 4, 'page%')
i = 1
for pid in pid_pcount:
    vprint(pid + '\t', pid_vcount[pid], '\t', pid_pcount[pid])
    process_sheet.write(i, 0, pid)
    process_sheet.write(i, 1, pid_vcount[pid])
    process_sheet.write(i, 2, pid_pcount[pid])
    formula = "B" + str(i + 1) + "/$F$1"
    process_sheet.write(i, 3, xlwt.Formula(formula), percentstyle)
    formula = "C" + str(i + 1) + "/$G$1"
    process_sheet.write(i, 4, xlwt.Formula(formula), percentstyle)
    i += 1
formula = 'sum(B2:B' + str(i) + ')'
process_sheet.write(0, 5, xlwt.Formula(formula))
formula = 'sum(C2:C' + str(i) + ')'
process_sheet.write(0, 6, xlwt.Formula(formula))

file_sheet = xbook.add_sheet('file')
vprint('\n=========================\nfilename\tcount')
file_sheet.write(0, 0, 'filename')
file_sheet.write(0, 1, 'count')
file_sheet.write(0, 2, 'percentage')
i = 1
for fname in file_pcount:
    vprint(fname + '\t', file_pcount[fname])
    file_sheet.write(i, 0, fname)
    file_sheet.write(i, 1, file_pcount[fname])
    formula = "B" + str(i + 1) + "/$D$1"
    file_sheet.write(i, 2, xlwt.Formula(formula), percentstyle)
    i += 1
formula = 'sum(B2:B' + str(i) + ')'
file_sheet.write(0, 3, xlwt.Formula(formula))

type_sheet = xbook.add_sheet('type')
vprint('\n=========================\ntype\tcount')
type_sheet.write(0, 0, 'type')
type_sheet.write(0, 1, 'count')
type_sheet.write(0, 2, 'percentage')
i = 1
for page_type in type_pcount:
    vprint(page_type + '\t', type_pcount[page_type])
    type_sheet.write(i, 0, page_type)
    type_sheet.write(i, 1, type_pcount[page_type])
    formula = "B" + str(i + 1) + "/$D$1"
    type_sheet.write(i, 2, xlwt.Formula(formula), percentstyle)
    i += 1
formula = 'sum(B2:B' + str(i) + ')'
type_sheet.write(0, 3, xlwt.Formula(formula))

pid_type_sheet = xbook.add_sheet('pid type')
vprint('\n=========================')
vprint('writing pid/type count in xls file...')
pid_type_sheet.write(0, 0, 'pid')
pid_type_sheet.write(0, 1, 'pgcache')
pid_type_sheet.write(0, 3, 'anon')
pid_type_sheet.write(0, 5, 'kernel')
pid_type_sheet.write(0, 2, 'pgcache (page)')
pid_type_sheet.write(0, 4, 'anon (page)')
pid_type_sheet.write(0, 6, 'kernel (page)')
i = 1
for pid in dpids_list:
    pid_type_sheet.write(i, 0, pid)

    if pid in type_pid_pvcount['vma']['pgcache']:
        pid_type_sheet.write(i, 1, type_pid_pvcount['vma']['pgcache'][pid])
    else:
        pid_type_sheet.write(i, 1, 0)

    if pid in type_pid_pvcount['vma']['anon']:
        pid_type_sheet.write(i, 3, type_pid_pvcount['vma']['anon'][pid])
    else:
        pid_type_sheet.write(i, 3, 0)

    if pid in type_pid_pvcount['vma']['kernel']:
        pid_type_sheet.write(i, 5, type_pid_pvcount['vma']['kernel'][pid])
    else:
        pid_type_sheet.write(i, 5, 0)

    if pid in type_pid_pvcount['page']['pgcache']:
        pid_type_sheet.write(i, 2, type_pid_pvcount['page']['pgcache'][pid])
    else:
        pid_type_sheet.write(i, 2, 0)

    if pid in type_pid_pvcount['page']['anon']:
        pid_type_sheet.write(i, 4, type_pid_pvcount['page']['anon'][pid])
    else:
        pid_type_sheet.write(i, 4, 0)

    if pid in type_pid_pvcount['page']['kernel']:
        pid_type_sheet.write(i, 6, type_pid_pvcount['page']['kernel'][pid])
    else:
        pid_type_sheet.write(i, 6, 0)

    i += 1

pid_file_sheet = xbook.add_sheet('pid file(page)')
vprint('writing pid/file count in xls file...')
j = 1
for pid in dpids_list:
    pid_file_sheet.write(0, j, pid)
    j += 1
pid_file_sheet.write(0, j, 'sum')
i = 1
for fname in dfiles_list:
    pid_file_sheet.write(i, 0, fname)
    j = 1
    for pid in dpids_list:
        if not pid in pid_file_pcount or not fname in pid_file_pcount[pid]:
            pid_file_sheet.write(i, j, 0)
        else:
            pid_file_sheet.write(i, j, pid_file_pcount[pid][fname])
        j += 1
    formula = 'sum(' + xlrd.cellname(i, 1) + ':' + \
        xlrd.cellname(i, j - 1) + ')'
    pid_file_sheet.write(i, j, xlwt.Formula(formula))
    i += 1
pid_file_sheet.write(i, 0, 'sum')
j = 1
for pid in dpids_list:
    formula = 'sum(' + xlrd.cellname(1, j) + ':' + \
        xlrd.cellname(i - 1, j) + ')'
    pid_file_sheet.write(i, j, xlwt.Formula(formula))
    j += 1

xbook.save(xlsfile)

vprint("Parsing " + options.input_file  + " done.")

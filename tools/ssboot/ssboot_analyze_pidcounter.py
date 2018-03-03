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

#============================ parse functions =================================
from yaml import load, dump
try:
    from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper

def value_inc(dic, key):
    dic.setdefault(key, 0)
    dic[key] += 1

def parse_line(line, pid_pcount, pid_vcount):

    data = yaml.load(line, Loader=Loader)

    if 'vma' in data:
        distinct_pids = set()
        for vma in data['vma']:
            pids = vma['pid']
            distinct_pids.update(set(pids))
            for pid in pids:
                value_inc(pid_vcount, pid)

        for pid in distinct_pids:
            value_inc(pid_pcount, pid)

    else:
        #these pages are not mapped
        pid = '-1(kernel-or-not-mapped)'
        value_inc(pid_pcount, pid)
        value_inc(pid_vcount, pid)

#============================== main ==========================================
#--------------------------- option parse -------------------------------------
from optparse import OptionParser

usage = "usage: %prog -i input"
parser = OptionParser(usage = usage)
parser.add_option("-i", "-f", "--input", "--file", dest = "input_file",
                  help = "Input filename")

(options, args) = parser.parse_args()

if not options.input_file:
    parser.print_help()
    sys.exit()
else:
    stream = file(options.input_file, 'r')

verbose = True
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

#------------------------------ parse -----------------------------------------
for line in stream.readlines():
    parse_line(line, pid_pcount, pid_vcount)

#------------------------------ output ----------------------------------------

vprint('pid\tmapped_page_count\tpage_count')

for pid in pid_pcount:
    vprint(pid + '\t', pid_vcount[pid], '\t', pid_pcount[pid])

vprint("Parsing " + options.input_file  + " done.")

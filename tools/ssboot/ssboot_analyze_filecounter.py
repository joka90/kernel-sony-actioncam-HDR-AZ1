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

def parse_line(line, file_pcount):

    data = yaml.load(line, Loader=Loader)
    page_type = data['type']

    if page_type == 'pgcache':
        fname = data['file']
        value_inc(file_pcount, fname)

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
file_pcount = {'libsonyc.so': 0}

#------------------------------ parse -----------------------------------------
for line in stream.readlines():
    parse_line(line, file_pcount)

#------------------------------ output ----------------------------------------
vprint('\n=========================\nfilename\tcount')
for fname in file_pcount:
    vprint(fname + '\t', file_pcount[fname])

vprint("Parsing " + options.input_file  + " done.")

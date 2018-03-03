#!/usr/bin/env ruby

#
# Copyright 2007-2009 Sony Corporation.
#
# This program is free software; you can redistribute	it and/or modify it
# under  the terms of	the GNU General	 Public License as published by the
# Free Software Foundation; 2 of the License.
#
# THIS	 SOFTWARE  IS PROVIDED	 ``AS  IS'' AND	  ANY  EXPRESS OR IMPLIED
# WARRANTIES,	 INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.	 IN
# NO  EVENT  SHALL   THE AUTHOR  BE	LIABLE FOR ANY	 DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED	 TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
# USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN	CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# You should have received a copy of the  GNU General Public License along
# with this program; if not, write  to the Free Software Foundation, Inc.,
# 675 Mass Ave, Cambridge, MA 02139, USA.

$:.unshift File.join(File.dirname(__FILE__), "lib")
require 'kbootchartlib'

kbc = Kbootchart.new("out/")
opts = kbc.parse_option
if kbc.loglist == true
  kbc.get_log_list.each {|v| puts "#{v}\n"}
  exit
end
if kbc.work_dir == nil
  puts "!! muset set -d DIR\n"
  puts opts.to_s
  exit
end
kbc.set_target_file
kbc.set_output_kind
kbc.output_kind.each {|v| puts "output_kind=#{v}\n";} if $f_debug == 1
kbc.file_ary.each {|v| kbc.process_file(v)}


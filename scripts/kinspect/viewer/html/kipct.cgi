#!/usr/bin/env ruby
#
# Copyright 2007-2009 Sony Corporation.
#
# This program is free software; you can redistribute	it and/or modify it
# under  the terms of	the GNU General	 Public License as published by the
# Free Software Foundation; version 2 of the License.
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

require 'cgi'
require './json/lexer'
require 'find'

ki_dir = "../../lib"
$: << ki_dir
require 'kbootchartlib'
require 'meminfolib'
require 'kipct_idx'

cgi = CGI.new('html3')

stat_j = Array.new

print "Content-Type: text/json\n\n"
req_name = cgi.params['name'].to_s
max_rcd = 90

def kipct_filtered_data(obj, from, num, filename, work_dir, max_rcd)
  obj.work_dir = work_dir
  obj.from_r = from
  if obj.from_r == 0
    obj.from_r = 1
  end
  obj.to_r = obj.from_r + num
  if obj.to_r == obj.from_r
    obj.to_r = obj.from_r + max_rcd
  end
  obj.process_kind_json(filename)
end

def kipct_filtered_data_tail(obj, tail, filename, work_dir, max_rcd)
  obj.work_dir = work_dir
  obj.tail_r = tail
  if obj.tail_r == 0
    obj.tail_r = 1
  end
  obj.process_kind_json(filename)
end

kbc = Kbootchart.new("out/")
if req_name == "startChart"
  kipct_filtered_data(kbc, cgi.params['scc'].to_s.to_i,
                      cgi.params['count'].to_s.to_i,
                      "proc_stat.log", cgi.params['dir'].to_s, max_rcd)
  stat_j = kbc.stat_j.measure_data
elsif req_name == "startChartTimer"
  kipct_filtered_data_tail(kbc, cgi.params['count'].to_s.to_i,
                           "proc_stat.log", cgi.params['dir'].to_s, max_rcd)
  stat_j = kbc.stat_j.measure_data
end

mi = Meminfo.new("out/")
if req_name == "startChartMeminfo"
  kipct_filtered_data(mi, cgi.params['scc'].to_s.to_i,
                      cgi.params['count'].to_s.to_i,
                      "meminfo", cgi.params['dir'].to_s, max_rcd)
  stat_j = mi.minfo_j.measure_data
elsif req_name == "startChartTimerMeminfo"
  kipct_filtered_data_tail(mi, cgi.params['count'].to_s.to_i,
                           "meminfo", cgi.params['dir'].to_s, max_rcd)
  stat_j = mi.minfo_j.measure_data
end

mib = Meminfo.new("out/")
if req_name == "startChartBuddyinfo"
  kipct_filtered_data(mib, cgi.params['scc'].to_s.to_i,
                      cgi.params['count'].to_s.to_i,
                      "buddyinfo", cgi.params['dir'].to_s, max_rcd)
  stat_j = mib.binfo_j.measure_data
elsif req_name == "startChartTimerBuddyinfo"
  kipct_filtered_data_tail(mib, cgi.params['count'].to_s.to_i,
                           "buddyinfo", cgi.params['dir'].to_s, max_rcd)
  stat_j = mib.binfo_j.measure_data
end

if req_name == "getLogDir"
  items = Array.new
  ary = kbc.get_log_list
  i = 0
  ary.sort!
  ary.each do |v|
    items.push(Hash["name", v, "label", v])
  end
  stat_j = Hash["items", items]
end

json_out = true

if req_name == "dummy"
  json_out = false
  list = Array.new
  puts "<html><body>"
  puts "</body></html>"
end

def get_target_info(dir)
  include IdxFileOps
  filename = dir + "target_info"
  File.open(filename, File::RDONLY) do |file|
    ary = get_idx_filepos_by_tail_rcd(file, 1)
    file.pos = ary[0];
    puts "<html><body>"
    file.each {|v| puts "#{v}<BR>\n" }
    puts "</body></html>"
  end
end

def get_log_dir_files(dir)
  list = Array.new
  Find.find(dir) do |f|
    if f != dir
      list.push(f) unless f =~ /.*\.idx/
    end
  end
  puts "<html><body>"
  list.each {|v| puts "#{v}<BR>\n" }
  puts "</body></html>"
end

if req_name == "getTargetInfo"
  dir = cgi.params['dir'].to_s + "/"
  get_target_info(dir)
  json_out = false
end

if req_name == "getLogDirFiles"
  dir = cgi.params['dir'].to_s + "/"
  get_log_dir_files(dir)
  json_out = false
end

if req_name == "checkLogDirFiles"
  validFileList = {
    "cpuChk" => ["proc_stat.log"],
    "ctxtChk" => ["proc_stat.log"],
    "prcsChk" => ["proc_stat.log"],
    "memChk" => ["meminfo"]
  }

  dir = cgi.params['dir'].to_s + "/"
  list = Array.new
  invalidList = Array.new
  invalidItem = {"invalid" => invalidList}
  Find.find(dir) do |f|
    if f != dir
      list.push(File.basename(f)) unless f =~ /.*\.idx/
    end
  end
  validFileList.each_key do |k|
    invalid = false
    validFileList[k].each do |v|
      invalid = true unless list.include?(v)
    end
    if invalid == true
      invalidList.push(k)
    end
  end
  stat_j = invalidItem
end

if json_out != false
  json_str = stat_j.to_json
  print json_str
end

print "\n"

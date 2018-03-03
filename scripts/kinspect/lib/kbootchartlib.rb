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

require 'optparse'
require 'fileutils'

require 'kipct_sup'

class BChart
  def initialize
  end
  def input_original(record, kio)
    record.each {|r| output(kio, r)}
  end
  def calc_time(line, kio)
    if line =~ /^(\d+)/
      now_time = $1.to_i / 10000000
      output(kio, now_time.to_s+"\n")
    else
      puts "error calc_time";
    end
  end
  def output(kio, arg)
      kio.write(arg)
  end
end

class BCheader < BChart
  def initialize
    @line_cnt = 0
    @counter = 0;
  end
  def input_original(record, kio)
    record.each {|r| input(kio, r)}
  end
  def input_rcd_file(record, kio)
    record.pos = 0
    input_original(record, kio)
    kio.write("\n")
  end
  def input(kio, line)
    if @line_cnt != 1
      case line
      when /^header_start\s(\d.*)/ : self.output(kio, "version = #{$1}\n");
      when /^%{1}\n/ : @counter += 1
      else
        case @counter
        when 1 : self.output(kio, "title = Boot chart for #{line}")
        when 2 : self.output(kio, "system.uname = #{line}")
        when 3 : self.output(kio, "system.release = #{line}")
        when 4 :
            if (line =~ /^model\sname/)
              @model_name = line.sub(/model name\s*:\s*/, '')
              self.output(kio, "system.cpu = #{@model_name}")
            elsif (line =~ /^Processor/)
              @model_name = line.sub(/Processor\s*:\s*/, '')
              self.output(kio, "system.cpu = #{@model_name}")
            end
        when 5 : self.output(kio, "system.kernel.options = #{line}")
        end
      end
    end
      @line_cnt += 1
  end
  def output(kio, arg)
    super
  end
end

class BCreadfile < BChart
  def initialize
    super
  end
  def input_rcd_file(record, kio)
    record.pos = 0
    calc_time(record.gets, kio)
    input_original(record, kio)
    kio.write("\n")
  end
end

class BCstatJson
  attr_accessor :measure_data
  def initialize
    @measure_data = Hash.new
    @time_ary = Array.new
    @user_ary = Array.new
    @system_ary = Array.new
    @iowait_ary =  Array.new
    @allintr_ary =  Array.new
    @ctxt_ary =  Array.new
    @prcs_ary =  Array.new
    @coordinates = {
      'user' => @user_ary,
      'system' => @system_ary, 'iowait' => @iowait_ary
    }
    @coordinates_intr_ctxt = {
      'allintr' => @allintr_ary,
      'ctxt' => @ctxt_ary
    }
    @coordinates_prcs = {
      'processes' => @prcs_ary
    }
    @axis = {'y' => {'max' => 100, 'min' => 0}}
    @axis_intr_ctxt = {'y' => {'max' => 100, 'min' => 0}}
    @axis_prcs = {'y' => {'max' => 100, 'min' => 0}}
    @legend = {
      'user' => "#1158FF", 'user_stc' => '1',
      'system' => "#FFFB11", 'system_stc' => '1',
      'iowait' => "#FF1128", 'iowait_stc' => '1'
    }
    @legend_intr_ctxt = {
      'allintr' => "#FF870E", 'allintr_stc' => '0',
      'ctxt' => "#4BFF0E", 'ctxt_stc' => '0'
    }
    @legend_prcs = {
      'processes' => "#8F0EFF",'processes_stc' => '0'
    }
    @table_info = {
      'title' => "CPU usage X: time(sec) Y: usage(%)",
      'legend' => @legend
    }
    @table_info_intr_ctxt = {
      'title' => " Interrupts / ContextSwitch X: time(sec) Y: usage(num)",
      'legend' => @legend_intr_ctxt
    }
    @table_info_prcs = {
      'title' => " Processes X: time(sec) Y: usage(num)",
      'legend' => @legend_prcs
    }
    @old_data = Hash.new
    @old_data_allintr = Hash.new
    @old_data_ctxt = Hash.new
    @old_data_processes = Hash.new
    @now_time = 0
  end
  def calc_time(line)
    if line =~ /^(\d+)/
      @now_time = $1.to_i / 10000000
      @now_time = @now_time.to_f / 100
    else
      puts "error calc_time";
    end
  end
  def analize_stat_cpu(line)
    first_data = false
    if @old_data.empty?
      old_user = 0; old_system = 0; old_idle = 0; old_iowait = 0;
      first_data = true
    else
      old_user = @old_data["user"];
      old_system = @old_data["system"];
      old_idle = @old_data["idle"];
      old_iowait = @old_data["iowait"];
    end
    # cpu user nice system idle iowait irq softirq steal
    re = /^cpu0\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)/
    md = re.match(line)
    # user = user + nice
    user = md[1].to_f + md[2].to_f - old_user
    @old_data["user"] = md[1].to_f + md[2].to_f
    # system = system + irq + softirq
    system = md[3].to_f + md[6].to_f + md[7].to_f - old_system
    @old_data["system"] = md[3].to_f + md[6].to_f + md[7].to_f
    # idle = idle
    idle = md[4].to_f - old_idle
    @old_data["idle"] = md[4].to_f
    # iowait = iowait
    iowait = md[5].to_f - old_iowait
    @old_data["iowait"] = md[5].to_f
    # sum = user + system + idle + iowait
    sum = user + system + idle + iowait
    if sum == 0
      user = 0; system = 0; iowait = 0;
    else
      user /= sum
      system /= sum
      iowait /= sum
      user *= 100
      system *= 100
      iowait *= 100
#      puts "#{@now_time}(#{user} #{system} #{iowait}) sum=#{sum} idle=#{idle}\n";
    end
    if first_data == false
      @time_ary.push(@now_time)
      @iowait_ary.push(Hash["x" => @now_time,
                            "y" => iowait.to_i])
      @system_ary.push(Hash["x" => @now_time,
                            "y" => iowait.to_i + system.to_i])
      @user_ary.push(Hash["x" => @now_time,
                          "y" => iowait.to_i + system.to_i + user.to_i])
    end
  end
  def analize_each_delta(line, re, old_data, ary, key)
    first = false
    if old_data.empty?
      old = 0
      first = true
    else
      old = old_data[key]
    end
    md = re.match(line)
    unless line == nil
      delta = md[1].to_i - old
      old_data[key] = md[1].to_i
    end
    if first == false
      ary.push(Hash["x" => @now_time, "y" => delta])
    end
  end
  def analize_stat_intr_ctxt_prcs(in_ct_pr)
    in_ct_pr.each_key do |key|
      case key
        when "intr" : analize_each_delta(in_ct_pr["intr"],
                                         /^intr\s(\d+)/,
                                         @old_data_allintr,
                                         @allintr_ary,
                                         "allintr")
        when "ctxt" : analize_each_delta(in_ct_pr["ctxt"],
                                         /^ctxt\s(\d+)/,
                                         @old_data_ctxt,
                                         @ctxt_ary,
                                         "ctxt")
        when "prcs" : analize_each_delta(in_ct_pr["prcs"],
                                         /^processes\s(\d+)/,
                                         @old_data_processes,
                                         @prcs_ary,
                                         "prcs")
      end
    end
  end
  def create_measure_data_each(coord_hash, coord_key, axis_hash, axis_key, table_hash, table_key)
    axis_hash["x"] = {"max" => @time_ary.last, "min" => @time_ary.first}
    max = 0
    coord_hash.each_value do |ary|
        ary.each do |h|
          if max < h["y"]
            max = h["y"]
          end
      end
    end
    axis_hash["y"] = {"max" => max, "min" => 0}
    @measure_data[coord_key] = coord_hash
    @measure_data[axis_key] = axis_hash
    @measure_data[table_key] = table_hash
  end
  def create_measure_data
    cpu_arg = [@coordinates, "coordinates", @axis, "axis",
               @table_info, "table"]
    intr_ctxt_arg = [@coordinates_intr_ctxt, "coordinates_intr_ctxt",
                     @axis_intr_ctxt, "axis_intr_ctxt",
                     @table_info_intr_ctxt, "table_intr_ctxt"]
    prcs_arg =[@coordinates_prcs, "coordinates_prcs", @axis_prcs, "axis_prcs",
               @table_info_prcs, "table_prcs"]
    measure = Hash["cpu" => cpu_arg, "intr_ctxt" => intr_ctxt_arg,
                   "prcs" => prcs_arg]
    @time_ary.sort!
    measure.each_value { |v| create_measure_data_each(*v) }
    @axis['y']['max'] = 100
  end
  def parse_stat_line(line, in_ct_pr)
    case line
      when /^cpu0/ : analize_stat_cpu(line)
      when /^intr/ : in_ct_pr["intr"] = line
      when /^ctxt/ : in_ct_pr["ctxt"] = line
      when /^processes/ : in_ct_pr["prcs"] = line
    end
  end
  def input_rcd_file(record)
    record.pos = 0
    calc_time(record.gets)
    intr_ctxt_prcs = Hash.new
    record.each {|r| parse_stat_line(r, intr_ctxt_prcs)}
    analize_stat_intr_ctxt_prcs(intr_ctxt_prcs)
  end
end

class Kbootchart < Kipct_support
  attr_reader :stat_j
  def initialize(outputdir)
    super
    @header = BCheader.new
    @proc_readfile = BCreadfile.new
    @stat_j = BCstatJson.new
  end
  def set_target_file
    if @file_ary == nil
      @file_ary = ["header", "proc_stat.log", "proc_ps.log", "proc_diskstats.log"]
    end
  end
  def set_output_kind
    if @output_kind == nil
      @output_kind = ["file"];
    end
    super
  end
  def process_file(file)
    @output_kind.each {|kind| process_kind(kind, file) }
  end
  def process_kind(kind, file)
    case kind
    when /^file/
      process_kind_file(file)
    when /^json/
      process_kind_json(file)
    else
      puts "unkown kind = #{kind}"
      return -1
    end
  end
  def process_kind_file(file)
    case file
    when /^(proc_stat.log|proc_ps.log|proc_diskstats.log)/
      get_record(file) do |r|
        file_kio = @output_kios["file"]
        @proc_readfile.input_rcd_file(r, file_kio[file])
        output_to_file(file_kio[file], file)
      end
    when /^header/
      get_record(file) do |r|
        file_kio = @output_kios["file"]
        @header.input_rcd_file(r, file_kio[file])
        output_to_file(file_kio[file], file)
      end
    end
  end
  def process_kind_json(file)
    case file
    when /^proc_stat.log/
      get_record(file) do |r|
        @stat_j.input_rcd_file(r)
      end
      @stat_j.create_measure_data
    end
  end
end



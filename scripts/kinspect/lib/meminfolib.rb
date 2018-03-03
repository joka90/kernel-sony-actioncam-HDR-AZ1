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

# MemTotal is y-axis label Max
class MeminfoJson
  attr_accessor :measure_data
  def initialize
    @measure_data = Hash.new
    @time_ary = Array.new
    @notfree_ary = Array.new
    @cached_ary = Array.new
    @cantfree_ary = Array.new
    @active_ary = Array.new
    @inactive_ary = Array.new
    @coordinates = {
      'used' => @notfree_ary,
      'cached' => @cached_ary,
      'cantfree' => @cantfree_ary,
      'active' => @active_ary,
      'inactive' => @inactive_ary
    }
    @axis = {'y' => {'max' => 100, 'min' => 0}}
    @legend = {
      'used' => "#FF870E",
      'cached' => "#160EFF",
      'active' => "#FF430E",
      'inactive' => "#C3FF0E",
      'cantfree' => "#8F0EFF"
    }
    @table_info = {
      'title' => "Memory usage X: time(sec) Y: usage(MB)",
      'legend' => @legend
    }
    @now_time = 0
    @mem_total = 0
  end
  def calc_time(line)
    if line =~ /^(\d+)/
      @now_time = $1.to_i / 10000000
      @now_time = @now_time.to_f / 100
    else
      puts "error calc_time";
    end
  end
  def input_rcd_file(record)
    record.pos = 0
    @mem_free = 0
    @mem_cached = 0
    @mem_active = 0
    @mem_inactive = 0
    calc_time(record.gets)
    record.each {|r| parse_line(r)}
    @time_ary.push(@now_time)
    @notfree_ary.push(Hash["x" => @now_time,
                           "y" => @mem_total - @mem_free])
    @cached_ary.push(Hash["x" => @now_time,
                           "y" => @mem_cached])
    @cantfree_ary.push(Hash["x" => @now_time,
                           "y" => (@mem_total - @mem_free - (@mem_buffers + @mem_cached))])
    @active_ary.push(Hash["x" => @now_time,
                           "y" => @mem_active])
    @inactive_ary.push(Hash["x" => @now_time,
                           "y" => @mem_inactive])
  end
  def parse_line(line)
    case line
    when /^MemTotal:\s*(\d+)\s/ : @mem_total = $1.to_i/1024
    when /^MemFree:\s*(\d+)\s/ : @mem_free = $1.to_i/1024
    when /^Buffers:\s*(\d+)\s/ : @mem_buffers = $1.to_i/1024
    when /^Cached:\s*(\d+)\s/ : @mem_cached = $1.to_i/1024
    when /^Active:\s*(\d+)\s/ : @mem_active = $1.to_i/1024
    when /^Inctive:\s*(\d+)\s/ : @mem_inactive = $1.to_i/1024
    end
  end
  def create_table_data
    @measure_data["table"] = @table_info
  end
  def create_measure_data
    @time_ary.sort!
    @axis["x"] = {"max" => @time_ary.last, "min" => @time_ary.first}
    @axis["y"] = {"max" => @mem_total, "min" => 0}
    @measure_data["coordinates"] = @coordinates
    @measure_data["axis"] = @axis
    create_table_data
  end
end

class BuddyinfoJson
  attr_accessor :measure_data
  def initialize
    @measure_data = Hash.new
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
  def input_rcd_file(record)
    record.pos = 0
    calc_time(record.gets)
    value = Array.new
    record.each do |r|
      value.push(parse_line(r));
    end
    @measure_data["coordinates"] = value

  end
  def parse_line(line)
    case line
    when /^Node/
      re = /^Node (\d)+, zone(\s+)(\S+)(.+)/
      md = re.match(line)
      value = Hash["node" => md[1], "zone" => md[3]]
      data = md[4].split
      tmp = Array.new
      i = 0;
      data.each {|v| tmp.push(Hash["x" => i, "y" => v]); i += 1 }
      value["z"] = tmp
      return value
    end
  end
  def create_label_data(min_time, x_max, x_unit, y_max, y_unit)
    axis = Hash.new
    axis["labelx"] = Array.new()
    axis["labely"] = Array.new()
    0.upto(x_max / x_unit) {|i| axis["labelx"].push((min_time + i * x_unit).to_i) }
    0.upto(y_max / y_unit) {|i| axis["labely"].push((i * y_unit).to_i) }
    axis["labelx"].uniq!
    axis["labely"].uniq!
    return axis
  end
  def create_measure_data
    max_v = 0
    @measure_data["coordinates"].each do |va|
      zary = va["z"]
      zary.each {|v| if max_v < v["y"].to_i  : max_v = v["y"].to_i end }
    end
    @measure_data["coordinates"].each do |va|
      if max_v < 50
        label_y_unit = 5
      elsif max_v > 300
        label_y_unit = 100
      else
        label_y_unit = 50
      end
      max_v += label_y_unit
      axis = Hash.new
      axis["y"] = {"upper" => max_v, "lower" => 0}
      axis["x"] = {"upper" => 10 , "lower" => 0}

      va["axis"] = axis.merge(create_label_data(0, 10, 1, max_v, label_y_unit))
    end
  end
end

class Meminfo < Kipct_support
  attr_reader :minfo_j
  attr_reader :binfo_j
  def initialize(outputdir)
    super
    @minfo_j = MeminfoJson.new
    @binfo_j = BuddyinfoJson.new
  end
  def set_target_file
    if @file_ary == nil
      @file_ary = ["meminfo"]
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
    when /^file/ : ; # do nothing
    when /^json/
      process_kind_json(file)
    else
      puts "unkown kind = #{kind}"
      return -1
    end
  end
  def process_kind_json(file)
    case file
    when /^meminfo/
      get_record(file) do |r|
        @minfo_j.input_rcd_file(r)
      end
      @minfo_j.create_measure_data
    when /^buddyinfo/
      get_record(file) do |r|
        @binfo_j.input_rcd_file(r)
      end
      @binfo_j.create_measure_data
    end
  end

end

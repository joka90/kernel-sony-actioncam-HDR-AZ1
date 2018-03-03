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

require 'fileutils'
require 'stringio'

# management log directory
class NManageLogDir
  @@dirs = nil
  def initialize(port_no)
    @index = 0
    @port_no = port_no
  end
  def get_dirname
    if @@dirs == nil
      @@dirs = ["/tmp/kinspect/#{@port_no}/log.#{Time.now.strftime("%Y%m%d.%H:%M:%S")}/"]
      FileUtils.mkdir_p(@@dirs[0])
    end
    return @@dirs[@index]
  end
  def change
    @index += 1
    if @index < @@dirs.length
      @index = @@dirs.length - 1
      return @@dirs.last
    else
      new_dir_name = "/tmp/kinspect/#{@port_no}/log.#{Time.now.strftime("%Y%m%d.%H:%M:%S")}"
      @@dirs.each do |d|
	if d.include?(new_dir_name)
	  if new_dir_name =~ /.*\.(\d+)$/
	    new_index = "." + ($1.to_i+1).to_s
	    new_dir_name.sub!(/\.(\d+)$/, new_index)
	    retry
	  else
	    new_dir_name += ".1"
	    retry
	  end
	end
      end
      new_dir_name += "/"
      @@dirs.push(new_dir_name)
      puts "new_dir = #{new_dir_name}\n" if $f_debug == 1
      FileUtils.mkdir_p(new_dir_name)
    end
    return get_dirname
  end
  # for test
  def clear
    @@dirs = nil
  end
end

# file unit
class NFileUnit
  attr_accessor :file_name
  attr_accessor :last_time
  def initialize(port_no, name)
    @mld = NManageLogDir.new(port_no)
    @file_name = name
    @last_time = nil
  end
  def log_dir_change
    return @mld.change
  end
  def get_log_dir
    return @mld.get_dirname
  end
  # for test
  def mld_clear
    @mld.clear
  end
end

class NClctr
  @file_info_key
  def initialize(clctr_name, port_no)
    @pname = clctr_name
    @sios = Hash.new
    @file_info = Hash.new
    @port_no = port_no
  end
  def add_info(combination)
    puts "combi=#{combination}" if $f_debug == 1
    re = /^(\d+):((\w|\.|-)+)/
    md = re.match(combination)
    unless @file_info.has_key?(md[1])
      @file_info[md[1]] = NFileUnit.new(@port_no, md[2])
    end
    self.create_sio(md[2])
  end
  def create_sio(file_name)
    @sios[file_name] = StringIO.open("") unless @sios.has_key?(file_name)
  end

  def set_info(line)
    line.sub!(/^%I:\d+:\w+:/, '')
    line.gsub(/(\d+):((\w|\.|-)+)/) { |match| self.add_info(match) }
    @file_info.each{|key, value| puts "set #{key} #{value.file_name}"} if $f_debug == 1
  end

  def cancel(file_info_key)
    file_name = @file_info[file_info_key].file_name
    puts "invalid identifier, cancel item of #{file_name}"
    if @sios.has_key?(file_name)
      @sios[file_name].close
      @sios[file_name] = StringIO.open("")
    end
  end

  def output(arg)
    fi = @file_info[@file_info_key]
    @sios[fi.file_name].write(arg)
  end

  def output_to_file(key)
    file_name = @file_info[key].file_name
    log_dir = @file_info[key].get_log_dir
    begin
      puts "file_name=#{file_name}" if $f_debug == 1
      @outfile_name = log_dir + file_name
      # get time
      @sios[file_name].pos = 0
      time = @sios[file_name].gets
      time = @sios[file_name].gets.to_i
      @sios[file_name].pos = 0
      # data file write
      index_pos = 0
      writed_bytes = 0
      File.open(@outfile_name, File::CREAT|File::APPEND|File::RDWR) do |file|
	writed_bytes = file.write(@sios[file_name].string)
	index_pos = file.pos - writed_bytes
      end
      # index file write
      File.open(@outfile_name + ".idx",
		File::CREAT|File::APPEND|File::RDWR) do |file|
	file.write([index_pos, time].pack("lq"))
      end
      @sios[file_name].close
      @sios[file_name] = StringIO.open("")

    rescue Errno::ENOENT
      $stderr.print "rescued IO failes: " + $! if $f_debug == 1
      self.create_sio(file_name)
      retry
    end
  end

  def input_start(key, line)
    @file_info_key = key
    self.input(line)
  end
  def input_end(line)
    self.input(line)
    self.output_to_file(@file_info_key)
  end
  def input(line)
    self.output(line)
  end
  def input_time_h(line)
    last_time = @file_info[@file_info_key].last_time
    if line =~ /^(\d+),(\d+),(\d)/
      unless  last_time == nil
	if last_time.to_i > $1.to_i
	  puts "time reverse=#{$1} #{Time.now.strftime("%Y%m%d.%H:%M:%S")}" if $f_debug == 1
	  @file_info[@file_info_key].log_dir_change
	end
      end
      @file_info[@file_info_key].last_time = $1
      self.output(line)
    end
  end
  # for test
  def mld_clear
    unless @file_info.empty?
      ary = @file_info.shift
      unit = ary[1]
      unit.mld_clear
    end
  end
end

# sorting by file
class SortingByFile
  attr_accessor :port_no
  def initialize
    @clctrs = Hash.new
    @p_state = {"s" => nil, "e" => nil}
    @next_time_h = false
  end

  def analyze_info(line)
    puts "ana_line=#{line}"  if $f_debug == 1
    re = /^%I:(\d+):(\w+):/
    md = re.match(line);
    @clctrs[md[1]] = NClctr.new(md[2], @port_no) unless @clctrs.has_key?(md[1])
    @clctrs[md[1]].set_info(line)
  end
  def clctr_input_start(line)
    if (line =~ /^%S(\d+):(\d+)/)
      if @clctrs.has_key?($1)
	@now_p = @clctrs[$1]
	if @p_state["s"] == nil and @p_state["e"] == nil
	  @p_state["s"] = "#{$1}:#{$2}"
	  valid = 1
	elsif @p_state["s"] == nil
	  @p_state["s"] = "#{$1}:#{$2}"
	  @p_state["e"] = nil
	  valid = 1
	else
	  valid = 0
	end
      else
	valid = 0
	return
      end
      if valid == 1
	@now_p.input_start($2, line);
	@next_time_h = true
      else
	@now_p.cancel($2);
	@p_state["s"] = nil
	@p_state["e"] = nil
	@now_p = nil
	@next_time_h = false
      end
    end
  end
  def clctr_input_end(line)
    if (line =~ /^%E(\d+):(\d+)/)
      if @clctrs.has_key?($1)
	if "#{$1}:#{$2}" == @p_state["s"]
	  @p_state["s"] = nil
	  @p_state["e"] = @clctrs[$1]
	  valid = 1
	else
	  valid = 0
	end
      else
	valid = 0
	return
      end
      if valid == 1
	@now_p.input_end(line)
	@now_p = nil
      else
	@now_p.cancel($2) unless @now_p == nil
	@p_state["s"] = nil
	@p_state["e"] = nil
	@now_p = nil
      end
    end
  end
  def clctr_input(line)
    unless @p_state["s"] == nil
      if @next_time_h == true
	@now_p.input_time_h(line)
	@next_time_h = false
      else
	@now_p.input(line)
      end
    end
  end
  def input(line)
    puts "line=#{line}" if $f_debug == 1
    case line
    when /^%I:/ : analyze_info(line)
    when /^%S\d+/ : clctr_input_start(line)
    when /^%E\d+/ : clctr_input_end(line)
    else
      clctr_input(line)
    end
  end
  # for test
  def mld_clear
    unless @clctrs.empty?
      ary = @clctrs.shift
      clctr = ary[1]
      clctr.mld_clear
    end
  end
end

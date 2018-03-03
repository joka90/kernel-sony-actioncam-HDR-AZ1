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
require 'kipct_idx'
require 'find'
require 'timeout'

$g_logdir = "/tmp/kinspect"

class Kio < StringIO
  def initialize(string, mode)
    super
  end
  def initialize(string)
    super
  end
  def clear
    self.flush
    self.truncate(0)
    self.pos = 0
  end
end

class Scop
  attr_accessor :from_r
  attr_accessor :to_r
  attr_accessor :head_r
  attr_accessor :tail_r
  attr_accessor :output_kind
  attr_accessor :work_dir
  attr_accessor :loglist
  attr_accessor :file_ary
  def initialize
    @from_r = -1
    @to_r = -1
    @head_r = -1
    @tail_r = -1
    @from_t = -1
    @to_t = -1
    @head_t = -1
    @tail_t = -1
    @output_kind = nil
    @work_dir = nil
    @loglist = false
    @file_ary = nil
  end
  def parse_option
    opts = OptionParser.new
    opts.on("-s", "--start_rec RECORD_NUM", Integer) {|val| @from_r = val}
    opts.on("-e", "--end_rec RECORD_NUM", Integer) {|val| @to_r = val}
    opts.on("-h", "--head_rec RECORD_NUM", Integer) {|val| @head_r = val}
    opts.on("-t", "--tail_rec RECORD_NUM", Integer) {|val| @tail_r = val}
    opts.on("--start_time TIME(s)", Integer) {|val| @from_t = val}
    opts.on("--end_time TIME(s)", Integer) {|val| @to_t = val}
    opts.on("--head_time TIME(s)", Integer) {|val| @head_t = val}
    opts.on("--tail_time TIME(s)", Integer) {|val| @tail_t = val}
    opts.on("-k", "--output_kind file,json", Array) {|val| @output_kind = val}
    opts.on("-d", "--dir DIR", String) {|val| @work_dir = val}
    opts.on("-l", "--list") {|val| @loglist = true}
    opts.on("-f", "--files FILE,FILE", Array) {|val| @file_ary = val}
    rest = opts.parse!(ARGV)
    return opts
  end
end

class Kipct_support < Scop
  include IdxFileOps
  attr_accessor :output_dir
  @kios
  def initialize(outputdir)
    super()
    @output_dir = outputdir
    @kios = Hash.new
    @output_kios = Hash.new
    @inputline = Hash.new
  end
  def parse_option
    super
  end
  def set_output_kind
    @file_ary.each do |file|
      if @output_kind.include?("file")
        @output_kios["file"] = Hash.new if @output_kios["file"] == nil
        s = @output_kios["file"]
        s[file]= Kio.open("")
      end
    end
  end

  def get_log_list
    list = Array.new
    Find.find($g_logdir) do |f|
      unless File.directory?(f)
        unless list.include?(File.dirname(f))
          list.push(File.dirname(f))
        end
      end
    end
    return list
  end

  def parse_input(kio, line, rcd_start)
    puts "parse_input in #{line}" if $f_debug == 1
    if (line =~ /^%S/)
      rcd_start = 1
    elsif (line =~ /^%E/)
      if rcd_start == 1
        rcd_start = 0
        return 1, rcd_start
      else
        kio.clear
        return 2, rcd_start
      end
    else
      if rcd_start == 1
        kio.write(line)
      end
    end
    return 0, rcd_start
  end

  def parse_file(kio, file)
    ret = 0
    rcd_start = 0
    file.each do |line|
      ret_ary = self.parse_input(kio, line, rcd_start)
      rcd_start = ret_ary[1]
      return ret_ary[0] unless ret_ary[0] == 0
    end
  end

  def setup_output_file(file_name, kio)
    FileUtils.mkdir_p(@output_dir)
    kio = Kio.open("")
  end

  def output_to_file(kio, file_name)
    begin
      outfile_name = @output_dir + file_name
      File.open(outfile_name, File::CREAT|File::APPEND|File::RDWR) do |file|
        file.write(kio.string)
      end
      kio.clear
      rescue Errno::ENOENT
      $stderr.print "rescued IO failes: " + $! +" created\n" if $f_debug == 1
      setup_output_file(file_name, kio)
      retry
    end
  end

  def set_range(file)
    ary = Array.new
    last_pos = file.stat.size
    if @head_r != -1  || @tail_r != -1  || @from_r != -1  || @to_r != -1
      if @head_r != -1
        ary = get_idx_filepos_by_rcd(file, @head_r)
      elsif @tail_r != -1
        ary = get_idx_filepos_by_tail_rcd(file, @tail_r)
      elsif @from_r != -1
        ary = get_idx_filepos_by_rcd(file, @from_r)
      else
        ary[0] = 0
      end
      if @to_r != -1
        to_ary = get_idx_filepos_by_rcd(file, @to_r)
        last_pos = to_ary[0]
      end
    elsif @head_t != -1  || @tail_t != -1  || @from_t != -1  || @to_t != -1
      if @head_t != -1
        ary = get_idx_filepos_by_time(file, @head_t)
      elsif @tail_t != -1
        ary = get_idx_filepos_by_tail_time(file, @tail_t)
      elsif @from_t != -1
        ary = get_idx_filepos_by_time(file, @from_t)
      else
        ary[0] = 0
      end
      if @to_t != -1
        to_ary = get_idx_filepos_by_time(file, @to_t)
        last_pos = to_ary[0]
      end
    else
      # force file.pos = 0
      ary[0] = 0
    end
    return ary[0], last_pos
  end

  def get_record(file_name)
    tm_sec = 60
    @kios[file_name] = Kio.open("")
    rcd_kio = @kios[file_name]
    inputfile_name = File.join(@work_dir,file_name)
    pos_ary = Array.new
    begin
      Timeout::timeout(tm_sec) do |timeout_length|
        begin
          rty = 1
          File.open(inputfile_name, File::RDONLY) do |file|
            pos_ary = set_range(file)
          end
          File.open(inputfile_name, File::RDONLY) do |file|
            file.pos = pos_ary[0]
            last_pos = pos_ary[1]
            puts "get_reocrd #{inputfile_name} filepos=#{file.pos} last=#{last_pos}\n" if $f_debug == 1
            while rty == 1
              parse_file(rcd_kio, file)
              if file.pos > last_pos && (@to_r != -1 || @to_t != -1)
                rty = 0
              elsif file.pos == last_pos
                rty = 0
              else
                rty = 1
              end
              yield(rcd_kio)
              rcd_kio.clear
            end
          end
        rescue Errno::ENOENT
          sleep(1)
          retry
        end
      end
    rescue Timeout::Error
      puts "timeout #{tm_sec} second, file(#{file_name}) is not found.\n"
      raise
    end
  end
end

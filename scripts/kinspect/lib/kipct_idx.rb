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

module IdxFileOps
  # RECORD = [file_position(32bit), time(64bit)]
  RECORD_SIZE= (32 + 64) / 8
  def get_idx_filepos(file)
    begin
      idxfile = file.path + ".idx"
      File.open(idxfile, File::RDONLY) do |f|
        ary = yield(f)
      end
      return ary
      rescue Errno::ENOENT
      retry
    end
  end
  def get_idx_filepos_by_rcd(file, rcd_num)
    get_idx_filepos(file) do |f|
      seek_pos = RECORD_SIZE * rcd_num
      if seek_pos > f.stat.size
        seek_pos = f.stat.size - RECORD_SIZE * 1
      end
      f.pos = seek_pos
      return f.read(RECORD_SIZE).unpack("lq")
    end
  end
  def get_idx_filepos_by_tail_rcd(file, rcd_num)
    get_idx_filepos(file) do |f|
      seek_pos = f.stat.size - RECORD_SIZE * rcd_num
      if seek_pos < 0
        seek_pos = 0
      end
      f.pos = seek_pos
      return f.read(RECORD_SIZE).unpack("lq")
    end
  end
  def get_idx_filepos_by_time(file, time)
    get_idx_filepos(file) do |f|
      while ((record = f.read(RECORD_SIZE)) != nil)
        ary = record.unpack("lq")
        if ary[1] >= (time * 1000 * 1000 * 1000)
          break
        end
      end
      return ary
    end
  end
  def get_idx_filepos_by_tail_time(file, time)
    get_idx_filepos(file) do |f|
      f.pos = f.stat.size - RECORD_SIZE * 1
      ary = f.read(RECORD_SIZE).unpack("lq")
      time = ary[1] - (time * 1000 * 1000 * 1000)
      f.pos = 0
      while ((record = f.read(RECORD_SIZE)) != nil)
        ary = record.unpack("lq")
        if ary[1] >= time
          break
        end
      end
      return ary
    end
  end
end


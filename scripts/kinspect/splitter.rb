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

$f_debug = 0

require 'lib/splitterlib'
require 'optparse'

# FilingNetcat is singleton
class FilingNetcat < SortingByFile
  attr_accessor :tarfile
  private_class_method :new
  @@filing = nil
  def FilingNetcat.create
    @@filing = new unless @@filing
    @@filing
  end
  def start(local)
    if (local)
      infile = open("| ./dummy_kinspect.rb")
    else
      infile = open("| nc -u -l #{@port_no}")
    end
    infile.each { |line| self.input(line)}
  end
end

local = false

fn = FilingNetcat.create
fn.port_no = 0

opts = OptionParser.new
opts.on("-p", "--port PORT", Integer) {|val| fn.port_no = val}
opts.on("-l", "--local-proc") {local = true}
opts.parse!(ARGV)

if (fn.port_no == 0 && !local)
  puts opts.to_s
  exit
end

puts "log dir will be created /tmp/kinspect/#{fn.port_no}/log.#{Time.now.strftime("%Y%m%d.%H:%M:%S")}/\n"

fn.start(local)

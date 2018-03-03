#!/usr/bin/env ruby

#
# Copyright 2007-2009 Sony Corporation.
#

#
# Prints out local /proc files to stdout.
# This script is to be used instead of netcat to debug kinspect.
#
# It polls and prints:
#   %S/E1:1  /proc/stat
#   %S/E1:2  /proc/<PID>/stat
#   %S/E1:3  /proc/diskstats
#   %S/E3:0  /proc/meminfo
#   %S/E3:1  /proc/buddyinfo
#

class KinspectDummyModule

    def initialize(plugin_id, plugin_name)
        @id = plugin_id
        @name = plugin_name
        @callback = Array.new
        self
    end

    # id; plugin callback id.
    # logfile: filename to be stored in host1
    # first: call callback first only.
    # func: a block that takes no argument and returns result in
    # string.
    def add_callback(id, logfile, first, func)
        @callback.push({"id"=>id,
                         "logfile"=>logfile,
                         "first"=>first,
                         "done"=>false,
                         "func"=>func})
        self
    end

    def header
        header="%I:#{@id}:#{@name}"
        @callback.each do |cb|
            header += ":#{cb["id"]}:#{cb["logfile"]}"
        end
        header += "\n"
    end

    # Returns current time in nsec in string.
    def time_now
        t = Time.now
        t.tv_sec.to_s + t.tv_usec.to_s + "000"
    end

    def call_callbacks
      data = ""
      @callback.each do |cb|
        if cb["done"] == false
          data += "%S#{@id}:#{cb["id"]}" + "\n"
          data += time_now + "\n"
          data += cb["func"].call
          data += "%E#{@id}:#{cb["id"]}" + "\n"
          if cb["first"] == true
            cb["done"] = true
          end
        end
      end
      data
    end
end

# Retunes contents of a file in a string.
def cat(filename)
    begin
        File::open(filename).readlines.to_s
    rescue Errno::ENOENT
        return ""
    end
end

# Dummies are array of dummy kinspect modules.
dummies = Array.new

# Kipct info
kipct_info = KinspectDummyModule.new(0, "kipct")
kipct_info.add_callback(0, "target_info", false,
                        proc {
                          data = ""
                          data += cat("/proc/sys/kernel/hostname")
                          data += "%\n"
                          data += cat("/proc/cpuinfo")
                          data
                        })
dummies.push kipct_info

# Bootchart
bootchart = KinspectDummyModule.new(1, "bootchart")
bootchart.add_callback(0, "header", true, proc {cat("/proc/version")})
bootchart.add_callback(1, "proc_stat.log", false, proc {cat("/proc/stat")})
bootchart.add_callback(2, "proc_ps.log", false,
                 proc {
                     data = ""
                     Dir::open("/proc").each do |filename|
                         data += cat("/proc/#{filename}/stat") if /[0-9]+/ =~ filename
                     end
                     data
                 })
bootchart.add_callback(3, "proc_diskstats.log", false, proc {cat("/proc/diskstats")})
dummies.push bootchart

# Meminfo
meminfo = KinspectDummyModule.new(3, "meminfo")
meminfo.add_callback(0, "meminfo", false, proc {cat("/proc/meminfo")})
meminfo.add_callback(1, "buddyinfo", false, proc {cat("/proc/buddyinfo")})
dummies.push meminfo


#
# Now start
#
dummies.each do |dum|
    puts dum.header
end

while (1)
    sleep 1
    dummies.each do |dum|
        puts dum.call_callbacks
    end
end

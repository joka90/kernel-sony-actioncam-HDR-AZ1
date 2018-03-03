#!/usr/bin/perl

# A basic perl port of the bootchart renderer, SVG only
# (c) 2005 Vincent Caron <v.caron@zerodeux.net>

# Copyright 2007-2009 Sony Corporation.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# BUGS
# - ignore number of CPUs (disk util.)

# 2005-06-29 - fixes
# - process name is latest sample argv value
# - sibling processes are properly PID-sorted from top to bottom
# - process start is now parsed from proc_ps
# - hiding bootchartd-related processes
#
# 2005-06-28
# - initial release

local $opt_source_file = '';
local $opt_result_file = '';
local $opt_cut = 0;
local $opt_pid = 0;
local $cut_leave_jiffies = 50;

my %headers;
my @disk_samples;
my @cpu_samples;

local @intr_samples;
local $intr_pins = 0;
local $intr_max = 0;

local @ctxt_samples;
local $ctxt_max = 0;

local @proc_samples;
local $proc_max = 0;

local @intrctxt_samples;
local $intrctxt_max = 0;

my %ps_info;
my %ps_by_parent;

my $chart_svg_template   = `cat svg/bootchart.svg.template` || die;
# chart_svg_template contains:
#  svg_width
#  svg_css_file
#  title_name
#  system_uname
#  system_release
#  system_cpu
#  system_kernel_options
#  system_boottime_sec
#  cpuload_ticks
#  cpuload_io
#  cpuload_usersys
#  disk_ticks
#  disk_utilization
#  disk_throughput
#  disk_file_opened
#  process_axis
#  process_ticks
#  process_tree
#  interrupt_points
#  interrupt_max
#  contextsw_points
#  contextsw_max
#  processcreate_points
#  processcreate_max
#  intrctxt_points
#  intrctxt_max


my $process_svg_template = `cat svg/process.svg.template` || die;
# chart_svg_template contains:
#  process_translate
#  process_status
#  process_border
#  process_parent_line
#  process_label_pos
#  process_label_name
#  process_title_name

my $svg_css_file         = `cat svg/style.css` || die;
$svg_css_file =~ s/^/\t\t\t/mg;
$svg_css_file =~ s/^\s+//g;

my $HZ        = 100;
my $min_img_w = 800;
my $sec_w     = 25;
my $proc_h    = 16;
my $header_h  = 490;
my $img_w;
my $img_h;
my $t_begin;
my $t_end;
my $t_dur;

my $kernel_version;

sub parse_logs {
  my $logfile = shift;
  my $tmp = "/tmp/bootchart-$$";

  mkdir $tmp or die;
  system "tar -C $tmp -xzf $logfile";

  parse_log_header($tmp);
  parse_log_cpu($tmp);
  parse_log_disk($tmp);
  parse_log_ps($tmp);

  system('rm', '-rf', $tmp);
#  print STDERR "Disk samples: ".scalar(@disk_samples)."\n";
#  print STDERR "CPU samples : ".scalar(@cpu_samples)."\n";
#  print STDERR "Processes   : ".scalar(keys %ps_info)."\n";
}

sub parse_log_header {
  my $tmp = shift;

  open(HEADER, "<$tmp/header");
  while (<HEADER>) {
    chomp;
    my ($key, $val) = split /\s*=\s*/, $_, 2;
    $headers{$key} = $val;
  }
  close HEADER;

  if ($headers{"system.uname"} =~ /2\.6/) {
      $kernel_version = 2.6;
  } else {
      $kernel_version = 2.4;
  }
#  print $kernel_version, "\n";
}

sub parse_log_cpu {
  my $tmp = shift;

  # The jiffy
  my ($time);

  # For line matched "cpu"
  #   Reads in: time, user, system, iowait
  my (@timings);
  my ($ltime, @ltimings);
  sub __stat_cpu {
      if (defined @ltimings) {
	  my %sample;
	  if ($kernel_version == 2.6) {
	      # From Documentation/filesystems/proc.txt,
	      # 2.6 has 7 entries in cpu line.
	      #   > cat /proc/stat
	      #   cpu  2255 34 2290 22625563 6290 127 456
	      # They are:
	      #   - user: normal processes executing in user mode
	      #   - nice: niced processes executing in user mode
	      #   - system: processes executing in kernel mode
	      #   - idle: twiddling thumbs
	      #   - iowait: waiting for I/O to complete
	      #   - irq: servicing interrupts
	      #   - softirq: servicing softirqs

	      my $dtime  = ($time - $ltime) / $HZ; # in seconds
	      my $user   = ($timings[0] + $timings[1]) - ($ltimings[0] + $ltimings[1]);
	      my $system = ($timings[2] + $timings[5] + $timings[6]) - ($ltimings[2] + $ltimings[5] + $ltimings[6]);
	      my $idle   = $timings[3] - $ltimings[3];
	      my $iowait = $timings[4] - $ltimings[4];
	      my $sum    = $user + $system + $idle + $iowait;
	      return if ($sum == 0);
	      $sample{time}   = $time;
	      $sample{user}   = $user / $sum;
	      $sample{system} = $system / $sum;
	      $sample{iowait} = $iowait / $sum;
	  } else {
	      # From fs/proc/proc_misc.c: kstat_read_proc(),
	      # 2.4 has only 4 entries in cpu line.
	      #   > cat /proc/stat
	      #   cpu  1000994 145202 122626 41801441
	      # They are:
	      #   - user
	      #   - nice
	      #   - system
	      #   - idle = jif * smp_num_cpus - (user + nice + system)

	      my $dtime  = ($time - $ltime) / $HZ; # in seconds
	      my $user   = ($timings[0] + $timings[1]) - ($ltimings[0] + $ltimings[1]);
	      my $system = $timings[2] - $ltimings[2];
	      my $idle   = $timings[3] - $ltimings[3];
	      my $iowait = 0;
	      my $sum    = $user + $system + $idle + $iowait;
	      return if ($sum == 0);
	      $sample{time}   = $time;
	      $sample{user}   = $user / $sum;
	      $sample{system} = $system / $sum;
	      $sample{iowait} = $iowait / $sum;
	  }
	  push @cpu_samples, \%sample;
      }

      $ltime = $time;
      @ltimings = @timings;
  }

  local (@intrs, @lintrs);
  local (@ctxts, @lctxts);
  local (@procs, @lprocs);
  local (@intrctxts, @lintrctxts);
  sub __stat_common {
      local($t, *new, *last, *smpls, *smpl_max) = @_;

      if (defined @last) {
	  my %smpl;
	  $smpl{time}  = $t;
	  @smpl{total} = @new[0] - @last[0];
	  $smpl_max = @smpl{total} if (@smpl{total} > $smpl_max);
	  push @smpls, \%smpl;
      }
      @last = @new;
  }

  open(PROC_STAT, "<$tmp/proc_stat.log");
  while (<PROC_STAT>) {
    chomp;
    $time = $1, next if /^(\d+)$/;

    if (/^$/) {
	__stat_cpu();

	# For line matched "intr" :
	# The "intr" line gives counts of interrupts serviced since boot
	# time, for each of the possible system interrupts.
	#   -. The first column is the total of all interrupts serviced;
	#   -. Each subsequent column is the total for that particular
	#     interrupt.
	__stat_common($time, *intrs, *lintrs, *intr_samples, *intr_max);

	# For lines matched "ctxt" :
	# This line gives the total number of context switches
	# across all CPUs.
	__stat_common($time, *ctxts, *lctxts, *ctxt_samples, *ctxt_max);

	# For lines matched "processes" :
	# This line gives number of processes created.
	__stat_common($time, *procs, *lprocs, *proc_samples, *proc_max);

 	# For lines matched "intrctxt" :
	# This line gives the time spent in interrupt context in microseconds.
	__stat_common($time, *intrctxts, *lintrctxts, *intrctxt_samples, *intrctxt_max);

	next;
    }
    @timings   = split /\s+/ if s/^cpu\s+//;
    @intrs     = split /\s+/ if s/^intr\s+//;
    @ctxts     = split /\s+/ if s/^ctxt\s+//;
    @procs     = split /\s+/ if s/^processes\s+//;
    @intrctxts = split /\s+/ if s/^intrctxt\s+//;
  }
  close(PROC_STAT);
}


sub parse_log_disk {
  my $tmp = shift;

  # Reads in: time, read, write, use
  my ($time, $ltime);
  my ($read, $write, $util) = (0, 0, 0);
  my ($lread, $lwrite, $lutil);

  # Some 2.4 do not have /proc/diskstat.
  return unless -e "$tmp/proc_diskstats.log";

  open(DISK_STAT, "<$tmp/proc_diskstats.log");
  while (<DISK_STAT>) {
    chomp;
    $time = $1, next if /^(\d+)$/;

    if (/^$/) {
      if (defined $ltime) {
        my $dtime = ($time - $ltime) / $HZ; # in seconds
        my $dutil = ($util - $lutil) / (1000 * $dtime);

        my %sample;
        $sample{time}  = $time;
        $sample{read}  = ($read - $lread) / 2 / $dtime; # in KB/s
        $sample{write} = ($write - $lwrite) / 2 / $dtime; # in KB/s
        $sample{util}  = ($dutil > 1 ? 1 : $dutil);
        push @disk_samples, \%sample;
      }

      $ltime  = $time;
      $lread  = $read, $read = 0;
      $lwrite = $write, $write = 0;
      $lutil  = $util, $util = 0;
      next;
    }

    s/\s+//;
    my @tokens = split /\s+/;
    next if scalar(@tokens) != 14 || not $tokens[2] =~ /hd|sd/;

    $read  += $tokens[5];
    $write += $tokens[9];
    $util  += $tokens[12];
  }
  close(DISK_STAT);
}

sub parse_log_ps {
  my $tmp = shift;

  my $time;

  open(PS_STAT, "<$tmp/proc_ps.log");
  while (<PS_STAT>) {
    chomp;
    next if /^$/;
    $time = $1, next if /^(\d+)$/;

    my @tokens = split /\s+/;
    my $pid    = $tokens[0];

    if (!defined $ps_info{$pid}) {
      my %info;
      my @empty;
      my $ppid  = $tokens[3];
      my $start = $tokens[21];
      $t_begin = $start if !defined $t_begin || $t_begin > $start;

      $info{ppid}    = $ppid;
      $info{start}   = $start;
      $info{samples} = \@empty;
      $ps_info{$pid} = \%info;

      if (!defined $ps_by_parent{$ppid}) {
        my @pidlist = ($pid);
        $ps_by_parent{$ppid} = \@pidlist;
      } else {
        push @{$ps_by_parent{$ppid}}, $pid;
      }
    }

    # argv may change, we'll store here the last sampled value
    my $comm = $tokens[1];
    $comm =~ s/[()]//g;
    $ps_info{$pid}->{comm} = $comm;

    my %sample;
    $sample{time}  = $time;
    $sample{state} = $tokens[2];
    $sample{sys}   = $tokens[13];
    $sample{user}  = $tokens[14];
    push @{$ps_info{$pid}->{samples}}, \%sample;
  }
  close PS_STAT;
}

sub unxml {
  my $x = shift;
  $x =~ s/</&lt;/g;
  $x =~ s/>/&gt;/g;

  return $x;
}

sub render_svg {
  my $output_file = shift;
  my $t_init = $cpu_samples[0]->{time};
  $t_end    = $cpu_samples[-1]->{time};
  $t_begin  = $t_init - $cut_leave_jiffies if ($opt_cut);
  $t_dur    = $t_end - $t_begin;
  $img_w    = $t_dur / $HZ * $sec_w;

  my %subst;
  $subst{svg_css_file}          = $svg_css_file;

  #
  # Headers
  #

  my $cpu = $headers{'system.cpu'};
  $cpu =~ s/model name\s*:\s*//;
  $subst{title_name}             = unxml($headers{title});
  $subst{system_uname}          = "System uname: ".unxml($headers{'system.uname'});
  $subst{system_release}        = "System release: ".unxml($headers{'system.release'});
  $subst{system_cpu}            = "CPU: ".unxml($cpu);
  $subst{system_kernel_options} = "Kernel options: ".unxml($headers{'system.kernel.options'});
  $subst{sample_period}         = "Sample period: ".unxml($headers{'sample.period'})." seconds";
  $subst{system_boottime_sec}   = "Measured time: ".int($t_end / $HZ)." seconds" if ($opt_cut == 0);

  #
  # CPU I/O chart
  #

  my $bar_h = 50;
  my $cpu_ticks = '';
  for (my $i = 0; $i < $t_dur / $HZ; $i++) {
    my $x = $i * $sec_w;
    $cpu_ticks .= "<line ".($i % 5 ? "" : "class=\"Bold\" ")."x1=\"$x\" y1=\"0\" x2=\"$x\" y2=\"$bar_h\"/>\n";
  }
  # 25%, 50%, 75% horizontal bars
  my $chart_width = $t_dur / $HZ * $sec_w;
  for (my $i = 1; $i < 4;  $i++) {
      my $line_y = $bar_h * $i / 4;
      $cpu_ticks .= "<line "."x1=\"0\" y1=\"$line_y\" x2=\"$chart_width\" y2=\"$line_y\"/>\n";
  }
  $cpu_ticks =~ s/^/\t\t\t/mg; $cpu_ticks =~ s/^\s+//g;

   my $io_points  = '';  my $io_points_low  = '';
   my $usr_points = '';  my $usr_points_low = '';
   my $sys_points = '';  my $sys_points_low = '';
   my $cpu_points = '';  my $cpu_points_low = '';

  if (@cpu_samples) {
    my $last_x = 0;

    for (@cpu_samples) {
      my $time    = $_->{time};
      my $iowait  = $_->{iowait};
      my $user    = $_->{user};
      my $sys     = $_->{system};

      my $pos_x = int(($time - $t_begin) * $img_w / $t_dur);

      my $io_height   = int($iowait * $bar_h);
      my $usr_height  = int($user * $bar_h);
      my $sys_height  = int($sys * $bar_h);

      my $io_hi  = int($bar_h - ($usr_height + $sys_height + $io_height));
      my $io_lo  = int($bar_h - ($usr_height + $sys_height));
      my $usr_hi = int($bar_h - ($sys_height + $usr_height));
      my $usr_lo = int($bar_h - ($sys_height));
      my $sys_hi = int($bar_h - $sys_height);
      my $sys_lo = int($bar_h);

      $io_points     .= "$pos_x,$bar_h" if $io_points eq '';
      $io_points     .= " $pos_x,$io_hi";
      $io_points_low =  " $pos_x,$io_lo" . $io_points_low;
#      printf("io    ($time)=($pos_x, $io_hi-$io_lo) (%0.2f)\n",$iowait);

      $usr_points     .= "$pos_x,$bar_h" if $usr_points eq '';
      $usr_points     .= " $pos_x,$usr_hi";
      $usr_points_low =  " $pos_x,$usr_lo" . $usr_points_low;
#      printf("usr   ($time)=($pos_x, $usr_hi-$usr_lo) (%0.2f)\n",$usr);

      $sys_points     .= "$pos_x,$bar_h" if $sys_points eq '';
      $sys_points     .= " $pos_x,$sys_hi";
      $sys_points_low =  " $pos_x,$sys_lo" . $sys_points_low;
#      printf("sys   ($time)=($pos_x, $sys_hi-$sys_lo) (%0.2f)\n",$sys);

      $last_x = $pos_x;
    }
    $io_points  .= " $last_x,$bar_h";
    $io_points  .= $io_points_low;

    $usr_points .= " $last_x,$bar_h";
    $usr_points .= $usr_points_low;

    $sys_points .= " $last_x,$bar_h";
    $sys_points .= $sys_points_low;
  }

  $subst{cpuload_ticks}  = $cpu_ticks;
  $subst{cpuload_io}     = $io_points;
  $subst{cpuload_usr}    = $usr_points;
  $subst{cpuload_sys}    = $sys_points;

  #
  # Disk usage chart
  #

  my $util_points = '';
  my $read_points = '';

  if (@disk_samples) {
    my $max_tput  = 0;
    my $max_tput_label = '';
    for (@disk_samples) {
      my $put = $_->{read} + $_->{write};
      $max_tput = $put if $put > $max_tput;
    }
    if ($max_tput == 0) {
	$max_tput = 1;
    }

    my $last_x = 0;
    my $last_y = 0;

    for (@disk_samples) {
      my $time  = $_->{time};
      my $read  = $_->{read};
      my $write = $_->{write};
      my $util  = $_->{util};

      my $pos_x = int(($time - $t_begin) * $img_w / $t_dur);
      my $pos_y = int($bar_h - $util * $bar_h);
      $util_points .= "$pos_x,$bar_h" if $util_points eq '';
      $util_points .= " $pos_x,$pos_y";

      $pos_y = int($bar_h - ($read + $write) / $max_tput * $bar_h);
      if ($read_points ne '') {
        $read_points .= "\t\t\t<line x1=\"$last_x\" y1=\"$last_y\" x2=\"$pos_x\" y2=\"$pos_y\"/>";
      }
      $read_points .= "\n";

      if ($max_tput_label eq '' && $read + $write == $max_tput) {
        my $label = int($max_tput / 1024)." MB/s";
        $max_tput_label = "\t\t\t<text class=\"DiskLabel\" x=\"$pos_x\" y=\"0\" dx=\"-".(length($label) / 3)."em\" dy=\"-2px\">$label</text>\n";
      }

      $last_x = $pos_x;
      $last_y = $pos_y;
    }
    $util_points .= " $last_x,$bar_h";
    $read_points .= $max_tput_label;
  }

  $subst{disk_ticks}       = $cpu_ticks;
  $subst{disk_utilization} = $util_points;
  $subst{disk_throughput}  = $read_points;
  $subst{disk_file_opened} = ''; # open_points, no openfile parser implemented

  #
  # Process tree
  #

  my $tree_h     = scalar(keys %ps_info) * $proc_h;
  my $axis       = '';
  my $proc_ticks = '';
  for (my $i = 0; $i < $t_dur / $HZ; $i++) {
    my $x = $i * $sec_w;
    $proc_ticks .= "<line ".($i % 5 ? "" : "class=\"Bold\" ")."x1=\"$x\" y1=\"0\" x2=\"$x\" y2=\"$tree_h\"/>\n";
    if ($i >= 0 && $i % 5 == 0) {
      my $label;
      if ($opt_cut) {
	  $label = int($i + $t_init / $HZ) .'s';
      } else {
	  $label = int($i) .'s';
      }
      $axis .= "<text class=\"AxisLabel\" x=\"$x\" y=\"0\" dx=\"-".(length($label) / 4)."\" dy=\"-3\">$label</text>\n";
    }
  }
  $proc_ticks =~ s/^/\t\t\t/mg; $proc_ticks =~ s/^\s+//g;
  $axis       =~ s/^/\t\t\t/mg; $axis       =~ s/^\s+//g;

  my $proc_tree = '';
  my @init      = (1);
  my $ps_count  = 0;
  $proc_tree    = render_proc_tree(\@init, \$ps_count);

  $subst{process_axis}  = $axis;
  $subst{process_ticks} = $proc_ticks;
  $subst{process_tree}  = $proc_tree;

  $img_h     = $proc_h * $ps_count + $header_h;
  $subst{svg_width}  = 'width="'.($img_w > $min_img_w ? $img_w : $min_img_w).'px" height="'.($img_h + 1).'px"';


  sub __common_chart {
      local($class, $unit,
	    $subst_point, $subst_max,
	    *samples, $sample_max,
	    $label_y_shift) = @_;

      my $points = '';
      my $max_label = '';

      if (@samples) {
	  my $last_x = 0;
	  my $retval = 0;
	  for (@samples) {
	      my $time    = $_->{time};
	      my $total   = $_->{total} / $sample_max;
	      my $pos_x = int(($time - $t_begin) * $img_w / $t_dur);
	      my $pos_y = int($bar_h - $total * $bar_h);
	      $points .= "$pos_x,$bar_h" if $points eq '';
	      $points .= " $pos_x,$pos_y";

	      if ($max_label eq '' && $_->{total} == $sample_max) {
		  my $label = $sample_max ." $unit";
		  $max_label = "\t\t\t<text class=\"$class\" x=\"$pos_x\" y=\"0\" dx=\"-".(length($label) / 3)."em\" dy=\"".$label_y_shift."px\">$label</text>\n";
		  $retval = $pos_x; # returns label position
	      }

	      $last_x = $pos_x;
	  }
	  $points .= " $last_x,$bar_h";
      }
      $subst{$subst_point} = $points;
      $subst{$subst_max}    = $max_label;

      return $retval;
  }

  #
  # Interrupt chart.
  #
  __common_chart("Interrupt", "ints",
		 interrupt_points, interrupt_max,
		 *intr_samples, $intr_max, -2);

  #
  # Context Switch chart.
  #
  __common_chart("ContextSw", "sw",
		 contextsw_points, contextsw_max,
		  *ctxt_samples, $ctxt_max, -12);

  #
  # Processes created chart.
  #
  __common_chart("ProcCreate", "procs",
		 processcreate_points, processcreate_max,
		  *proc_samples, $proc_max, -2);

  #
  # Time spent in interrupt context chart.
  #
  __common_chart("IntrCtxt", "us",
		 intrctxt_points, intrctxt_max,
		  *intrctxt_samples, $intrctxt_max, -12);

  #
  # Finally output.
  #

  if ($output_file eq '') {
      print template_subst($chart_svg_template, \%subst);
  } else {
      open(OUTPUT_FILE, ">$output_file");
      print OUTPUT_FILE template_subst($chart_svg_template, \%subst);
      close(OUTPUT_FILE);
  }
}

sub render_proc_tree {
  my $ps_list  = shift;
  my $ps_count = shift;
  my $level    = shift || 0;

  my $proc_tree = '';
  for (sort {$a <=> $b} @{$ps_list}) {
    # Hide bootchartd and its children processes
    next if $ps_info{$_}->{comm} eq 'bootchartd';

    my $children = $ps_by_parent{$_};
    $proc_tree  .= render_proc($_, $ps_count, $level);
    $proc_tree  .= render_proc_tree($children, $ps_count, $level + 1) if defined $children;
  }
  return $proc_tree;
}

sub process_opacity {
    my $load = shift;
    my $opacity = 0;
    if ($load > 1.0) {
	$opacity = 1.0;
    } elsif ($load > 0.1) {
	$opacity = $load;
    } elsif ($load > 0) {
	$opacity = 0.1;
    } else {
	$opacity = 0;
    }
    return $opacity;
}

sub render_proc {
  my $pid      = shift;
  my $ps_count = shift;
  my $level    = shift;

  my $info    = $ps_info{$pid};
  return '' if defined $info->{done};  # Draw once

  my @samples = @{$info->{samples}};
  return '' if scalar(@samples) < 2; # Skip processes with only 1 sample

  $p_begin  = $info->{start};
  if ($opt_cut) {
      $p_begin = $t_begin if ($p_begin < $t_begin);
  }
  my $p_period = $headers{"sample.period"};
  my $p_end    = $samples[-1]->{time};
  my $p_dur    = $p_end - $p_begin + $p_period;
  #print "p_begin=$p_begin, p_period=$p_period, p_end=$p_end, p_dur=$p_dur\n";

  my $x = ($p_begin - $t_begin) * $sec_w / $HZ;
  my $y = ${$ps_count} * $proc_h;
  my $w = $p_dur * $sec_w / $HZ;

  # Save for children to use.
  $info->{x} = $x;
  $info->{y} = $y;

  my %subst;

  my $position = "$x,$y";
  #print "pid=$pid = ($position)\n";
  $subst{process_translate} = $position;

  my $border   = "width=\"$w\" height=\"$proc_h\"";
  my $timeline = "<rect class=\"Sleeping\" x=\"0\" y=\"0\" $border/>\n";
  $subst{process_border}    = $border;

  # parent process connector
  my $ppid = $info->{ppid};
  if ($ppid != 0) {
      my $px = $ps_info{$ppid}->{x};
      my $py = $ps_info{$ppid}->{y};

      my $p_from  = int($proc_h * (1/3));
      my $p_to    = int($proc_h * (2/3));
      my $p_stickout = 5;
      my $pdx = $px-$x - $p_stickout;
      my $pdy = $py-$y + $p_to;
      #print "$level : $pid($info->{comm})(x=$x,y=$y) => $ppid($ps_info{$ppid}->{comm})(x=$px,y=$py)\n";

      my $parent = "<line style=\"stroke-dasharray: 2,2;\" x1=\"0\" y1=\"$p_from\" x2=\"$pdx\" y2=\"$p_from\"/>";
      $parent   .= "<line style=\"stroke-dasharray: 2,2;\" x1=\"$pdx\" y1=\"$p_from\" x2=\"$pdx\" y2=\"$pdy)\"/>";
      $parent   .= "<line style=\"stroke-dasharray: 2,2;\" x1=\"$pdx\" y1=\"$pdy\" x2=\"".($pdx+$p_stickout)."\" y2=\"$pdy)\"/>";
      #print "$pid($info->{comm})(x=$x,y=$y) => $ppid($ps_info{$ppid}->{comm})(x=$px,y=$py)\n";
      $subst{process_parent_line} = $parent;
  }
  my ($last_tx, $last_sample);
  for my $sample (@samples) {
    my $time  = $sample->{time};
    my $state = $sample->{state};
    my $tx = ($time - $p_begin) * $sec_w / $HZ;
    $tx = 0 if $tx < 0;

    if (defined $last_sample) {
      my $tw = $tx - $last_tx;

      if ($state ne 'S') {
	my $dt   = $sample->{time} - $last_sample->{time};
        my $sys  = ($sample->{sys} - $last_sample->{sys}) / $dt;
        my $user = ($sample->{user} - $last_sample->{user}) / $dt;

        my %class   = ('D' => 'UnintSleep', 'R' => 'Running', 'T' => 'Traced', 'Z' => 'Zombie');
        my $opacity = ($state eq 'R') ? ' fill-opacity="'.(process_opacity($sys + $user)).'"' : '';
	#print "$info->{comm} $state:$opacity ($sys + $user)\n";

        $timeline  .= "<rect class=\"$class{$state}\"$opacity x=\"$last_tx\" y=\"0\" width=\"$tw\" height=\"$proc_h\"/>\n";
      }
    }

    $last_tx = $tx;
    $last_sample = $sample;
  }
  $timeline =~ s/^/\t\t/mg; $timeline =~ s/^\s+//g;

  $subst{process_status} = $timeline;

  my $label = $info->{comm};
  $label .= "(".$pid.")" if ($opt_pid);
  my $label_pos = ($w < 200 && ($x + $w + 200) < $img_w) ?
    "dx=\"2\" dy=\"".($proc_h - 1)."\" x=\"".($w + 1)."\" y=\"0\"" :
    "dx=\"".($w / 2)."\" dy=\"".($proc_h - 1)."\" x=\"0\" y=\"0\"";

  $subst{process_label_pos}  = $label_pos;
  $subst{process_label_name} = $label;
  $subst{process_title_name} = "";		# title

  $info->{done} = 1;
  ${$ps_count}++;
  return template_subst($process_svg_template, \%subst);
}

sub template_subst {
  my $template = shift;
  my $tmp = shift;
  my %assoc = %{$tmp};

  foreach $i (keys %assoc) {
    #print "\Q{$i}\E  ==> $assoc{$i}\n";
    $template =~ s/\Q{$i}\E/$assoc{$i}/g;
  }
  return $template;
}

sub help {
  print("bootchart.pl [-c] [-oOUTPUT.svg] INPUT.tgz \n");
  print("Parse INPUT.tgz and generate SVG to OUTPUT.svg or STDOUT.\n");
  print("     -c          : Cut output before starting\n");
  print("     -p          : Show process id in process chart\n");
  print("     -oFILENAME  : Output file (STDOUT if not specified)\n");
}

if ($#ARGV == -1) {
    help();
    exit(-1);
}

while ($_ = $ARGV[0]) {
    shift;
    /^-c/ && ($opt_cut = 1);
    /^-p/ && ($opt_pid = 1);
    /^-o(.+)/ && ($opt_result_file = $1);
    /^(.+)/ && ($opt_source_file = $1);
}

die "Input file not specified" if ($opt_source_file eq '');
die "File $opt_source_file not found" unless -e $opt_source_file;

parse_logs($opt_source_file);
render_svg($opt_result_file);

# Interuppts.
# for (@intr_samples) {
#     my $intrs = $_->{total};
#     print "total: $intrs\n";
#     for (my $i = 0; $i < $intr_pins; $i++) {
# 	print "$_->{$i},"
#     }
#     print "\n";
# }

# Context switches.
# for (@ctxt_samples) {
#     my $ctxts = $_->{total};
#     print "total: $ctxts\n";
# }

# Processes created
# for (@proc_samples) {
#     my $procs = $_->{total};
#     print "total: $procs\n";
# }
# print "$proc_max\n";

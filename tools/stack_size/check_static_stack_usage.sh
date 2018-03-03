#/bin/sh

# generate checkstack-all.pl report
${CROSS_COMPILE}objdump -d $KBUILD_OUTPUT/vmlinux | \
   tools/stack_size/checkstack-all.pl arm >checkstack1.txt
cat checkstack1.txt | cut -b 12- | sed "s/[[]vmlinux.*:/(checkstack-all.pl)/" \
   >checkstack-f.txt

# generate frame warn report
ttc set_config CONFIG_FRAME_WARN=1
# if kbuild parallelizes the build, the output gets messed up;
# do 'make uImage' (with only one thread) instead
#ttc kbuild 2>framewarn1.txt
make uImage 2>framewarn1.txt
cat framewarn1.txt | grep -B 1 "frame size.*larger than" | \
    sed "s/.*function '\(.*\)':/\1 (frame_warn)/" | \
    sed "s/.*warning: the frame size of//" | \
    sed "s/bytes is larger than 1 bytes//" | \
    grep -v [-][-] >framewarn2.txt
cat framewarn2.txt | python tools/stack_size/joinlines.py >framewarn-f.txt
if [ ! -s framewarn-f.txt ] ; then
	echo problem generating framewarn3.txt using CONFIG_FRAME_WARN
fi

# reset build for next time, and prepare for next test
ttc set_config CONFIG_FRAME_WARN=1024
ttc kbuild

# generate stack tracer report
ttc kinstall
ttc reboot

echo waiting for target to boot
sleep 60
ttc run "echo 1 >/proc/sys/kernel/stack_tracer_enabled"
ttc run "mount -t debugfs debugfs /debug"
ttc run "ls / | sed s/root/foo/ | grep foo"
ttc run "cat /debug/tracing/stack_trace" >stacktrace1.txt
# reformat the report, eliminating trailing offsets, leading whitespace,
# and reordering the function name and stack size
cat stacktrace1.txt | grep "[\)]" | cut -b 14- | sed "s/\+.*//" | \
    sed "s/^[ ]*//" | \
    sed "s/\([0-9]\{1,5\}\)[ ]*\(.*\)/\2 (stack_trace)   \1/" >stacktrace-f.txt

# generate stack_size report
python tools/stack_size/stack_size -f $KBUILD_OUTPUT/vmlinux >stacksize1.txt
cat stacksize1.txt | sed "s/ \{1,40\}/ (stack_size) /" >stacksize-f.txt

# collate the results
cat checkstack-f.txt framewarn-f.txt stacktrace-f.txt stacksize-f.txt | \
    env LC_COLLATE=C sort -s | \
    python tools/stack_size/columnize.py >stackusage.txt


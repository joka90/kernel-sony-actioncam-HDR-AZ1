#!/bin/bash
for i in `cat kptool_cpu0 | grep "ret value" | awk {'print $4'} | sort -u`
do
   echo "-------------------$i------"
   for j in `grep $i kptool_cpu0 | awk {'print $1'} | sort -u`
   do
       echo "$j= `grep $i kptool_cpu0 | grep -c $j`"
   done
done

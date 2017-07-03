#!/bin/bash

dpath="../REDIS/2017-03-23/TC300/1/"
dpath="../LEVELDB/2017-03-24/TC300/2/"
fstat=$dpath"stats.csv"
rm -f $fstat
filename="perf_data"


iface_s="ens3"
iface_c="eth1"
disk_="dev8-0"


#CPU Count
cpu_c=$(sar -f $dpath$filename -u | awk 'NR==1{print substr($9,2)}')

# Rx_KBps , Tx_KBps , Utilization %
nw_c=$(sar -f $dpath$filename -n DEV | grep $iface_c | tail -n 1 | awk '{print $5 "," $6 "," $10}')
nw_s=$(sar -f $dpath$filename -n DEV | grep $iface_s | tail -n 1 | awk '{print $5 "," $6 "," $10}')

# CPU utilization %
cpu_u=$(sar -f $dpath$filename -u | tail -n 1 | awk '{print 100-$8}')

# Memory utilization %
mem_u=$(sar -f $dpath$filename -r | tail -n 1 | awk '{print $2+$3","$4}')

# Disk rd_KBps , wr_KBps , utilization % [each sector is of 512 Bytes each conversion : $4*512/1024]
disk_u=$(sar -f $dpath$filename -d | awk '$1=="Average:" && $2=="'$disk_'" {print $4/2 "," $5/2 "," $10}')


echo $cpu_c
echo $cpu_u
echo $nw_c$nw_s
echo $mem_u
echo $disk_u 

echo "CPU count, CPU util, NW rx(KBps), NW tx(KBps), NW util %, Mem Size(Bytes), Mem util %, Disk rd (KBps), Disk wr (KBps), Disk util %, Page Faults" >> $fstat
echo $cpu_c","$cpu_u","$nw_c$nw_s","$mem_u","$disk_u >> $fstat

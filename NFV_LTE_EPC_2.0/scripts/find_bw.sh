#!/bin/sh

usage(){
echo "Usage: $0 [-i INTERFACE] [-s INTERVAL] [-c COUNT] [-n NUM_UE]"
echo
echo "-i INTERFACE"
echo "    The interface to monitor, default is eth0."
echo "-s INTERVAL"
echo "    The time to wait in seconds between measurements, default is 3 seconds."
echo "-c COUNT"
echo "    The number of times to measure, default is 10 times."
echo "-n NUM_UE"
echo "    The number of UE."
exit 4
}

readargs(){
while [ "$#" -gt 0 ] ; do
  case "$1" in
   -i)
    if [ "$2" ] ; then
     interface="$2"
     shift ; shift
    else
     echo "Missing a value for $1."
     echo
     shift
     usage
    fi
   ;;
   -s)
    if [ "$2" ] ; then
     sleep="$2"
     shift ; shift
    else
     echo "Missing a value for $1."
     echo
     shift
     usage
    fi
   ;;
   -c)
    if [ "$2" ] ; then
     counter="$2"
     shift ; shift
    else
     echo "Missing a value for $1."
     echo
     shift
     usage
    fi
   ;;
   -n)
    if [ "$2" ] ; then
     numue="$2"
     shift ; shift
    else
	 echo "Missing a value for $1."
     echo
     shift
     usage
    fi
   ;;
   *)
    echo "Unknown option $1."
    echo
    shift
    usage
   ;;
  esac
done
}

checkargs(){
if [ ! "$interface" ] ; then
  interface="eth0"
fi
if [ ! "$sleep" ] ; then
  sleep="3"
fi
if [ ! "$counter" ] ; then
  counter="10"
fi
}

printrxbytes(){
/sbin/ifconfig "$interface" | grep "RX bytes" | cut -d: -f2 | awk '{ print $1 }'
}

printtxbytes(){
/sbin/ifconfig "$interface" | grep "TX bytes" | cut -d: -f3 | awk '{ print $1 }'
}

#################################################
bytestohumanreadable(){
multiplier="0"
number="$1"
while [ "$number" -ge 1024 ] ; do
  multiplier=$(($multiplier+1))
  number=$(($number/1024))
done
case "$multiplier" in
  1)
   echo "$number Kb"
  ;;
  2)
   echo "$number Mb"
  ;;
  3)
   echo "$number Gb"
  ;;
  4)
   echo "$number Tb"
  ;;
  *)
   echo "$1 b"
  ;;
esac
}
##################################################
  
printresults(){
duration=$(($counter * $sleep))
total=0
totalRecv=0
totalSend=0
while [ "$counter" -ge 0 ] ; do
  counter=$(($counter - 1))
  if [ "$rxbytes" ] ; then
   oldrxbytes="$rxbytes"
   oldtxbytes="$txbytes"
  fi
  rxbytes=$(printrxbytes)
  txbytes=$(printtxbytes)
  if [ "$oldrxbytes" -a "$rxbytes" -a "$oldtxbytes" -a "$txbytes" ] ; then
   temp1=$(($rxbytes - $oldrxbytes))
   temp2=$(($txbytes - $oldtxbytes))
   totalRecv=$(($totalRecv + $temp1))
   totalSend=$(($totalSend + $temp2))
   total=$(($total + $temp1))
   total=$(($total + $temp2))
   echo "RXbytes = $(($rxbytes - $oldrxbytes)) TXbytes = $(($txbytes - $oldtxbytes))"
  else
   echo "Monitoring $interface every $sleep seconds. (RXbyte total = $rxbytes TXbytes total = $txbytes)"
  fi
  if [ "$counter" -ne -1 ] ; then
  	sleep "$sleep"
  fi
done

total=`echo "scale=2; $total/$duration" | bc `
recv=`echo "scale=2; $totalRecv/$duration" | bc `
snd=`echo "scale=2; $totalSend/$duration" | bc `
printf "$numue "
echo $recv $snd $total
echo $numue $recv $snd $total >> bw.txt
}

readargs "$@"
checkargs
printresults

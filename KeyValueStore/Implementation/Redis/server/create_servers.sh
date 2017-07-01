if [ $# -lt 3 ]
then
	echo "Count, IPAddress and Port number are required as parameters"
else
  sport=$3
	sdirp=server
  for i in `seq 1 $1`
  do
    sdir=$sdirp"_$i"
  	rm -r $sdir
  	mkdir -p $sdir
  	port=$((i+sport-1))
  	sed -e "s/IPADDRESS/$2/g" -e "s/PORTNUM/$port/g" redis.conf > $sdir/redis.conf
  done
fi

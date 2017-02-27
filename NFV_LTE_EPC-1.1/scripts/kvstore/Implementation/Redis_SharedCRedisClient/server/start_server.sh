if [ $# -lt 2 ]
then
	echo "IPAddress and Port number required as parameter"
else
	sdir=serverinstance
	mkdir -p $sdir
	sed -e "s/IPADDRESS/$1/g" -e "s/PORTNUM/$2/g" redis.conf > $sdir/redis.conf
	cd $sdir
	../redis-3.2.4/src/redis-server ./redis.conf
fi

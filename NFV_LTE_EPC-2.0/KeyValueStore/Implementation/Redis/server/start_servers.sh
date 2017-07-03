if [ $# -lt 1 ]
then
	echo "Count required as parameter"
else
	for i in `seq 1 $1`
	do
		sdir=server
		cd $sdir"_$i"
		../redis-3.2.4/src/redis-server ./redis.conf &
		cd ..
	done
fi

#!/bin/bash

cd /home/pi/public_html/data

for rrd in *.rrd; do
	ds="`basename "$rrd" .rrd`";
#	echo "Exporting $ds from $rrd to ${ds}_day.json";
	rrdtool xport \
		--daemon=/var/run/rrdcached.sock \
		-m 86400 \
		--step 128 \
		-s -2days \
		--json \
		DEF:avg=${rrd}:${ds}:AVERAGE \
		XPORT:avg:avg |sed -e "s/'/\"/g;s/\([a-z]*\):/\"\1\":/g" >export/${ds}_day.json.new;
		mv export/${ds}_day.json.new export/${ds}_day.json;
done

#		DEF:min=${rrd}:${ds}:MIN \
#		DEF:max=${rrd}:${ds}:MAX \
#		XPORT:min:min \
#		XPORT:max:max 

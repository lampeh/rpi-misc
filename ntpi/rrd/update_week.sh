#!/bin/bash

cd /home/pi/public_html/data

for rrd in *.rrd; do
	ds="`basename "$rrd" .rrd`";
#	echo "Exporting $ds from $rrd to ${ds}_week.json";
	rrdtool xport \
		--daemon=/var/run/rrdcached.sock \
		-m 86400 \
		--step 640 \
		-s -1week \
		--json \
		DEF:avg=${rrd}:${ds}:AVERAGE \
		DEF:min=${rrd}:${ds}:MIN \
		DEF:max=${rrd}:${ds}:MAX \
		XPORT:avg:avg \
		XPORT:min:min \
		XPORT:max:max |sed -e "s/'/\"/g;s/\([a-z]*\):/\"\1\":/g" >export/${ds}_week.json.new;
		mv export/${ds}_week.json.new export/${ds}_week.json;
done

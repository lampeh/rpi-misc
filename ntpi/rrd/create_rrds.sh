#!/bin/bash

# 16s (1 sample) 1yr
# 64s (4 samples) 5 yrs
# 1h (225 samples) 30 yrs

for v in leap stratum precision rootdelay rootdisp tc mintc frequency sys_jitter clk_jitter clk_wander offset; do
	rrdtool create ${v}.rrd \
		-s 16 \
		DS:${v}:GAUGE:60:U:U \
		RRA:AVERAGE:0.5:1:1976400 \
		RRA:AVERAGE:0.5:4:2470500 \
		RRA:MIN:0.5:4:2470500 \
		RRA:MAX:0.5:4:2470500 \
		RRA:AVERAGE:0.5:225:262800 \
		RRA:MIN:0.5:225:262800 \
		RRA:MAX:0.5:225:262800
done

for v in poll noreply badformat baddata ; do 
	rrdtool create ${v}.rrd \
		-s 16 \
		DS:${v}:COUNTER:60:0:U \
		RRA:AVERAGE:0.5:1:1976400 \
		RRA:AVERAGE:0.5:4:2470500 \
		RRA:MIN:0.5:4:2470500 \
		RRA:MAX:0.5:4:2470500 \
		RRA:AVERAGE:0.5:225:262800 \
		RRA:MIN:0.5:225:262800 \
		RRA:MAX:0.5:225:262800
done

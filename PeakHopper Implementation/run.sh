#!/bin/sh
./waf --run "scratch/simulate -tracing=true -duration=50 -peakHopper=true"
gnuplot rtt_rto.plt
gnuplot cwnd.plt
gnuplot ssth.plt
gnuplot mean_retransmission.plt
gnuplot rto_by_rtt.plt

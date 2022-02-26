set terminal png
set output "rto_by_rtt.png"
set xlabel "time"
set ylabel "RTO_n/RTT_l"
plot "rto_by_rtt.data" with lines title "RTO_n/RTT_l"

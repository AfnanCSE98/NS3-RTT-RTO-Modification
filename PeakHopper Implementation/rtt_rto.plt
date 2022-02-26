set terminal png
set output "rtt_rto_graph.png"
set xlabel "time"
set ylabel "RTT/RTO"
plot "rtt.data" with lines title "rtt", "rto.data" with lines title "rto"

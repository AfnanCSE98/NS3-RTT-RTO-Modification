set terminal png
set output "mean_retransmission.png"
set xlabel "time"
set ylabel "RTO_i/RTT_i at Timeout"
plot "mean_retransmission.data" with lines title "RTO_i/RTT_i"

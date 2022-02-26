set terminal png
set output "cwnd_graph.png"
set xlabel "time"
set ylabel "Congestion Window"
plot "cwnd.data" with lines title "Cwnd"

set terminal png
set output "ssth_graph.png"
set xlabel "time"
set ylabel "Slow Start Threshold"
plot "ssth.data" with lines title "Ssth"

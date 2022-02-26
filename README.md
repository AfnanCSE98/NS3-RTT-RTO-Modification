# NS3-RTT-RTO-Modification
NS3 is an open-source event-driven simulator designed specifically for research
in computer communication networks. In this project, I have to vary different
parameters such as number of nodes, number of flows, number of packets per
second and measure some metrics and plot them in some graphs.
I also had to modify some mechanisms of Retransmission Timeout calculation
and compare the modified version with the original implementation of NS3.The
paper from which I have taken idea from is The Peak-Hopper: A New End-to-
End Retransmission Timer for Reliable Unicast Transport
### Simulated Networks 
- Wireless 802.11 (static) network using AODV routing in Random topology
- Wireless 802.15.4 (static) network using Ripng routing in Random Topology
- Dumbbell topology consisting of same number of senders and receivers and two routers for showing RTT and RTO modification. 

### Parameters and Metrics 
#### Parameters 
- Number of nodes (20, 40, 60, 80, and 100)
- Number of flows (10, 20, 30, 40, and 50)
- Number of packets per second (10, 20, 30, 40, and 50)
- Coverage Area (1000 , 1500 , 2000 , 2500 ,2800 $m^2$) 
#### Metrics 
- Network Throughput
- End-to-End Delay
- Packet Delivery Ratio
- Packet Drop Ratio 

### How to simulate
To run the modified peakHopper implememntation of RTT and RTO ,  replace the four modified files , namely `tcp-socket-base.h`,`tcp-socket-base.cc`,`rtt-estimator.h`,`rtt-estimator.cc` into `ns3.35/src/internet/model` and `simulate.cc` into `/scratch` location and run the script `run.h` from `ns-3.35` directory and you will see the output.

To run 802.11 and 802.15.4 protocols , paste the .cc files in `/scratch` directory and run the script `run.h` from `ns-3.35` directory and you will see the output.

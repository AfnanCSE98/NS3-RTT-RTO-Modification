from xml.etree import ElementTree as ET
import sys
from statistics import mean
#for csv
import csv 
et=ET.parse(sys.argv[1])
bitrates=[]
losses=[]
delays=[]
#drop
drop_ratio = []
delivery_ratio = []
totalrx = 0
totaltx = 0
packet_drop_count = 0
for flow in et.findall("FlowStats/Flow"):
	for tpl in et.findall("Ipv6FlowClassifier/Flow"):
		if tpl.get('flowId')==flow.get('flowId'):
			break
	
	losses.append(int(flow.get('lostPackets')))
	packet_drop_count+=int(flow.get('lostPackets'))

	
	rxPackets=int(flow.get('rxPackets'))
	totalrx+=int(flow.get('rxPackets'))
	totaltx+=int(flow.get('txPackets'))
	# for pktdrop in flow.findall('packetsDropped'):
	# 	packet_drop_count+=int(pktdrop.get('number'))
		#print("drop count : \n",int(pktdrop.get('number')))
	if rxPackets==0:
		bitrates.append(0)
	else:
		t0=float(flow.get('timeFirstRxPacket')[:-2])
		t1=float(flow.get("timeLastRxPacket")[:-2])
		duration=(t1-t0)*1e-9
		if duration==0:
			continue;
		bitrates.append(8*int(flow.get("rxBytes"))/duration*1e-3)
		delays.append(float(flow.get('delaySum')[:-2])*1e-9/rxPackets)

delivery_ratio.append(totalrx/totaltx)
drop_ratio.append(packet_drop_count/totaltx)

print("Throughput : %.2f" % (mean(bitrates)))
print("Average end-to-end delay : %.2f ms" % mean(delays))
print("Average packet loss ratio : %.2f %%" % (mean(losses)))
print("Delivery ratio : %.2f" % (mean(delivery_ratio)))
print("Drop ratio : %.2f" % (mean(drop_ratio)))


#csv insertion
f_name = "output.csv"
with open(f_name,"a",newline="") as csv_file:
	writer = csv.writer(csv_file)
	writer.writerow([sys.argv[2],mean(bitrates),mean(delays),mean(delivery_ratio),mean(drop_ratio)])
csv_file.close()

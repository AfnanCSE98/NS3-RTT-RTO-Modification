./waf --run "802.15_test --coverageArea=100"
./waf --run "802.15_test --coverageArea=150"
./waf --run "802.15_test --coverageArea=200"
./waf --run "802.15_test --coverageArea=250"
./waf --run "802.15_test --coverageArea=300"

#write to csv
python parser_802.15.4.py 802.15_coverageArea_100.xml 100
python parser_802.15.4.py 802.15_coverageArea_150.xml 150
python parser_802.15.4.py 802.15_coverageArea_200.xml 200
python parser_802.15.4.py 802.15_coverageArea_250.xml 250
python parser_802.15.4.py 802.15_coverageArea_300.xml 300

#plot
gnuplot vary.code

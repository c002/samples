#!/bin/sh
#
./scan_db.py -lp -f tmp/adsl.radar.121110.db > /tmp/hr1
./scan_db.py -lp -f tmp/adsl.radar.121111.db >> /tmp/hr1
./scan_db.py -lp -f tmp/adsl.radar.121112.db >> /tmp/hr1
./scan_db.py -lp -f tmp/adsl.radar.121113.db >> /tmp/hr1
./scan_db.py -lp -f tmp/adsl.radprd01.121110.db >> /tmp/hr1
./scan_db.py -lp -f tmp/adsl.radprd01.121111.db >> /tmp/hr1
./scan_db.py -lp -f tmp/adsl.radprd01.121112.db >> /tmp/hr1
./scan_db.py -lp -f tmp/adsl.radprd01.121113.db >> /tmp/hr1
./scan_db.py -lp -f tmp/adsl.siri.121112.db >> /tmp/hr1
./scan_db.py -lp -f tmp/adsl.siri.121113.db >> /tmp/hr1

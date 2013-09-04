#!/bin/sh
#
# Wrapper script for LinxFaultScraper.
#
# Normally run out of cron 
#	linxfaults.sh generate
# followed some time later (1/2hr) by a cron run of
#       linxfaults.sh fetch
#
# It will do this against all keystore's found 
#
# $Source$
# $Id$

if [ $# -ne 1 ]; then
    echo "usage: $0 generate | fetch"
    exit
fi
if [ $1 != "generate" -a $1 != "fetch" ]; then
    echo "usage: $0 generate | fetch"
    exit
fi

cd /opt/servicenet/linxfaults

VERBOSE=""
LOGS=/var/log/linxfaults/special
GNUDATE=/opt/gnu/bin/date
STARTDATE="06/03/2013"
ENDDATE="05/04/2013"

ACTION=$1
KEYS=`ls resources/RS*.jks`

JAVA=/opt/jdk1.6.0	# Remember , cacerts needs to Telstra CA certs
CP="classes"
for f in lib/*jar; do
   CP=$CP:$f
done

for keystore in $KEYS; do
    $JAVA/bin/java -cp $CP au.com.aapt.linxfaults.LinxFaultScraper $VERBOSE --$ACTION --keystore=$keystore --fromdate=$STARTDATE --todate=$ENDDATE
done

#
# After fetching, merge the data , fiddle it and email it 
#
#if [ "$ACTION" = "fetch" ]; then
#    ./merge.py
#fi

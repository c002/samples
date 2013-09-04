#!/bin/sh -x
#
# Usualy run via ssh from adslProcess.sh
#
# Retrieve the ADSL Radius Acct Berkeley files from yesterday 
# and and day before and process these. 
#
# 
# 

MAILTO="radprocess@connect.com.au"

if [ -f /tmp/adslProcess.LCK ]; then
/usr/bin/mailx -s "Daily ADSL processing error" $MAILTO << EOF
Lockfile found koro:/tmp/adslProcess.LCK
Aborting further processing.
EOF
    exit
fi

#/usr/bin/touch /tmp/adslProcess.LCK

SERVERS="radii siri radar"
SOURCE="/var/log/e3adsl"
DEST="tmp"
HOME="/import/bta4/adsl"
REPORT="TRUE"

SCP="/opt/local/bin/scp -C"
GNUDATE="/opt/bta4/bin/gnudate"
SED=/usr/bin/sed

ALT_TWODAYS=`$GNUDATE +%y%m%d --date='2 days ago'`
ALT_YESTERDAY=`$GNUDATE +%y%m%d --date=yesterday`

ALT2_YESTERDAY=`$GNUDATE +%Y%m%d --date=yesterday`

XML_DAILY_DATE=""

YESTERDAY=`$GNUDATE +%d-%b-%Y --date=yesterday`

# Handle reprocessing. The data given as param must be the day of when 
# this job was supposed to run but failed. Day is in 'DD-MON-YYYY' format.
# Although gnudate can parse several formats, they don't all work.
if [ x"$1" != "x" ]; then
    REPORT="FALSE"
    DATE=$1
    ALT_TWODAYS=`$GNUDATE +%y%m%d --date="$DATE 2 days ago"`

    ALT_YESTERDAY=`$GNUDATE +%y%m%d --date="$DATE 1 day ago"`
    YESTERDAY=`$GNUDATE +%d-%b-%Y --date="$DATE 1 day ago"`
    XML_DAILY_DATE="-date="`$GNUDATE +%Y%m%d --date="$DATE 2 days ago"`
fi

cd $HOME

# Get Berkley db files from ADSL Radius servers
for svr in $SERVERS; do
   fname=$svr
   if [ "$svr" = "radii" ]; then
	fname="radprd01"
   fi
   $SCP $svr:$SOURCE/adsl.$fname.$ALT_TWODAYS.db $DEST > /dev/null 2>&1
   $SCP $svr:$SOURCE/adsl.$fname.$ALT_YESTERDAY.db $DEST > /dev/null 2>&1
done

# Check that we have files for the 2 days or we end up with bad data.
# Expect at least 2 for each day
COUNT_A=`ls $DEST/adsl.*.$ALT_YESTERDAY.db|wc -w`
COUNT_B=`ls $DEST/adsl.*.$ALT_TWODAYS.db|wc -w`

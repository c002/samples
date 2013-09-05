#!/bin/sh

# run kenan to Forte data import.
# 
# 2 ways to run.
# 1) Pull kenan customer data from the DataWarehouse and insert into Forte
# 2) or Take all files in "INDIR" directory , and process these for 
# insert into FORTE.
#
# Do a --help to see options for kenan2forte.jar
# See etc/application.properties for db connections config

#INSERT="--insert"	# --insert to insert data into Forte
INSERT=""
DEBUG="--debug=1"	# 1 for Mapping check. >1 for more noise.

INDIR="infiles"
DONEDIR="donefiles"
LOCKFILE="/tmp/k2frun.lock"

if [ -f "/tmp/k2frun.lock" ]; then
    echo "$0 Lockfile found $LOCKFILE.  Aborting run."
    exit
fi

JAVAHOME=/opt/jdk1.6.0
OPTS="-Xms32m -Xmx512m"
GNUDATE=/opt/gnu/bin/date
DATE=`$GNUDATE +%Y%m%d`

ERRLOG=log/k2f-$DATE-error.log
RUNLOG=log/k2f-$DATE-run.log

_2DAYSAGO=`$GNUDATE --date="-2 days" +%Y%m%d`
YESTERDAY=`$GNUDATE --date="-1 day" +%Y%m%d`
TODAY=`$GNUDATE +%Y%m%d`

THISHOST=`/usr/bin/hostname`
cd `/usr/bin/dirname $0`

if  [ "$#" -eq 0 ]; then
    echo "$0 [--help | --filemode | --dbmode [--fromdate=yyyymmdd --todate=yyyymmdd]]"
    exit
fi

if  [ "$1" != "--help" -a "$1" != "--filemode" -a "$1" != "--dbmode" ]; then
    echo "$0 [ --help | --filemode | --dbmode ]"
    exit
fi

if  [ "$1" = "--help" ]; then
    echo "Options available on kenan2forte.jar:"
    $JAVAHOME/bin/java $OPTS -jar kenan2forte.jar --help
    exit
fi

touch $LOCKFILE
if  [ "$1" = "--filemode" ]; then
    for f in `ls $INDIR/*`; do
	echo $f
        $JAVAHOME/bin/java $OPTS -jar kenan2forte.jar $DEBUG $INSERT --import=$f
	mv $f $DONEDIR
    done
elif [ "$1" = "--dbmode" ]; then 
    #$JAVAHOME/bin/java $OPTS -jar kenan2forte.jar $DEBUG $INSERT --fromdate=$_2DAYSAGO --todate=$YESTERDAY >> $RUNLOG 2>>$ERRLOG
    if [ x"$2" != x"" -a x"$3" != x"" ]; then
        $JAVAHOME/bin/java $OPTS -jar kenan2forte.jar $DEBUG $INSERT $2 $3
    else
        $JAVAHOME/bin/java $OPTS -jar kenan2forte.jar $DEBUG $INSERT --fromdate=$_2DAYSAGO --todate=$TODAY 
    fi

fi

#    $JAVAHOME/bin/java $OPTS -jar kenan2forte.jar --fromdate=$_2DAYSAGO --todate=$YESTERDAY >> $RUNLOG 2>>$ERRLOG
rm $LOCKFILE

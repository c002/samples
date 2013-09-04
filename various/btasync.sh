#!/bin/sh
#
# Run as nm
#
# Synchronise remote queue of timestamps from master to local 'ready' dir
# Sync completed files back to master host.
# 
# 
# 

MASTER=inkata
HOST=`/bin/hostname`
ECHO=

if [ x"$1" = "x" ]; then
    echo "usage: $0 bta_configfile"
    exit
fi

#BTACONF="/import/bta4/etc/bta4test.conf"
BTACONF="$1"

READBTACONF="/import/bta4/bin/readbtaconf.py"
SSYNC=/opt/local/bin/rsync
SSH=/opt/local/bin/ssh
RSYNC_RSH=$SSH
export RSYNC_RSH
SED=/usr/bin/sed
HEAD=/usr/bin/head
RUNTIME=`/usr/bin/date '+%Y%m%d %H:%M:%S'`
LOGDIR="/tmp"

if [ ! -f "$READBTACONF" ] ; then 
    exit
fi

if [ ! -f "$BTACONF" ]; then
    exit
fi

HOST=`/bin/hostname`
LSPOOL=`$READBTACONF -c $BTACONF -s $HOST`
MSPOOL=`$READBTACONF -c $BTACONF -o masterspooldir`
MAINQ=`$READBTACONF -c $BTACONF -o mainqueuedir`
QUEUE=`$READBTACONF -c $BTACONF -p $HOST`
READY=`$READBTACONF -c $BTACONF -o localreadydir`
DONE=`$READBTACONF -c $BTACONF -o donedir`
STAGE=`$READBTACONF -c $BTACONF -o stagedir`
PROCD=`$READBTACONF -c $BTACONF -o procdir`
DAYSCH=`$READBTACONF -c $BTACONF -o dayschanged`
DUMPD=`$READBTACONF -c $BTACONF -o dumpdir`
MINSPACE=`$READBTACONF -c $BTACONF -o minspace`
MAXMOVE=`$READBTACONF -c $BTACONF -o maxmove`

if [ $HOST = $MASTER ]; then
    LSPOOL=$MSPOOL
fi

if [ ! -d "$LSPOOL/$READY" ]; then
    mkdir -p "$LSPOOL/$READY"
fi
if [ ! -d "$LSPOOL/$STAGE" ]; then
    mkdir -p "$LSPOOL/$STAGE"
fi
if [ ! -d "$LSPOOL/$DONE" ]; then
    mkdir -p "$LSPOOL/$DONE"
fi
if [ ! -d "$LSPOOL/$PROCD" ]; then
    mkdir -p "$LSPOOL/$PROCD"
fi
if [ ! -d "$LSPOOL/$DAYSCH" ]; then
    mkdir -p "$LSPOOL/$DAYSCH"
fi
if [ ! -d "$LSPOOL/$DUMPD" ]; then
    mkdir -p "$LSPOOL/$DUMPD"
fi

# Build exclude file of anything in PROCD and DONE.  
if [ -f "/tmp/excludes.$$" ]; then
    /usr/bin/rm /tmp/excludes.$$
fi
if [ -f "$LOGDIR/log.$$" ]; then
    /usr/bin/rm $LOGDIR/log.$$
fi

# Get file list in this hosts queue dir
$SSH -f $MASTER ls $MSPOOL/$QUEUE/ > /tmp/include0.$$ 2> /dev/null 

# Build an exlude list from files in PROCD
# NOTE: This files command gets directories explicitly. If we use .tgz later we must adjust
#FILES=`ls -d $LSPOOL/$PROCD/200* 2> /dev/null`
#for F in $FILES; do
#    echo "/$F/d" >> /tmp/excludes.$$    
#done
find $LSPOOL/$PROCD -name "20*.*" -type directory > /tmp/procfiles.$$
$SED -e 's/.*\/20/\/20/' -e 's/$/\/d/' /tmp/procfiles.$$ > /tmp/excludes.$$


# Add the DONE files to exclude list
# ETG: Removed mtime as we need to look at ALL local done files at this point
# 20041227: trying to put mtime back because exclude list not working
#find $LSPOOL/$DONE -mtime +7 -name "2*.tgz" > /tmp/donefiles.$$
find $LSPOOL/$DONE -name "2*.tgz" > /tmp/donefiles.$$
# add directories as well as tgz
find $LSPOOL/$DONE -name "20*.*" -type directory >> /tmp/donefiles.$$
$SED -e 's/.*\/20/\/20/' -e 's/$/\/d/' -e 's/\.tgz//' /tmp/donefiles.$$ >> /tmp/excludes.$$
echo "$RUNTIME $HOST: `cat /tmp/excludes.$$ | wc -l` files in processing and done" >> $LOGDIR/log.$$

# Remove exlude list from the QUEUE files and only move 
# a small number so we don't blow space.
INCLUDE=""
if [ -s /tmp/excludes.$$ ]; then
    $SED -f /tmp/excludes.$$ /tmp/include0.$$ | $HEAD -$MAXMOVE > /tmp/include00.$$

     INCLUDE="--include-from /tmp/include00.$$"
elif [ -s /tmp/include0.$$ ]; then

     $HEAD -$MAXMOVE /tmp/include0.$$ > /tmp/include00.$$
     INCLUDE="--include-from /tmp/include00.$$"
fi

# Check local space before grabbing more stuff
SPACE=`df -k $LSPOOL/$READY | $SED 's/  */ /g' | cut -f4 -d' ' | egrep '[^a-z]'`

if [ $SPACE -gt $MINSPACE ]; then
    # Move stuff in this hosts remote work QUEUE to local READY
    $ECHO $SSYNC -ar $INCLUDE --exclude "2*" $MASTER:$MSPOOL/$QUEUE/ $LSPOOL/$READY 2> /dev/null > /tmp/qmoved.$$
else
    echo "$RUNTIME $HOST: , Only $SPACE kB remaining (thresh=$MINSPACE kb)." >> $LOGDIR/log.$$
fi

COUNT="	     0"
if [ -s /tmp/qmoved.$$ ]; then
    COUNT="`cat /tmp/qmoved.$$ | wc -l`"
fi
echo "$RUNTIME $HOST: $COUNT files moved into ready" >> $LOGDIR/log.$$

echo "$RUNTIME $HOST: `ls -d $LSPOOL/$READY/2* 2> /dev/null | wc -l` timestamps to be processed" >> $LOGDIR/log.$$

# Create a timestamp 1hour ago. Useful for the 'find's  below 
# to make sure we don't grab too recent files which maybe active
 
TIME_A=`/bin/date '+%H'`
TIME_A=`expr $TIME_A - 1`
if [ $TIME_A -lt 10 ]; then
    TIME_A=0$TIME_A
fi
if [ $TIME_A -lt 0 ]; then
    TIME_A=23
fi

# Look for stale/stuck PROCD files. Simple remove older then 1 day
# These will get re-copied from its queue again
# Relies on mtime , so we 'refresh' the ready dir entries
find $LSPOOL/$READY -type d -exec /bin/touch -m {} \;
find $LSPOOL/$PROCD -type d -mtime +2 -exec /usr/bin/rm -rf {} \;

# MASTER host may also take part in processing in which case this
# stuff doesn't have to move.

TIME=`/bin/date +%m%d$TIME_A%M`

if [ "$HOST" != "$MASTER" ]; then
  
    /bin/touch -m $TIME $LSPOOL/timestamp.$$
    # Move any done stuff across
    # The queue handling depends on the "done" dir at both
    # the MASTER and processing host end.
    #find $LSPOOL/$DONE -type f ! -newer $LSPOOL/timestamp.$$ -name "2*.tgz" > /tmp/include1.$$ 2> /dev/null
    #if [ -s /tmp/include1.$$ ]; then
    #$ECHO $SSYNC -ar --include-from /tmp/include1.$$ --exclude "2\*" $LSPOOL/$DONE/ $MASTER:$MSPOOL/$DONE 2> /dev/null > /tmp/transferred.$$
    $SSYNC -ar $LSPOOL/$DONE $MASTER:$MSPOOL 2> /dev/null > /tmp/transferred.$$
    #fi
    if [ ! -f /tmp/transferred.$$ ]; then
	echo "      0" > /tmp/transferred.$$
    fi
    echo "$RUNTIME $HOST: `cat /tmp/transferred.$$ | wc -l` done files moved to $MASTER" >> $LOGDIR/log.$$


    # cleanup the done dir. Must be old 'done' file so MASTER has time
    # to remove the timestamp from the queue.
    # Otherwise we may end up processing it again.
    cd $LSPOOL/$DONE
    find . -type f -mtime +1 -name "2*.tgz" > /tmp/rmtargets.$$ 2> /dev/null
    for f in `cat /tmp/rmtargets.$$`; do 
	$SSH -xn $MASTER test -f $MSPOOL/$DONE/$f
	RESULT=$?
	if [ $RESULT -eq 0 ]; then	# File exists on master.
            /usr/bin/rm $f
	else
   	    echo "$RUNTIME $HOST File $f does not exist on $MASTER. Sync problem.  rmtargets"
	fi
    done
    cd $LSPOOL
    if [ -s /tmp/rmtargets.$$ ]; then
        echo "$RUNTIME $HOST: `cat /tmp/rmtargets.$$ | wc -l` done files removed" >> $LOGDIR/log.$$
    fi

    # Move across any DAYSCH and DUMPD 
    find $LSPOOL/$DAYSCH -type f ! -newer $LSPOOL/timestamp.$$  > /tmp/include2.$$
    if [ -s /tmp/include2.$$ ]; then
        $ECHO $SSYNC -arq  --include-from /tmp/include2.$$ --exclude "2\*" $LSPOOL/$DAYSCH/ $MASTER:$MSPOOL/$DAYSCH 2> /dev/null
    fi

    for f in `cat /tmp/include2.$$`; do
	/usr/bin/rm $f
    done

    echo "$RUNTIME $HOST: `cat /tmp/include2.$$ | wc -l` dayschanged files moved to $MASTER" >> $LOGDIR/log.$$

    # Move dump stuff across
    find $LSPOOL/$DUMPD -type f ! -newer $LSPOOL/timestamp.$$  > /tmp/include4.$$
    if [ -s /tmp/include4.$$ ]; then
        $ECHO $SSYNC -arq  --include-from /tmp/include4.$$ --exclude "2\*" $LSPOOL/$DUMPD/ $MASTER:$MSPOOL/$DUMPD 2> /dev/null
    fi

    # Cleanup dump stuff
    cd $LSPOOL/$DUMPD
    for f in `cat /tmp/include4.$$`; do 
        bf=`basename $f`
        $SSH -xn $MASTER test -f $MSPOOL/$DUMPD/$bf
        RESULT=$?
        if [ $RESULT -eq 0 ]; then      # File exists on master.
            /usr/bin/rm $f
        else
            echo "$RUNTIME $HOST File $f does not exist on $MASTER. Sync problem. include4"
        fi
    done

    cd $LSPOOL

#    echo "$RUNTIME $HOST: `cat /tmp/rmtargets2.$$ | wc -l` dump files moved to $MASTER" >> $LOGDIR/log.$$

    # Cleanups
    if [ -f "/tmp/excludes.$$" ]; then
        rm /tmp/excludes.$$
    fi
    if [ -f "/tmp/include0.$$" ]; then
        rm /tmp/include0.$$
    fi
    if [ -f "/tmp/include00.$$" ]; then
        rm /tmp/include00.$$
    fi
    if [ -f "/tmp/include1.$$" ]; then
        rm /tmp/include1.$$
    fi
    if [ -f "/tmp/include2.$$" ]; then
        rm /tmp/include2.$$
    fi
    if [ -f "/tmp/include3.$$" ]; then
        rm /tmp/include3.$$
    fi
    if [ -f "/tmp/include4.$$" ]; then
        rm /tmp/include4.$$
    fi
    if [ -f "/tmp/donefiles.$$" ]; then
	rm /tmp/donefiles.$$
    fi
    if [ -f "/tmp/rmtargets.$$" ]; then
	rm /tmp/rmtargets.$$
    fi
    if [ -f "/tmp/transferred.$$" ]; then
	rm /tmp/transferred.$$
    fi
    if [ -f "$LSPOOL/timestamp.$$" ]; then
	rm $LSPOOL/timestamp.$$
    fi
    if [ -f "/tmp/qmoved.$$" ]; then
	rm /tmp/qmoved.$$
    fi
    if [ -f "/tmp/procfiles.$$" ]; then
	rm /tmp/procfiles.$$
    fi
fi # Not master

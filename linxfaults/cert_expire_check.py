#!/opt/bta4/bin/python
#
# Take a date string from a keytool output of the valid date for a cert

import string, re, os, sys, time

test="Valid from: Thu Feb 07 09:35:59 EST 2013 until: Sun Feb 07 09:46:04 EST 2016"

line=sys.stdin.readline()

valid_re=re.compile("^.+until:\s*(?P<exdate>.*)\s*$")

result=valid_re.match(test)
nowtime=time.time()
if result:
    exdatestr=result.group("exdate")
    exdate=time.strptime(exdatestr,"%a %b %d %H:%M:%S %Z %Y")
    expire_days = int((time.mktime(exdate) - nowtime)/86400)
    if expire_days<1444:
        print line


#!/opt/bta4/bin/python
#
# Merge the csv files into a single csv and mail it of
#
# Potentially do further mainpulation to csv also.
#

import string, os, glob, re
import time, sys
sys.path.append('/opt/bta4/lib')
import sendmail

SENDTO="callum.shaw@aapt.com.au,anthony.loguarro@aapt.com.au"
#SENDTO="harry.raaymakers@aapt.com.au,callum.shaw@aapt.com.au"
#SENDTO="harry.raaymakers@aapt.com.au"
SENDFROM="nm@connect.com.au"
SUBJECT="Linx Fault reports"
DIR="/var/log/linxfaults"

DEBUG=0

re_date=re.compile("^\d{2}/\d{2}/\d{4}\s+\d{2}:\d{2}:\d{2}$")

def debugline(line):

    i=0
    parts = string.split(line,',')
    for part in parts:
	print i,part
	i=i+1

#
# Take a date and change numeric month to 3 letter month
# 
def _to3month(field):

    MONTHS=['jan','feb','mar','apr','may','jun','jul','aug','sep','oct','nov','dec']
    try:
	if re_date.match(field):
	    #print "MATCH:"
            parts=string.split(field,"/")
            day=parts[0]
            year=parts[2]

            imonth=int(parts[1]) -1
	    month=MONTHS[imonth]
            #print	day+"/"+month+"/"+year + string.join(parts[3:]) 
            return(	day+"/"+month+"/"+year + string.join(parts[3:]) )
	else:
	    return(field)
    except:
	return(field)
     

#
# Filter date so excel  will treat it consistently as date type
#
def filter_date(line):

    if not line:
	return
    if DEBUG: debugline(line)
    parts=string.split(line,',')
    if len(parts)<25:
	return(line)

    #print parts, len(parts)
#    print "XXX:",parts[19:25]
#    print "20:",parts[20]
#    print "21:",parts[21]
#    print "21:",parts[21]
    
    newparts=[]
    for part in parts:
	newparts.append(_to3month(part))
	
    new_line = ','.join(newparts)
    #print "BBB:", len(parts), len(string.split(new_line))
    return(new_line)

files = glob.glob(DIR+'/[0-9]*.csv')

TODAY=time.strftime('%Y%m%d',time.localtime(time.time()))

have_header=False
lines=[]

for file in files:
    f=open(file,"r")
    line=1
    while line:
	line=f.readline()
  	if have_header==True:
	    if line[0:5] == "ORDER":
		continue
	    lines.append(line)
	else:
	    lines.append(line)
	    have_header = True
    f.close()	

REPORT=DIR+"/linxfault-%s.csv" % TODAY

if os.path.exists(REPORT):
    OLDREPORT=DIR+"/linxfault-%s-%s.csv" % (TODAY,os.getpid() )
    os.rename(REPORT, OLDREPORT)

data=""
fo=open(REPORT,"w")
for line in lines:
   if not line:
	continue
   if line[0:5]=="ORDER":	# Header
       new_line=line
   else:
       new_line= filter_date(line)
   data= data + new_line
   fo.write(new_line)
fo.close()

if len(data)>0:
    BODY="Attached is the daily Telstra linx open fault report"
    sendmail.SendAttachedMail(SENDTO, SENDFROM, SENDFROM, SUBJECT, data, REPORT)

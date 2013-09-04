#!/opt/bta4/bin/python
#
# Takes an rfc822 mail message as stdin, confirms its from
# an expected source and correct subject. It then stores
# the attachment.
#
# 

import sys, time, os, stat, string
sys.path.append("/opt/servicenet/natdial/lib")
sys.path.append("../lib")
import email, email.Parser
import base64
import Log, Config, AttachmentProcess

DEBUG=0

# Accept message if:
subject="MegaPoP CLI <-> CCA"

# Various msgid domains we accept
msgids=(".telstra.com.au", ".telstra.com","au.tcnz.net")

conf=Config.Config()
if DEBUG:
    conf.spool="./"

def msg_verify(msg):
    """Verify this message is valid and recognised"""

    status=0
    log.add(0,"  Subject: %s\n" % msg.get("Subject"))
    log.add(0,"  Date: %s\n" % msg.get("Date"))
    log.add(0,"  msgid: %s\n" % msg.get("Message-ID"))
    log.add(0,"  Reply: %s\n" % msg.get("Return-Path"))
    mid=msg.get("Message-ID")
    for m in msgids:
	if string.find(mid, m)>=0:
            if string.find(msg.get("Subject"), subject)>=0:
                log.add(0,"  Status is Accepted.\n")
	        status=1
		break

    return status

def file_verify(fname):
    stat=os.stat(fname)
    if stat[6]<100000: 
	log.add(0,"  Error: small attachment size < 100KB.")
	return 0
    f=open(fname)
    lines=f.readlines()
    if len(lines)<15000:
	log.add(0,"  Error: Attachment < 15000 lines.")
	return 0

    log.add(0,"  Attached file appears to be Ok\n")

    return 1

def handle_msg(lines,log):

    log.add(0, "Incoming message\n")

    #sys.stdout.write(lines)

    p=email.Parser.Parser()
    msg=p.parsestr(lines)
    parts=[]

    for w in msg.walk():
	if w.get("Subject"):
	    if not msg_verify(w):
		return None

	if w.get_filename():
	    payload=w.get_payload()
	    if type(payload)==type(""):
	        data=base64.decodestring(payload)
    
		fname="%s.%s" % (w.get_filename(),time.strftime("%Y%m%d-%H%M%S",time.localtime(time.time())))
	        if os.path.exists(fname):
		    i=0
		    newfname="%s.%d" % (fname,i)
		    while os.path.exists(newfname):		
			i=i+1
		        newfname="%s.%d" % (fname,i)
		    fname=newfname

	        f=open(os.path.join(conf.spool,fname),"w")
	        f.write(data)
	        f.close()
		os.chmod(os.path.join(conf.spool,fname), stat.S_IROTH | stat.S_IRGRP | stat.S_IRWXU)

    if file_verify(os.path.join(conf.spool,fname)):
	return os.path.join(conf.spool,fname)
    else:	
	return None	

if __name__=="__main__":

    line="x"
    linelist=[]
    lines=""
    count=0
    while line:
	line=sys.stdin.readline()
	if line:
	    count=count+1
	    linelist.append(line)
    lines=string.join(linelist,'')	# is fast this way	

    log=Log.Log(4)

    #fname="../var/DCCS03V.DAT.20050214-162218"
    fname=handle_msg(lines, log)
    if not fname: 
        log.add(0,"Error in parsing attachment.\n")
    else:
        log.add(0,"  Start processing attachment %s\n" % (fname))
        process=AttachmentProcess.AttachmentProcess(fname,"cliprefix")
        log.add(0,"  Processing returned: %s, (%d entries)\n" % (process.result,process.count))

    subject="Telstra CLI prefix process"
    if DEBUG: 
	print log
    else:
        log.SendMail(conf.SENDTO, conf.SENDFROM, subject)

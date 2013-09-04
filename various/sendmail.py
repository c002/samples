##
#
# Send a string in an email message or as an attachement
#
# 
# 

def SendMail(to, sendfrom, replyto, subject, body):
    """send an email message"""

    import os
    smpath='/usr/lib/sendmail'

    if not os.path.exists(smpath):
        raise IOError, 'file %s not found' % smpath

    p = os.popen('%s -oi -t'%smpath,'w')

    data = "Reply-to: %s\nFrom: %s\nTo: %s\nSubject: %s\n\n%s" % (replyto, sendfrom, to, subject, body)
 
    p.write(data)    
    p.close()

def SendAttachedMail(to, sendfrom, replyto, subject, attachement, body=None):
    """send a email message a mime attachement"""

    import MimeWriter, time, os

    smpath='/usr/lib/sendmail'

    if not os.path.exists(smpath):
        raise IOError, 'file %s not found' % smpath

    p = os.popen('%s -oi -t'%smpath,'w')

    data = "Reply-to: %s\nFrom: %s\nTo: %s\nSubject: %s\n" % (replyto, sendfrom, to, subject)
 
    boundary="__"+str(int(time.mktime(time.localtime(time.time()))))+"__"
    p.write(data)    

    m=MimeWriter.MimeWriter(p)
    p=m.startmultipartbody("text",boundary)
    m.nextpart()
    p.write("Content-Type: text/plain; charset=us-ascii\n\n")
    if not body:
        p.write("File Attached\n")
    else:
	p.write(body)
    m.nextpart()

    p.write("Content-Type: text/plain; charset=us-ascii\n\n")
    p.write(attachement)
    m.lastpart()
    p.close()

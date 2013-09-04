import time
class Log:
    """A Simple logging facility"""

    def __init__(self, level):
        self.level=level
	self.lines=0
        self.log=""
       
    def __str__(self):
	return(self.log)

    def __call__(self, level, line, ts=1):
	self.add(level, line, ts)

    def __len__(self):
        return(self.lines)
	
    def add(self, level, line, ts=1):
	if type(line)==type(""):
	    line=[line]
	if type(line)==type([]) or type(line)==type(()):
	    for l in line:
                if self.level>=level:
	            prefix=time.strftime('%d-%b-%Y %H:%M:%S',time.localtime(time.time()))+": "
		    if ts:
                        self.log=self.log+prefix+l
		    else:
                        self.log=self.log+l
                    self.lines=self.lines+1

    def SendMail(self, to, frm, subject):
        import sendmail
        if self.log:
            replyto=frm
            sendmail.SendMail(to, frm, replyto, subject, self.log) 

    def SendAttachedMail(self, to, frm, subject):
        import sendmail
        if self.log:
            replyto=frm
            sendmail.SendAttachedMail(to, frm, replyto, subject, self.log) 


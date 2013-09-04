#!/opt/python/bin/python
#
# Simple standalone RADIUS client implementation
# 
# 
# $Author: harryr $
#

import exceptions
class RadiusException(exceptions.Exception):
    def __init__(self, errno, msg):
	self.errno = errno
	self.errmsg = msg

class Attributes:
    "Routines for RADIUS attribute manip"

    def __init__(self, dictfile=None):
	"Load dictionary"

        import sys, string

	self.adebug=0
        self.attributes=''
        self.attrlen=0			# Attributes total length
        self.dictionary={}
	self.password=""		# xorred md5 pwd

	if not dictfile:
	    import os
	    try:
	        dictfile=os.environ["RAD_DICTIONARY"]
	    except:
		dictfile="dictionary"

	try:
	    dict=open(dictfile,"r")
	    lines=dict.readlines()
	except:
	    if self.debug>1:
	        sys.stderr.write("WARNING: Failed to open dictionary file: %s\n" % dictfile)
	    lines=\
["ATTRIBUTE       User-Name               1       string",
 "ATTRIBUTE       User-Password           2       string",
 "ATTRIBUTE       CHAP-Password           3       string",
 "ATTRIBUTE       NAS-IP-Address          4       ipaddr",
 "ATTRIBUTE       NAS-Port                5       integer",
 "ATTRIBUTE       Service-Type            6       integer",
 "ATTRIBUTE       Framed-Protocol         7       integer",
 "ATTRIBUTE       Framed-IP-Address       8       ipaddr",
 "ATTRIBUTE       Framed-IP-Netmask       9       ipaddr"]

	for line in lines:
	    line=string.strip(line)	
	    if not len(line) or line[0]=="#":
		continue
	    items=map(string.strip, string.split(line))
	    if items[0]=="ATTRIBUTE":
	        self.dictionary[items[2]]={}
 	        self.dictionary[items[2]]['type']=items[3]
 	        self.dictionary[items[2]]['name']=items[1]

    def set_attdebug(self, debug):
	self.adebug=debug

    def AddAttr(self, attribute, value):
	"Add an attribute to the attribute list"

        import sys, string, struct

# Types , string 0-253 Octets, 
#	  address 32bit msb, 
#	  integer 32bit msb.
#	  time 32bit epoch

	attr = str(attribute)

	if not self.dictionary.has_key(attr):
	    sys.stderr.write("Error: Unknown attribute: %d\n" % attribute)
	    sys.exit(1)

	if self.dictionary[attr]['type']=="string":
	    fmt="BB%ds"%len(value)
	    length=len(value)+2
	elif self.dictionary[attr]['type']=="integer":
	    fmt="BB!L"
	    length=6
	elif self.dictionary[attr]['type']=="ipaddr":
	    fmt="BB!L"
	    length=6
	elif self.dictionary[attr]['type']=="time":
	    fmt="BB!L"
	    length=6
	else:
	    sys.stderr.write("Error: Unknown RADIUS type: %s\n" % self.dictionary[attr]['type'])
	    sys.exit(-1)

	self.attributes=self.attributes + struct.pack(fmt, int(attribute),int(length), str(value))
	self.attrlen=self.attrlen+length

    def AttributeLen(self):
	""	
    def GetVal(self, attribute):
	"Get attribute value"

    def Search(self, attribute):
	""

class RadiusClient(Attributes):
    """An implementation of a radius client"""

    def __init__(self, host="localhost", port=1645, secret="", debug=0):

	self.debug=debug	# 0 = No debug
				# 1 = Extra messages
				# 3 = Full packet dumps	
	self.secret=secret
        self.headlen=20
	self.header=""
	self.packlen=0
	self.calledfinal=0
	self.id=0
	self.sock=None
	self.code=0
	self.length=0
	self.attr=None
	self.packet=None
	self.vector=""
	self.md5auth=""
	self.rcvpacket=""

	self.timeout=5
	self.host=host
	self.port=port

	self.set_debug(debug)

# Load dictionary
	Attributes.__init__(self)

#	self.RequestAuth()

 	from socket import *

	self.sock = socket(AF_INET, SOCK_DGRAM)

    def set_debug(self,debug):
	self.debug=debug	
	self.set_attdebug(debug)
	
    def RequestAuth(self,vector=""):
	"""Make a random 16byte Authenticator and create the 
	   MD5 xor string to apply to password
	"""

	from whrandom import randint
	import struct, md5

	v=""
        args=[]
	args.append("%ds16s" % len(self.secret) )   
	args.append(self.secret)   

	if not vector:
	    for i in range(0,16):
	        r=chr(randint(0,255))
		vector=vector+r
	    self.vector=vector

	args.append(vector)
	    	
	s=apply(struct.pack, tuple(args))

	self.md5auth= md5.new(s).digest()	

    def PasswordHash(self, password=None):
	"Hash the password"
	
	import struct
	from operator import xor
	from string import atoi

	if not password:
	    return

	plen=len(password)

	seg, part = divmod(plen, 16)
	rem = 16 - part

	for j in range(0,seg+1):
	    if j==seg:
	        pwargs=["%ds%dB" % (part, rem)]
        	pwargs.append(password[j*16:(j+1)*16])
		for i in range(0,rem):
	    	    pwargs.append(0)
	    else:
        	pwargs=["16s"]
        	pwargs.append(password[j*16:(j+1)*16])
 
	    pwd=apply(struct.pack,tuple(pwargs)) 

	    if j and self.password:
	        self.RequestAuth(self.password[(j-1)*16:j*16])
	    else:
	        self.RequestAuth()

	    for i in range(0,len(self.md5auth)):
  	        x=chr(xor(ord(self.md5auth[i]), ord(pwd[i])))
	        self.password=self.password+x

	return(self.password)

    def AddPasswordAttr(self, password):
	"Password attribute special treatment"

        hash=self.PasswordHash(password=password)
        self.AddAttr(2,hash)

    def alarm_handler(self, signum, frame):
	"Handle timeout alarm on socket"
	raise IOError, "No reponse"
#	raise RadiusException(1, "No reponse")
	
    def send(self):
	"Send the packet to the server"

	if not self.calledfinal:
	    self.Final()

# Send the request packet

	if self.debug>2:
	    self.dump(self.packet)
	
	if self.debug>0:
	    print "Sending Packet to: %s port %d" % (self.host, self.port)

	n=self.sock.sendto(self.packet, (self.host,self.port))	

	if self.debug>0:
	    print "%d bytes send" % n

    def verify(self):
	"verify the receipt packet"

	import struct, md5

	fmt="cch16s"
	head=struct.unpack(fmt, self.rcvpacket[:20])
	attributes=self.rcvpacket[20:]

	chkstr=self.rcvpacket[0:4] + self.vector+self.rcvpacket[20:]+self.secret
	md5chk= md5.new(chkstr).digest()	

	if self.debug >1:
	    print "Reply Auth calc=",self.makestring(md5chk)
	    print "Reply Auth pkt =",self.makestring(self.rcvpacket[4:20])

	if ord(head[0])==2 and md5chk == self.rcvpacket[4:20]:
	    return 2
	elif ord(head[0])==3 and md5chk == self.rcvpacket[4:20]:
	    return 3
	else:
	    return 0
 
    def Final(self):
	"Calculate MD5 check sums, and pack it ready for sending."
	
	import struct
	from whrandom import randint

	self.packlen=self.headlen + self.attrlen

	self.code=1
	self.id=randint(0,255)

	# Do attributes

	fmt="BBH16s%ds" % self.attrlen
	if self.debug>0:
	    print "Constructing Packet"
	args=[]
	args.append(fmt)
	args.append(int(self.code))
	args.append(int(self.id))
	args.append(int(self.packlen))
	args.append(self.vector)
	args.append(self.attributes)
	self.packet=apply(struct.pack, tuple(args))

	self.calledfinal=1

    def makestring(self,string):
	"write string in nice hex if non-writable string"
 
	import sys
 
	i=0
	mystr=""
	nx=filter(lambda x : ord(x)<30 or ord(x)>127, string)
	if not nx:
	    mystr=string
	else:
	    for c in string:
		if ord(c)<16:
	            mystr=mystr+("0%x" % ord(c))
		else:
	            mystr=mystr+("%x" % ord(c))
	        if i % 2:
	            mystr=mystr+" "
	        i=i+1

	return mystr

    def receive(self):
	"Receive a raply packet"

	import signal, struct
	import socket

	if self.debug>0:
	    print "Timeout is %d seconds" % self.timeout
	    print "Waiting for response ..."

	signal.signal(signal.SIGALRM, self.alarm_handler)
	signal.alarm(self.timeout)
	(self.rcvpacket,address)=self.sock.recvfrom(1024)
	signal.alarm(0)

	if self.debug>2:
	    self.dump(self.rcvpacket)

    def authenticate(self,user,passwd):
	"""Authenticate a username and password"""

	self.AddAttr(1,user)
	self.AddPasswordAttr(password=passwd)
	self.calledfinal=0

	self.send()		# Send the packet
	self.receive()		# Wait for reply
	return self.verify()	# Validate packet

    def dump(self, packet):
	"Debug, dump content of attributes"

	import struct, sys

	print "-"*60
	self.dump_header(packet)
	self.dump_attributes(packet)
	print "-"*60

    def dump_header(self, packet):
	"Debug, dump content of header"

	import struct, sys

	fmt="!2BH16s"
	fields=struct.unpack(fmt,packet[0:20])

	sys.stdout.write("code=[%d], " % fields[0])
	sys.stdout.write("id=[%d], " % fields[1])
	sys.stdout.write("length=[%d],\n" % fields[2])
	sys.stdout.write("Authenticator=[%s]" %  self.makestring(fields[3]))
    def get_attributes(self, packet):

	import struct, sys

	attrlst=[]
	attributes=packet[20:]
 	length=len(attributes)
	attr=attributes
	while length>0:
	    t=()
	    l=ord(attr[1]) 
	    fmt="BB%ds" % (l-2)
	    (tipe,attrlen,value)=struct.unpack(fmt, attr[:l])
	    strattr=str(tipe)

	    if not self.dictionary.has_key(strattr):
		self.dictionary[strattr]={}
		self.dictionary[strattr]['type']="string"
		self.dictionary[strattr]['name']="Attribute %s" % strattr
		 
	    t=self.dictionary[strattr]['type']	
	    length=length - attrlen 
	    if t=="string":
		t=(self.dictionary[strattr]['name'], self.makestring(value))
		        
	    if t=="integer":
		value=struct.unpack("!L", value)
		t=(self.dictionary[strattr]['name'], value[0])
	    if t=="ipaddr":
		value=struct.unpack("!BBBB", value)
		t=(self.dictionary[strattr]['name'], "%d.%d.%d.%d" % (value[0],value[1],value[2],value[3]))
	    if t=="time":
		value=struct.unpack("!L", value)
		t=(self.dictionary[strattr]['name'], value[0])

	    attr=attr[l:]
	    attrlst.append(t)
	return(attrlst)

    def dump_attributes(self, packet):

	attributes = self.get_attributes(packet)

	print
	for attr, value in attributes:
	    if type(value)==type(1) or type(value)==type(1L):
	        print "%s = %d" % (attr, value)
	    else:
	        print "%s = %s" % (attr, value)

#####
# Main
#

if __name__=="__main__":

    import sys

    rp=RadiusClient(host="flacco.nu",port=1645, secret="minty",debug=4)
    rp.timeout=3 

    if len(sys.argv) ==3:
        result = rp.authenticate(sys.argv[1], sys.argv[2])
    else:	
        result = rp.authenticate('harrytest@flacco.nu','fred')

    print "Dumping attributes"
#    attr = rp.get_attributes(rp.rcvpacket)
    rp.dump_attributes(rp.rcvpacket)
   
    if result==2:
	print "Authenticated"
    elif result==3:
	print "Request Denied"
    else:
	print "Failed Authenticator"

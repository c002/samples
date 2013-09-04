#!/opt/bta4/bin/python
#
# cisco_view 
#	Parse Cisco config files and pull out intreresting
#	lines. Build a bunch of interface and route objects
#	and tie as much together as we can.
#

import string, re, math, exceptions, glob, socket

from lib import IPTools
from lib import Log
from attrs import Attributes
from constants import Const

DEBUG=1
STATE=["interface"]

True=1
False=0

######
# Interesting router config lines
# dialer map ip 210.10.84.147 name crcpoly1-gw broadcast 

# global config options
pats= { 'interface' : "^interface (?P<interface>\S+)($|\s\S+$)",
         'route' : "^ip route\s+(?P<route>\d+\.\d+\.\d+\.\d+)\s+(?P<mask>\d+\.\d+\.\d+\.\d+)\s+(?P<dest>\S+)$",
	 'hostname' : "^hostname\s+(?P<hostname>.+)$",
	 'flowsrc' : "^ip flow-export source\s*(?P<ifsrc>.+)$",
	 'flowver' : "^ip flow-export version\s*(?P<flowver>\d+)\s*peer-as$",
	 'flowdst' : "^ip flow-export destination\s*(?P<flowdst>\d+\.\d+\.\d+\.\d+)\s*(?P<flowport>\d+)$",
	 'flowsam' : "^ip flow-sampling-mode packet-interval\s+(?P<flowfreq>\d+)$",
	 'chapuser': "^username\s+(?P<user>.+)\s+password\s+(?P<ptype>\d+)\s+(?P<passwd>.+)$"
         #'route' : "^ip route.*$"

      }
# hostname bri-cor1

# Interface config options
#'ip' : "^\s*ip address\s+(?P<ip>\d+\.\d+\.\d+\.\d+)\s+(?P<mask>\d+\.\d+\.\d+\.\d+)($|\s*(?P<sec>secondary)$)",
intpats = {'ip' : "^\s*ip address\s+(?P<ip>\d+\.\d+\.\d+\.\d+)\s+(?P<mask>\d+\.\d+\.\d+\.\d+).*$",
	   'description' : "^\s*description\s+(?P<description>.*$)",
	   'state' : "^(\s*|\s*(?P<no>no\s+))shutdown$",
	   'ratein' : "^\s*rate-limit input\s+(?P<rate>\d+)\s+(?P<burstnorm>\d+)\s(?P<burstmax>\d+).*$",
	   'rateout' : "^\s*rate-limit output\s+(?P<rate>\d+)\s+(?P<burstnorm>\d+)\s(?P<burstmax>\d+).*$",
	   'dialer' : "^\s*dialer map ip (?P<gw>\d+\.\d+\.\d+\.\d+) name (?P<chap>\S+).*$",
	   'cache'  : "\s*(?P<cache>ip route-cache flow)($|\s*(?P<sampled>sampled)$)"
 	  }

# Get the BGP neighbours
re_bgp = re.compile("router bgp 2764")

neighbor="^\s*neighbor\s+(?P<ip>\d+\.\d+\.\d+\.\d+)\s+route-map\s+(?P<routemap>\S+)\s+in\s*$"
re_neighbor=re.compile(neighbor)

# Get the routemap definiation
routemap="^\s*route-map\s+(?P<acl>\d+)\s*$" 

# Get the AccessList number
acl="^\s*match ip address\s+(?P<acl>\d+)\s*$" 
re_acl=re.compile(acl)

# acl1 = 
# access-list 150 permit ip 203.83.208.0 0.0.3.0 host 255.255.255.0
# access-list 150 permit ip host 210.9.64.128 host 255.255.255.192
# acl2 = 
# access-list 136 permit ip 203.30.206.0 0.0.1.0 255.255.254.0 0.0.1.0

aclpats = { 'acl1' : "^\s*access-list\s+(?P<acl>\d+)\s+permit ip(\s+host\s+|\s+)(?P<src>\d+\.\d+\.\d+\.\d+)(\s+host\s+|\s+)(?P<mask>\d+\.\d+\.\d+\.\d+).*$",
	    'acl2' : "^\s*access-list\s+(?P<acl>\d+)\s+permit ip\s+(?P<src>\d+\.\d+\.\d+\.\d+)\s+(?P<cmask1>\d+\.\d+\.\d+\.\d+)\s+(?P<dst>\d+\.\d+\.\d+\.\d+)\s+(?P<cmask2>\d+\.\d+\.\d+\.\d+).*$",
	  }

#re_accesslist = re.compile(accesslist)

# test
#minilist = "^\s*access-list\s+(?P<acl>\d+)\s+.*$"
minilist = "^\s*access-list\s+(?P<acl>\d+)\s+permit ip(\s+host\s+|\s+)(?P<src>\S+)\s+(?P<mask>\S+).*$"
re_minilist = re.compile(minilist)

# Can pull stuff from interface descriptions
descpats= {"one" : "^.*\s+(?P<sid>A0\d+[a-zA-Z]\d+).*$",
	   "two" : "",
	   "three" : ""
	  }

cpats={}
for pat in pats.keys():
    cpats[pat]=re.compile(pats[pat])
cintpats={}
for pat in intpats.keys():
    cintpats[pat]=re.compile(intpats[pat])
cdescpats={}
for pat in descpats.keys():
    cdescpats[pat]=re.compile(descpats[pat])
caclpats={}
for pat in aclpats.keys():
    caclpats[pat]=re.compile(aclpats[pat])

class Router:
    """Information about a router"""
    def __init__(self):
	self.rtrname=""		# Source interface for netflow
	self.rtriosver=""		# ios version
	self.rtrflowsrc=""		# Source interface for netflow
	self.rtrflowver=0		# Netflow version
	self.rtrflowdst=""		# Netflow destination ip
	self.rtrflowport=0		# Netflow destination port
	self.rtrflowfreq=0		# Netflow sample interface (packets)
	self.rtrchaps=[]		# List of ChapUser objects

    def __str__(self):
	s=""
	if self.rtrname: s = s + "rtr=%s," % self.rtrname
	if self.rtrflowsrc: s = s + "flowsrc=%s," % self.rtrflowsrc
	if self.rtrflowver: s = s + "flowver=%d," % self.rtrflowver
	if self.rtrflowdst: s = s + "flowdst=%s," % self.rtrflowdst
	if self.rtrflowport: s = s +"flowport=%d," % self.rtrflowport
	if self.rtrflowfreq: s = s + "sampled=%s" % self.rtrflowfreq
	for i in range(len(self.rtrchaps)):
	    if i==0:
		s=s+"\n"
	    s = s + "\t%s\n" % str(self.rtrchaps[i-1])
	s=s+"\n"
	return(s)	 

class Interface:
    """Everything we want to know about a Cisco router interface"""
    def __init__(self, interface):

	# TODO : On a 252 mask, need to store gateway address of both ends
	#	 DB will have customer end, config will have our end.

	self.crouter=""
	self.cstate="up"
	self.ccache="off"	# Netflow route-cache. is "on","off" or "sampled"
	self.cinterface=interface
	self.cdescription=""
	self.cip=[]		# Interface IP. Can be multiples
	self.cmask=[]
	self.cip_cidr=[]
	self.croutes=[]		# list of route objects resolved via self.gw or self.chap
	self.cratein=0
	self.crateout=0
	self.cgw=[]	# multiple dialer maps under single interface to tie chap and gateway.
	self.cchap=[]	# chapusers for this link
	self.csid=None
	self.cisindb=0	# Set if interface was found in db

	self.attributes=[]
	self.crelatives=[]	# related objects
	self.coptions=[]

    def setcinterface(self,interface):
 	self.cinterface=interface
    def setcrouter(self,router):
	try:
 		self.crouter=socket.gethostbyaddr(router)[0]
	except:
 		self.crouter=router
	# Normalise into loc-rtr format
	parts=string.split(self.crouter,'.')
	if len(parts)>1:
	   self.crouter=parts[1]+"-"+parts[0]

    def setcip(self,ip):
 	self.cip.append(ip)
    def setcmask(self,mask):
	self.cmask.append(mask)
	self.setccidr(mask)
    def setcoptions(self,options):
	self.coptions=options
    def setcdescription(self,description):
	self.cdescription=description
    def setcstate(self,state):
	self.cstate=state
    def setccache(self,cache):
	self.ccache=cache
    def setcratein(self,rate):
	self.cratein=long(rate)
    def setcrateout(self,rate):
	self.crateout=long(rate)
    def setcchap(self,chap):
	self.cchap.append(chap)
    def setcgw(self,gw):
	self.cgw.append(gw)
    def setcsid(self,sid):
	self.csid=sid
    def setccidr(self, mask):
	self.cip_cidr.append(IPTools.mask2cidr(mask))

    def final(self):
	"""Last method called which populates derived fields which it is
	   assumed are now all available.
	"""
 	
	# Set rateout based on interface type if pos and not set
	if not self.crateout:
	    if self.cinterface[:8]=="Ethernet":
		self.crateout=10000000
		self.attributes.append(Const.DEFAULT_OUTRATE)
	    if self.cinterface[:12]=="FastEthernet":
		self.crateout=100000000
		self.attributes.append(Const.DEFAULT_OUTRATE)
	    if self.cinterface[:15]=="GigabitEthernet":
		self.crateout=1000000000
		self.attributes.append(Const.DEFAULT_OUTRATE)
	    if self.cinterface[:3]=="ATM":
		self.crateout=155000000
		self.attributes.append(Const.DEFAULT_OUTRATE)

	if not self.cratein:
	    if self.cinterface[:8]=="Ethernet":
		self.cratein=10000000
		self.attributes.append(Const.DEFAULT_INRATE)
	    if self.cinterface[:12]=="FastEthernet":
		self.cratein=100000000
		self.attributes.append(Const.DEFAULT_INRATE)
	    if self.cinterface[:15]=="GigabitEthernet":
		self.cratein=1000000000
		self.attributes.append(Const.DEFAULT_INRATE)
	    if self.cinterface[:3]=="ATM":
		self.cratein=155000000
		self.attributes.append(Const.DEFAULT_INRATE)

	
    def __str__(self):
	
	s = "Rtr=%s, interface=%s, state=%s" % (self.crouter, self.cinterface, self.cstate)
	#if self.cratein: s = s + ", ratein=%d" % self.cratein
	#if self.crateout: s = s + ", rateout=%d" % self.crateout

	if "routes" in self.coptions and self.cip:
	    s=s +"\n\t, ip(s)="
	    for i in range(0,len(self.cip)):
		#s=s+", %s/%s" % (self.cip[i], self.cmask[i])
		s=s+", %s/%s" % (self.cip[i], self.cip_cidr[i])

#	if self.ip_cidr:
#	    s=s +", mask="
#	    for i in range(0,len(self.ip_cidr)):
#		s=s+", /%d" % (self.ip_cidr[i])

	if "details" in self.coptions:
	    if self.cdescription: s = s +"\n    desc=%s" % self.cdescription
	    s=s+"\n    "
	    if self.cratein: s = s + ", ratein=%d" % self.cratein
	    if self.crateout: s = s + ", rateout=%d" % self.crateout
	    if self.cgw: s = s + ", gw=%s" % self.cgw
	    if self.cchap: s = s + ", chap=%s" % self.cchap
	    if self.csid: s = s + ", sid=%s" % self.csid
	    if self.ccache in ("on","sampled","off"): s = s +", flowcache=%s" % self.ccache
	    #s = s + "\n"

	if "routes" in self.coptions:
	    s=s+"\n"
	    for r in self.croutes:
	        s=s+ "\t" + str(r) + "\n"
	s=s+"\n"

	return(s)

    def addroute(self, object):
	"""Check to see if this routeobj belongs with this interface
	   It can then be tied to this Interface object
	"""
	if not isinstance(object, Route):
	    return

	isdn=0

	if self.crouter != object.crouter:
	    return

	# Destination is interface or gateway.
	# Check ISDN type 
	# DEBUG
	#print "obj.cdest=",object.cdest, "gw=",self.cgw, "int=",self.cinterface
	
	for gw in self.cgw:
	    isdn=1
	    if object.cdest==gw:
		if self.cstate=="shutdown":
	    	    object.attributes.append(Const.INTERFACE_SHUTDOWN)
	        self.croutes.append(object)

	    #print "%s %s DOWN" % (self.crouter, self.cinterface)
#	else:
#	    print "%s %s UP" % (self.crouter, self.cinterface)

	# Check other permcons links if a gateway on a route is part of ip block
	# on an interface.

	#print "Debug addroute(): obj.cdest [%s] <-> self.cinterface [%s]" % (object.cdest,self.cinterface)

	# For a destination IP
	if not isdn and IPTools.checkip(object.cdest):
	    if type(self.cip)==(""):
		self.cip=[self.cip]
	    if type(self.cip_cidr)==(""):
		self.cip_cidr=[self.cip_cidr]
	    i=0
	    for ip in self.cip:
		 ipcidr = ip+'/'+ str(self.cip_cidr[i])
		 net = IPTools.findnet(ipcidr)

	    	 bits =  IPTools.mask2int(self.cip_cidr)
	    	 lowip = IPTools.ip2int(net)
	    	 highip= IPTools.ip2int(net) + bits	
	
	         testip = IPTools.ip2int(object.cdest)	

	         if testip>=lowip and testip<=highip:
	            self.croutes.append(object)
		    if self.cstate=="shutdown":
	    	        object.attributes.append(Const.INTERFACE_SHUTDOWN)
		 i=i+1

	# For a route with an interface as destination 
	elif object.cdest==self.cinterface:
	        #print "Debug addroute(): obj.cdest [%s] <-> self.cinterface [%s]" % (object.cdest,self.cinterface)
		if self.cstate=="shutdown":
	    	    object.attributes.append(Const.INTERFACE_SHUTDOWN)

		for i in range(0, len(self.cip)):
		    ipcidr=self.cip[i] + "/" + str(self.cip_cidr[i])

		    net = IPTools.findnet(ipcidr) + "/" + str(self.cip_cidr[i])

		    object.cdest_net.append(net)
		    ip=None
	    	    if self.cmask[i]=="255.255.255.252":
			parts=string.split(self.cip[i],'.')
			if len(parts)==4:
			    if ( (int(parts[3])-1) % 4) ==0:
		    	        byte4=int(parts[3]) + 1
			    else:		    
		    	        byte4=int(parts[3]) - 1
	            	    ipcidr=("%s.%s.%s.%s" % (parts[0], parts[1], parts[2], byte4))

		            #object.cdest_ip.append(ipcidr)
		            ip=ipcidr

		    object.cdest_ip.append(ip)	

	            #print " ** ** addroute() net=", object.cdest_net[-1], "ip=",object.cdest_ip[-1]
	        self.croutes.append(object)
	return

    def __add__(self,object):
	"""Tie a route to an interface"""

	if not isinstance(object, Route):
	    return
	
	self.croutes.append(object.croute)


class Route:
    """Container for info about a ip route"""
    def __init__(self):

	self.crouter=None
	self.croute=None
	self.cmask=None
	self.cslashmask=None
	self.cip_cidr=None	# destination route as ip/mask
	self.cdest=None		# ip or interface route destination
	self.cdest_if=None	# interface for cdest
	self.cdest_ip=[]	# ip for cdest, derived from interface ip (multi) 
				# if /30 , use other end, othersise its None
	self.cdest_net=[]	# net for cdest, derived from interface ip (multi) 
				# used to compare if subnet destination matches
	self.cconfirmed=0

	self.crelatives=[]	# related objects
	self.attributes=[]	# Stuff we learned about this route
	self.options=[]

    def setcrouter(self,router):
 	self.crouter=router
	# Normalise into loc-rtr format
	parts=string.split(self.crouter,'.')
	if len(parts)>1:
	   self.crouter=parts[1]+"-"+parts[0]

    def setcip_cidr(self, route):
	self.cip_cidr=route

    def setcdest(self,dest):
 	self.cdest=dest
	# break into interface and ip
	if IPTools.checkip(dest):
	    self.cdest_ip.append(dest)		# an ip
	else:
	    self.cdest_if=dest		# should be interface

    def setcmask(self,mask):
	self.cmask=mask
	self.setcslashmask(mask)
	if self.cip_cidr:
	    self.cip_cidr=str(self.cip_cidr)+'/'+str(self.cslashmask)

    def setrevmask(self, revmask):
	# flip a cisco reverse mask to a standard mask. XXX.
	rmask = IPTools.ip2int(revmask)
	mask = 0xFFFFFFFFL - rmask
	self.cmask=IPTools.ip2str(mask)

	self.setcslashmask(revmask)

    def setcslashmask(self, mask):
	self.cslashmask=IPTools.mask2cidr(mask)

    def setmask(self, mask):
	self.setcslashmask(mask)	

    def __str__(self):
	s=""
	if self.crouter: s=s+"Router=%s " % self.crouter
	if self.cslashmask and self.cip_cidr: s=s+"Route=%s" % (self.cip_cidr)
	if self.cdest: s=s+" Gateway=%s " % self.cdest
	for ip in self.cdest_ip:
	    s=s+" remotegw=%s," % ip

	#s=s+"\n"

	return(s);

class ChapUser:
    def __init__(self, chapuser=None, passwd=None, ptype=0):
	self.chapuser=chapuser
	self.passwd=passwd
	self.ptype=int(ptype)
	self.itied=False

    def setitied(self, tied=False):
	self.itied=tied

    def __str__(self):
	s="chap=%s, passwd=%s, ptype=%s,Tied=%s" % (self.chapuser, self.passwd, self.ptype, self.itied)
	return s

class RouteMap:
    """Holds info about BGP route maps"""
    def __init__(self):
	self.gatewayip=None	# Customer end gw ip
	self.routemapName=None	# Name of import route map
	self.accesslistNumbers=[]# Number of accesslist
	self.routes=[]		# list of CIDR IPs in route map

    def __str__(self):
	s=""
	if self.gatewayip:
	    s="Neighbor=" + self.gatewayip 
	if self.routemapName:
	    s=s+ ", Map=" + self.routemapName 
	if self.accesslistNumbers:
	    acls=""
	    for a in self.accesslistNumbers:
		acls=acls + a + ","
	    s=s + "," + acls 
	if self.routes:
	    s=s + ", routes=", self.routes

	return(s)
 
    def setGatewayIp(self, ip):
	self.gatewayip=ip
    def setRouteMapName(self, routemap):
	self.routemanpName=routemap
 
class RtrConfigParser:
    """This class parses a classic Cisco Config file and creates
	objects from the interesting bits. 
	In particular the interface sections and static routes
	are turned into  Interface and Route objects.
    """

    def __init__(self, configfile=None, dir=None, attrs=None):

	# Public
	# Global cicso config options that interesting.
	self.crouters=[]	# List of router objects
	self.cinterfaces=[]	# List of interface objects
	self.croutes=[]		# List of route objects
	self.croutemaps={}	# Hash of route map objects keyed on RouteMap Tag
	self.chashroutes={}	# hash of route objects keyed on cidr route
	self.caccesslist={}	# Access lists with a list of Route Objects

	self.options=attrs.options
	self.debug=attrs.debug

	if self.debug>0: print "DEBUG RtrConfigParser: Debug level is:" , self.debug

   	if not configfile and not dir:
	    return None

	if dir:
	    #print "dir=",dir
	    #configfiles = map(lambda x:string.split(x,'/')[-1],glob.glob(dir+'/*-confg'))
	    configfiles = map(lambda x:string.split(x,'/')[-1],glob.glob(dir+'/%s-*' % attrs.pop))
	else:
	    configfiles=[configfile]

	for configfile in configfiles:
	    self.crouters.append(Router())

            bits=string.split(configfile,'-')
	    router=None
            if len(bits)>=2: 
		router=bits[0]+'-'+bits[1]

	    ##############################################################
	    # Pass 1  - This attempts to resolve BGP routes by 
	    #	  	going through the routemaps and accesslists. 
	    if "routemaps" in self.options:
	      re_routemaps=[]
              fcf=open(dir+"/"+configfile,"r")
              line=1
	      astag=None
	      state="base"
              while line:
                line=fcf.readline()
	        line=string.strip(line)
	        if line and line[0]=='!':
	            continue

	        if state=="base":
		    result=re_neighbor.match(line)
		    if result and result.group("ip") and result.group("routemap"):
			rm = result.group("routemap")
			self.croutemaps[rm]=RouteMap()
			self.croutemaps[rm].neighbor=result.group("ip")
			self.croutemaps[rm].routemap=result.group("routemap")
			# Add routemap re to search for
			routemap="^\s*route-map\s+(?P<astag>" + result.group("routemap") + ")\s+permit\s+(?P<acl>\d+)\s*$" 
			#print "routemap = ", routemap
			re_routemaps.append( re.compile(routemap) )

		        #print "R", self.croutemaps[rm].neighbor
		        #print "  M", self.croutemaps[rm].routemap
			continue

		if state=="base":
		    for r in re_routemaps:
		        result=r.match(line)
			if result and result.group("astag"):
			    state="routemap"
			    astag=result.group("astag")
			    break

		if state=="routemap":
		    #print state,line

		    result=re_acl.match(line)
		    if result and result.group("acl"):
			#print "  FOUND", astag
			acl = result.group("acl")
			if acl not in  self.croutemaps[astag].accesslistNumbers:
		             self.croutemaps[astag].accesslistNumbers.append(acl)
			state="base"
			astag=None
		        #continue

		# Turn access lists entries into Route objects
		result=caclpats['acl1'].match(line)
		if result and result.group("acl") and result.group("src") and result.group("mask"):
			#print line
			acl = result.group("acl")
			src = result.group("src")
			
			#print "A ", result.group("acl")
			#print "  S ", result.group("src")
			#print "  M ", result.group("mask")

			if not self.caccesslist.has_key(acl):
			    self.caccesslist[acl]=[]

			self.caccesslist[acl].append(Route())
			self.caccesslist[acl][-1].setcrouter(router)
			
			self.caccesslist[acl][-1].setcip_cidr(result.group("src"))
			self.caccesslist[acl][-1].setrevmask(result.group("mask"))
			self.caccesslist[acl][-1].setcmask(result.group("mask"))

			# Cisco's reverse netmask

		result=caclpats['acl2'].match(line)
		if result and result.group("acl") and result.group("src") and result.group("cmask2"):
			if not self.caccesslist.has_key(acl):
			    self.caccesslist[acl]=[]
			
			self.caccesslist[acl].append(Route())
			self.caccesslist[acl][-1].setcrouter(router)
			
			self.caccesslist[acl][-1].setcip_cidr( result.group("src"))
			self.caccesslist[acl][-1].setmask(result.group("cmask2"))
			self.caccesslist[acl][-1].setcmask(result.group("cmask2"))

			    # Resolve the gateway aka neighbor later.
	        	    #if result.group("dest"):
			    #    self.croutes[-1].setcdest(result.group("dest"))

			# A 'Normal' netmask

	      fcf.close()

	    ##############################################################
	    # Pass 2
            f=open(dir+"/"+configfile,"r")

            state="base"

            #interfaces=[]	# Collection of Interface objects
            #routes=[]		# Collection of route objects
            line=1
            while line:
                line=f.readline()
	        line=string.strip(line)
	        if line and line[0]=='!':
	            state="base"
	            continue
	        #print state,":",line 
	        #print "STATE=",state 

		# New config patterns to parse should be added here	
	        if state=="base":
	            for pat in cpats.keys():
	                result=cpats[pat].match(line)

		        if result and pat=="hostname":
			    self.crouters[-1].rtrname=result.group("hostname")
			### ip flow-export source Loopback0
			if result and pat=="flowsrc" and result.group("ifsrc"):
			    self.crouters[-1].rtrflowsrc=result.group("ifsrc")
			### ip flow-export version 6 peer-as
			if result and pat=="flowver" and result.group("flowver"):
			    self.crouters[-1].rtrflowver=int(result.group("flowver"))
			### ip flow-export destination 203.8.183.97 9991
			if result and pat=="flowdst" and result.group("flowdst"):
			    self.crouters[-1].rtrflowdst=result.group("flowdst")
			if result and pat=="flowdst" and result.group("flowport"):
			    self.crouters[-1].rtrflowport=int(result.group("flowport"))
			### ip flow-sampling-mode packet-interval 10
			if result and pat=="flowsam" and result.group("flowfreq"):
			    self.crouters[-1].rtrflowfreq=int(result.group("flowfreq"))
			### username UUUU password 7 NNNNNN
			if result and pat=="chapuser" and result.group("user") and \
			    result.group("passwd"):
			    self.crouters[-1].rtrchaps.append(ChapUser())
			    self.crouters[-1].rtrchaps[-1].chapuser=result.group("user")
			    self.crouters[-1].rtrchaps[-1].passwd=result.group("passwd")
			    self.crouters[-1].rtrchaps[-1].ptype=result.group("ptype")

			#### interface FastEthernet1/1
	                if result and pat=="interface":
			    self.cinterfaces.append(Interface(result.group("interface")))
			    self.cinterfaces[-1].setcrouter(router)
			    self.cinterfaces[-1].setcoptions(self.options)

		            state="interface"

			#### ip route 192.0.2.0 255.255.255.0 Null0
			# or
			#### ip route 202.44.152.0 255.255.255.0 210.9.87.45
		        elif result and pat=="route":
			    self.croutes.append(Route())
			    self.croutes[-1].setcrouter(router)
			
	        	    if result.group("route"):
			        self.croutes[-1].setcip_cidr( result.group("route"))
	        	    if result.group("mask"):
			        self.croutes[-1].setcmask(result.group("mask"))
	        	    if result.group("dest"):
			        self.croutes[-1].setcdest(result.group("dest"))

#XXX
			    self.chashroutes[self.croutes[-1].cip_cidr]=self.croutes[-1]

			    state="base"
			    if self.debug>0:
				if self.debug>2: print "DEBUG: Add Route:", self.croutes[-1]
	
	        elif state=="interface":

		     #### interface Loopback0
		     ####  ip address 203.63.80.248 255.255.255.255
		     # or
		     ####  ip address 203.63.80.248 255.255.255.255 secondary
	             result=cintpats['ip'].match(line)
	             if result and result.group('ip'):
		        self.cinterfaces[-1].setcip(result.group('ip'))

	             if result and result.group('mask'):
		        self.cinterfaces[-1].setcmask(result.group('mask'))
			
		     #### interface Loopback0
		     ####  description description DQLD136719 QLD Studies Authority
	             result=cintpats['description'].match(line)
	             if result and result.group('description'):
		        self.cinterfaces[-1].setcdescription(result.group('description'))
		        # break down description line
	                result=cdescpats['one'].match(line)
	                if result and result.group('sid'):
		            self.cinterfaces[-1].setcsid(result.group('sid'))
		
		     #### interface Loopback0
		     ####  rate-limit input 10000000 8000 8000 conform-action transmit
		     # or
		     ####  rate-limit output 1000000 187500 375000 conform-action transmit
	             result=cintpats['ratein'].match(line)
	             if result and result.group('rate'):
		        self.cinterfaces[-1].setcratein(result.group('rate'))

	             result=cintpats['rateout'].match(line)
	             if result and result.group('rate'):
		        self.cinterfaces[-1].setcrateout(result.group('rate'))

	             # ISDN type links , chapusers and gateways
		     #### interface Loopback0
		     ####  dialer map ip 210.10.84.147 name crcpoly1-gw broadcast
	             result=cintpats['dialer'].match(line)
	             if result and result.group('gw'):
		        self.cinterfaces[-1].setcgw(result.group('gw'))
	             if result and result.group('chap'):
		        self.cinterfaces[-1].setcchap(result.group('chap'))

		     #### interface Loopback0
		     ####  shutdown
		     # or 
		     ####  no shutdown
	             result=cintpats['state'].match(line)
	             if result and not result.group('no'):
		        self.cinterfaces[-1].setcstate('shutdown')

		     #### interface Loopback0
		     ####  ip route-cache flow
		     # or 
		     ####  ip route-cache flow sampled
		     result=cintpats['cache'].match(line)
		     if result and result.group('cache'):
			 self.cinterfaces[-1].setccache('on')
		         if result.group('sampled'):
			     self.cinterfaces[-1].setccache('sampled')

		     #self.cinterfaces[-1].final()
			

	    # Resolve routes to interfaces
            for i in self.cinterfaces:
		i.final()	# Last minute object adjustments now that
				# we have completed the parsing.
                for r in self.croutes:
	            i.addroute(r)
	   
	    # Tag Chapusers in main section of config if interface tied (ISDN)
	    # self.cinterfaces[-1].setcchap
	    # self.cinterfaces.cchap[]
	    # self.crouters[-1].rtrchaps[]
	    for rchap in self.crouters[-1].rtrchaps:
	        for iface in self.cinterfaces:
		    if rchap.chapuser in iface.cchap:
		        rchap.setitied(True)
		 	print "Chap %s belongs to interface %s" % \
				(rchap.chapuser, iface.cinterface)
		   
if __name__=="__main__":

    TEST=2

    options=[]

    if TEST==1:
        options.append("details")
        options.append("routes")
    if TEST==2:
        options.append("routemaps")

    Attributes.options=options

    cfg = RtrConfigParser(dir="./conf", attrs=Attributes)

    print cfg.chashroutes.keys()

    if TEST==1:
        # output router info
        for r in cfg.crouters:
            print r
        print "--------"
        # output interface with route info
        for i in cfg.cinterfaces:
            print i

    if TEST==2:
	#print cfg.croutemaps
        for rm in cfg.croutemaps.keys():
            print rm, cfg.croutemaps[rm]

	for acl in cfg.caccesslist.keys():
	    print acl
	    for route in cfg.caccesslist[acl]:
	        print "  ",route

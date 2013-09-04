#!/opt/bta4/bin/python
#
# audit
#	Compare cisco config files with what our datebase says.
#	Reports discrepencies.
# 
# Config files on wocha:/opt/rancid/cor/configs etc.
#
# 
# 

import getopt, sys, glob
sys.path.insert(0,"/opt/bta4/lib/python2.3")

import db_view
import cisco_view
from cisco_view import *
from db_view import *

from lib import Log
from attrs import Attributes

PERFECT=1
PARTIAL=2
UNRESOLVED=3

ROUTE_OK=1			# Have Route and Gateway match.
ROUTE_NETOK=2			# Have Route and destination net matches.
ROUTE_INVALID_GATEWAY=3		# Have Route but gateway is unconfirmed.

configfiles=['hay-cor4-confg','mel-cor7-confg','mel-cor1-confg']

# not used
faults={ 0: "Record Correct.",
	 1: "Bandwidth mismatch.",
	 2: "Live in db but router interface is down.",
	 3: "Route on router but not in db.",
	 4: "Route in db but incorrect gateway.",
	 5: "Interface up and not in db.",
	 6: "Interface down and not in db."
       }

def loadconfigfiles(dir):
    files = map(lambda x:string.split(x,'/')[-1],glob.glob('%s/*-confg'% dir))
    files2 = map(lambda x:string.split(x,'/')[-1],glob.glob('%s/*-*'% dir))
   
    ### router Filters
    nfiles=[]

    for file in files:
	if string.find(file,'hub')<0:
	    nfiles.append(file)	
    if not nfiles:
        for file in files2:
	    if string.find(file,'hub')<0:
	        nfiles.append("%s-confg" % (file))	

    return nfiles

def cfg2rtr(configs):
    
    if not configs:
	return None
    routers=[]
    for cfg in configs:
	rtr,filler=string.split(cfg,'-confg')
	routers.append(rtr)
    return routers

######
# Inherit the dbview and the cisco cnfig view

class BTAInterfaces(NetCustomers,RtrConfigParser):
    """Compare all interfaces in BTA with whats on routers"""

    ISONRTR=1
    ISINDB=2
    IGNORE=3

    def __init__(self,dir=None, routers=None, attrs=None):

	# Get all Router and db records about interfaces and routes
	RtrConfigParser.__init__(self, dir=dir, attrs=attrs)
	NetCustomers.__init__(self,routers=routers, attrs=attrs)

	self.output=[]
	self.options=attrs.options

	self.attributes=[]	# Indicate what we know
	self.state=0		# Resolved against Router config
	self.ifstatus=0		# Router Interface status
	self.router=None
	self.interface=None
	self.bandwidth=0
	self.description=None
	self.problems=[]	# Problems with record.

	self.interface_matchup()

    def __str__(self):
	if self.status==FOUND:
	    s="rtr=%s if=%s bw=%s st=%s" % (self.router, self.interface, self.bandwidth, self.state)
	#if self.description:
	#    s=s+"\n  "+self.description

	return(s)

    def addattribute(self, attr):
	self.attributes.append(attr)

    def interface_matchup(self):
        """Try to match db interface objects with its equiv router interface objects
        """

        for rtrif in self.cinterfaces:

	  ####### FILTERS ###############
	  # Interfaces to ignore or rewrite
	  # 
	  #  Group-Async*
	  #  Loopback*
	  #  Null*
	  #  Virtual-Template*
	  #  VLAN*
	  #  Async*
	  #  Tunnel*

	  ri=rtrif.cinterface
	  # ISDN
	  if ri[:6]=='Serial' and \
	    	ri[-3:]==':15':
		ri=ri[:-3]		

	  # ignore these
	  if rtrif.cinterface[:8]=="Loopback":
		 rtrif.attributes.append(BTAInterfaces.IGNORE)
	  if rtrif.cinterface[:4]=="Null":
		 rtrif.attributes.append(BTAInterfaces.IGNORE)
	  if rtrif.cinterface[:11]=="Group-Async":
		 rtrif.attributes.append(BTAInterfaces.IGNORE)
	  if rtrif.cinterface[:16]=="Virtual-Template":
		 rtrif.attributes.append(BTAInterfaces.IGNORE)
	  if rtrif.cinterface[:4]=="VLAN":
		 rtrif.attributes.append(BTAInterfaces.IGNORE)
	  if rtrif.cinterface[:6]=="Tunnel":
		 rtrif.attributes.append(BTAInterfaces.IGNORE)
	  
	  if BTAInterfaces.IGNORE in rtrif.attributes:
		continue
	  #
	  #######
          for dbif in self.dbinterfaces:
	    if rtrif.crouter!=dbif.dbrouter:
		continue

	    if ri != dbif.dbinterface:
		continue

	    rtrif.crelatives.append(dbif)
	    rtrif.attributes.append(BTAInterfaces.ISINDB)
	    dbif.dbrelatives.append(rtrif)
	    dbif.attributes.append(BTAInterfaces.ISONRTR)

#	    if rtrif.cinterface=="GigabitEthernet0/3.5":
#	        print "** Interface MATCH: [%s]<->[%s]" % (rtrif.cinterface, dbif.dbinterface)

    def chap_matchup(self):
	"""Attempt to matchup chapusers, gateways and interfaces
	"""
	pass
#	for rtrif in self.cinterfaces:
#	    if len(rtrif.cchap)>0:	# Has chapuser(s)
#		for chap in rtrif.cchap:
			


    def addattribute(self, attr):
	self.attributes.append(attr)

    def report_rtrdowninterface(self):
	self.output=[]
	for riface in self.cinterfaces:
	    if BTAInterfaces.IGNORE in riface.attributes:
		continue

	    riface.coptions=[]
	    if BTAInterfaces.ISINDB in riface.attributes and riface.cstate=="shutdown":
		label=False
		for rel in riface.crelatives:
		    if rel.dbcustomer and rel.dblink_id:
			if label==False:
			    self.output.append("Active in db and down on router: %s" % riface)
			    label=True
		        self.output.append("\t\t%s\n" % rel)

    def report_nortrinterface(self):
	"""Generate a report on interface mismatches
	"""
	self.output=[]
	for ciface in self.dbinterfaces:
	    if BTAInterfaces.IGNORE in ciface.attributes:
		continue

	    ciface.dboptions=[]
	    if not BTAInterfaces.ISONRTR in ciface.attributes:
		self.output.append("Not on router: %s\n" % ciface)

    def report_nodbinterface(self):
	"""Generate a report on interface mismatches.
	   Can ignore interfaces that are shutdown
	"""

	self.output=[]
	for riface in self.cinterfaces:
	    if BTAInterfaces.IGNORE in riface.attributes:
		continue

	    riface.coptions=[]
	    if not BTAInterfaces.ISINDB in riface.attributes:
		if "up" in self.options and riface.cstate=="up":
		    self.output.append("Not in db: %s" % riface)
		else:
		    self.output.append("Not in db: %s" % riface)

class BTACustomer(NetCustomers,RtrConfigParser):
    """Merge the Router view of the network and the db View of the network"""

    ISONRTR=1		# db Interface exists on router
    ISINDB=2		# router interface is in db

    HAVE_ROUTER=3
    HAVE_INTERFACE=4
    HAVE_BANDWIDTH=5
    HAVE_DESCRIPTION=6
    HAVE_IFSTATUS=7
    HAVE_CUSTOMER=8
    
    HAVE_ROUTE=10	# db & conig both have the route
    HAVE_GATEWAY=11	# db & config Destination gateways match
    HAVE_NETGATEWAY=12	# db & config Subnets match in destination for a route
    INTERFACE_SHUTDOWN=101	# Set extern

    def __init__(self,dir=None, routers=None, attrs=None):

	# Get all Router and db records about interfaces and routes
	RtrConfigParser.__init__(self, dir=dir, attrs=attrs)
	NetCustomers.__init__(self, routers=routers, attrs=attrs)

	self.options=attrs.options
	self.log=attrs.log
	self.debug=attrs.debug

	self.attributes=[]	# flags to indicate what we know about this object
	self.state=0		# Resolved against Router config
	self.ifstatus=0		# Router Interface status
	self.customer=0
	self.router=None
	self.interface=None
	self.bandwidth=0
	self.description=None
	self.routes=[]
	self.problems=[]	# Problems with record.
	self.output=""

	if "interfaces" in self.options:
	    self.link_matchup()

	if "routes" in self.options:
	    self.route_matchup()

    def addattribute(self, attr):
	self.attributes.append(attr)

    def addroute(self, croute):
	self.routes.append(croute)
		
    def testattributes(self,attrs):
	if type(attrs)==type(int):
	    if attrs in self.attributes:
	        return(1)
	elif type(attrs)==type(()) or type(attrs)==type([]):
	    i=1
	    for a in attrs:
	        if a not in self.attributes:
		    i=0	
	    return(i)
	else:
	    return(0)

    def setrouter(self, router):
	self.router = router
	self.addattribute(BTACustomer.HAVE_ROUTER)
    def setstatus(self, status):
	self.status = status
	self.addattribute(BTACustomer.HAVE_IFSTATUS)
    def setstate(self, state):
	self.state = state
    def setcustomer(self, customer):
	self.customer = customer
	self.addattribute(BTACustomer.HAVE_CUSTOMER)
    def setinterface(self, interface):
	self.interface = interface
	self.addattribute(BTACustomer.HAVE_INTERFACE)
    def setbandwidth(self, bandwidth):
	self.bandwidth = bandwidth
	self.addattribute(BTACustomer.HAVE_BANDWIDTH)
    def	setdescription(self,description):
	self.description = description
	self.addattribute(BTACustomer.HAVE_DESCRIPTION)

    def addproblem(self,fault):
	self.problems.append(fault)

    def update(self):
	if (self.testattributes((BTACustomer.HAVE_ROUTER , BTACustomer.HAVE_INTERFACE))):
	    self.status=PARTIAL
	    #print "*,", self.attributes, (BTACustomer.HAVE_ROUTER | BTACustomer.HAVE_INTERFACE)
	if (self.testattributes((BTACustomer.HAVE_ROUTER , BTACustomer.HAVE_INTERFACE , BTACustomer.HAVE_BANDWIDTH))):
	    self.status=PERFECT

    def __str__(self):
	if self.status==PERFECT:
	    s="PERFECT: cust=%d, rtr=%s if=%s bw=%s st=%s" % (self.customer, self.router, self.interface, self.bandwidth, self.state)
	elif self.status==PARTIAL:
	    s="PARTIAL: cust=%d, rtr=%s if=%s st=%s rate=%d" % (self.customer, self.router, self.interface, self.state, self.bandwidth)
	else:
	    s="UNRESOLVED:"

	if self.description:
	    s=s+"\n  "+self.description

	return(s)

    def route_compare(self, dbroute, croute):
        """Take cust route object and config route object and compares 
	    for equivalence. Tie the objects together when matching
	    Also compare destinations
	    - dbroute is the route object from the database
	    - croute is the routing object from the cisco config
	"""

        result=0
	# Test
	#print "Compare Route: [%s] <-> [%s]\n" % (dbroute.dbip_cidr,croute.cip_cidr)
	#if croute.cip_cidr[:12]=="210.9.64.128":
	if dbroute.dbip_cidr[:12]=="210.9.64.128":
	    print "Compare Route: [%s] <-> [%s]\n" % (dbroute.dbip_cidr,croute.cip_cidr)
	    
        if dbroute.dbip_cidr == croute.cip_cidr:		# Route match
	    #if croute.cip_cidr[:11]=="210.11.72.0":
	    if croute.cip_cidr[:12]== "210.9.64.128":
	        print "Compare Route: [%s] <-> [%s]\n" % (dbroute.dbip_cidr,croute.cip_cidr)
            #if self.debug>3: print "Compare Route: [%s] <-> [%s]\n" % (dbroute.dbip_cidr,croute.cip_cidr)
            #print "Compare Route: [%s] <-> [%s]\n" % (dbroute.dbip_cidr,croute.cip_cidr)
            #print "\t[%s] <-> [%s]\n" % (dbroute.dbip_cidr,croute.cdest_ip)

	    result=ROUTE_INVALID_GATEWAY

	    # gateways which have an IP as destination for a route in a cisco config
	    if dbroute.dbgateway==croute.cdest:
                #print "\t*gateway match*"
	        result=ROUTE_OK
	    else:
		# Should be interface based routing.
		# For each ip address on an interface in a cisco config
		# croute.cdest_ip is the other end if its a /30
		for ip in croute.cdest_ip:
		    if not ip:			# not /30, compare subnet
		        mask = string.split(dbroute.dbip_cidr,'/')[1]
	 	        net = IPTools.findnet(dbroute.dbip_cidr) + "/" + str(mask)
            	        #print "\ttest net [%s] <-> cdest[%s]" % (net,croute.cdest_net)
			if net in croute.cdest_net:
			    #print "\t* net match"
			    result=ROUTE_NETOK

		    else:	# Its a /30, so croute.cdest_ip is other end
            		#print "\ttest ip [%s]<->[%s]\n" % (ip, dbroute.dbgateway)
		        if ip==dbroute.dbgateway:
			    #print "\t* ip match"
	    		    result=ROUTE_OK
			    break

        return result

    def link_matchup(self):
        """Try to xlink db interface objects with its equiv router interface objects
	   And also the route objects.
        """

        for rtrif in self.cinterfaces:
          for cust in self.dbinterfaces:
	    if rtrif.crouter!=cust.dbrouter:
		continue
	    if rtrif.cinterface!=cust.dbinterface:
		continue

	    rtrif.crelatives.append(cust.dbinterface)
	    rtrif.attributes.append(BTACustomer.ISINDB)
	    cust.dbrelatives.append(rtrif.cinterface)
	    cust.attributes.append(BTACustomer.ISONRTR)

#	    if rtrif.cinterface=="GigabitEthernet0/3.5":
#	         print "** Interface MATCH: [%s]<->[%s]" % (rtrif.cinterface, cust.dbinterface)

	    # Test routes
	    for croute in rtrif.croutes:
	        for dbroute in cust.dbroutes:
	            result=self.route_compare(dbroute, croute)
		    if result==ROUTE_INVALID_GATEWAY or result==ROUTE_OK or  result==ROUTE_NETOK:

			#print "Match:", dbroute,"<->", croute

	                rtrif.crelatives.append(dbroute)
		        rtrif.attributes.append(BTACustomer.HAVE_ROUTE)
	                cust.dbrelatives.append(dbroute)
		        cust.attributes.append(BTACustomer.HAVE_ROUTE)
	            if result==ROUTE_OK:
		        rtrif.attributes.append(BTACustomer.HAVE_GATEWAY)
		        cust.attributes.append(BTACustomer.HAVE_GATEWAY)
	            if result==ROUTE_NETOK:
		        rtrif.attributes.append(BTACustomer.HAVE_NETGATEWAY)
		        cust.attributes.append(BTACustomer.HAVE_NETGATEWAY)

	    # Try bandwidth
	    if rtrif.crateout==cust.dbbandwidth and cust.dbbandwidth!=0:
	        rtrif.attributes.append(BTACustomer.HAVE_BANDWIDTH)
	        cust.attributes.append(BTACustomer.HAVE_BANDWIDTH)
	    	        
	    if rtrif.cdescription:
	 	rtrif.attributes.append(BTACustomer.HAVE_DESCRIPTION)
	
    def route_matchup(self):
	""" Do the comparison between router config and bta db routes
	    and gateways
	"""

	if self.debug: print "Doing route_matchup()"
	if self.debug>1: print "DEBUG route_matchup: start"
	if self.debug>1: print "DEBUG dbroutelist:", self.dbroutelist 
#XXX
# dbgateway
#	    self.chashroutes.keys()

	for rroute in self.croutes:
	    if self.debug>3: print "DEBUG: R Find ", rroute
	    if self.debug>3: print "DEBUG: Find ", rroute.cip_cidr

#	    if rroute.cip_cidr not in self.dbroutelist:
#		    continue
	    for croute in self.dbroutes:

		    if croute.dbip_cidr=='Y':
	    		continue

	    	    if self.debug>3: print "DEBUG: C Find ", croute

		    if self.debug>2: print "DEBUG: croute=", croute.dbip_cidr

	            result=self.route_compare(croute, rroute)
		    if result==ROUTE_INVALID_GATEWAY or result==ROUTE_OK:
			if self.debug>2:  print "Match:", croute,"<->", rroute
	                rroute.crelatives.append(croute)
		        rroute.attributes.append(BTACustomer.HAVE_ROUTE)
	                croute.dbrelatives.append(croute)
		        croute.attributes.append(BTACustomer.HAVE_ROUTE)
	            if result==ROUTE_OK:
		        rroute.attributes.append(BTACustomer.HAVE_GATEWAY)
		        croute.attributes.append(BTACustomer.HAVE_GATEWAY)

	    # Check if the route has been configured in a Cisco Access-list
	    # used for BGP route filtering.

	    if "routemaps" in self.options:
                for acl in self.caccesslist.keys():
   	            for ciscoroute in self.caccesslist[acl]:
		        #print "Compare Cisco: ", ciscoroute
		        #print "        To Db: ", rroute

		        result=self.route_compare(croute, ciscoroute)

		        if result==ROUTE_INVALID_GATEWAY or result==ROUTE_OK:
			    print "Found BGP route match:", ciscoroute, rroute
		            if BTACustomer.HAVE_ROUTE not in rroute.attributes:
	                        rroute.crelatives.append(ciscoroute)
		                rroute.attributes.append(BTACustomer.HAVE_ROUTE)
	                        croute.dbrelatives.append(ciscoroute)
		                croute.attributes.append(BTACustomer.HAVE_ROUTE)

    def report_goodinterfaces(self):
	"""Generate a report on interface mismatches"""
	
	for ciface in self.dbinterfaces:
	    if BTACustomer.ISONRTR in ciface.attributes:
		print "interface is Ok: ", ciface, ciface.attributes

    def report_nortrinterface(self):
	"""Generate a report on interface mismatches"""
	
	for ciface in self.dbinterfaces:
	    if not BTACustomer.ISONRTR in ciface.attributes:
		print "No interface on router: ", ciface, ciface.attributes
    def report_nodbinterface(self):
	"""Generate a report on interface mismatches"""

	for riface in self.cinterfaces:
	    if not BTACustomer.ISINDB in riface.attributes:
		print "No interface in db: ", riface, riface.attributes

    def report_bandwidth_mismatch(self):
	pass

    def report_badroutes(self):
	for route in self.croutes:
	    if BTACustomer.HAVE_ROUTE not in route.attributes:
		self.log(0, "Route not in db: %s\n" % ( route))

    def report_badgateways(self):
	"""Generate report on incorrect gateway addresses"""

	for route in self.croutes:
	    if BTACustomer.HAVE_ROUTE in route.attributes and \
		(BTACustomer.HAVE_GATEWAY not in route.attributes or \
		BTACustomer.INTERFACE_SHUTDOWN in route.attributes) and \
		BTACustomer.HAVE_NETGATEWAY not in route.attributes:
		    #relatives=self.GetRelatives(route, dbRoutes)
		    relatives=self.GetRelatives(Route,route)
		    if not IPTools.checkip(route.cdest):
		        self.log(0, "Incorrect gateway:\n")
		        if BTACustomer.INTERFACE_SHUTDOWN in route.attributes:
		            self.log(0, "\tRouter has: %s (interface shutdown)\n" %  route)
		        else:
		            self.log(0, "\tRouter has: %s\n" %  route)
		        self.log(0, "\t    db has: ")
		        for rel in relatives:
			    self.log(0, "%s, " % rel, 0)
		        self.log(0, "\n",0)
		    #print "gw=",rel.dbgateway
	    if BTACustomer.HAVE_NETGATEWAY in route.attributes:
		    relatives=self.GetRelatives(Route,route)
		    if not IPTools.checkip(route.cdest):
		        self.log(0, "Unverifiable gateway:\n")
		        self.log(0, "\tRouter has: %s\n" %  route)
		        self.log(0, "\t    db has: ")
		        for rel in relatives:
			    self.log(0, "%s, " % rel, 0)
		        self.log(0, "\n",0)

    def report_ok(self):
	pass
 
    def GetRelatives(self,objecttype, object):
	"""Return all 'related' objects of requested class type 
	"""

	ret=[]
	#print " * * GetRel:", str(object.__class__)

	if str(object.__class__)=="cisco_view.Route":
	    for rel in object.crelatives:
		ret.append(rel)
	    
	if str(object.__class__)=="db_view.dbRoute":
	    for rel in object.dbrelatives:
		ret.append(rel)
		     
	return ret

class Report(BTACustomer):
    def __init__(self):
	pass

class UnResolved:
    def __init__(self):
	pass

def usage():
    print "usage: audit.py [-a][-D][-c][-h][-r][-i][-u][-d configdir]"
    print "\t-D = Debug. More D's. more debug."
    print "\t-d configdir = Dir where cisco config files are. Default is 'conf'"
    print "\tit will reead all config files in dir, is slow if many."
    print "\t-c = Cache db results locally."
    print "\t-h = Help. This is it"
    print "\t-r = Report on routing discrepancies"
    print "\r-i = Report on interface discrenancies"
    print "\r-u = Report on rtr up interfaces but active in BTA only"
    print "\t-a = Report on routing and interface discrepancies with customer and"
    print "link details"

def legend():
    """Problems that are detected"""
    print "\nRouting discrepancies"
    print "\nRoute no in db"
    print "\t1. The route does not exist in the bta4.customer_routes table"
    print "\t2. The route exist in the bta4.customer_routes table, but is marked as a gateway"
    print "\nIncorrect gateway"
    print "\t1. Route exists, but the gateway address is wrong."
    print "\t   a) The ip's of the link are wrong way around."
    print "\t   b) The address is just wrong (typo ?)."
    print "\t2. Route is via a PVC to 3rd party and may actually be correct."
    print "\nInterface discrepancies"
    print "\nNot in db"
    print "\t1. The interface is not in table bta4.interface or not in a state of"
    print "\t   SPARE, RESERVED, VIRTUAL or AOK"
    print "\nNot on Router"
    print "\t1. Interface was found active in bta4.interface but was not found"
    print "\t   on the router or is in a state of 'shutdown'."
    print "\t2. like 1) and interface is also assigned to the indicated customer_id/link_id"


if __name__=="__main__":

    try:
        opts,args=getopt.getopt(sys.argv[1:],'hcriauDl:d:')
    except:
        sys.stderr.write("Invalid args\n")
        usage()
        sys.exit(-1)

    level=1
    cache=0
    debug=0
    options=[]
    configdir="conf"
    for o,v in opts:
        if o=='-h':
            usage()
	    legend()
            sys.exit(0)
        if o=='-c':
	    cache=1
        if o=='-d':
	    configdir=v
        if o=='-l':
	    level=int(v)
        if o=='-D':
	    debug=debug+1
        if o=='-r':
            #options.append("routemaps")	# Works Ok, not needed atm
            options.append("routes")
        if o=='-a':
            options.append("routes")
            options.append("interfaces")
            options.append("all")
        if o=='-i':
            options.append("interfaces")
            options.append("up")	# do as default
        if o=='-u':		# Interfaces down but in BTA
            options.append("interfaces")
            options.append("up")
#        if o=='-t':
#            st[0:3] = [int(v[0:4]) , int(v[4:6]), int(v[6:8]) ]

    log = Log.Log(level)
    #options.append("level")

    log(0,"Started\n") 
    
    #options=("interfaces")

    if "all" in options:
	options.append("routemaps")
	options.append("routes")
	options.append("interfaces")

    if cache==1:
	options.append("cache")

    Attributes.debug=debug
    Attributes.log=log
    Attributes.options=options

    if debug>0: print "DEBUG main: debug level is:",debug

    configfiles=loadconfigfiles(configdir)
    routers = cfg2rtr(configfiles)

    if "interfaces" in options:
	
	ic = BTAInterfaces(dir="conf/", routers=routers, attrs=Attributes)

	log(0,"-- Interfaces with bandwidth mismatch ---\n")

	#report_bandwidth_mismatch()

	log(0,"-- Interfaces in DB but shutdown on Router ---\n")
	ic.report_rtrdowninterface()
	log(0,ic.output)

	log(0,"-- Interfaces in DB but missing on Router ---\n")
	
	ic.report_nortrinterface()
	log(0,ic.output)

	log(0, "-- Interfaces on Router but missing from DB ---\n")

	ic.report_nodbinterface()
	log(0,ic.output)

    if ("routes" in options) or ( "routemaps" in options):
 
        bc= BTACustomer(dir="conf/",routers=routers,attrs=Attributes)

	log(0,"-- Static Routes and Gateways check ---\n")
        bc.report_badgateways()
   
        #print "Doing report badroutes..."
        bc.report_badroutes()
	log(0,bc.output)

        #print "Doing No Router interface..."
        #bc.report_nortrinterface()
        #print "Doing No db interface..."
        #bc.report_nodbinterface()
        #print "Good db interfaces..."
        #bc.report_goodinterfaces()

#    log.SendMail("harryr", "nm", "audit")
	
    print log


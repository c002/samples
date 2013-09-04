#!/opt/bta4/bin/python
#
# db_view
#	Interesting network stuff the db knows about like links
#	routers and routes.
#
# 
# 

import sys, os, cPickle, time, string
sys.path.append('/opt/bta4/lib')
sys.path.append('/opt/bta4/lib/python2.3')

import dbconnectcx
from lib import Log
from lib import RtrCfg

from constants import Const
from attrs import Attributes

class dbRoute:
    def __init__(self):
	pass

class dbInterface:
    def __init__(self):
	self.dbcustomer=0
	self.dbsid=None
	self.dblink_id=None
	self.dbchapuser=None
	self.dbgateway=None	# If chapuser, should have gateway too
	self.dbrouter=None
	self.dbrtr=None
	self.dblocale=None
	self.dbinterface=None
	self.dbstatus=None
	self.dbbandwidth=0
	self.dbvpi=0
	self.dbvci=0
	self.dbroutes=[]	# Route objects for this interface

	self.dbrelatives=[]	# Related objects
	self.attributes=[]
	self.dboptions=[]

    def setdbcustomer(self,cust):
	self.dbcustomer=cust
    def setdbsid(self,sid):
	self.dbsid=sid
    def setdblink_id(self,link_id):
	self.dblink_id=link_id
    def setdbchapuser(self,chap):
	self.dbchap=chap
    def setdbgateway(self,gateway):
	self.dbgateway=gateway
    def setdbrtr(self,rtr):
	self.dbrtr=rtr
    def setdblocale(self,locale):
	self.dblocale=locale
    def setdbinterface(self,iface):
	self.dbinterface=iface
    def setdbstatus(self,status):
	self.dbstatus=status
    def setdbbandwidth(self,bandwidth):
	if not bandwidth:
	    bandwidth=0
	self.dbbandwidth=long(bandwidth)
    def setdbvpi(self,vpi):
	if not vpi:
	    vpi=0
	self.dbvpi=int(vpi)
    def setdbvci(self,vci):
	if not vci:
	    vci=0
	self.dbvci=int(vci)
    def setdbrouter(self):
	self.dbrouter=self.dblocale+'-'+self.dbrtr

    def __str__(self):
	s=""
	if self.dbcustomer and self.dblink_id:
	    s="cust=%d/%d " % (self.dbcustomer, self.dblink_id)

	s=s + "rtr=%s iface=%s, " %  (self.dbrouter ,self.dbinterface)
	if self.dbchapuser: s=s+ "chap=%s, " % self.dbchapuser
	if self.dbgateway: s=s+ "gw=%s, " % self.dbgateway
	if self.dbvpi and self.dbvci: s=s+ " vpi/vci=%d/%d, " % (self.dbvpi, self.dbvci)
	if self.dbbandwidth: s=s+ "bw=%s, " % self.dbbandwidth
	if self.dbstatus: s=s+ "status=%s, " % self.dbstatus

	if len(self.dbroutes):
	    s=s+"\n\t"
	    for r in self.dbroutes:
		s=s+"%s, " % r
	    s=s+"\n"
	return(s)

class dbRoutes:
    def __init__(self):
	self.dbcustomer=0
	self.dbip_cidr=None
	self.dblink_id=None
	self.dbchapuser=None
	self.dbrouter=None
	self.dbinterface=None
	self.dbgateway=None	# Destination gateway IP
	self.dbas_number=None
	self.dbconfirmed=0
	self.dbtied=0		# is 1 if succesfully tied to an interface in db

	self.dbrelatives=[]	# Related objects
	self.attributes=[]

    def setdbrid(self, od):
	self.db_r_id=id

    def setdbcustomer(self, cust):
	self.dbcustomer=cust
    def setdbipcidr(self, ipcidr):
	self.dbip_cidr=ipcidr
    def setdblinkid(self, link_id):
	self.dblink_id=link_id
    def setdbgateway(self, gateway):
	tmp=string.split(gateway,"/")	# Remove trailing mask
	self.dbgateway=tmp[0]
    def setdbasnumber(self, as_number):
	self.dbas_number=as_number
    def setdbrouter(self, router):
	self.dbrouter=router
    def setdbchapuser(self, chapuser):
	self.dbchapuser=chapuser
    def setdbinterface(self, interface):
	self.dbinterface=interface

    def __str__(self):
	s="cust=%d/%d %s gw=%s" % (self.dbcustomer,self.dblink_id, self.dbip_cidr, self.dbgateway)
	return s

class NetCustomers:
    """ties routes and links to customers
	according to database
    """

    def __init__(self, routers=None, attrs=None):
	
        self.options = attrs.options
	self.debug=attrs.debug
	self.log=attrs.log

	if self.debug>0: print "DEBUG NetCustomers: debug is:",self.debug

	self.targets=routers
	self.log(0," Routers: %s\n" % str(self.targets))
 
	self.fname="/tmp/netcusts.pickle"	

	self.dbinterfaces=[]
	self.dbroutes=[]
	self.dbroutelist=[]

	self.db=dbconnectcx.dbConnect("bta4_www/bta4_www98@bta4")
	
	if "cache" in self.options and os.path.exists(self.fname):
	    self.unpickle()
	else:
	    if "routes" in self.options:
	        self.dbgetlinks()
	        self.dbgetroutes()
	    if "interfaces" in self.options:
	        self.dbgetinterfaces()
	        self.dbgetlinks()	# XXX test
	
	    self.pickle()

	if "routes" in self.options:
	    self.dbmerge_routes()

	self.db.close()

    def pickle(self):
	f=open(self.fname,"w")
	p=cPickle.Pickler(f)
	obj=(self.dbinterfaces, self.dbroutes, self.dbroutelist)
	p.dump(obj)
	f.close()

    def unpickle(self):
	f=open(self.fname,"r")
	u=cPickle.Unpickler(f)
	obj = u.load()
	self.dbinterfaces, self.dbroutes, self.dbroutelist=obj
	f.close()

    def dbgetinterfaces(self):
	"""Get all interfaces that are SPARE , RESERVED, VIRTUAL, AOK
	   These can be checked to see if they exist in the routers
	"""

	subq=""
	if self.targets:
	    s="("
	    l="("
	    for target in self.targets:
		s=s+"'"+target+"',"
	    s=s[:-1]+")"
	    l=l[:-1]+")"
	    subq=" and locale||'-'||router in %s" % (s) 

        query="""select unique interface, router, locale, bandwidth, 
		vpi, vci,status from bta4.interface
	   where
	   status in ('VIRTUAL','SPARE','RESERVED','AOK')
	   and sysdate between start_time and end_time
	   %s
	  """ % (subq)

	if self.debug>1: print "DEBUG (a)",query

	self.db.execute(query)
	current_interfaces=dbconnectcx.ResultList(self.db)

	for iface in current_interfaces:
	    self.dbinterfaces.append(dbInterface())

	    self.dbinterfaces[-1].setdbinterface(iface[0])
	    self.dbinterfaces[-1].setdbrtr(iface[1])
	    self.dbinterfaces[-1].setdblocale(iface[2])
	    self.dbinterfaces[-1].setdbbandwidth(iface[3])
	    self.dbinterfaces[-1].setdbvpi(iface[4])
	    self.dbinterfaces[-1].setdbvci(iface[5])
	    self.dbinterfaces[-1].setdbstatus(iface[6])

	    self.dbinterfaces[-1].setdbrouter()

    def dbgetlinks(self):

	subq=""
	if self.targets:
	    s="("
	    l="("
	    for target in self.targets:
		s=s+"'"+target+"',"
	    s=s[:-1]+")"
	    l=l[:-1]+")"
	    subq=" and i.locale||'-'||i.router in %s" % (s) 

	# Get gateway ips for links with chapusers
	query="""select unique l.customer, l.link_id, a.ip_cidr, 
		 l.chapuser, r.ip_id
		 from 
		 bta4.customer_links l, bta4.customer_ip_address a,
		 bta4.customer_routes r, bta4.interface i
		 where
		 l.chapuser is not null 
		 and r.ip_id=a.ip_id
		 and i.interface_id=l.interface_id_fk
		 and r.customer=a.customer
		 and a.customer=l.customer
		 and r.link_id=l.link_id
		 and lower(r.gateway)='y'
		 and l.end_time > sysdate 
		 and a.end_time > %d
		 %s
	      """ % (time.time(), subq)

	if self.debug>1: print "DEBUG (b)",query

	self.db.execute(query)
	rawchaps=dbconnectcx.ResultList(self.db)
 	chaphash={}
	for entry in rawchaps:
	    key=str(int(entry[0]))+ ":" + str(entry[1])
	    if chaphash.has_key(key):
		self.log(0,"Multiple Chapusers for %s gw:ch=%s:%s, already have %s\n" %(key, entry[2],entry[3],chaphash[key]))
	    else:
		chaphash[key]=entry[2]+":"+entry[3]

	# Customer link details
        query="""select customer, i.sid, l.link_id, chapuser, router, locale, interface, 
		status, LINKS_BANDWIDTH, vpi, vci
	   from bta4.customer_links l, bta4.interface i 
	   where
	   i.interface_id=l.interface_id_fk and
	   i.end_time>sysdate 
	   and l.end_time>sysdate
	   and status in ('VIRTUAL','SPARE','RESERVED','AOK')
	   %s
	  """ % (subq)

	if self.debug>1: print "DEBUG(c):",query

	self.db.execute(query)
	rawinterfaces=dbconnectcx.ResultList(self.db)


# customer, sid, link_id, chapuser, router, locale, interface, status, bandwidth, vpi, vci	
	id=1
	for iface in rawinterfaces:
	    self.dbinterfaces.append(dbInterface())

	    self.dbinterfaces[-1].setdbcustomer(iface[0])
	    self.dbinterfaces[-1].setdbsid(iface[1])
	    self.dbinterfaces[-1].setdblink_id(iface[2])

	    key = str(int(iface[0]))+":"+str(iface[1])
	    if chaphash.has_key(key):
		(a,b)=string.split(chaphash[key],":")
	   	self.dbinterfaces[-1].setdbgateway(a)
	        self.dbinterfaces[-1].setdbchapuser(b)

	    self.dbinterfaces[-1].setdbrtr(iface[4])
	    self.dbinterfaces[-1].setdblocale(iface[5])
	    self.dbinterfaces[-1].setdbinterface(iface[6])
	    self.dbinterfaces[-1].setdbstatus(iface[7])
	    self.dbinterfaces[-1].setdbbandwidth(iface[8])
	    self.dbinterfaces[-1].setdbvpi(iface[9])
	    self.dbinterfaces[-1].setdbvci(iface[10])

	    self.dbinterfaces[-1].setdbrouter()

	    id=id+1
	
    def dbgetroutes(self): 
	""" Get regular static routes excl. gateways.
	"""
	
	subq=""
	if self.targets:
	    s="("
	    l="("
	    for target in self.targets:
		s=s+"'"+target+"',"
	    s=s[:-1]+")"
	    l=l[:-1]+")"
	    subq=" and i.locale||'-'||i.router in %s" % (s) 

	routehash={}
	# Use unique, since E1 ISDN interfaces have 30 entries
	#
	query="""select unique a.customer, r.link_id, a.ip_cidr, l.chapuser, i.interface, 
		(i.locale || '-' || i.router) as router
		from bta4.interface i, bta4.customer_links l,
		bta4.customer_ip_address a, bta4.customer_routes r
		where 
		lower(r.gateway)!='y' and
		l.interface_id_fk=i.interface_id
		and l.end_time > sysdate
		and r.ip_id = a.ip_id 
		and r.link_id = l.link_id 
		and r.customer=l.customer 
		and l.customer=a.customer 
		and %d between a.start_time and a.end_time
		%s
		""" % (time.time(), subq)

	if self.debug>1: print "DEBUG dbgetroutes():",query

	self.db.execute(query)
	rawroutes=dbconnectcx.ResultList(self.db)

	query="""select unique a.customer, r.link_id, a.ip_cidr, l.chapuser, i.interface, 
		(i.locale || '-' || i.router) as router
		from bta4.interface i, bta4.customer_links l,
		bta4.customer_ip_address a, bta4.customer_routes r
		where 
		lower(r.gateway)='y' and
		l.interface_id_fk=i.interface_id
		and l.end_time > sysdate
		and r.ip_id = a.ip_id 
		and r.link_id = l.link_id 
		and r.customer=l.customer 
		and l.customer=a.customer 
		and %d between a.start_time and a.end_time
		%s
		""" % (time.time(), subq)
	
	if self.debug>1: print "DEBUG dbgetroutes()",query

	self.db.execute(query)
	rawgates=dbconnectcx.ResultList(self.db)
	for entry in rawgates:
	    key=str(int(entry[0]))+":"+str(entry[1])
	    #print "key=", key
	    if routehash.has_key(key):
	        self.log(0, "Multiple gateways in db for cust:link=%s gw=%s, already have gw=%s\n" % (key,entry[2] , routehash[key])) 
	    else:
	        routehash[key]=entry[2]

	id=1
	for entry in rawroutes:
	    self.dbroutes.append( dbRoutes() )		# New dbRoutes obj

	    #self.dbroutes[-1].setdbrid(entry)
	    self.dbroutes[-1].setdbcustomer(entry[0])
	    self.dbroutes[-1].setdblinkid(entry[1])
	    self.dbroutes[-1].setdbipcidr(entry[2])

	    key=str(int(entry[0]))+":"+str(entry[1])
	    if routehash.has_key(key):			# Set gateway address if any
	        self.dbroutes[-1].setdbgateway(routehash[key])

	    self.dbroutes[-1].setdbchapuser(entry[3])
	    self.dbroutes[-1].setdbinterface(entry[2])
	    self.dbroutes[-1].setdbrouter(entry[5])
	
	    self.dbroutelist.append(entry[2])
	    id=id+1

    def dbmerge_routes(self):
	"""Merge the route objects with the interface they belong to"""

	if self.debug: 
	    print "dbmerge_routes()"

	if self.debug>1:
		print "DEBUG len routes=",len(self.dbroutes), len(self.dbinterfaces)

	for route in self.dbroutes:
	    for iface in self.dbinterfaces:
		#if iface.router==route.router:
		    #print "compare ir=%s <-> %s (%s<->%s)" % (iface.router,route.router, iface.interface,route.interface)
		if iface.dbrouter==route.dbrouter and iface.dbinterface==route.dbinterface:
		    #print "\t*match*"

		    iface.dbroutes.append(route)
		    route.dbtied=route.dbtied+1

    def mycmpfunc(self, a, b):
	
	if a.dbcustomer > b.dbcustomer:
	    return 1
	elif a.dbcustomer < b.dbcustomer:
	    return -1
	elif a.dbcustomer==b.dbcustomer:
	    if a.dblink_id> b.dblink_id:
	        return 1
	    elif a.dblink_id < b.dblink_id:
	 	return -1

	return 0

if __name__=="__main__":

    options=[]
    options.append("routes")
    options.append("interfaces")

    log = Log.Log(3)

    Attributes.log=log
    Attributes.options=options
    Attributes.debug=4

    targets=RtrCfg.RtrCfg("conf/")
    custs=NetCustomers(routers=targets.routers, attrs=Attributes)
    clist=custs.dbinterfaces
    clist.sort(custs.mycmpfunc)
    for iface in clist:
	print iface

    for route in custs.dbroutes:
	print route

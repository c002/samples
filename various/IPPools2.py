#!/opt/python2/bin/python
#
# Tables:
# SQL> desc bta4.ip_pool
"""
 Name                            Null?    Type
 ------------------------------- -------- ----
 UNALLOCATED_IP_ID               NOT NULL NUMBER
 LOCALE                          NOT NULL VARCHAR2(10)
 ROUTER                          NOT NULL VARCHAR2(10)
 SERVICE_TYPE                             VARCHAR2(20)
 INTERFACE                                VARCHAR2(40)
 START_IP_NUMBER                 NOT NULL VARCHAR2(20)
 END_IP_NUMBER                   NOT NULL VARCHAR2(20)
 START_TIME                               DATE
 END_TIME                                 DATE
 PREFERENCE                               NUMBER
 IP_START                        NOT NULL NUMBER
 IP_END                          NOT NULL NUMBER
 IP_PRODUCT_ID                            NUMBER
 PRODUCT                                  VARCHAR2(40)
 STATUS                                   VARCHAR2(10)
"""
#
#
# Allocate A an address Pool. 
#  

import os,sys, string, time, exceptions
import IPRules
sys.path.append('/import/bta4/lib')
sys.path.append('/opt/bta4/lib')
import dbconnect
from socket import gethostname

UPDATE=1

testenv=0
if gethostname()=='tanji':
    testenv=1
if testenv:
    DBHANDLE="bta4/bta4_uat@mt1_oket"   # test
else:
    DBHANDLE="bta4/bta498@bta4"

class IPPoolError(exceptions.Exception):
     def __init__(self, errno, errstr):
         self.errno = errno
         self.errstr=errstr
     def __str__(self):
         return `self.errno,self.errstr`

from IPPoolClass import IPPool

class AutoIP:
    """Manages IP address pools.
        The idea is to instantiate one of these objects based on
        criteria such as router, local, interface and service_id.
        This may return any number of pools.
        The spares flag indicates, if we want to only get pools that have
        available IP's or also the depleted pools.
    """
       
    def __init__(self, router=None, locale=None, interface=None, hint=None,
                 service_type=None, preference=100,poolid=0, spares=0, product=None, status="SPARE", blocksize=None, debug=0):
 
	self.reduced=0			# if 1, pool reduction has been run
        self.router=router
        self.status=status
        self.locale=locale
        self.interface=interface
        self.service_type=service_type
        self.preference=int(preference)
        self.pools=None
        self.poollist=[]		# Collection of IPPool objects
        self.poolcount=0
        self.poolid=int(poolid)
        self.product=product
	self.rules=None
	self.hint=hint
	self.debug=debug

	if self.debug>2: print "In AutoIP, product:", self.product

	if self.product:
	    
            self.db = dbconnect.dbConnect(dbc=DBHANDLE)

	    # Apply a ruleset to given product
	    try:
	        self.rules=IPRules.ProductRules(self.product)
	    except:
		raise IPPoolError, (20, "Rules Violation, Unknown product")

	    # Check if have required criteria for current ruleset
#	    if self.rules.PR_locale and not self.locale:
#		raise IPPoolError, (13, "Rules Violation, Must supply a locale")
#	    if self.rules.PR_router and not self.router:
#		raise IPPoolError, (13, "Rules Violation, Must supply a router")
#	    if self.rules.PR_service_type and not self.service_type:
#		raise IPPoolError, (13, "Rules Violation, Must supply a service_type")
#	    if self.rules.PR_interface and not self.interface:
#		raise IPPoolError, (13, "Rules Violation, Must supply a interface")

            self.getpools(poolid=self.poolid, spares=spares, status=status)

    def __str__(self):
	return("locale=%s router=%s product=%s" % (self.locale, self.router, self.product))

    def __del__(self):
	if self.product:
	     self.db.close()

    def allocate(self, poolid=0, blocksize=32):
	""" Allocate an IP block out of an address pool and update the db
            We must have a unique pool to work with. This is done by 
            calling the reduce_pools() method.
            Can pass the poolid to select unique pool to use also.
	    The size parameter is the mask of the requested pool as in /30 without /
        """
        blocksize=int(blocksize)

	if self.debug>2: print "allocating:",blocksize

	if (blocksize > self.rules.PR_min_mask) or (blocksize < self.rules.PR_max_mask):
	      raise IPPoolError, (13, "Rule Violation, requested block must be between /%d and /%d" % (self.rules.PR_max_mask, self.rules.PR_min_mask) )

	if not self.reduced:
	    if self.debug>3: print "Reducing pools"
	    self.reduce_pools(blocksize=blocksize)

        if poolid and not self.poolid:
            self.poolid=int(poolid)

        if not self.poolid:
            if len(self.poollist)==0:
                raise IPPoolError, (3, "No available pools for IP assignment (%d pools)" % len(self.poollist) )
            elif not len(self.poollist)==1:
                raise IPPoolError, (3, "Too many pools in instance for IP assignment (%d pools)" % len(self.poollist) )

        ips = self._ip2int(self.poollist[0].start_ip_number)
        ipe = self._ip2int(self.poollist[0].end_ip_number)

        if ips == ipe:                        # No numbers left in the pool.
            raise IPPoolError, (4,"No numbers left in pool: %s to %s" % (self.poollist[0].start_ip_number, self.poollist[0].end_ip_number) )


	bits=self._mask2int(blocksize)
	if self.debug: print "Request is for /%d (%d bits)" % (blocksize, bits)
	# Find suitable block
	found=0
	new_ips=0
	x = int(ips % bits)
	if x==0:
	    new_ips = ips
	else:
	    new_ips= ips + (bits - x)

	top_leftover = (new_ips - ips)

	if (new_ips + bits) <= ipe:
		found=1
		new_ips_str=self._ip2str(new_ips)	# Allocated Block
		new_ipe=new_ips + bits
		new_ipe_str=self._ip2str(new_ipe)

		if top_leftover>0:			# save the top gap if any
		    top_ips=ips
		    top_ips_str=self._ip2str(top_ips)
		    top_ipe=new_ips
		    top_ipe_str=self._ip2str(top_ipe)

		# Bottom leftovers			# save the bottom gap if any
		bot_ips = new_ipe
		bot_ipe = ipe 
		bot_leftover=bot_ipe - bot_ips
		if bot_leftover:
		    bot_ips_str=self._ip2str(bot_ips)
		    bot_ipe_str=self._ip2str(bot_ipe)
	else:
                raise IPPoolError, (3, "No available pools for request IP block size")

	if not found:
            if len(self.poollist)==0:
                raise IPPoolError, (3, "No available pools for IP assignment (%d pools)" % len(self.poollist) )
	    
        ipstr = self._ip2str(new_ips + bits)            # next avail IP
        
        # Close the pool we used. Take 1 sec from the update time to avoid 
	# a duplicate pool trying to be inserted error

	try:
            query="""update bta4.ip_pool set end_time=sysdate-(1/86400)
                     where unallocated_ip_id=%d and product='%s'
                  """ % (self.poollist[0].unallocated_ip_id, self.product)
	    if self.debug>3: 
	        print "Updating available pools ..."
	        print query

	    if UPDATE:
                self.db.execute(query)
              
	    # Add the new pool(s) with the leftovers
	    # Add alocated pool, status="USED"
            pool_id=self.add_pool(start_ip_number=new_ips_str,
                end_ip_number=new_ipe_str,
                router=self.poollist[0].router, locale=self.poollist[0].locale,                       
                interface=self.poollist[0].interface,
                service_type=self.poollist[0].service_type,
                preference=self.poollist[0].preference,
		status="USED")

	    if top_leftover>0:	# status="SPARE"	
                self.add_pool(start_ip_number=top_ips_str,
                    end_ip_number=top_ipe_str,
                    router=self.poollist[0].router, locale=self.poollist[0].locale,                       
                    interface=self.poollist[0].interface,
                    service_type=self.poollist[0].service_type,
                    preference=self.poollist[0].preference, status="SPARE")

	    if bot_leftover:	# status="SPARE"	
                self.add_pool(start_ip_number=bot_ips_str,
                    end_ip_number=bot_ipe_str,
                    router=self.poollist[0].router, locale=self.poollist[0].locale,                       
                    interface=self.poollist[0].interface,
                    service_type=self.poollist[0].service_type,
                    preference=self.poollist[0].preference, status="SPARE")

	    if UPDATE:
                self.db.commit()
        except:
		self.db.rollback()
		raise

	return (new_ips_str, blocksize, pool_id)

    def reduce_pools(self, blocksize=32):
        """Reduce the poollist to a single pool that we can use to allocate from.  
	   Using the following rules:

        1. If we have no pools, return 
        2. If 1 or more pools 
	     2a. Keep Pools that satisfy the blocksize of request.
	     2b. Keep Pools that match given criteria (router, service, local, interface).
             2c. Pools > 1 , Keep Pool with highest preference.
             2d. Pools > 1 , If equal preference, take pool with lowest IP number.
           else
                return
        """

	self.reduced=1

	# 1.
        if len(self.poollist)<=1:
            return

        router=self.poollist[0].router
        locale=self.poollist[0].locale
        service_type=self.poollist[0].service_type
        interface=self.poollist[0].interface
        poolid=self.poollist[0].unallocated_ip_id
        highwater=self.poollist[0].preference

	if self.debug>3: print " 1) Poolist size is", len(self.poollist)

	# 2a) Keep pools with blocks that satisfy request
        newlist=[]
	bits=self._mask2int(blocksize)
	for pool in self.poollist:
	    if self.debug>3: print "2a) check pool=", pool.start_ip_number,pool.end_ip_number
	    found=0
	    if not pool.preference:		# ignore pools with pref of 0 or None
	 	continue
	    x = int(pool.ip_start % bits)
	    if x ==0: 
		 if (pool.ip_start + bits) <= pool.ip_end:
		    found=1
		    new_ips=pool.ip_start
	    else:
		new_ips= pool.ip_start + (bits - x)
		if (new_ips + bits) <= pool.ip_end:
		    found=1
		else:
		    new_ips=0

	    #print "    new_ips=", self._ip2str(new_ips), x
	    if found:
		newlist.append(pool)

	self.poollist=newlist

	if self.debug>3: print "2a) Pool size check, Poolist size is", len(self.poollist)

        if len(self.poollist)==1:		# Have one, we're done
	    self.poolid=self.poollist[0].unallocated_ip_id
	    return
	elif len(self.poollist)==0:		# None available
	    self.poolid=0
	    return


        # 2b) Find the highest preference and check all required criteria matches
	newlist=[]
        for pool in self.poollist:
            if self.router and (pool.router != self.router): continue
            if self.locale and (pool.locale != self.locale): continue
            if self.service_type and (pool.service_type) != self.service_type: continue
            if self.interface and (pool.interface) != self.interface: continue
	    newlist.append(pool)	    

	self.poollist=newlist

	if self.debug>3: print "2b) Criteria Check, Pool size now is", len(self.poollist)

        # 2c) Find the highest preference and check all required criteria matches
        for pool in self.poollist[1:]:

            if pool.preference and pool.preference>highwater:
                highwater=pool.preference

        newlist=[]
        # Keep all pools with a preference equal to the highwater mark
        for pool in self.poollist:
            if pool.preference >= highwater:
                newlist.append(pool)
                
        if len(newlist)>=1:
            self.poollist=newlist

	if self.debug>3: print "2c) Preference check, Pool size now is", len(self.poollist)
            
        # Found 1 pool with higher preference. We're done.
        if len(self.poollist)==1:
            self.poolid=self.poollist[0].unallocated_ip_id
            return
            
        newlist=[]
        # 2d) Look for lowest IP
        lowestip=self.poollist[0].ip_start
        newlist=self.poollist[:1]
        
        for pool in self.poollist[1:]:
            if pool.ip_start < lowestip:
                lowestip=pool.ip_start
                newlist=[pool]
                self.poolid=pool.unallocated_ip_id
                
        self.poollist=newlist

	if self.debug>3: print "2d) Use lowest IP, Pool size now is", len(self.poollist),self.poolid
        self.poolid=poolid
	if self.debug>3: print "    -->",self.poolid
        
        return
       
    def add_pool(self, start_ip_number=None, end_ip_number=None,
                 router=None, locale=None, interface=None,
                 service_type=None, preference=100, status="SPARE"):
        """ Add a new IP address pool. 
        """
            
	if self.debug>2>2:
	    print "Adding avail Pool: ips=",start_ip_number, "ipe=",end_ip_number, "loc=",locale

        if not start_ip_number or not end_ip_number:
	    raise IPPoolError, (1,"No IP number(s) given")

	preference=int(preference)

	if self.debug: print "Validate: ",start_ip_number, end_ip_number, status

        ips=self._ip2int(start_ip_number)
        ipe=self._ip2int(end_ip_number)

        # Check if IP pool is cool.
        if not self._checkip(start_ip_number) or not self._checkip(end_ip_number):
            self.db.rollback()
            raise IPPoolError, (1,"Invalid IP number given")

	# Check against the current ruleset
	if not self.rules.PR_RFC1918:
	    for (lo,hi) in self.rules.RFC1918_ips:
	        if ((ips >= lo and ips < hi) and (ipe >=lo and ipe < hi)): 
		    raise IPPoolError,(18,'Rules Violation, RFC1918 blocks not permitted')

	if not self.rules.PR_public:
	    ok=0
	    for (lo,hi) in self.rules.RFC1918_ips:
	        if ((ips >= lo and ips < hi) and (ipe >=lo and ipe < hi)): 
		    ok=1
	    if not ok:
		raise IPPoolError,(18,'Rules Violation, public blocks not permitted')

	# Maybe just locale as a minimum
        # Check if we have minm info
        #if not router or not locale:
        if not locale:
            self.db.rollback()
            #raise IPPoolError, (2,"Router or Locale cannot be empty")
            raise IPPoolError, (2,"Locale cannot be empty")
        
        # Check for any overlap with any IP pool
        # Check if new start_ip or end_ip fall fall within any existing pool range

        if ips>ipe:
            self.db.rollback()
            raise IPPoolError,(8,'Invalid pool, Start IP > End IP')
        
        # ipe and ips are longs. We're done doing comparisons so using %s below 
        # with 'L' removed to avoid overflow.
        ips = string.replace(str(ips), 'L','')
        ipe = string.replace(str(ipe), 'L','')

        query="""select unallocated_ip_id, start_ip_number, end_ip_number
                 from bta4.ip_pool
                 where product='%s' and
		 status!='USED' and
                 sysdate between start_time and end_time
                 and ( ( %s >= ip_start and %s < ip_end) or
                       ( %s > ip_start and %s <= ip_end))
              """ % ( self.product, ips, ips, ipe, ipe)


        self.db.execute(query)
        res= dbconnect.ResultList(self.db)
        
        if len(res)>0:
            self.db.rollback()
            raise IPPoolError, (9,"Trying to insert ovelapping IP Pool: poolid %d has %s -> %s" % (res[0][0],res[0][1], res[0][2]) )
        
        query="""select bta4.unallocated_ip_id_seq.nextval from dual"""
        self.db.execute(query)
        nextval= int (dbconnect.ResultList(self.db)[0][0])

        if interface and service_type:        
             query="""insert into bta4.ip_pool 
                 (unallocated_ip_id, locale, router, service_type, 
                  interface, start_ip_number, end_ip_number, start_time,
                  end_time, preference, ip_start, ip_end, product, status) values 
                 (%d, '%s','%s','%s','%s','%s','%s',sysdate, trunc(sysdate)+999999, %d, %s, %s, '%s','%s')
              """ % (nextval, locale, router, service_type, interface, start_ip_number,
                     end_ip_number, preference, ips, ipe, self.product, status)

        elif interface:
             query="""insert into bta4.ip_pool 
                 (unallocated_ip_id, locale, router, 
                  interface, start_ip_number, end_ip_number, start_time,
                  end_time, preference, ip_start, ip_end, product, status) values 
                 (%d, '%s','%s','%s','%s','%s',sysdate, trunc(sysdate)+999999, %d, %s,%s,'%s','%s')
              """ % (nextval, locale, router, interface, start_ip_number,
                     end_ip_number, preference, ips, ipe, self.product, status)

        elif service_type:
             query="""insert into bta4.ip_pool 
                 (unallocated_ip_id, locale, router, service_type, 
                  start_ip_number, end_ip_number, start_time,
                  end_time, preference, ip_start, ip_end, product, status) values 
                 (%d, '%s','%s','%s','%s','%s',sysdate, trunc(sysdate)+999999, %d, %s, %s, '%s','%s')
              """ % (nextval, locale, router, service_type, start_ip_number,
                     end_ip_number, preference,ips, ipe, self.product, status)

        else:
             query="""insert into bta4.ip_pool 
                 (unallocated_ip_id, locale, router, 
                  start_ip_number, end_ip_number, start_time,
                  end_time, preference,ip_start, ip_end, product, status) values 
                 (%d, '%s','%s','%s','%s',sysdate, trunc(sysdate)+999999, %d, %s, %s, '%s','%s')
              """ % (nextval, locale, router, start_ip_number,
                     end_ip_number, preference, ips, ipe, self.product, status)

	if self.debug>3: print query
	if UPDATE:
            self.db.execute(query)
#            self.db.commit()

        return nextval                # Pool id just added

    def close_pool(self, poolid=0):
        """Close off an IP pool, must have a single IP pool to do this"""

        if poolid:
            self.poolid=int(poolid)
            
        if not self.poolid:
            id=self.poollist[0].unallocated_ip_id
            if not len(self.poollist)==1:
                raise IPPoolError, (3, "Too many pools in instance for IP assignement (%d pools)" % len(self.poollist) )                                                   
                                                                                     
        else:   
            id=self.poolid
            
        query="""update bta4.ip_pool set end_time=sysdate
                 where unallocated_ip_id=%d
                """ % int(id)
        self.db.execute(query)

    def getpools(self, poolid=0, spares=0, status=None):
        """ Pull all the pools that qualify from table, shouldn't be too many.
            interface or service_type may not exist so we include these pools also
        """

        if poolid:
            qstr="unallocated_ip_id=%d" % int(poolid)
        else:
            qstr="sysdate between start_time and end_time"

	    if status:
                qstr=qstr + " and status='%s'" % status

            if self.hint:
                prefix=string.split(self.hint, '.')
                qstr=qstr + " and START_IP_NUMBER like '%s.%%'" % prefix[0]
            if self.locale and self.locale!='Any':
               qstr=qstr + " and locale='%s'" % self.locale
            if self.router:
               qstr=qstr + " and router='%s'" % self.router
            if self.interface:
               qstr=qstr + " and lower(interface)=lower('%s')" % self.interface
            if self.service_type:
               qstr=qstr + " and service_type='%s'" % self.service_type

        if spares:
            qstr=qstr + " and ip_start < ip_end"

	# and status='%s'
        query="""select unallocated_ip_id, locale, router, service_type, interface,
                 start_ip_number, end_ip_number, to_char(start_time, 'dd-mon-yyyy hh24:mi:ss'),
                 to_char(end_time, 'dd-mon-yyyy hh24:mi:ss'),preference,ip_start, 
		 ip_end, product, status from bta4.ip_pool
                 where product = '%s' and
                 %s
                 order by locale, router, service_type, interface, preference
              """ % (self.product, qstr)

	if self.debug>3: print "getpools:",query
	
        self.db.execute(query)
        self.pools = dbconnect.ResultList(self.db)

        self.poollist=[]
        for pool in self.pools:
            self.poollist.append(apply(IPPool, tuple(pool)))

        if len(self.poollist)==1:
            self.poolid=self.poollist[0].unallocated_ip_id

    def listpools(self, poolid=0, locale=None, router=None,
                  service_type=None, interface=None, status=None,**kwargs):
        """return list of ippools, convenient for a Zope call"""

        if locale: self.locale=locale
        if router: self.router=router
        if service_type: self.service_type=service_type
        if interface: self.interface=interface

        if not self.poolid and poolid:
            self.poolid=poolid

        if not self.poollist:
            self.getpools(poolid=self.poolid, status=status)

        thelist=[]
        for pool in self.poollist:
            thelist.append(pool.__dict__)

        return thelist

###
# Private methods

    def _checkip(self, ipstr):
        """Check if Legit IP Format"""

        ipbytes = string.split(ipstr,'.')        
        if len(ipbytes)!=4:
            return None 

        for ipb in ipbytes:
            try:
                ipi=int(ipb)
            except ValueError:
                return None

            if ipi<0 or ipi>255:
                return None

        return 1

    def _ip2int(self, ipstr):
        """Turn an ip quad into an ip integer"""

        ipbytes=string.split(ipstr,'.')
        if len(ipbytes) != 4:
            return -1

        try:
            ipint = long(ipbytes[0]) << 24
            ipint = ipint + (long(ipbytes[1]) << 16)
            ipint = ipint + (long(ipbytes[2]) << 8)
            ipint = ipint + long(ipbytes[3])
        except ValueError:
            return -1

        return ipint
 

    def _ip2str(self, ip):
        """Convert int IP into quad IP"""

        ip=long(ip)
        byte1 = int ((ip & 0xFF000000) >> 24)
        byte2 = int ((ip & 0x00FF0000) >> 16)
        byte3 = int ((ip & 0x0000FF00) >> 8)
        byte4 = int  (ip & 0x000000FF)

        return "%d.%d.%d.%d" % (byte1, byte2, byte3, byte4)

    def _mask2int(self, mask):
        """return number of bits in the mask so we can do a div test
       later on network vailidity
        """
        if type(mask)==type(""):
            mask=int(mask)
        n = 32 - mask

        bits= long(pow(long(2),n))
        return bits

    def _dump(self):
        """dump the ip pool to stdout"""

        for pool in self.ippools:
            print pool


if __name__ == "__main__":
    "test bits"

    #sys.exit(0)        # disable testing

    # router, locale, interface, service_type
    # returns a list of IPPool objects

    line="fred"
    while line and line[0]!='\n':
        pools = AutoIP(router='cor5' , locale='bur', product='CCIP')

	sys.stdout.write("Requested Block size: ")
	req=sys.stdin.readline()

        block=pools.allocate(blocksize=int(req)) 

        print "Allocated block is:", block

        print "Available blocks for this product is now:"
        ptmp = AutoIP(locale='bur', product='VPNSC', status="SPARE")
	i=0
	for p in ptmp.listpools():
	    i=i+1
	    print i,"ips=",p['start_ip_number'],"ipe=", p['end_ip_number'],p['start_time'], p['end_time'], p['unallocated_ip_id']

	#del(ptmp)
	#del(pools)
	

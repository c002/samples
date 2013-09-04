#!/opt/bta4/bin/python
#
# adsl2rads.py
#
# Reads Berkeley db files of Radius Acc records and generates entries for 
# radconnections. Should run daily - window hrs (see below for "window")
#
# It applies the follogin filters:
#	1. Throw sessions which have an IP registered in BTA
#	2. Direct CCIP realms into another table.
#
# Daily processing should include a time range extending a little before and
# beyond a single day as we need to see data around that time.
#
# log levels
#	0 - quiet, bad idea.
#	1 - Error messages and warning, normal
#	2 - Informational
#	3 - max debug
# 
# Notes: 30000 users takes about 1hour to processes using 
#	 FastInsert -- April 2005
#
# Try -h for options
#
# 
# 
#

import os, sys, string,time,getopt
sys.path.append('/opt/bta4/lib/python2.3')
sys.path.append('/opt/bta4/lib')
sys.path.append('/opt/servicenet/lib')
sys.path.append('/export/00/python/lib/python2.3/site-packages')
sys.path.append('/export/00/python/lib/python2.3')
import bsddb3

testenv=0	# 1 if running in testenvironment
DEBUG=0

from sendmail import SendMail
import cx_Oracle
import datetools

os.environ['ORACLE_HOME'] = '/opt/oracle/product/7.3.2'
DatabaseError=cx_Oracle.DatabaseError

connection_type=5

# Constants, Record creation options
NOINSERT=0	# Dont insert anything
SQLINSERT=1	# Create a dump file of sql statements for sqlplus import
DBINSERT=2	# Insert straight into oracle
FASTINSERT=3	# Insert straight into oracle
FLUSHCOUNT=5000	# Number of records to write at once into db
CCIP_DOMAINLIST=('.connectvpn.com.au',
	 	 '.vpdn.aapt')

# Stream labels to control output direction
DEFAULT=0
CCIP_STREAM=1
BTA_STREAM=2	
RADS_STREAM=3

if testenv==1:
    dbstr="cust/cust_mt2@MT2_oket"
    db_orinstr="cust/cust_mt2@MT2_oket"
    e3_rads="e3_radconnections"
    e3_adsl="adsl_connections"
    e3_sum="adsl_summary"
    bta_rads="replace_this"
    ccip_rads="bta4_summary.ccip_radconnections"
    btaip="bta4.customer_ip_address"
else:
    dbstr="super/mario2@cust"
    db_orinstr="cust/custropw@traf2_orin"
    e3_rads="cust.e3_radconnections"
    e3_adsl="cust.adsl_connections"
    e3_sum="cust.adsl_summary"
    bta_rads="replace_this"
    ccip_rads="bta4_summary.ccip_radconnections"
    btaip="bta4.customer_ip_address@bta4"

RADHOSTS=('radio','radii','radar','siri')
_32bits=4294967296L

#sendto="adsllogs@connect.com.au"
sendto="harryr@connect.com.au"
#sendto="radprocess@connect.com.au"
sendfrom="nm"

# window is the db data grab window, this is the number of hours prior 
# yesterday. 
window=24*60*60	# seconds, 72 hours

class Log:
    def __init__(self,level=0):
        self.DebugFD = sys.stderr
        self.level = level
	self.rep=""

    def report(self,what, lev, mesg):
        if self.level >= lev:
            self.rep=self.rep+"LOG(%s): %s\n"% (what, mesg)

    def msg(self,what,lev, mesg):
        if self.level >= lev:
            self.DebugFD.write("LOG(%s): %s\n"%(what, mesg))
	    self.report(what,lev,mesg)

    def __str__(self):
	return(self.rep)

def get_adslconnections(cur, starttime, endtime, data_start,data_end, sessionkeys, db_fds, btaips, merge_short=0):
    """Loads all records from Berkeley db file for a given time range.
    """

    # Get all records from dbfiles for user.    

    records=[]
    adslentries=[]
    merged=None

    for ses in sessionkeys:
	dstr,host,sid,user=string.split(ses,':',3)
	dbf="adsl.%s.%s.db" % (host, dstr)
	db=db_fds[dbf]

	key=sid+":"+user

	if not db.has_key(key):
	    continue		# The record must be in one of the other db files.
        data = db[key];
        fields = string.split(data,chr(0))

#	try:
#	    if not db.has_key(key):
#		print "Barf, key: %s not found in %s" % (key,dbf)
#            data = db[key];
#            fields = string.split(data,chr(0))
#	except:
#	    raise

	stream=DEFAULT
	# Apply filters here

	# CCIP filter.  This data needs to make its way into BTA land.
        # into bta4_summary.ccip_radconnections
        for dom in CCIP_DOMAINLIST:
	    if fields[16][-len(dom):]==dom:
	        stream=CCIP_STREAM 					# redirect CCIP output
#        if stream==1: print "-", fields[16], stream
	
	# BTA Filter. Don't count stuff BTA will
	if (fields[7]+'/32') in btaips:	
	   stream=BTA_STREAM
	   #continue

	# TODO. Merge short session filter. Speed things up.
	# Collapse seesions <=900 seconds into single sessions. These Should only
	# ever be Stop records and we only take these if occuring for the day we're
	# running. We need to create the same key for these for the user

 	try:
	    if not merge_short:	# Throw really short ones
                if ((int(fields[4]) - int(fields[3]))<=5):
                    continue
                if ((int(fields[4]) - int(fields[3]))<=30) and float(fields[10])<100:       
                    continue
	except:
	    continue

	if merge_short:
 	    # with AAPT we get same from and to times.
	    if ((int(fields[4]) - int(fields[3]))>0) and ((int(fields[4]) - int(fields[3]))<=900) and (int(fields[4])>=starttime):
	        if (fields[2]) !='Stop':
		    print "* * * *Barf, unusual"
	        else:
		    log.report("merge",5,"--- merge: %s " % string.lower(fields[1]) )
		    if not merged:
		        merged=adslEntry(stream=stream)

		    merged.session_id='00MERGED'
		    merged.session_id_org='00MERGED'
		    merged.user_id=string.lower(fields[1])
		    merged.type=fields[2]
		    merged.port=fields[6]		# any port will do
		    merged.ip_address="0.0.0.0"	# could be anything
		    merged.from_time=int(starttime)
		    merged.to_time=int(endtime)-1
		    merged.ip_address=fields[7]
		    merged.service=fields[8]
		    merged.domain=fields[16]
		    merged.locale=fields[17]
		    merged.server=fields[18]
		    merged.input_octets=merged.input_octets + long(fields[9])
		    merged.output_octets=merged.output_octets + long(fields[10])	

	    else:
	        adslentries.append(adslEntry(fields, stream=stream))
	else:
	    adslentries.append(adslEntry(fields, stream=stream))

        if merged:
	    log.report("merged",4,"--- Merged: %s, %s" % (merged, len(adslentries)))
	    adslentries.append(merged)

    return adslentries

def get_btaips(cur, ts, te):

    query="""select unique ip_cidr from %s where instr(IP_CIDR, '/32') > 0
                and %d>=start_time and %d<end_time 
          """ %(btaip, ts, ts)

    cur.execute(query)
    results=cur.fetchall()
    dalist=[]
    for x in results:
        dalist.append(x[0])

    return dalist
 
def get_adslusers(db_fds, sessions=0, usrlist={}):
    """If session, than get create session keys for each user.
       If not session, get a userlist only.
    """

    count=0

    for dbf in db_fds.keys():
        db=db_fds[dbf]
        fields=string.split(dbf,'.')
        dstr=str(fields[2])
        srv=str(fields[1])
        try:
            record=db.first()
            while record:
                count=count+1
                (key, data) = record;
                fields = string.split(data,chr(0))
           
                sid,usr=string.split(key,':',1)

		# When we have a usrlist, we don't want to add to it.
		# So we can 2 passes.
                if usr!='0732781733c@aaptbusiness.net.au':
                    record=db.next()
                    continue

                if not sessions and not usrlist.has_key(usr):
                    usrlist[usr]=[]

		if not sessions:
                    record=db.next()
		    continue

                if not usrlist.has_key(usr):	# ignore users not in list
                    record=db.next()
		    continue

                # Session details for a user may be scattered between
                # Radius servers.
                nkey=dstr + ':' + srv + ':' + key
                
                usrlist[usr].append(nkey)       # Save all keys, incl any dups

                record=db.next()
	except:
            pass

    return usrlist

class adslEntry:
#  session_id, user_id, from_time, to_time, input_octets, output_octets, port, ip_address, type,
#  service, billed, domain, locale, server
#  ----
# ['0004EDE4', '0740310980@dsl.iig.com.au', 'Alive', '1059843191', '1059918679', '75488', 'cor3.for.connect.com.au/806', 'NULL', 'Framed-User', '141891', '378532', '0', '0', '0', '312301100470', '0', 'dsl.iig.com.au', 'for', 'radio']
#
    """Describes a single radius ADSL entry. This can be an Alive or Stop record.
       There's no sanity checking on the arithmetic on these objects.
    """
   
    def __init__(self,rec=None,**kwargs):

	if not rec:
	    rec=["","","",0,0,0,"","","",0,0,0,0,0,"",0,"","",""]

	self.session_id=rec[0][-8:]	# e3_radconnection can only handle 8 chars
	self.session_id_org=rec[0]	# We need this for the "update" later
	self.user_id=string.lower(rec[1])	# Force to lower case
	self.from_time=int(rec[3])
	self.to_time=int(rec[4])
	self.input_octets=long(rec[9])
	self.output_octets=long(rec[10])
	self.port=rec[6]
	self.ip_address=rec[7]
	self.type=rec[2]
	self.service=rec[8]
	self.billed=0
	if len(rec)<12:
	    self.domain='NULL'
	else:
	    self.domain=rec[16]
	if len(rec)<17 or not rec[17] or rec[17]=="None": 
	    self.locale="NA"
	else:
	    self.locale=rec[17]
	if len(rec)<18 or not rec[18] or rec[18]=="None": 
	    self.server="NA"
	else:
	    self.server=rec[18]

	# Set some flags to control processing based on any filters
	self.stream=0
	if kwargs and kwargs.has_key('stream'):
	    self.stream=kwargs['stream']

    def compare(self,b):
	""" Compare self with another instance , except session_id and billed"""

	res=1
	if (self.user_id != b.user_id): res=0
	if (self.from_time != b.from_time): res=0
	if (self.to_time != b.to_time): res=0
	if (self.input_octets != b.input_octets): res=0
	if (self.output_octets != b.output_octets): res=0
	if (self.port != b.port): res=0
	if (self.ip_address != b.ip_address): res=0
	if (self.service != b.service): res=0
	#	if (self.billed != b.billed): res=0

	return res

    def __sub__(a,b):
	c=adslEntry()	

	c.input_octets = a.input_octets - b.input_octets
	c.output_octets = a.output_octets - b.output_octets

	# Special cutover to 64bit. 
        # Need to compensate for obscene jump in usage for those that have wrapped already.
	# Should not be possible to wrap in 1 day (~2GB max possible).
	#if b.to_time < 1049731200:
	#    while c.input_octets > _32bits:
	#	c.input_octets = c.input_octets - _32bits
	#    while c.output_octets > _32bits:
	#	c.output_octets = c.output_octets - _32bits
	    
	c.session_id=a.session_id
	c.session_id_org=a.session_id_org
	c.user_id=a.user_id
	c.from_time=a.from_time
	c.to_time=a.to_time
	c.service=a.service
	c.port=a.port
	c.ip_address=a.ip_address
	c.type=a.type
	c.billed=a.billed
	c.domain=a.domain
	c.locale=a.locale
	c.server=a.server
	c.stream=a.stream
	
	return c

    def __add__(a,b):
	c=adslEntry()	
	c.input_octets = a.input_octets + b.input_octets
	c.output_octets = a.output_octets + b.output_octets


	c.session_id=a.session_id
	c.session_id_org=a.session_id_org
	c.user_id=a.user_id
	c.from_time=a.from_time
	c.to_time=a.to_time
	c.service=a.service
	c.port=a.port
	c.ip_address=a.ip_address
	c.type=a.type
	c.billed=a.billed
	c.domain=a.domain
	c.locale=a.locale
	c.server=a.server
	c.stream=a.stream

	return c

    def __str__(self):
	s= "%s,%s,%d,%d,%s,%f,%f,%s,%s,%s,%d,%s,%s,stream=%s" % \
	    (self.session_id,self.user_id,self.from_time,self.to_time,self.ip_address,
	     self.input_octets,self.output_octets, self.port, self.type,self.service, self.billed, self.locale, self.server,self.stream)
	return s

class Sessions:
    """Groups the adsl entries by session_id. Apply a duplicate check filter also"""
    
    def __init__(self, cur, inquery, starttime, endtime, sessionkeys, db_fds, btaips, dodbdump=None, insert_type=NOINSERT, merge_short=0):
	self.starttime=starttime
	self.endtime=endtime
	self.data_start=starttime - window
	self.data_end=endtime + window
	self.insertcount=0
	self.dbdump_fd=dodbdump[0]
	self.dbdump_fd2=dodbdump[1]
	self.dbdump_fd3=dodbdump[2]
	self.insert_type=insert_type
	self.btaips=btaips
	self.cur=cur
	#self.db=db
	self.inquery=inquery
	self.sessions={}

	self.record_count=0
	self.radsdata=[]
	self.ccipdata=[]	
	self.btadata=[]	
	self.alldata=[]	

	adslentries = get_adslconnections(self.cur, self.starttime, self.endtime,self.data_start, self.data_end, sessionkeys, db_fds, btaips, merge_short=merge_short)

	# Group the sessions. Session id is unique per router (while up). 
	# We probably be dealing with multiple routers. 
	# IP address provides a bit more uniquenes. from_time/to_time is not always the same
	# for the same session, bummer.

	for entry in adslentries:
	    key=entry.user_id+":"+entry.session_id
#	    key=entry.ip_address+":"+entry.session_id
	    if not self.sessions.has_key(key):
		self.sessions[key]=[]
	    self.sessions[key].append(entry)

	if len(self.sessions):
	    pass
	    #self.dupfilter()

    def dupfilter(self):
	"""Do a scan to warn about likely duplicate records. These may
	   exists if the radius dictionary doesn't contain the 
	   Control-Info and Service-Info attributes.
	   These are generally sequential session_id's but 
	   not always !
	"""

	cleansessions={}
	for key in self.sessions.keys():
	    if (len(self.sessions[key][0].session_id) != 
		len(filter(lambda x : x in string.hexdigits, self.sessions[key][0].session_id))):
		continue

	    sih=hex(string.atoi(self.sessions[key][0].session_id,16)+1)[2:]
	    sih='0'*(8-len(sih))+sih
	    sih=string.upper(string.split(sih,'L')[0])
	    sikey=self.sessions[key][0].ip_address+':'+sih
	    res=0
	    if self.sessions.has_key(sikey):
		    for item in self.sessions[key]:
			for i in range(0,len(self.sessions[sikey])):
		             res=item.compare(self.sessions[sikey][i])
			     if res:	
			         break
			if res:
			    break	
	    if res:
		    log.report("dupfilter",1,"Filtered duplicates, check radius dictionary (1 for each session shown only):")
		    log.report("dupfilter",1,"\n   1>%s\n   2>%s"%(self.sessions[sikey][i], item) )
	    else:
		    cleansessions[key]=self.sessions[key]

	self.sessions = cleansessions
	
	return
		
    def dump(self):
	"debug. Output all the session data"	

	print "#session_id,user_id,from_time,to_time,input_octets,output_octets, port, type,service"
	for ses in self.sessions.keys():
	    for item in self.sessions[ses]:	
		print item			# overloaded

    def update_adsl_connections(self):
	"""Set a flag on the Alive record that was used at the end of
	   the period, in case someting goes bad with the radius
	   logging in the near future.
	   We may have a gap in e3_radconnections data and need 
	   to know the previous alive record that we used last time. 
	   This is important as any reprocessing of those logs may 
	   introduce more recent Alive records which will have to be 
	   ignored.
	"""

	if self.finalalive:
	    input_octets= filter(lambda a : a in string.digits, str(long(self.finalalive.input_octets)))
	    output_octets=filter(lambda a : a in string.digits,str(long(self.finalalive.output_octets)))
		
 	    query="""update %s set billed=1 where
		     session_id='%s' and to_time=%d and from_time=%d and
		     user_id='%s' and port='%s' and type='%s' and
		     ip_address='%s' and output_octets=%s and 
		     input_octets=%s
		  """ % (e3_adsl, self.finalalive.session_id_org,
			 self.finalalive.to_time,	    
			 self.finalalive.from_time,	    
			 self.finalalive.user_id,	    
			 self.finalalive.port,	    
			 self.finalalive.type,	    
			 self.finalalive.ip_address,	    
			 output_octets, input_octets)

	    self.db.execute(query)
	    

    def dbdump(self, sql=0):

        # This is a hack to get around the unique constraint in e3_radconnections
	# which is port, output_octets, user_id, session_id. 
	# Make the port number of port the day + last 3 digits of port number

	d = time.strftime('%y%m%d',time.localtime(self.starttime))
	p = string.split(self.period.port,'/')
	self.period.port= p[0][-23:] + '/' + d

	# python >1.6 has no long 'L' issue, but 1.5 does.
	input_octets= filter(lambda a : a in string.digits, str(long(self.period.input_octets)))
	output_octets=filter(lambda a : a in string.digits,str(long(self.period.output_octets)))

	rads_table=e3_rads
	if self.period.stream==CCIP_STREAM:
	    rads_table=ccip_rads
	if self.period.stream==BTA_STREAM:
	    rads_table=bta_rads

	if self.period.user_id:
	    if sql:
	        stm="""insert into %s 
			(session_id, user_id, from_time, to_time, port, ip_address, service, 
			 input_octets, output_octets, domain,connection_type, locale, server, PROCESSED_DATE) 
			 values ('%s','%s',%d,%d,'%s','%s','%s',%s,%s,'%s',%d,'%s','%s',sysdate);
		    """ %(rads_table, self.period.session_id, self.period.user_id,
		       self.period.from_time, self.period.to_time,
		       self.period.port, self.period.ip_address,
		       self.period.service, input_octets, output_octets, self.period.domain, 
		       connection_type, self.period.locale, self.period.server)

		if self.period.stream==CCIP_STREAM:
	            self.dbdump_fd2.write(stm)
		elif self.period.stream==BTA_STREAM:
	            self.dbdump_fd3.write(stm)
		else:
	            self.dbdump_fd.write(stm)
		
	    else:
	        self.dbdump_fd.write('"%s","%s","%d","%d","%s","%s","%s","%s","%s","%s","%d","%s","%s"\n' %
		      (self.period.session_id, self.period.user_id,
		       self.period.from_time, self.period.to_time,
		       self.period.port, self.period.ip_address,
		       self.period.service, input_octets, output_octets, self.period.domain, 
		       connection_type, self.period.locale, self.period.server) )

    def collectdata(self, final=None):

	d = time.strftime('%y%m%d',time.localtime(self.starttime))
	p = string.split(self.period.port,'/')
	self.period.port= p[0][-23:] + '/' + d

	input_octets= filter(lambda a : a in string.digits, str(long(self.period.input_octets)))
	output_octets=filter(lambda a : a in string.digits,str(long(self.period.output_octets)))

	if self.period.user_id:
	    	self.alldata.append( 
			{":p1" : self.period.session_id, 
                	":p2" : self.period.user_id,
                	":p3" : self.period.from_time,
                	":p4" : self.period.to_time,
                	":p5" : self.period.port,
                	":p6" : self.period.ip_address,
                	":p7" : self.period.service,
                	":p8" : input_octets,
                	":p9" : output_octets,
                	":p10" : self.period.domain,
                	":p11" : connection_type,
                	":p12" : self.period.locale,
                	":p13" : self.period.server})
		if self.period.stream==CCIP_STREAM:
	    	    self.ccipdata.append( 
			{":p1" : self.period.session_id, 
                	":p2" : self.period.user_id,
                	":p3" : self.period.from_time,
                	":p4" : self.period.to_time,
                	":p5" : self.period.port,
                	":p6" : self.period.ip_address,
                	":p7" : self.period.service,
                	":p8" : input_octets,
                	":p9" : output_octets,
                	":p10" : self.period.domain,
                	":p11" : connection_type,
                	":p12" : self.period.locale,
                	":p13" : self.period.server})

		elif self.period.stream==BTA_STREAM:
	    	    self.btadata.append( 
			{":p1" : self.period.session_id, 
                	":p2" : self.period.user_id,
                	":p3" : self.period.from_time,
                	":p4" : self.period.to_time,
                	":p5" : self.period.port,
                	":p6" : self.period.ip_address,
                	":p7" : self.period.service,
                	":p8" : input_octets,
                	":p9" : output_octets,
                	":p10" : self.period.domain,
                	":p11" : connection_type,
                	":p12" : self.period.locale,
                	":p13" : self.period.server})

		else:
	    	    self.radsdata.append( 
			{":p1" : self.period.session_id, 
                	":p2" : self.period.user_id,
                	":p3" : self.period.from_time,
                	":p4" : self.period.to_time,
                	":p5" : self.period.port,
                	":p6" : self.period.ip_address,
                	":p7" : self.period.service,
                	":p8" : input_octets,
                	":p9" : output_octets,
                	":p10" : self.period.domain,
                	":p11" : connection_type,
                	":p12" : self.period.locale,
                	":p13" : self.period.server})

    def summarise(self):
	"""Reduces the radius accounting records into real sessions producing
	   something that can go into the e3_radconnections db table.
	   (Could do prorata perhaps one day.)
	"""

	for ses in self.sessions.keys():
	    #sys.stderr.write(".")
	    old=adslEntry()
	    earliest=adslEntry()
	    earliest.to_time=self.endtime
	    latest=adslEntry()
	    stop=adslEntry()
	    next=adslEntry()
	    from_time=self.endtime
	    flagged_from_time=None
 
	    self.finalalive=None 
	    self.flaggedalive=None

	    for item in self.sessions[ses]:
		log.report("record",3,str(item))

		# (1) get latest Alive record for the period
	        if (item.type=='Alive' and
	           item.to_time > self.starttime and
	           item.to_time <= self.endtime and
	           item.to_time>latest.to_time):
		        latest=item
	    		self.finalalive=item
		
		# (2) Look for Stop record within the period. Takes precedence over Alive record
	        if (item.type=='Stop' and
	           item.to_time > self.starttime and
	           item.to_time <= self.endtime): 
			if not old.from_time:
			    from_time=item.from_time
		    	if stop.to_time!=0:
		            log.report("summarise",1,"Multiple Stop records for same session");
		        else:
		            latest=item
	    		    self.finalalive=item

	        # (3) Get latest alive record before period start that has 
		# billed==1 (as this was used perviously)
	        if (item.type=='Alive' and
	           item.to_time < self.starttime and
	           item.to_time>old.to_time and item.billed==1):
		        self.flaggedalive=item
			flagged_from_time=item.to_time

	        # (4) Get latest alive record before period start. Under
		# healthy circumstances this should be the same as (3)
	        if (item.type=='Alive' and
	           item.to_time < self.starttime and
	           item.to_time>old.to_time):
		        old=item
			from_time=old.to_time

		# (5) get the first Alive record for the session if it exists to get from_time
		# Used for new sessions.
	        if (item.type=='Alive' 
		   and item.to_time > self.starttime and 
	           item.to_time <= self.endtime and
		   item.to_time < earliest.to_time and not old.from_time):
			earliest=item
			from_time=item.from_time

	    if self.flaggedalive:
		old=self.flaggedalive
		from_time=flagged_from_time
	    
	    self.period = latest - old	# overloaded
	    self.period.from_time=from_time
	  
	    self.record_count=self.record_count+1

	    if self.dbdump_fd and self.insert_type==NOINSERT:
		self.dbdump()
	    elif self.dbdump_fd and self.insert_type==SQLINSERT:
		self.dbdump(sql=1)
 	    elif self.insert_type==FASTINSERT:
	        self.collectdata()
	    else:
	        pass

	    log.report("-",3,"-"*72)

class FastInserter:
    """Collect the data to be inserted. Do a bulk write every self.flushcount
       records.
    """

    def __init__(self, cur, cur_orin):
	self.flushcount=FLUSHCOUNT

	self.cur=cur
	self.cur_orin=cur_orin
	self.alldata=[]
	self.ccipdata=[]
	self.radsdata=[]

	self.totalcount=0
	self.rtotal=0
	self.ctotal=0
 
	self.cur.arraysize = self.flushcount
 
	# data insert querys
	self.radsstm="""insert into %s 
            (session_id, user_id, from_time, to_time, port, ip_address, service, 
             input_octets, output_octets, domain,connection_type, locale, server, PROCESSED_DATE)
       	     values (:p1,:p2,:p3,:p4,:p5,:p6,:p7,:p8,:p9,:p10,:p11,:p12,:p13,sysdate)
		""" % e3_rads
	self.ccipstm="""insert into %s 
            (session_id, user_id, from_time, to_time, port, ip_address, service, 
             input_octets, output_octets, domain,connection_type, locale, server, PROCESSED_DATE)
       	     values (:p1,:p2,:p3,:p4,:p5,:p6,:p7,:p8,:p9,:p10,:p11,:p12,:p13,sysdate)
		""" % ccip_rads
#	self.btastm="""insert into %s 
#            (session_id, user_id, from_time, to_time, port, ip_address, service, 
#             input_octets, output_octets, domain,connection_type, locale, server, PROCESSED_DATE)
#       	     values (:p1,:p2,:p3,:p4,:p5,:p6,:p7,:p8,:p9,:p10,:p11,:p12,:p13,sysdate)
#		""" % bta_rads

	self.allstm="""insert into %s 
            (session_id, user_id, from_time, to_time, port, ip_address, service, 
             input_octets, output_octets, domain,connection_type, locale, server, PROCESSED_DATE)
       	     values (:p1,:p2,:p3,:p4,:p5,:p6,:p7,:p8,:p9,:p10,:p11,:p12,:p13,sysdate)
		""" % e3_sum

    def add(self, sessionobj):
	self.alldata.extend(sessionobj.alldata)
	self.ccipdata.extend(sessionobj.ccipdata)
	self.radsdata.extend(sessionobj.radsdata)

	#if len(self.alldata)>self.flushcount:
        #self.flush()

    def flush(self):

	self.totalcount=self.totalcount+len(self.alldata)
	self.rtotal=self.rtotal+len(self.radsdata)
	self.ctotal=self.ctotal+len(self.ccipdata)

	if DEBUG:
            print "\nFlush: ", self.totalcount
	    print "CCIP=",len(self.ccipdata), self.ctotal
	    if len(self.ccipdata): print self.ccipdata[-1]
	    print "RADS=",len(self.radsdata), self.rtotal
	    if len(self.radsdata): print self.radsdata[-1]
	    print "ALL=",len(self.alldata),self.totalcount
	    if len(self.alldata): print self.alldata[-1]

	if not NOINSERT:
            #if len(self.ccipdata): cur.executemany(self.ccipstm, self.ccipdata)
            if len(self.radsdata): cur.executemany(self.radsstm, self.radsdata)
            #if len(self.alldata): cur.executemany(self.allstm, self.alldata)
            if len(self.alldata): cur_orin.executemany(self.allstm, self.alldata)

        self.ccipdata=[]
        self.radsdata=[]
        self.alldata=[]

def usage():
    print "%s: -f dd-mon-yyyy -p path [-l n][-x] [-h][-d]" % (sys.argv[0],) 
    print "\t-h this help."
    print "\t-f Day to process. Typically yesterday"
    print "\t-l n. Loglevel, 1 is default and normal, 4 is debug"
    print "\t-m Merge short session <=900 seconds. Should always be 'Stop' records only."
    print "\t-p path. Path to the Berkeley db files to process"
    print "\t-d Produce dump file of records to be imported."
    print "\t-u Ignore unique constraint, do next."
    print "\t-x Test. Runs but does not insert anything into e3_radconnections"
    sys.exit(0)

if __name__ == "__main__":

    now=time.time()
    today=time.localtime(now)
    yesterday=time.localtime(now-86400)			# Yesterday sometime
    starttime=time.strftime('%d-%b-%Y',yesterday)
    endtime=time.strftime('%d-%b-%Y',time.localtime(now))
    
    try:
        opts,args=getopt.getopt(sys.argv[1:],'bmsxudhf:p:l:')
    except:
        sys.stderr.write("Invalid args\n")
        usage()

    loglevel=1
    insert_type=DBINSERT	
    dodbdump=0
    unique_const=1
    berkpath="./"
    merge_short=0
    for o,v in opts:
        if o=='-h':
            usage()
        if o=='-f':
            starttime=v
        if o=='-d':
            insert_type=NOINSERT
            dodbdump=1
        if o=='-s':
            insert_type=SQLINSERT
            dodbdump=1
        if o=='-b':
            insert_type=FASTINSERT
        if o=='-x':
            insert_type=NOINSERT
        if o=='-m':
            merge_short=1
        if o=='-l':
            loglevel=string.atoi(v)
        if o=='-y':
            ystr=v
        if o=='-p':
            berkpath=v

    # init log
    log=Log(loglevel)
    log.report(sys.argv[0],2,"Run starting at %s" % time.strftime('%d-%b-%Y %H:%M:%S',time.localtime(time.time())))

    ts=datetools.date2sec(starttime)
    te=ts+86400			#datetools.date2sec(endtime)
    endtime=time.strftime('%d-%b-%Y',time.localtime(te))

    # dbfile timestamp format is YYMMDD
    dstr=time.strftime('%y%m%d',time.localtime(ts))
    ystr=time.strftime('%y%m%d',time.localtime(ts-7200))	# Get prev day, was Daylight saving bug
    #print "dstr",dstr, ystr, starttime, endtime

    log.report("time",2,"Period Start=%s (%d) , Period End=%s (%d)"% (starttime, ts,endtime,te))

    fd=None 
    fd2=None
    fd3=None
    if dodbdump:
	if insert_type==SQLINSERT:
            fd=open("/var/tmp/%s.sql" % dstr,  "w") 
            fd2=open("/var/tmp/ccip-%s.sql" % dstr,  "w") 
            fd3=open("/var/tmp/bta-%s.sql" % dstr,  "w") 
	else:
            fd=open("/var/tmp/%s.dump" % dstr,  "w") 
            fd2=open("/var/tmp/ccip-%s.dump" % dstr,  "w") 
            fd3=open("/var/tmp/bta-%s.dump" % dstr,  "w") 

    db=cx_Oracle.connect("bta4_www/bta4_www98@bta4")
    cur=db.cursor()
    log.report("btaips",4,"get btaips list")
    btaips=get_btaips(cur, ts, te)
    log.report("btaips",4, "done btaips")
    db.close()

    if insert_type==FASTINSERT:
        db=cx_Oracle.connect(dbstr)
        db_orin=cx_Oracle.connect(db_orinstr)
        cur=db.cursor()
        cur_orin=db_orin.cursor()

    inquery=None

    log.report("btaips", 4,"get user list")
    db_fds={}	# Open all the dbfiles for fast access.
    for host in RADHOSTS:
        dbf="adsl.%s.%s.db" % (host,dstr)
	dbf_path="%s/%s" % (berkpath, dbf)
	log.report("dbfile",4, dbf_path)
        if os.path.exists(dbf_path):
	    log.report("input",1,"Adding file %s for processing" % dbf_path)
	    try:
	        db_fds[dbf]=bsddb3.hashopen(dbf_path, "r")
	    except bsddb3._db.DBOldVersionError,msg:	# Old v1.85
    		d=bsddb3.db.DB()
    		d.upgrade(dbf_path)			# upgrade on the fly
        	db_fds[dbf]=bsddb3.hashopen(dbf_path, "r")

    adslusers=get_adslusers(db_fds)		# just get users for that day

    for host in RADHOSTS:	# Add yesterdays sessions for reference
        dbf="adsl.%s.%s.db" % (host,ystr)
	dbf_path="%s/%s" % (berkpath, dbf)
	log.report("dbfile",4, dbf_path)
        if os.path.exists(dbf_path):
	      log.report("input",1,"Adding file %s for processing" % dbf_path)
	      try:
	          db_fds[dbf]=bsddb3.hashopen(dbf_path, "r")
	      except bsddb3._db.DBOldVersionError,msg:	# Old v1.85
    		  d=bsddb3.db.DB()
    		  d.upgrade(dbf_path)			# upgrade on the fly
        	  db_fds[dbf]=bsddb3.hashopen(dbf_path, "r")

    adslusers=get_adslusers(db_fds, sessions=1, usrlist=adslusers) # Just get sessions this time
    log.report("input",1,"Distinct users: %d " % len(adslusers))
    log.report("user count", 4,len(adslusers) )

    count=1 
    s=None

    if insert_type==FASTINSERT:	
        fi=FastInserter(cur, cur_orin)

    for sessionkeys in adslusers.values(): 
        s=Sessions(cur, inquery, ts, te, sessionkeys, db_fds, btaips, dodbdump=(fd,fd2,fd3), insert_type=insert_type, merge_short=merge_short)
        log.report("--",4, "%d process user: %s [sessions=%d]" % (count, sessionkeys[0], len(sessionkeys)) )
        s.summarise()
	if insert_type==FASTINSERT: 
	    fi.add(s)

	count=count+1
	if (count % FLUSHCOUNT==0) and insert_type==FASTINSERT:
	    fi.flush()

    if insert_type==FASTINSERT:
        fi.flush()		# Remainder
        db.commit()
        db_orin.commit()
    
    for bdb in db_fds.values():
	bdb.close()

#    if s:
#        log.report("insert",2,"%d records inserted into e3_radconnections" % s.insertcount)

    subject="ADSL connections report %s to %s" % (starttime, endtime)
    log.report(sys.argv[0],2,"Run completed at %s" % time.strftime('%d-%b-%Y %H:%M:%S',time.localtime(time.time())))

    if dodbdump:
	fd.close()
	fd2.close()
	fd3.close()

    cur.close()	# and commit
    cur_orin.close()	# and commit

    if testenv==0:
        SendMail(sendto, "nm", "nm", subject, log.rep)
    else:
	print log.rep

    day=time.strftime('%Y%m%d',time.localtime(time.time()))
    try:
        if testenv:
	    rfile="/tmp/adsl2rads-%s.log"% day
        else:
            rfile="/import/www/internal/www.off.connect.com.au/htdocs/Reports/ADSL/adsl2rads-%s.log" % day
        f=open(rfile,'a')
        f.write(log.rep)
        f.close()
    except:
        #print log
	pass
    sys.exit(128)	# To signal calling script a clean finish.

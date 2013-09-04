#!/opt/bta4/bin/python
#
# Read a Berkeley db file as produced by radiusProcess for ADSL
# connections and scanit. for debug purposes
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

RADHOSTS=('radio','radii','radar','siri')

testenv=0
INSERT=0

True=1
False=0

BASE="tmp"

if testenv:
    handle="cust/cust_test@MT2_oket"
    e3_adsl="cust.adsl_connections"
else:
    handle="super/mario2@cust"
    e3_adsl="cust.adsl_connections"

def usage():
    print "%s: -d yymmdd -y yymmdd | -f fname [ -u user] [-l] [-t]"
    print "\t -d yymmdd is timestamp to process"
    print "\t -y yymmdd is yestersdays timestamp required for calculating diff usages"
    print "\t -f fname  is dbfile to scan"
    print "\t -u user is user record to look for and output"
    print "\t -t user usage."
    print "\t -r attempt db recovery"
    print "\t -l dump to text"
    

def adsl_insert(dbo, fields):
    """Insert dbfile records into adsl_connections"""

    connection_type=5

    session_id=fields[0]
    user_id=fields[1]
    rtype=fields[2]
    from_time=int(fields[3])
    to_time=int(fields[4])
    duration=int(fields[5])
    port=fields[6]
    ipaddress=fields[7]
    service=fields[8]
    input_octets=long(fields[9])
    output_octets=long(fields[10])
    sched_id=int(fields[11])
    filler=fields[12]
    termreason=int(fields[13])
    callid=fields[14]
    dialednum=fields[15]
    domain=fields[16]
    locale=fields[17]
    server=fields[18]

    inquery="""insert into %s 
               (session_id, user_id, from_time, to_time,
               port, ip_address, service, input_octets,
               output_octets, domain, connection_type, locale, server) values 
               ('%s','%s',%d,%d,'%s','%s','%s',%s,%s,'%s',%d,'%s','%s') 
            """ % (e3_adsl, session_id, user_id, from_time, to_time,
                   port, ipaddress, service, input_octets, output_octets, 
		   domain, connection_type, locale, server)

    if INSERT:
        dbo.execute(inquery)
        dbo.commit()
    else:
        print inquery

    return

def process(dbo, user, dbfiles, dump=False, doip=False, dousage=False):

    count=0
    usrlist={}

    if type(dbfiles)==type(""):
        dbfiles=[dbfiles] 
    for dbf in dbfiles:
	if not user:
	    print "Scanning ",dbf

        try:
           db=bsddb3.hashopen(dbf, "r")
        except bsddb3._db.DBOldVersionError,msg:    # Old v1.85
           d=bsddb3.db.DB()
           d.upgrade(dbf)                     # upgrade on the fly
           db=bsddb3.hashopen(dbf, "r")

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
		if dump==True:
		    if doip==True:
		        print "%d: %s:%s" % (count, key, fields[7])
		    else:
		        print "%d: %s" % (count, key)
	        if not usrlist.has_key(usr):
		    usrlist[usr]=[]
		# Session details for a user may be scattered between
		# Radius servers.
		nkey=dstr + ':' + srv + ':' + key
		#print nkey
	        usrlist[usr].append(nkey)	# Save all keys, incl any dups
		
		
		#if 0 and usr=="canberrafertility@netspeed.com.au":
		if user and usr==user:
		    data=db[key]
		    fields = string.split(data,chr(0))
		    if dousage==True:
		        print "%s %s %d %d" % (dbf, user, int(fields[9])/1000000, int(fields[10])/1000000)
	  	    else:
		        print fields
		#else:
		#    print usr,"=",fields

                record=db.next()

        except KeyError, msg:
	    pass

        db.close()

    #print "usrlist=",len(usrlist)

    return count

if __name__=="__main__":

    try:
        opts,args=getopt.getopt(sys.argv[1:],'hcltpd:y:f:u:')
    except:
        sys.stderr.write("Invalid args\n")
        usage()
        sys.exit(-1)

    #yesterday=time.strftime("%y%m%d", time.localtime(time.time()-86400))
    #dbfile="adsl.%s.db" % yesterday
    #print dbfile
    dbfile=None
    ystr=None
    fname=None
    user=None
    dstr=None
    countonly=0
    recover=False
    dump=False
    doip=False
    dousage=False
    for o,v in opts:
        if o=='-h':
            usage()
        if o=='-r':
            recover=True
        if o=='-f':
            fname=v
        if o=='-l':
            dump=True
        if o=='-t':
            dousage=True
        if o=='-p':
            doip=True
        if o=='-u':
            user=v
        if o=='-d':
            dstr=v
        if o=='-y':
            ystr=v	# yesterday

    #dbo=dbconnect.dbConnect(dbc=handle)

    if not (dstr or fname):
	usage()
	sys.exit(-1)

    dbfiles=[]
    if fname:
	dbfiles.append(fname)
    else:
        for host in RADHOSTS:
	    if ystr:
	        dbf="adsl.%s.%s.db" % (host,ystr)	# Get yesterdays also
	        if os.path.exists(dbf):
	            dbfiles.append(dbf)
	    dbf=BASE + "/adsl.%s.%s.db" % (host,dstr)
	    if os.path.exists(dbf):
	        dbfiles.append(dbf)

    nrecs=process(None, user, dbfiles, dump=dump, doip=doip, dousage=dousage)

    if dousage==False and dump==False:
        print "Number of records: %d" % nrecs
    #dbo.close()

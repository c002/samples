#!/opt/python/bin/python
#
# Customer Traffic report of BTA compared to SNMP
#
# 
# 

import sys, string,time, getopt

sys.path.append('/opt/bta4/lib')
sys.path.append('/import/bta4/lib')
import dbconnect

d=time.localtime(time.time())
mon=d[1]
year=d[0]
months=[]

#db = dbconnect.dbConnect("bta4/bta498@lbta4_ochre")
#snmpdb = dbconnect.dbConnect("bta4_py/bta4_py98@bta4")
snmpdb = dbconnect.dbConnect("bta4_hist/krakenbta4@traf2_orin")
btadb = dbconnect.dbConnect("bta4_hist/krakenbta4@traf2_orin")
btalinkdb = dbconnect.dbConnect("bta4_www/bta4_www98@bta4")

False=0
True=1
simple=False

def flatten(object):
    """" turn any object into list """
    if type(object) == type([]):
        if len(object)>1:
            links = map(flatten,object)
        else:
            links = object[0]
    elif type(object) == type({}):
        links = map(flatten, object.values())
    else:   
        links = object
    return links


def dosnmp(cust,i,clip=0,months=months,fromdate=None, todate=None, linkid=None):
        snmpdata=[]

# Daylight savings and GMT adjustment

	sanity=0
	if clip: 
	    sanity=1
	    clip = (clip * 1000000) / 8

        ttup1=time.gmtime(time.time())
        ttup2=list(time.localtime(time.time()))
        ttup2[-1]=0                # -1 is supposed to work
        t1=time.mktime(ttup1)
        t2=time.mktime(tuple(ttup2))
        hours=int((t2 - t1) / (60 * 60))

        if fromdate and todate:
            d1,m1,y1=string.split(fromdate,'-')
            d2,m2,y2=string.split(todate,'-')
        else:
            d1=d2='1'
            m1,y1=string.split(months[i],'-')
            m2,y2=string.split(months[i+1],'-')

# sanity filter. Get some real bogus snmp stats sometimes
 
        if sanity:
#            filter=" and kbytes_out/period < 78000000 and kbytes_in/period < 78000000 "
#            filter=" and kbytes_out/period < 256000 and kbytes_in/period < 256000 "
            filter=" and kbytes_out/period < %f and kbytes_in/period < %f " % (clip, clip)
        else:
           filter=" "

# linkid filter
        if linkid:
            filter = filter + " and link_id in (%s) " % linkid

# kbytes_in/out are actually in bytes just to confuse people.

        query="""select sum(kbytes_in)/1000000, sum(kbytes_out)/1000000 
            from snmp_logs where customer in (%s) and
            timestamp>=to_date('%s-%s-%s %d','DD-MM-YYYY HH24') and           
            timestamp<to_date('%s-%s-%s %d','DD-MM-YYYY HH24')
            %s
            """ % (cust,d1,m1,y1,hours, d2,m2,y2,hours,filter)

        snmpdb.execute(query)
        res=dbconnect.ResultList(snmpdb)
        snmpdata.append(res[0])

        return(snmpdata)

def dobta(cust,i,months=months,fromdate=None, todate=None, linkid=None, debug=0):

        if fromdate and todate:
            d1,m1,y1=string.split(fromdate,'-')
            d2,m2,y2=string.split(todate,'-')
        else:
            d1=d2='1'
            m1,y1=string.split(months[i],'-')
            m2,y2=string.split(months[i+1],'-')

        # if theres a link need a list of subs... 
        if linkid:
            query = """SELECT distinct addr.sub as subcustomer
                FROM bta4.interface iface, bta4.customer_links link,
                        bta4.customer_routes route, bta4.customer_ip_address addr
                WHERE
                iface.interface_id = link.interface_id_fk
                AND addr.customer in (%s)
                AND link.link_id in (%s)
                AND link.link_id = route.link_id
                AND route.ip_id = addr.ip_id
		AND to_date('%s-%s-%s','DD-MM-YYYY') <= route.end_time
		AND to_date('%s-%s-%s','DD-MM-YYYY') >= route.start_time
                AND addr.customer=link.customer""" % (cust, linkid, d1,m1,y1, d2,m2,y2)
	    if debug>0:
		print query
            btalinkdb.execute(query)
            res=dbconnect.ResultList(btalinkdb)
            subs = `flatten(res)`[1:-1]
        else:
            subs = "0"
        btadata=[]

        query="""
            select bill_types.bill_type_name,
            sum(daily_summary.to_usage)/1000000 "to_usage",
            sum(daily_summary.from_usage)/1000000 "from_usage"
            from daily_summary,flow_types,bill_types
            where daily_summary.daystamp >= to_date('%s-%s-%s','DD-MM-YYYY')
            and daily_summary.daystamp < to_date('%s-%s-%s','DD-MM-YYYY')
            and daily_summary.cid in (%s)
            and daily_summary.flow_type = flow_types.flowid
            and daily_summary.bill_type_id = bill_types.bill_type_id
            and daily_summary.sub in (%s)
            group by bill_types.bill_type_name
            """ % (d1,m1,y1, d2,m2,y2, cust, subs)
	if debug>0:
	    print query
        btadb.execute(query)
        res=dbconnect.ResultList(btadb)
        
        touse=0
        fromuse=0
        for entry in res:
            touse=touse+entry[1]
            fromuse=fromuse+entry[2]
        total=['Total',touse,fromuse]
        res.append(total)
        return(res)

def process(snmp,bta,nohead, direction, reverse,month=None,fromdate=None,todate=None,linkid=None):
    if not nohead:
        if linkid: 
            custid = "%s-%s" % (cust,linkid)
        else:
            custid = cust
	if simple==False:
            print "Cid=%-10s %10s %14s %14s %14s %14s %14s %7s %14s" % \
                (custid,"Cache","External","Exchange","National","Local","Unbillled","Total","SNMP (diff)")
	else:
        #    print "Cid=%-10s %7s %14s" % (custid,"Netflow","SNMP (diff)")
            sys.stdout.write("Cid=%-10s" % (custid))

    entry=bta
    if direction=='to':
        x=1 
        r=1                # snmp out
        if reverse:        
            r=0                # snmp in
    else:
        x=2
        r=0                # snmp in
        if reverse:
           r=1                # snmp out
    
    traf={'unb':0, 'cache' :0, 'exch':0 ,'ext':0,'sattel':0,'total':0,'nat':0,'loc':0}
    for item in entry:        
        if item[0]=='Cache': traf['cache']=item[x]
        if item[0]=='Exchange': traf['exch']=item[x]
        if item[0]=='National': traf['nat']=item[x]
        if item[0]=='External': traf['ext']=item[x]
        if item[0]=='Sattel': traf['sattel']=item[x]
        if item[0]=='Local': traf['loc']=item[x]
        if item[0]=='Unbilled': traf['unb']=item[x]
        if item[0]=='Total': traf['total']=item[x]

    if not snmp[0][r]:
        snmp[0][r]=0        
    if not traf['total']:
        traf['total']=0        
    if traf['total']:   
        cachep=100 * traf['cache']/traf['total']
        extp  =100 * traf['ext']/traf['total']
        exchp =100 * traf['exch']/traf['total']
        natp  =100 * traf['nat']/traf['total']
        loc=100 * traf['loc']/traf['total']
        unbilp=100 * traf['unb']/traf['total']
        diff=(snmp[0][r]-traf['total'])*100/ traf['total']
    else:
        if not snmp[0][r]:
            diff=0 # don't have either, no difference.
        else:
            diff=100
        cachep=0;extp=0;exchp=0;natp=0;unbilp=0;loc=0

    if month:
        print "%10s     %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f %7.1f (%+4.1f%%)"\
          % (month, 
             traf['cache'],cachep,
             traf['ext'],extp,
             traf['exch'],exchp,
             traf['nat'],natp,
             traf['loc'],loc,
             traf['unb'],unbilp,traf['total'],
             snmp[0][r], diff)
    else:
	if simple==False:
            print "%10s %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f %7.1f (%+4.1f%%)"\
              % ("", 
                 traf['cache'],cachep,
                 traf['ext'],extp,
                 traf['exch'],exchp,
                 traf['nat'],natp,
                 traf['loc'],loc,
                 traf['unb'],unbilp,traf['total'],
                 snmp[0][r], diff)
	else:
            #print "%14s %7.1f %7.1f (%+4.1f%%)" % ("",traf['total'], snmp[0][r], diff)
            print "%4s,  Netflow=%7.1f MB, SNMP=%7.1f MB, (%+4.1f%%)" % ("",traf['total'], snmp[0][r], diff)
	    

def usage():
    print "usage: %s -c cust[,cust,...] [-l id[,id,..]] [ [-f dd-mm-yyyy -t dd-mm-yyyy] | [-m n]] [-r] [-a] [-s mpbs] [-x][-d]" % sys.argv[0]
    print "\tReports MBytes send To customer(s) (See -x for From).\n\t-c cust = customer id, \n\t-x report MBytes From customer instead\n\t-f is from date,\n\t-t is to date (not inclusive),\n\t-a simpleoutput\n\t-r reverse SNMP Bytes in and out\n\t-m is monthly breakdown between this month and n months ago\n\t-s mbps Clip samples to max mbps\n\t-l Link Id (Can be comma sep list)\n\t-d = debug"

if __name__=="__main__":

    try:
        opts,args=getopt.getopt(sys.argv[1:],'ads:rxc:t:f:m:l:')
    except:
        sys.stderr.write("Invalid args\n")
        usage()
        sys.exit(-1)

    direction="to"; reverse=0;sanity=0; clip=0
    fromdate="" ; todate=""; nmon=6
    cust=""; link = None ; debug=0
    for o,v in opts:
        if o=='-a':
            simple=True
        if o=='-x':
            direction="from"
        if o=='-r':
            reverse=1
        if o=='-c':
            cust=v
        if o=='-m':
            nmon=int(v)
        if o=='-t':
            todate=v
        if o=='-f':
            fromdate=v
        if o=='-s':
            sanity=1
	    clip=int(v)
        if o=='-l':
            link=v
        if o=='-d':
            debug=1

    if not cust:
        usage()
        sys.exit(1)

    months=[]
    for i in range(mon-nmon,mon+1):
        m=str(i)
        y=str(year)
        if i<1:
            m=str(12+i)        
            y=str(year-1)
        if i>12:
            y=str(year+1)
            m=str(i-12)
        val=m + '-' + y
        months.append(val)

    def process_link(thislink=None):
        if not (fromdate and todate):
            for i in range(0,len(months[:-1])):
                snmp=dosnmp(cust,i,months=months,clip=clip,linkid=thislink)
                bta=dobta(cust,i,months=months,linkid=thislink, debug=debug)
                process(snmp, bta,i,direction, reverse, month=months[i],linkid=thislink)
        else:
            i=0
            snmp=dosnmp(cust,i,clip=clip,fromdate=fromdate,todate=todate,linkid=thislink)
            bta=dobta(cust,i,fromdate=fromdate,todate=todate,linkid=thislink, debug=debug)
            process(snmp, bta,i,direction,reverse,fromdate=fromdate,todate=todate,linkid=thislink)

    if fromdate and todate:
        print "%s to %s" % (fromdate,todate)
    elif fromdate:
        print "From %s" % (fromdate,)
    elif todate:
        print "To %s" % (todate,)
    process_link()
    try:
        for somelink in string.split(link,","):
            process_link(thislink=somelink)
    except TypeError:
        pass
    if link:
        process_link(thislink=link)


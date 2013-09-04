#!/opt/bta4/bin/python
#
# Customer Traffic report of BTA compared to RADIUS
#
# 
# 

import sys, string,time, getopt

sys.path.append('/opt/bta4/lib')
sys.path.append('/import/bta4/lib')

sys.path.append('/opt/bta4/lib/python2.3')
sys.path.append('/opt/bta4/lib')
sys.path.append('/export/00/python/lib/python2.3/site-packages')
sys.path.append('/export/00/python/lib/python2.3')

import cx_Oracle

DEBUG=0

d=time.localtime(time.time())
mon=d[1]
year=d[0]
months=[]

btat=0
adslt=0

adsldb = cx_Oracle.connect("cust/custropw@traf2_orin")
adslcur=adsldb.cursor()
btadb = cx_Oracle.connect("bta4_hist/krakenbta4@traf2_orin")
btacur=btadb.cursor()
btalinkdb = cx_Oracle.connect("bta4_www/bta4_www98@bta4")
btalinkcur=btalinkdb.cursor()

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


def doadsl(chapuser,fromdate=None, todate=None, adjust=0):
        adsldata=[]

        ft=time.mktime(time.strptime(fromdate,'%d-%m-%Y'))
        tt=time.mktime(time.strptime(todate,'%d-%m-%Y'))

# Adjust for BTA being in GMT, this is an approximation as the ADS
# records are on produced daily in the adsl_summary table
# So adding 1-day, gets a little closer

	if adjust:
	    ft=ft + 36000	# +10hrs
	    tt = tt + 36000	# +10hrs
 
	if DEBUG:
	    print "fd=", fromdate, "td=",todate
	query="""select sum(output_octets)/1000000 as mbytes 
                 from cust.adsl_summary where ip_address!='NULL' 
		 and to_time between %d and %d and user_id='%s'
	      """ % (ft, tt, chapuser)
          
	if DEBUG:
	    print "rad=", query
        adslcur.execute(query)
        res=adslcur.fetchall()
	if not res[0] or not res[0][0]:
            adsldata.append([0])
	else:
            adsldata.append(res[0])

        return(adsldata)

def dobta(cust,i,months=months,fromdate=None, todate=None, linkid=None):
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
                AND addr.customer=link.customer""" % (cust, linkid)

	    if DEBUG:
	         print "btasubs=", query

            btalinkcur.execute(query)
            res=btalinkcur.fetchall()
	    
	    if (len(res[0])>=1 and len(res[0])<2):
		try:
			result=[res[0][0],res[1][0]]
		except IndexError:
			result=[res[0][0],res[0][0]]
                subs = `flatten(result)`[1:-1]
 	    else: 
                subs = `flatten(res)`[1:-1]
	    
        else:
            subs = "0"
        btadata=[]

        if fromdate and todate:
            d1,m1,y1=string.split(fromdate,'-')
            d2,m2,y2=string.split(todate,'-')
        else:
            d1=d2='1'
            m1,y1=string.split(months[i],'-')
            m2,y2=string.split(months[i+1],'-')

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

	if DEBUG:
	    print "bta=", query

        btacur.execute(query)
        res=btacur.fetchall()
        
        touse=0
        fromuse=0
        for entry in res:
            touse=touse+entry[1]
            fromuse=fromuse+entry[2]
        total=['Total',touse,fromuse]
        res.append(total)
        return(res)

def getCustLinks(todate, chapuser=None, customer=None, linklist=None):

     if customer:
	subq=""
	if linklist:
	    subq="and link_id in (%s) " % linklist
	
        query="""select customer, chapuser, link_id from bta4.customer_links 
	      where customer=%d and to_date('%s','DD-MM-YYYY') between  start_time and end_time
	      and chapuser is not null %s
	      """ % (customer, todate, subq);
     else:
         query="""select customer, chapuser, link_id from bta4.customer_links 
	      where chapuser='%s' and to_date('%s','DD-MM-YYYY') between  start_time and end_time
	      and chapuser is not null
	      """ % (chapuser, todate);
     btalinkcur.execute(query)
     res=btalinkcur.fetchall()

     return(res)

def process(custid, adsl,bta,nohead, month=None,fromdate=None,todate=None,linkid=None):
    if not nohead:
        if linkid: 
            custid = "%s-%s" % (custid,linkid)
        else:
            custid = custid
	if simple==False:
            print "Cid=%-10s %10s %14s %14s %14s %14s %7s %14s" % \
                (custid,"Cache","External","Exchange","National","Unbillled","Total","RADIUS (diff)")
	#else:
        #    sys.stdout.write("Cid=%-10s" % custid)

    entry=bta

    total=[]

    direction="to"
    reverse=0

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
    
    traf={'unb':0, 'cache' :0, 'exch':0 ,'ext':0,'sattel':0,'total':0,'nat':0}
    for item in entry:        
        if item[0]=='Cache': traf['cache']=item[x]
        if item[0]=='Exchange': traf['exch']=item[x]
        if item[0]=='National': traf['nat']=item[x]
        if item[0]=='External': traf['ext']=item[x]
        if item[0]=='Sattel': traf['sattel']=item[x]
        if item[0]=='Unbilled': traf['unb']=item[x]
        if item[0]=='Total': 
		traf['total']=item[x]
		total.append(item[x])
		total.append(adsl[0][0])

    if not traf['total']:
        traf['total']=0        
    if traf['total']:   
        cachep=100 * traf['cache']/traf['total']
        extp  =100 * traf['ext']/traf['total']
        exchp =100 * traf['exch']/traf['total']
        natp  =100 * traf['nat']/traf['total']
        unbilp=100 * traf['unb']/traf['total']
        diff=(adsl[0][0]-traf['total'])*100/ traf['total']
    else:
        if not adsl[0][0]:
            diff=0 # don't have either, no difference.
        else:
            diff=100
        cachep=0;extp=0;exchp=0;natp=0;unbilp=0; 

    if simple==False:
      if month:
        print "%10s     %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f %7.1f (%+4.1f%%)"\
          % (month, 
             traf['cache'],cachep,
             traf['ext'],extp,
             traf['exch'],exchp,
             traf['nat'],natp,
             traf['unb'],unbilp,traf['total'],
             adsl[0][0], diff)
      #else:
      #  print "%10s %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f (%3d%%) %7.1f %7.1f (%+4.1f%%)"\
      #    % ("", 
      #       traf['cache'],cachep,
      #       traf['ext'],extp,
      #       traf['exch'],exchp,
      #       traf['nat'],natp,
      #       traf['unb'],unbilp,traf['total'],
      #       adsl[0][0], diff)


    return(total)

def usage():
    print "usage: %s -c custid [-l linkid,...] | -u chapuser  -f dd-mm-yyyy -t dd-mm-yyyy [-a]" % sys.argv[0]
    print "\tReports MBytes send To customer(s) via Netflow and RADIUS."
    print "\t-u chapuser , use link by chapuser"
    print "\t-a Loosly adjust for GMT diff"
    print "\t-f from_date"
    print "\t-t to_date (not inclusive)"
    print "\t-l linkid, optional list of linkid's . Must also use -c custid"

if __name__=="__main__":

    try:
        opts,args=getopt.getopt(sys.argv[1:],'su:c:t:f:al:')
    except:
        sys.stderr.write("Invalid args\n")
        usage()
        sys.exit(-1)

    direction="to"
    nmon=3
    chapuser=None
    customer=None
    linklist=None
    adjust=0
    simple=True
    fromdate="" ; todate=""
    cust=""; link = None
    for o,v in opts:
        if o=='-t':
            todate=v
        if o=='-f':
            fromdate=v
        if o=='-c':
            customer=int(v)
        if o=='-u':
            chapuser=v
        if o=='-a':
            adjust=1
        if o=='-l':
            linklist=v
        if o=='-s':
            simple=False

    if not (fromdate and todate):
	usage()
	sys.exit(-1)
    if not customer and not chapuser:
	usage()
	sys.exit(-1)

    if chapuser:	# get all links
        links = getCustLinks(todate, chapuser=chapuser)
    else:
        links = getCustLinks(todate, customer=customer, linklist=linklist)

    if len(links)==0:
	print "No links found"

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

    def process_link(linkid=None, chapuser=None, cust=None):

	    global adslt
	    global btat
	
	    if simple==False:
    	        if fromdate and todate:
        	        print "%s (%s to %s) in MBytes" % (chapuser, fromdate,todate)
    	        elif fromdate:
        	    print "From %s" % (fromdate,)
    	        elif todate:
                    print "To %s" % (todate,)

            i=0
            adsl=doadsl(chapuser,fromdate=fromdate,todate=todate,adjust=adjust)
            bta=dobta(cust,i,fromdate=fromdate,todate=todate,linkid=linkid)
            totals = process(int(cust), adsl, bta,i,fromdate=fromdate,todate=todate,linkid=linkid)

	    adslt = adslt + totals[1]
	    btat  = btat + totals[0]

    for link in links:
        process_link(linkid=link[2], chapuser=link[1], cust=link[0])
    if btat==0:
	diff=0
    else:
        diff=(adslt - btat ) *100/ btat

    if simple==True:
	print "%s, Netflow=%7.1f MBytes Radius=%7.1f MBytes (%+4.1f%%)" % ( chapuser, btat, adslt,diff)
    else:
        print " " * 86 , "---------------------"
        print " " * 85, "%7.1f %7.1f (%+4.1f%%)" % ( btat, adslt,diff)

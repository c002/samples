#!/opt/bta4/bin/python
#!/opt/python2/bin/python
#
# Check to see if 'netflow' is configured properly 
# on routers.  We conpare the CIsco router config files for
# routers listed in our flowrules "Makefile".
#

import getopt, sys, glob, socket
sys.path.append("/import/bta4/bin")
import cisco_view
from cisco_view import *
import ParseMakefile
import flowrule_view

from lib import Log
from attrs import Attributes

False=0
True=1

RTRCFGDIR="/import/bta4/rtrconfigs"
FLOWRULES="/import/bta4/etc/meter_config/mpls"

# Some constants for comparisons
FLOWPORT=9991
VERSION=6
INTERFACE="Loopback0"
IGNORE_GROUPS=["MPLS_UNAVAILABLE","IGNORE"]

def cmpmtr(ip, mtr):
   """Compare ip with meter. Resolve if required"""

   hosttup1=socket.gethostbyaddr(ip)
   hosttup2=socket.gethostbyaddr(mtr)
   if hosttup1[0]==hosttup2[0]:
	return(1)
   else:
	return(0)

def usage():
    print "%s [-l level] [-p pop] [-D]" % sys.argv[0]
    print "\tWhere -l is log level. 0 is least and 4 is most"
    print "\t-p pop , limit reporting by pop (3 letter mnenomic)."
    print "\t-D is debug. Add more D's for more debug"

if __name__=="__main__":

    try:
        opts,args=getopt.getopt(sys.argv[1:],'hDl:p:')
    except:
        sys.stderr.write("Invalid args\n")
        usage()
        sys.exit(-1)

    level=2
    debug=0
    pop="*"
    options=[]
    for o,v in opts:
        if o=='-h':
            usage()
            sys.exit(0)
        if o=='-l':
            level=int(v)
        if o=='-p':
            pop=v
        if o=='-D':
            debug=debug+1

    options=["details"]
    Attributes.pop=pop
    Attributes.options=options

    log=Log.Log(level)

    import conf
    conffile="/import/bta4/v2/etc/bta4.conf"
    conf.loadconf(conffile)

    mf=ParseMakefile.ParseMakefile(conf=conf)
    for rtr in mf.rtrdict.keys():
        mtr = mf.getRouterMeter(rtr=rtr)
        #print "%s = %s" % (rtr, mtr)

    cfg = RtrConfigParser(dir=RTRCFGDIR, attrs=Attributes)

    # Routers in the flowrules makefile
    for r in mf.rtrdict.keys():
	found=0
	for s in cfg.crouters:	# The config files
	    if found: break
	    #print "[%s] <=> [%s]" % (s.rtrname, r)
	    if r==s.rtrname:
                mtr = mf.getRouterMeter(rtr=r)
	        if not mtr:
	            print r,"Not assigned to meter"
		#s.rtrflowcache		
		s.rtrflowdst
		host=socket.gethostbyaddr(s.rtrflowdst)[0]
		if not cmpmtr(s.rtrflowdst, mtr):
		    log(0,"%s, Incorrect destination '%s', expected '%s'\n" % (host,r,mtr))
		    #log(0,"\tflowdst=%s, mtr=%s\n" % (s.rtrflowdst,mtr))
		else:
		    log(2,"%s, Destination host Ok. '%s' <=> '%s'\n" % (host,r,mtr))

		if s.rtrflowport!=FLOWPORT:
		    log(0,"%s, Incorrect destination port '%d', expected '%d'\n" % (host,s.rtrflowport,FLOWPORT))
		else:
		    log(2,"%s, Destination port Ok. '%d' <=> '%d'\n" % (host,s.rtrflowport,FLOWPORT))

		if s.rtrflowsrc!=INTERFACE:
		    log(0,"%s, Incorrect source interface '%s', expected '%s'\n" % (host,s.rtrflowsrc,INTERFACE))
		else:
		    log(2,"%s, Source interface Ok. '%s' <=> '%s'\n" % (host,s.rtrflowsrc,INTERFACE))

		if s.rtrflowver!=VERSION:
		    log(0,"%s, Incorrect netflow version '%d', expected '%d'\n" % (host,s.rtrflowver,VERSION))
		else:
		    log(2,"%s, Netflow version Ok. '%d' <=> '%d'\n" % (host,s.rtrflowver,VERSION))

		if s.rtrflowfreq!=0:
		    log(2,"%s, Sampled Netflow set at 1/%dth packet.\n" % (host,s.rtrflowfreq))


    # Check the interfaces in our flowrule definitions have netflow set.
    fp = flowrule_view.FlowruleParser(dir=FLOWRULES+"/flowrules", configfile="flowrules.conf")

    for host in fp.fr.routers.keys():
	chost = socket.gethostbyaddr(host)[0]	
	#chost=host

	for gr in fp.fr.routers[host].groups.keys():
	    for fr_if in fp.fr.routers[host].groups[gr].finterfaces:
		subcmp=0
	        for i in cfg.cinterfaces:
		    if i.cstate!="up":		# Only look at interfaces that are up
			continue
		    if i.cinterface=="Loopback0":
			continue
		    slen=len(fr_if)
		    if fr_if[-1]=='*':
		        slen=slen-1
		    elif len(i.cinterface) > slen:
		       slen=len(i.cinterface)
		    if string.rfind(i.cinterface,'.')>0:
			continue		# dont check subinterfaces.
	    	    #log(4, "[%s/%s] <=> [%s/%s]\n" % (i.crouter,i.cinterface,host,fr_if))
	            if i.crouter==host and i.cinterface[:slen]==fr_if[:slen]:
#			log(0, "*** %s, i.c=%s, slen=%s, i.cinterface=%s , fr_if=%s" % (host,slen, i.cinterface, fr_if))
			if gr in IGNORE_GROUPS:
		            log(3, "%s, Netflow 'off' for Interface %s, but is in ignore group\n" % (i.crouter, i.cinterface ))
		        elif i.ccache=="off":
		            log(1, "%s, Netflow 'off' for interface %s (%s)\n" % (i.crouter, i.cinterface, gr))
		        elif i.ccache=="on":
		            log(3, "%s, Netflow 'on' for interface %s (%s)\n" % (i.crouter, i.cinterface, gr))
		        elif i.ccache=="sampled":
		            log(3,"%s, Netflow 'sampled' for interface %s (%s)\n" % (i.crouter, i.cinterface, gr))

    # The results
    print log

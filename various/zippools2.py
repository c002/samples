#!/opt/python2/bin/python
# zope ippools (zippolls),  Zope interface to IPPools.py
#
# 
#

import sys
#sys.path.append("/opt/zope/Zope-harry/Extensions")
sys.path.append("/opt/zope/Zope/Extensions")

from IPPools2 import IPPoolError

DEBUG=1

def listpools(*args,**kwargs):

    try:
        if sys.modules.has_key('IPPools2'):
            sys.modules['IPPools2'] = reload(sys.modules['IPPools2'])
        else:
    # DEBUG
            import IPPools2
    except:
        return "1 An error has occurred - looks like this file is being worked on.  Please email devreq"
    # DEBUG
    f=open("/tmp/z.harry","a")
    f.write("C: %s\n" % str(kwargs))
    f.close()

    if not kwargs.has_key('product'):
        kwargs['product']='BTA'
    pools = apply( sys.modules['IPPools2'].AutoIP, (),kwargs)
  
    # DEBUG
    f=open("/tmp/z.harry","a")
    f.write("D: %s\n" % str(pools))
    f.close()
    
    retVal = pools.listpools(status=kwargs['status'])

    return retVal

def add_pool(*args, **kwargs):
    try:
        if sys.modules.has_key('IPPools2'):
            sys.modules['IPPools2'] = reload(sys.modules['IPPools2'])
        else:
            import IPPools2
    except:
        return "An error has occurred - looks like this file is being worked on.  Please email devreq"
    # DEBUG
    f=open("/tmp/z.harry","a")
    f.write("add_pool A: %s\n" % str(kwargs))
    f.close()

    if kwargs.has_key('product'):
        pools =  sys.modules['IPPools2'].AutoIP(product=kwargs['product'])
	del(kwargs['product'])
    else:
        pools =  sys.modules['IPPools2'].AutoIP(product='BTA')

    # DEBUG
    f=open("/tmp/z.harry","a")
    f.write("add_pool B: %s\n" % str(kwargs))
    f.close()

    poolid = apply(pools.add_pool,(),kwargs)

    return poolid

def close_pool(*args, **kwargs):
    try:
        if sys.modules.has_key('IPPools2'):
            sys.modules['IPPools2'] = reload(sys.modules['IPPools2'])
        else:
            import IPPools2
    except:
        return "An error has occurred - looks like this file is being worked on.  Please email devreq"

    if kwargs.has_key('product'):
	pools =  sys.modules['IPPools2'].AutoIP(product=kwargs['product'])
	del(kwargs['product'])
    else:
        pools =  sys.modules['IPPools2'].AutoIP(product='BTA')
    apply(pools.close_pool,(),kwargs)

    return pools.poolid

def assign_ip(*args, **kwargs):

    # Check if PE_LOOP IP is required

    try:
        if sys.modules.has_key('IPPools2'):
            sys.modules['IPPools2'] = reload(sys.modules['IPPools2'])
        else:
            import IPPools2
    except:
        return "An error has occurred - looks like this file is being worked on.  Please email devreq"

    if not kwargs.has_key('product'):
        kwargs['product']='BTA'
    kwargs['spares']=1
    kwargs['debug']=4

    if DEBUG: print "Doing Assign:", str(kwargs)
    pools = apply( sys.modules['IPPools2'].AutoIP, (),kwargs)

    if not kwargs.has_key('blocksize'):
	raise IPPoolError, (6,"Block size of request is unspecified. (%s)" % str(kwargs))

    if len(pools.poollist)>1:
    	pools.reduce_pools(blocksize=kwargs['blocksize'])

    if len(pools.poollist)>1:
	raise IPPoolError, (4,"Unable to obtain unique Pool (%s)" % str(kwargs))
    if len(pools.poollist)==0:
	raise IPPoolError, (5,"Unable to find qualifying Pool (%s)" % str(kwargs))

    if DEBUG: print "Assign: 2"
    if pools.poolid:
        valtup = pools.allocate(poolid=pools.poolid, blocksize=kwargs['blocksize'])
	ip = valtup[0]+'/'+str(valtup[1])
	poolid=valtup[2]
    else:
	ip = None
	poolid=0
    
    if DEBUG: print "Assign:",ip, str(kwargs)

    return ((poolid, ip))

def OLD_assign_ip(*args, **kwargs):

    # Check if PE_LOOP IP is required

    try:
        if sys.modules.has_key('IPPools2'):
            sys.modules['IPPools2'] = reload(sys.modules['IPPools2'])
        else:
            import IPPools2
    except:
        return "An error has occurred - looks like this file is being worked on.  Please email devreq"

    if not kwargs.has_key('product'):
        kwargs['product']='BTA'
    kwargs['spares']=1
    kwargs['debug']=4

    if DEBUG: print "Doing Assign:", str(kwargs)
    pools = apply( sys.modules['IPPools2'].AutoIP, (),kwargs)

    if not kwargs.has_key('blocksize'):
	raise IPPoolError, (6,"Block size of request is unspecified. (%s)" % str(kwargs))

    if len(pools.poollist)>1:
    	pools.reduce_pools(blocksize=kwargs['blocksize'])

    if len(pools.poollist)>1:
	raise IPPoolError, (4,"Unable to obtain unique Pool (%s)" % str(kwargs))
    if len(pools.poollist)==0:
	raise IPPoolError, (5,"Unable to find qualifying Pool (%s)" % str(kwargs))

    if DEBUG: print "Assign: 2"
    if pools.poolid:
        ip = pools.allocate(poolid=pools.poolid, blocksize=kwargs['blocksize'])
	ip = ip[0]+'/'+str(ip[1])
    else:
	ip = None
    
    if DEBUG: print "Assign:",ip, str(kwargs)

    return ip

if __name__=="__main__":
    """Test 1 2 3"""
    #raise IPPoolError, (1,"stuffed")
    test=1
    p=0

    if test==1:
        #p=listpools(locale='can', product='BTA_AAPT')
  
	ipn=116
	#for (loc, rtr) in (('syd','cor1'), ('ade','cor5'), ('bri','cor3'), ('mel','cor6'), ('per','cor1')):
	for (loc, rtr) in (('can','cor1'),):
	    #for svc in ('AAPT_SHDSL','AAPT_ADSL','LMDS_E1','LMDS_ETH'):
	        ips="10.%d.0.1" % ipn
	        ipn=ipn+1
	        ipe="10.%d.0.0" % ipn
                #x=add_pool(product='BTA_AAPT',service_type=svc,locale=loc, router=rtr,start_ip_number=ips, end_ip_number=ipe)
                x=add_pool(product='BTA_AAPT',locale=loc, router=rtr,start_ip_number=ips, end_ip_number=ipe)


    if test==2:
        #p=listpools(locale='bur', product='CCIP')
        p=0

        #x=add_pool(product='CCIP-PE-CE-LINK-ADDRESS',locale='bur')
        x=assign_ip(spares=1,product='CCIP-PE-L2TP-LOOPBACK',blocksize='32',locale='ade')

#start_ip_number=top_ips_str,
#                    end_ip_number=top_ipe_str,
#                    router=self.poollist[0].router, locale=self.poollist[0].locale,                    
#   
#                    interface=self.poollist[0].interface,
#                    service_type=self.poollist[0].service_type,
#                    preference=self.poollist[0].preference, status="SPARE")

    print p

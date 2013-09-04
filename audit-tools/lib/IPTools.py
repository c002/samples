import exceptions, string

def ip2int(ip):
   	if type(ip)==type([]) and len(ip)==1:
	    ip=ip[0]
	elif type(ip)!=type(""): 	
	    return None

	ip1,ip2,ip3,ip4=map(lambda x: long(x), string.split(ip,'.'))

	n=ip1 <<24
	n=n + (ip2 << 16)
	n=n + (ip3 << 8)
	n=n + ip4
	
	return n

def _mask2cidr( mask=None):
	"""Convert a 4byte mask to a /cidr bit mask"""
	if not mask:
	    return None

	bytes=string.split(mask,'.')
	if len(bytes) != 4:
	    return None
	
	n = long(0)
	n = n+ int(bytes[0]) << 24
	n = n+ int(bytes[1]) << 16
	n = n+ int(bytes[2]) << 8
	n = n+ int(bytes[3]) 

        bits=-1
	while n:
	    n = n & 0xFFFFFFFFL
	    n = n << 1
	    bits=bits+1

	return bits

def _revmask2cidr( mask=None):
        """Convert a 4byte mask to a /cidr bit mask"""
        if not mask:
            return None

        bytes=string.split(mask,'.')
        if len(bytes) != 4:
            return None
        
        if int(bytes[0]) & 0x80 >0:     # Looks like not a reverse mask
            return None

        n = long(0)
        n = n+ (long(bytes[0]) << 24)
        n = n+ (long(bytes[1]) << 16)
        n = n+ (long(bytes[2]) << 8)
        n = n+ long(bytes[3]) 

        bit=n
        bits=0
        nomorezero=0
        for r in range(31,-1,-1):
            if (bit & 0x80000000L)==0: 
                    #print "%i (0x%x)=0" % (r, bit)
                    if nomorezero:
                        break
                    bits=bits+1
            else:
                    break
                    #print "%i (0x%x)=1" % (r, bit)
                    nomorezero=True
                    #bits=bits+1

            bit = bit << 1

        return bits

def mask2cidr(mask=None):
        """Convert a 4byte mask to a /cidr bit mask"""
        if not mask:
            return None

        bytes=string.split(mask,'.')
        if len(bytes) != 4:
            return None

        if int(bytes[0]) & 0x80 >0:     # Assume not a reverse mask
            return _mask2cidr(mask)
        else:
            return _revmask2cidr(mask)

def isinblock(superset, ipcidr):
	"""Check to see if ipcidr is part of superset block. both params
	   must be given as ip/mask format
	"""
#	start_super = 
#end_super =
#	ips=self.ip2int(start_ip_number)
#        ipe=self.ip2int(end_ip_number)
	pass

def ip2str( ip):
        """Convert int IP into quad IP"""

        ip=long(ip)
        byte1 = int ((ip & 0xFF000000L) >> 24)
        byte2 = int ((ip & 0x00FF0000L) >> 16)
        byte3 = int ((ip & 0x0000FF00L) >> 8)
        byte4 = int  (ip & 0x000000FFL)

        return "%d.%d.%d.%d" % (byte1, byte2, byte3, byte4)

def mask2int( mask):
        """return number of bits in the mask
        """
	#print mask
        if type(mask)==type(""):
            mask=int(mask)
 	elif type(mask)==type([]) and len(mask)==1:
	    mask=mask[0]
	else:
	    mask=32

        n = 32 - mask

        bits= long(pow(long(2),n))
        return bits

def findnet(ipcidr):
	"""given an ip/mask, return network/mask"""

	if type(ipcidr)!=type(""):
	    raise IPToolsError, (1, "Expected a string ip/cidr but got type '%s'" % type(ipcidr))

	parts = string.split(ipcidr,"/")
	if len(parts)!=2:
	    raise IPToolsError, (2, "Expected a ip/cidr")
	(ip, mask) = parts
	
	ipi=ip2int(ip)
	bits=mask2int(mask)

	netip = int( ipi / bits ) * bits
	
	net = ip2str(netip)
	return net	

def checkip( ipstr):
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


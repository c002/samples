#!/opt/python2/bin/python
#

import sys

DEBUG=0
VERBOSE=0

class ProductRules:
    """This class describes rules regarding assigning and 
        adding pools for a product.
    """

    def __init__(self, product=None):
	self.product=product
	self.module=None

        # Default rules and rules that we know about
	self.rule="yes"
        self.PR_min_mask=32
        self.PR_max_mask=0
        self.PR_min_ip=0
        self.PR_max_ip=4294967296L
        self.PR_RFC1918=1		# RFC1918 address space is Ok
        self.PR_public=1		# public (routed) address space is Ok
        self.PR_router=0		# Don't have to specify router for this pool
        self.PR_locale=1		# Must give Locale info for this pool
        self.PR_interface=0		# Don't have to give interface info for this pool
        self.PR_service_type=0		# Don't have to give service_type for this pool

	self.RFC1918_ips=( (167772160, 184549376),	# 10.0.0.0 - 11.0.0.0	(/8)
			   (3232235520L, 3232301056L),	# 192.168.0.0 - 192.169.0.0 (/16)
			   (2886729728L, 2887778304L))	# 172.16.0.0 - 172.32.0.0  (/12)

        self.rulemap={ 
		   "CCIP-PE-L2TP-LOOPBACK" : "RuleSet_3",
		   "CCIP-CE-LOOPBACK" : "RuleSet_2",
		   "CCIP-PE-CE-LINK-ADDRESS" : "RuleSet_2",
		   "CCIP" : "RuleSet_2",
		   "BTA" : "RuleSet_2",
		   "BTA_AAPT29" : "RuleSet_5",
		   "BTA_AAPT30" : "RuleSet_4",
		   "BTA_BIZ32" : "RuleSet_6",
		   "BTA_PWTEL30" : "RuleSet_2"
		 }

  
	if self.rulemap.has_key(self.product):
	    module=self.rulemap[self.product]
	    try:
                # Load/Reload the module and make it a runnable object
                if DEBUG: print "Loading Allocation Rules ..."
                if sys.modules.has_key(module):
                    if VERBOSE: print "Reloading module", module
                    self.module=reload(sys.modules[module])
                exec("from RuleSets import %s" % (module))        # Load the module
                self.module=eval("%s()" % module)           # Create the code object
            except ImportError, msg:
                print "Warning:", msg

	    
	else:
	   raise "Unknown Product" 

	for obj in self.module.__dict__.keys():
	    self.__dict__[obj]=self.module.__dict__[obj]

#	print "val=", self.rule
#	print "self.PR_RFC1918=",self.PR_RFC1918
 
if __name__=="__main__":
    
    prod=ProductRules("VPNSC")
    if prod.PR_RFC1918=="no":
	print "No RFC1918 addrs permitted"
	
    

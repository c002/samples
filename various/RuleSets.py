#
# Ruleset definitions for IP allocation/management. These Rulesets
# map to a product in "IPRules.py". If adding a product, need to 
# map that to a ruleset in IPRules.py
# 
# 
# 

class RuleSet_1:
    def __init__(self):

        self.PR_min_mask=32		# Smallest permissable allocation request
        self.PR_max_mask=16		# Largest permissable allocation request
        self.PR_min_ip=0		# Smallest permissable IP number
        self.PR_max_ip=4294967296L	# Largest permissable IP number
        self.PR_RFC1918=1		# RFC1918 addresses permitted 
        self.PR_public=1		# Public addresses permitted
	self.PR_locale=1		# Must supply locale info for allocation requests
	self.PR_router=0		# Don't have to provide router info for allocation requests.
	self.PR_interface=0		# Don't have to provide interface info for allocation requests.
	self.PR_service_type=0		# Don't have to provide service_type info for allocation requests.


class RuleSet_2:
    def __init__(self):

        self.PR_min_mask=32
        self.PR_max_mask=30
        self.PR_min_ip=0
        self.PR_max_ip=4294967296L
        self.PR_RFC1918=1
        self.PR_public=1
	self.PR_locale=1

class RuleSet_3:
    def __init__(self):

        self.PR_min_mask=32
        self.PR_max_mask=16
        self.PR_min_ip=0
        self.PR_max_ip=4294967296L
        self.PR_RFC1918=1
        self.PR_public=0
	self.PR_locale=1

class RuleSet_4:
    def __init__(self):

        self.PR_min_mask=30
        self.PR_max_mask=30
        self.PR_min_ip=0
        self.PR_max_ip=4294967296L
        self.PR_RFC1918=1
        self.PR_public=1
        self.PR_locale=1
        self.PR_router=0

class RuleSet_5:
    def __init__(self):

        self.PR_min_mask=32
        self.PR_max_mask=29
        self.PR_min_ip=0
        self.PR_max_ip=4294967296L
        self.PR_RFC1918=1
        self.PR_public=1
        self.PR_locale=1
        self.PR_router=0

class RuleSet_6:
    def __init__(self):

        self.PR_min_mask=32
        self.PR_max_mask=32
        self.PR_min_ip=0
        self.PR_max_ip=4294967296L
        self.PR_RFC1918=1
        self.PR_public=1
	self.PR_locale=1


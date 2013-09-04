#!/opt/python2/bin/python
#
# flowrule_view 
#	Parse a flowrule files and pull out intreresting
#	lines. Build some objects that refelct the flowrules.
#

import string, re, math, exceptions, glob

from lib import IPTools
from lib import Log
from attrs import Attributes
from constants import Const

DEBUG=1
STATE=["interface"]

# The patterns in a flowrule file
pats= { 'host' : "^\s*host\s+(?P<host>.+)\s+{$",
        'group' : "^\s*group\((?P<group>.+)\)\s*=\s*(?P<iflist>.+)$",
        'flowrule' : "^\s*flowrule\((?P<key>.+)\)\s*=\s*(?P<rule>.+)$",
      }

# Compiled patterns
cpats={}
for pat in pats.keys():
    cpats[pat]=re.compile(pats[pat])
cintpats={}

class FlowruleException(Exception):
    def __init__(self,errstr):
        self.errstr=errstr
    def __str__(self):
        return `self.errstr`

class FlowruleGroup:
    """Information about a router"""
    def __init__(self, groupname=None):
	self.fgroupname=groupname	# Name of group
	self.finterfaces=[]		# List of interfaces in this group

    def __str__(self):
	s="    Group=[%s]\n" % self.fgroupname
	if len(self.finterfaces):
	    s=s+"        Interfaces="
	    for iface in self.finterfaces:
	        s = s + "[%s]," % iface
	    s=s+"\n"
	return(s)	 

class GroupDefs(FlowruleGroup):
    """Describes a group section in a flowrules file."""

    def __init__(self):
	FlowruleGroup.__init__(self)
	self.groups={}		# Dict of FlowruleGroup objects

    def addgroup(self, group):
	if self.groups.has_key(group):
	    raise FlowruleException, "Error. Group already defined, %s" % (group)
	else:
	   self.groups[group]=FlowruleGroup(group)

    def addinterface(self, group, interface):
	for gr in self.groups.keys():
	    if interface in self.groups[gr].finterfaces:
		raise FlowruleException, "Error Interface %s already defined in group %s" % (interface, gr)

	self.groups[group].finterfaces.append(interface)


class RuleDefs:
    """Defines the rules section. ie. the flows between which interfaces groups
	we're interested in
	Place holder only for now 
    """

    def __init__(self):
        self.rules={} 

class TagDefs:
    """Describes what tags are assigned to flows.
	Place holder only for now 
    """
    def __init__(self):
        self.tags={} 

class RouterDefs(GroupDefs, RuleDefs, TagDefs):
    """Describes all components that make up a flowrule set
	for a router/host
    """ 
    def __init__(self, host=None):
	self.routername=host
	GroupDefs.__init__(self)
	RuleDefs.__init__(self)
	TagDefs.__init__(self)

    def __str__(self):
	s=""
	for g in self.groups.keys():
	    s=s+str(self.groups[g])
	for r in self.rules.keys():
	    s=s+str(self.rules[r])
	for t in self.tags.keys():
	    s=s+str(self.tags[t])
	return(s)

class FlowruleFile(GroupDefs, RuleDefs, TagDefs):
    """Everything we want to know about 'flowrules' file.
	These are made up of sections
    """

    def __init__(self):
	self.routers={}		# Dict of RouterDefs objects
	self.currenhostname=None
	self.currentgroup=None

    def addhost(self,host):
	self.routers[host]=RouterDefs(host)
	self.currenhost=host

    def addgroup(self,group):
	self.routers[self.currenhost].addgroup(group)
	self.currentgroup=group

    def addifacelist(self,iflist):
	"""iflist is comma seperated line of interfaces"""
	ifaces = map(lambda x : string.strip(x), (string.split(iflist,',')))
	try:
	    for iface in ifaces:
	        self.routers[self.currenhost].addinterface(self.currentgroup, iface)
	except FlowruleException, msg:
	    raise FlowruleException, "(Host=%s, Group=%s) %s" % (self.currenhost, self.currentgroup,msg)
	    #print "(Host=%s, Group=%s) %s" % (self.currenhost, self.currentgroup,msg)

    def final(self):
	"""Last method called which populates derived fields which it is
	   assumed are now all available.
	"""
 	pass
	
    def __str__(self):
	
	s=""
 	for h in self.routers.keys():
	    s=s+"Router = %s\n" % h
	    s=s+ str(self.routers[h])
	    s = s + "\n"
	return(s)

class FlowruleParser:
    """Parse flowrules"""

    def __init__(self, configfile=None, dir=None, attrs=None):

	    f=open(dir+"/"+configfile,"r")

	    self.fr = FlowruleFile()

	    line=1
	    linenumber=0
	    state="host"
	    while line:
		linenumber=linenumber+1
                line=f.readline()
	        sline=string.strip(line)
	        for pat in cpats.keys():
	            result=cpats[pat].match(line)
	
		    if result and pat=="host":
			if result.group("host"):
			    #print "--> [%s]" % result.group("host")
			    self.fr.addhost(result.group("host"))
			    state="group"
			    continue
	        
		    elif result and pat=="group" and result.group("group"):
			 self.fr.addgroup(result.group("group"))
			 if result.group("iflist"):
			     self.fr.addifacelist(result.group("iflist"))

#            for i in self.cinterfaces:
#		i.final()	# Last minute object adjustments now that
#				# we have completed the parsing.

	    f.close()

    def __str__(self):
	
	return str(self.fr)
	    
if __name__=="__main__":

    options=["details"]
    Attributes.options=options

    fp = FlowruleParser(dir="./", configfile="flowrules.conf")

    #print fp
    for host in fp.fr.routers.keys():
	print host	
	for gr in fp.fr.routers[host].groups.keys():
	    print "    ",gr
	    for i in fp.fr.routers[host].groups[gr].finterfaces:
		print "        ", i

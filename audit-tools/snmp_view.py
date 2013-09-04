#!/opt/python2/bin/python

import sys, os, cPickle, time
sys.path.append('/opt/bta4/lib')

import dbconnectcx

class sInterface:
    def __init__(self):
	self.scustomer=0
	self.slink_id=None
	self.sbandwidth=0

	self.srelatives=[]	# Related objects
	self.attributes=[]

    def setscustomer(self,cust):
	self.scustomer=cust
    def setslink_id(self,link_id):
	self.dblink_id=link_id
    def setsbandwidth(self,bandwidth):
	if not sbandwidth:
	    sbandwidth=0
	self.sbandwidth=long(sbandwidth)

    def __str__(self):
	s="cust=%s/%s bw=%d" %(self.scustomers, self.slink_id,self.sbandwidth)
	return(s)

class SnmpCustomers:
    """ties routes and links to customers
	according to database
    """

    def __init__(self):

	self.fname="/tmp/snmpcusts.pickle"	

	self.sinterfaces=[]

	self.db=dbconnectcx.dbConnect("bta4/bta498@lbta4_ochre")

	if os.path.exists(self.fname):
	    self.unpickle()
	else:
	    self.sgetlinks()
	    self.pickle()

	self.db.close()

    def pickle(self):
	f=open(self.fname,"w")
	p=cPickle.Pickler(f)
	obj=self.sinterfaces
	p.dump(obj)
	f.close()

    def unpickle(self):
	f=open(self.fname,"r")
	u=cPickle.Unpickler(f)
	obj = u.load()
	self.sinterfaces=obj
	f.close()

    def sgetlinks(self):

        query="""select unique customer, link_id, bandwidth
	   from snmp_logs where
	   trunc(timestamp) between trunc(sysdate)-4 and trunc(sysdate)
	   and rownum<10
	  """
	print query

	self.db.execute(query)
	rawinterfaces=dbconnectcx.ResultList(self.db)

	for iface in rawinterfaces:
	    self.sinterfaces.append(sInterface())

	    self.sinterfaces[-1].setscustomer(iface[0])
	    self.sinterfaces[-1].setslink_id(iface[1])
	    self.sinterfaces[-1].setsbandwidth(iface[2])

if __name__=="__main__":

    custs=SnmpCustomers()

    for iface in cust.sinterfaces:
	print iface


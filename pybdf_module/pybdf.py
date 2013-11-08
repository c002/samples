#!/opt/python2/bin/python
#
# Emulates binaryfiles.py using the BDFReader.so module instead.
# This typically provides a 3x speed improvement.
#
# Prolly some more speed to be had by moving away from the binaryfiles.py
# emulation.
#
# $Source: /usr/local/cvsroot/netflow/netflow/btalib/pybdf.py,v $

import sys, math
sys.path.append('/import/src/cca/bta4/code')
import btatools
import BDFReader

log2 = math.log(2)

class pyBDFReader:
    """A class that emulates 'BinaryFilereader' 
    """
	
    def __init__(self, fname, batchmode=0, verbose=0, meter="None"):
	self.verbose=verbose
	self.meter=meter
	self.batchmode=batchmode

        # Opening a BDF File
        self.fd=BDFReader.fopen(fname,'r')

        self.readheader()

	if self.version_major!=4 or self.version_minor not in (2,3,4):
            raise "VersionError", "Unknown version %s.%s in data" % (self.version_major,self.version_minor)

        if self.batchmode:
	    self.readdata()

    def readheader(self):

	# Read the BDF file headers
	self.header=BDFReader.readhead(self.fd)

	self.version_major=self.header[0][1]
	self.version_minor=self.header[0][2]

	self.file_size=self.header[0][3]
	self.collector_ip=self.header[0][4]
	self.checkpoint=self.header[0][5]
	self.samp_start=self.header[0][6]
	self.samp_end=self.header[0][7]

	self.data = DataSet(meter=self.meter,version_major=self.version_major,version_minor=self.version_minor, verbose=self.verbose)

	self.data.timestamp=self.checkpoint

	self.data_rows=self.header[0][8]
	self.remain=self.data_rows

    def dataleft(self):
	if self.remain:
	    return 1

    def readdata(self, fd=None):
        "read the data block from the file descriptor"
        if self.data_rows == 0:
            print "WARNING! No DATA for Timestamp: %s, Meter: %s" % (`self.data.timestamp`, self.data, self.meter)
	#flowdata=BDFReader.readflow(self.fd)
	print self.data_rows
        for r in range(self.data_rows):
            self.data.append(self.readnext())

    def readnext(self):
	fl=BDFReader.readflow(self.fd)
	if fl:
	    self.remain=self.remain - 1

	    alt_tags=fl[9]
	    tag = self.data.lookuptag(fl[9])
            if alt_tags: 
                alt_tags, subtag1 = self.data.lookupsubtag(alt_tags)
                alt_tags, subtag2 = self.data.lookupsubtag(alt_tags)
            else:
                subtag1 = 'none'
                subtag2 = 'none'


	    return (fl[0], fl[2], fl[4], fl[5], fl[6], fl[8], fl[9], subtag1, subtag2, self.data.meter)
	else:
	    return None

    def readtagtable(self, fd=None):
        for tagid in range(self.header[1][1]):
	    self.data.addtag(BDFReader.taglookup(tagid), tagid)


    def readsubtagtable(self, fd=None):
        for tagid in range(self.header[2][1]):
	    self.data.addsubtag(BDFReader.alttaglookup(tagid), tagid)

    def __del__(self): 
	self.fd.close()

class DataSet:
    def __init__(self,meter="None", version_major=0, version_minor=0, verbose=0):
        self.tagtable = {}
        self.subtagtable = {}
        self.data = []
        self.timestamp = None
        self.meter=meter
        self.version_major=version_major
        self.version_minor=version_minor
        self.verbose = verbose

    def append(self, d):
   	self.data.append(d)
 
    def addsubtag(self, name, tagid):
        self.subtagtable[tagid] = name

    def addtag(self, name, tagid):
        self.tagtable[tagid] = name

    def lookupsubtag(self, what):
        """Sub tags are stored as bits since any data set can have multiples.
         they are returned in a highest bit first order"""
        if what:
            subtagindex = int(math.log(what)/log2)
            if self.subtagtable.has_key(subtagindex):
                subtag = self.subtagtable[subtagindex]
            else:
                raise "Tag not found", subtagindex
            return long(what-(1 << subtagindex)), subtag
        else:
            return what, "none"

    def lookuptag(self, what):
        if self.tagtable.has_key(what):
            return self.tagtable[what]
        else:
            raise "TagNotFound",what

    def extract_ips(self):
	"Used in batchmode, which is not really used"
        l = map(lambda x:x[0], self.data) + map(lambda x:x[1], self.data) 
        #print len(l), "ip addresses in data set,",
        l = btatools.unique_list(l)
        #print len(l), "unique."
        return l

    def merge(self):
 	"Don't think its used. Parameter mismatch from binaryfiles.py on purpose"
	pass

    def sort(self):
	" Not needed, bdf files are usually these days"
	pass

if __name__=="__main__":
    """Test methods of BDFReader"""
     
    batchmode=0 
    try:
        b=pyBDFReader('xxx.bdf', batchmode=0)
    except IOError, msg:
	print msg
	sys.exit(-1)

    print "b.data.timestamp=", b.data.timestamp
    print "Version maj min=",b.version_major, b.version_minor

    if 1:
	print "Read tagtable..."
	b.readtagtable()
	print "tagtable=", b.data.tagtable
	b.readsubtagtable()
	print "tagtable=", b.data.subtagtable

	print "Lookup tag 20:", b.data.lookuptag(20)
	print "Lookup subtag 15:", b.data.lookupsubtag(15)
	print "Lookup subtag 2:", b.data.lookupsubtag(2)
	print "Lookup subtag 4:", b.data.lookupsubtag(4)
	print "Lookup subtag 8:", b.data.lookupsubtag(8)
	print "Lookup subtag 16:", b.data.lookupsubtag(16)

        if batchmode:
	    print b.data.extract_ips()
	
    if 1:
        record=1
        while record:
            record = b.readnext()
            if record: print "Record=",record

    if 0:
        record=1
        while b.dataleft():
            record = b.readnext()
            if record: print "Record=",record
     
#    if 0:
#        flow=BDFReader.readflow(b.fd)
#	if flow:
#	    print (b.flow.ip_addr,b.flow.ip_proxy,
#	       b.flow.bytes_to,b.flow.bytes_from,
#	       b.flow.aas,'subtag1','subtag2')

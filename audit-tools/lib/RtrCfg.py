#!/opt/bta4/bin/python

import glob, string

class RtrCfg:

    def __init__(self,dir=None):
	if dir:
	    self.configdir=dir
	
	    self.configfiles=self.loadconfigfiles(self.configdir)
	    self.routers = self.cfg2rtr(self.configfiles)

    def __str__(self):
	s=str(self.routers)
	return(s)
	
    def cfg2rtr(self,configs):
    
        if not configs:
            return None
        routers=[]
        for cfg in configs:
            rtr,filler=string.split(cfg,'-confg')
            routers.append(rtr)
        return routers

    def loadconfigfiles(self,dir):
        files = map(lambda x:string.split(x,'/')[-1],glob.glob('%s/*-confg'% dir))
        files2 = map(lambda x:string.split(x,'/')[-1],glob.glob('%s/*-*'% dir))
   
        ### router Filters
        nfiles=[]

        for file in files:
            if string.find(file,'hub')<0:
                nfiles.append(file) 
        if not nfiles:
            for file in files2:
                if string.find(file,'hub')<0:
                    nfiles.append("%s-confg" % (file))      

        return nfiles

if __name__=="__main__":
   
    configdir="conf/"

    rc=RtrCfg(dir=configdir)
    print rc


#!/opt/bta4/bin/python
#
# Save unique files, ie. ignore the files with duplicate content
#
import string, os, glob, md5

DIR="/var/log/linxfaults"

files = glob.glob(DIR+'/[0-9]*.csv')
savefiles={}

for i in range(0,len(files)-1):
    if os.path.getsize(files[i])==455:
	continue
    f1=open(files[i],"r")
    data1=f1.read()
    f1.close()

    m1=md5.new()
    m1.update(data1)
    sum1=m1.hexdigest()

    if not savefiles.has_key(sum1):
	savefiles[sum1]=files[i]

    for j in range(1,len(files)):
	if os.path.getsize(files[j])==455:
	    continue
	if files[i]==files[j]:
	    continue

        f2=open(files[j],"r")
        data2=f2.read()
	f2.close()
   
        m2=md5.new()
        m2.update(data2)
        sum2=m2.hexdigest()

        if not savefiles.has_key(sum2):
	    savefiles[sum2]=files[j]
        
for f in savefiles.values():
    print "mv %s save" % f

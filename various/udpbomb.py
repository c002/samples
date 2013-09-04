#!/usr/local/bin/python
import sys, getopt, time, struct
from socket import *

def usage():
    print "%s [-p udpport -l length -s packets] [dst IP]"
    print "\t Generates UDP traffic to a destination"
    print "\t-l length  = Amount of data per packet to send. Default is 160 bytes"
    print "\t-s packets = Number of packets to send. Default is infinite"
    print "\t-p udpport = The UDP port to send to. Default is 9999"
    print "\tdstIP   = Destination IP. Default is localhost"

doseq=1

if __name__=="__main__":

    try:
        opts,args=getopt.getopt(sys.argv[1:],'b:p:l:s:')
    except:
        sys.stderr.write("Invalid args\n")
        usage()
        sys.exit(-1)

    i=0
    bytes=0
    packets=0
    port=9999 
    special=0
    length=160
    target='127.0.0.1'
    for o,v in opts:
        if o=='-x':
	    special=1
        if o=='-b':
	    bytes=float(v)
        if o=='-p':
	    port=int(v)
        if o=='-l':
	    length=int(v)
        if o=='-s':
	    packets=int(v)

    req="X" * length

    sock = socket(AF_INET, SOCK_DGRAM)
    if args:
	target=args[0]
    seq=0
    sseq=0
    blob="x"*160
    fmt="I160s"
    print "Sending %d packets of length %d to %s:%d" % (packets, length, target, port)
    if packets>0:
        for i in range(0,packets):
    	    packet = struct.pack(fmt,i,blob)
	    sys.stdout.write('.')
	    seq=seq+1
	    sseq=str(seq)
    	    sock.sendto(packet, (target, int(port)))
	    if (i+40) % 40 == 0:
	        sys.stdout.write('\n%d ' % (i * length))
    else:
        while 1:
	    i=i+1
    	    packet = struct.pack(fmt,i,blob)
            sock.sendto(packet, (target, int(port)))
	    if doseq:
	        print sseq
	    else:
	        sys.stdout.write('.')
	        if (i+40) % 40 == 0:
	            sys.stdout.write('\n%d ' % (i * length))

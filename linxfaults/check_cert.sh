#!/bin/sh
#

jks=$1
expire=`/opt/jdk1.6.0/bin/keytool --list -v -storepass testing -keystore $jks | grep Valid | ./cert_expire_check.py`
if [ x"$expire" != "x" ]; then
    echo "Cert $jks due to expire: $expire"
fi


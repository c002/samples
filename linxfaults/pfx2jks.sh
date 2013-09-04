#!/bin/sh -x
#
# Convert From PFX (PKCS12) that has no Private key password to java keystore
# ( The way these are issued, but tricky to convert to java keystore ) 
#
# This is one way of doing it ....
#
# $Source$
# $Id$

if [ $# -ne 2 ]; then
    echo "usage: in_pfx out_jks"
    exit
fi

KEYTOOL=/opt/jdk1.6.0/bin/keytool
OPENSSL=/opt/openssl/bin/openssl

# Info in PFX File. 
#keytool -v -list -storetype pkcs12 -keystore Harry\ Raamaykers\ CON.PFX

# Extract Private and Cert
$OPENSSL pkcs12 -in "$1" -nocerts -out $$new.key -passout pass:testing -passin pass:
# Get certs
$OPENSSL pkcs12 -in "$1" -nokeys -out $$new.pem -passin pass:

# Create PFX
$OPENSSL pkcs12 -export -inkey $$new.key -in $$new.pem -out $$new.pfx  -passin pass:testing -passout pass:testing

#Import PFX to JKS (pfx must have password on private key)  
$KEYTOOL -importkeystore -srckeystore $$new.pfx -srcstorepass testing -srcstoretype pkcs12  -destkeystore "$2"  -deststorepass testing  -deststoretype jks

# cleanup
rm $$new.pem $$new.key $$new.pfx

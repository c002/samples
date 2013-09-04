#!/bin/sh -x

jarsigner -verbose -keystore android-release.keystore bin/"AAPT BTA Usage widget.apk" harry

mv  bin/"AAPT BTA Usage widget.apk"  bin/"AAPT BTA Usage widget unaligned.apk"
/cygdrive/d//development/android-sdks/tools/zipalign.exe -v 4 bin/"AAPT BTA Usage widget unaligned.apk" bin/"AAPT BTA Usage widget.apk"

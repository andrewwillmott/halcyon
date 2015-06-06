#!/bin/bash

BASE="$(dirname $0)"

p4 sync
p4 edit //depot/Radiator/Lab/Fluid/Fluid_iOS.plist

xcodebuild -project Lab/Fluid/Fluid.xcodeproj -scheme Fluid_Develop_iOS archive | tee /tmp/build-Fluid_Develop_iOS.txt

# xcodebuild does not return a valid error code in archive builds -- it is
# dumb enough to run the archive step regardless. So figure it out for
# ourselves
! grep -q "failed" /tmp/build-Fluid_Develop_iOS.txt

${BASE}/notify-build.sh $? Fluid_iOS

p4 submit -d "iOS build version update" //depot/Radiator/Lab/Fluid/Fluid_iOS.plist

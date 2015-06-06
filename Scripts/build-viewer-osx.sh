#!/bin/bash

BASE="$(dirname $0)"

p4 sync
p4 edit //depot/Radiator/Lab/Viewer/Viewer_OSX.plist

xcodebuild -project Lab/Viewer/Viewer.xcodeproj -scheme "Viewer OSX" archive | tee /tmp/build-Viewer_Develop_OSX.txt

# xcodebuild does not return a valid error code in archive builds -- it is
# dumb enough to run the archive step regardless. So figure it out for
# ourselves
! grep -q "failed" /tmp/build-Viewer_Develop_OSX.txt

${BASE}/notify-build.sh $? Viewer_OSX

p4 submit -d "OSX Build version update" //depot/Radiator/Lab/Viewer/Viewer_OSX.plist

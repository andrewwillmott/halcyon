#!/bin/sh

if (($# == "0")); then
	echo "Usage: $0 <Build> <OSX|iOS>"
	echo "  pushes the given build to build server or TestFlight"
	exit
fi

p4 sync
p4 edit //depot/Radiator/Lab/$1/$1_$2.plist 
xcodebuild -project Lab/$1/$1.xcodeproj -scheme $1_$2 archive
p4 submit -d "Version update" //depot/Radiator/Lab/Fluid/$1_$2.plist


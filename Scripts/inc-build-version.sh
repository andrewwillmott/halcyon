#!/bin/bash

echo "PROJECT_DIR = $PROJECT_DIR"
echo "INFOPLIST_FILE = $INFOPLIST_FILE"

export P4CONFIG=p4config.txt
cd ${PROJECT_DIR} && ${HOME}/bin/p4 edit ${INFOPLIST_FILE}

buildVersion=$(/usr/libexec/PlistBuddy -c "Print CFBuildVersion" ${INFOPLIST_FILE})
buildNumber=$(/usr/libexec/PlistBuddy -c "Print CFBuildNumber" ${INFOPLIST_FILE})
buildDate=$(date "+%Y%m%d.%H%M%S")

buildNumber=$(($buildNumber + 1))

/usr/libexec/PlistBuddy -c "Add :CFBundleLongVersionString string" ${INFOPLIST_FILE}
/usr/libexec/PlistBuddy -c "Add :CFBuildDate string" ${INFOPLIST_FILE}

/usr/libexec/PlistBuddy -c "Set :CFBuildNumber $buildNumber" ${INFOPLIST_FILE}
/usr/libexec/PlistBuddy -c "Set :CFBuildDate $buildDate" ${INFOPLIST_FILE}

/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $buildVersion.$buildNumber" ${INFOPLIST_FILE}
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $buildVersion.$buildNumber" ${INFOPLIST_FILE}
/usr/libexec/PlistBuddy -c "Set :CFBundleLongVersionString $buildVersion.$buildNumber.$buildDate" ${INFOPLIST_FILE}

echo "Build version updated to $buildVersion.$buildNumber"

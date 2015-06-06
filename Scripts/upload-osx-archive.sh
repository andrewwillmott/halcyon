#!/bin/bash
#
# Script for extracting app from xcarchive and uploading to a build server.
#

BUILD_SERVER=cobalt.local

LOG="/tmp/upload-osx-archive.log"
/bin/rm -f $LOG

echo "Starting Upload Process" > $LOG

if [ "$SHOW_DEBUG_CONSOLE" = "YES" ]; then
    /usr/bin/open -a /Applications/Utilities/Console.app $LOG
fi

echo >> $LOG

echo "CODE_SIGN_IDENTITY: $CODE_SIGN_IDENTITY" >> $LOG
echo "ARCHIVE_PATH: $ARCHIVE_PATH" >> $LOG
echo "ARCHIVE_PRODUCTS_PATH: $ARCHIVE_PRODUCTS_PATH" >> $LOG
echo "WRAPPER_NAME: $WRAPPER_NAME" >> $LOG
echo "ARCHIVE_DSYMS_PATH: $ARCHIVE_DSYMS_PATH" >> $LOG
echo "DWARF_DSYM_FILE_NAME: $DWARF_DSYM_FILE_NAME" >> $LOG
echo "INSTALL_PATH: $INSTALL_PATH" >> $LOG
echo "PRODUCT_NAME: $PRODUCT_NAME" >> $LOG

ARCHIVE_NAME=$(basename "$ARCHIVE_PATH" .xcarchive)

echo "ARCHIVE_NAME: $ARCHIVE_NAME" >> $LOG

# TODO: how can we enable this within xcode but not command line?
#SHOULD_UPLOAD=`osascript -e "tell application \"Xcode\"" -e "set noButton to \"Cancel\"" -e "set yesButton to \"OK\"" -e "set upload_dialog to display dialog \"Do you want to upload this build to $BUILD_SERVER?\" buttons {noButton, yesButton} default button noButton with icon 1" -e "set button to button returned of upload_dialog" -e "if button is equal to yesButton then" -e "return 1" -e "else" -e "return 0" -e "end if" -e "end tell"`

echo "SHOULD_UPLOAD: $SHOULD_UPLOAD" >> $LOG

# Exit this script if the user indicated we shouldn't upload:
if [ "$SHOULD_UPLOAD" = "0" ]; then
    exit 0
fi

env >> $LOG

set -xv

# In case p4 has replicated permissions...
chmod go-rwx ${CLIENT_ROOT}/Scripts/build_key_rsa

rsync -e "ssh -o IdentityFile=${CLIENT_ROOT}/Scripts/build_key_rsa" \
    -p --chmod=ug=rwX -avzu \
    "${ARCHIVE_PRODUCTS_PATH}/Applications/${WRAPPER_NAME}" \
    "${ARCHIVE_DSYMS_PATH}/${DWARF_DSYM_FILE_NAME}" \
    build@${BUILD_SERVER}:/volume1/Radiator/Builds/"\"${ARCHIVE_NAME}\""

# ${ARCHIVE_DSYMS_PATH}/${DWARF_DSYM_FILE_NAME}

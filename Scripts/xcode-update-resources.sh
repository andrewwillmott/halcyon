#!/bin/sh
echo "Updating Data"
rsync -avzuCL "${PROJECT_DIR}"/Data/ "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/Data" --delete --exclude="OSX" --exclude="iOS" --exclude=.DS_Store


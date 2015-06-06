#!/bin/bash

# Expecting notify-build $? <ProjectLabel>

NOTIFY_ROOM=General
NOTIFY_USER=Builder
AUTH_TOKEN=b1a85c0a7661668e7dec2ee193e49e
CHANGE_LIST=`p4 changes -t -m1 ...#have`

if [[ $1 == 0 ]]; then
    curl -d "room_id=${NOTIFY_ROOM}&from=${NOTIFY_USER}&message=Build+$2:+Success++++++(${CHANGE_LIST})&color=green" "https://api.hipchat.com/v1/rooms/message?auth_token=${AUTH_TOKEN}"
else
    curl -d "room_id=${NOTIFY_ROOM}&from=${NOTIFY_USER}&message=Build+$2:+Fail+$1++++++(${CHANGE_LIST})&color=red" "https://api.hipchat.com/v1/rooms/message?auth_token=${AUTH_TOKEN}"
fi

echo "<- notification"

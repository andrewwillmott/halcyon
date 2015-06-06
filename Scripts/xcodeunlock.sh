#!/bin/bash

exec > /tmp/xcode-p4.txt 2>&1

## Set the timeout interval for dismissing the dialog:
timeout=10

## Turn on all feedback (messages).
## If set to 1, the command result is displayed
## If set to 0, the file is unlocked and checked out of Perforce
## silently (messages and errors will still be logged to console):
feedback=0

export P4CONFIG=p4config.txt
source ~/.bashrc

echo "input=" $1

# Delete the URL part from the file passed in from Xcode:
filename=${1#file://}
filename=${filename#localhost}
echo "filename=" ${filename}

# Get the enclosing directory for the file being unlocked:
filedir=$(dirname ${filename})
echo "filedir=" ${filedir}

# A check to confirm the current P4CONFIG setting:
conf=$($HOME/bin/p4 -d ${filedir} set P4CONFIG 2>&1)
echo ${conf}

if [ -a ${filename} ]; then

    # Save the dialog timeout to an environment variable:
    export XC_TO=${timeout}
	
    # Check the file out from Perforce:
    $(chmod u+w ${filename} 2>&1)
    res=$($HOME/bin/p4 -d ${filedir} edit ${filename} 2>&1)
    echo "res=" ${res}

    # Save the result as an environment variable
    export XC_RES=${res}

    if [ ${feedback} -gt 0 ]; then

        # Tell Xcode to display a dialog to with the result of the command.
        # The timeout is set by '${to}', above:
        osascript -e "tell application \"xcode\" to display dialog (system attribute \"XC_RES\") buttons {\"OK\"} default button 1 giving up after (system attribute \"XC_TO\")"
    fi
else
    echo "file not found:" ${filename}
fi


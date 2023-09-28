#!/bin/bash

#LVAR_REMOTE_HOST="octopi"
#LVAR_PATH="opencv_mjpeg_streaming_server"

LVAR_REMOTE_HOST="httas"
LVAR_PATH="ProgCpp/opencv_mjpeg_streaming_server"

#rm build/*.png 2>/dev/null

if [ -z "$LVAR_REMOTE_HOST" ]; then
	echo "Missing LVAR_REMOTE_HOST"
	exit 1
fi
if [ -z "$LVAR_PATH" ]; then
	echo "Missing LVAR_PATH"
	exit 1
fi

rsync -va --delete \
	comp.sh run.sh src \
	$LVAR_REMOTE_HOST:$LVAR_PATH/ || exit 1
rsync -va config-$LVAR_REMOTE_HOST.json \
	$LVAR_REMOTE_HOST:$LVAR_PATH/config.json || exit 1

ssh -t "$LVAR_REMOTE_HOST" "cd $LVAR_PATH; bash run.sh -c ../config.json $@"

#echo "Copying PNGs to build/ ..."
#test -d build || mkdir build
##scp $LVAR_REMOTE_HOST:$LVAR_PATH/build/*.png build/ >/dev/null
#scp $LVAR_REMOTE_HOST:/media/usbhd/*.png build/ >/dev/null

##ssh -t "$LVAR_REMOTE_HOST" "cd $LVAR_PATH; rm build/*.png" >/dev/null
#ssh -t "$LVAR_REMOTE_HOST" "rm /media/usbhd/*.png" >/dev/null

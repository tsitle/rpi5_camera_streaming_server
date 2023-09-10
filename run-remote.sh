#!/bin/bash

LVAR_REMOTE_HOST="octopi"
LVAR_PATH="opencv_mjpeg_streaming_server"

#rm build/*.png 2>/dev/null

rsync -va --delete \
        comp.sh run.sh src \
        $LVAR_REMOTE_HOST:$LVAR_PATH/ || exit 1

ssh -t "$LVAR_REMOTE_HOST" "cd $LVAR_PATH; bash run.sh $@"

#echo "Copying PNGs to build/ ..."
#test -d build || mkdir build
##scp $LVAR_REMOTE_HOST:$LVAR_PATH/build/*.png build/ >/dev/null
#scp $LVAR_REMOTE_HOST:/media/usbhd/*.png build/ >/dev/null

##ssh -t "$LVAR_REMOTE_HOST" "cd $LVAR_PATH; rm build/*.png" >/dev/null
#ssh -t "$LVAR_REMOTE_HOST" "rm /media/usbhd/*.png" >/dev/null

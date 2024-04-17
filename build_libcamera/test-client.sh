#!/bin/bash

#CFG_HOST="octopi"
CFG_HOST="rpi5pnp"

#gst-launch-1.0 tcpclientsrc host=$CFG_HOST port=8091 ! \
#		queue leaky=1 max-size-buffers=0 ! \
#		multipartdemux ! jpegdec ! \
#		videoconvert ! \
#		autovideosink

gst-launch-1.0 tcpclientsrc host=$CFG_HOST port=8091 ! \
		queue leaky=1 max-size-buffers=0 ! \
		jpegdec ! \
		videoconvert ! \
		autovideosink

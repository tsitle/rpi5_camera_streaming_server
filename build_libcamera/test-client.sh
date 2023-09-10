#!/bin/bash

#gst-launch-1.0 tcpclientsrc host=octopi port=8091 ! \
#		queue leaky=1 max-size-buffers=0 ! \
#		multipartdemux ! jpegdec ! \
#		videoconvert ! \
#		autovideosink

gst-launch-1.0 tcpclientsrc host=octopi port=8091 ! \
		queue leaky=1 max-size-buffers=0 ! \
		jpegdec ! \
		videoconvert ! \
		autovideosink

# Camera Streaming Server for Raspberry Pi 5

Minimalistic multithreaded server that provides a MJPEG stream via HTTP and also
a REST API for controlling the server.

Under the hood it uses

- Raspberry's version of [libcamera](https://github.com/raspberrypi/libcamera) through a GStreamer pipeline for acquiring the camera images
- [OpenCV](https://github.com/opencv/opencv) for adjusting the camera images

Features:

- support for one and two cameras
- support for multiple simultaneous HTTP and streaming clients
- various camera image adjustments:
  - calibration (using a chessboard pattern to find four corners of a rectangle)
  - perspective transformation (using the calibration data the image will be adjusted in order to remove distortions)
  - region of interest (removes parts of the image that are blank due to perspective transformation)
  - translation (moves the image in the X- or Y-direction)
  - horizontal and/or vertical image flipping
  - brightness and contrast
  - grid overlay
  - text overlay
- support for GStreamer and external MJPEG streams as input source for the camera image(s)
- support for adaptive FPS setting (will reduce the frames per second setting of the GStreamer input if necessary)
- REST API to control all settings

The software has been tested with:

- Raspberry Pi 5
- with two Raspberry Camera Module v3 (IMX708) cameras
- on Debian Bookworm 12.9

## Prerequisites

A custom build of OpenCV is required:

```
$ cd build_opencv/4.9.0
$ ./build-opencv-rpi.sh
```

Or if you want to test the software on another (Linux) computer you can use `./build-opencv-client.sh` instead.

This will install all dependencies, build OpenCV with appropriate flags and install it.

The dependencies notably include

- GStreamer (for `build-opencv-rpi.sh` and `build-opencv-client.sh`)
- libcamera (on `build-opencv-rpi.sh` only)

## Configuration

There are two example configuration files:

- `config-example-gstreamer.json`: uses GStreamer as input for the camera image stream(s)
- `config-example-mjpeg.json`: uses external MJPEG stream(s) as input for the camera image stream(s)

The `config-example-mjpeg.json` is intended for testing the server on a computer that doesn't have the camera(s) attached to it.  
And `config-example-gstreamer.json` is intended for using on the Raspberry Pi.

Choose one of them and copy it to `config.json` in the project's root directory.  
Then you can edit the file and adjust the settings as required or desired.

## Compiling and Running the Server

Simply run the following commands:

```
$ ./comp.sh
$ ./run.sh
```

# OpenCV MJPEG Streaming Server

```
$ cd opencv_mjpeg_streaming_server/build_opencv/4.9.0
$ ./build-opencv-rpi.sh
```

Camera support should be working out of the box on Raspberry Pi 5.

```
$ ./comp.sh
$ ./run.sh
```

## Head Adjustment - Home Z

- attach (vacuum) Needle to Head
- in Frontend go to "Head Adjustment"
- press "Ignore Z Limit"
- carefully adjust the Z-position such that the Needle is just above the Printer Plate
- press "Set Home Z"

## Camera Calibration

- in Frontend go to "Controller" (or "Head Adjustment")
- select only left or right Camera (i.e. not "CAM BOTH")
- enable "Show Grid"
- place Calibration Sheet on Printer Plate such that the slimmer side is on the Y-axis
	and the wider side (more columns of squares) is on the X-axis
	and the sheet is more or less aligned with the grid
- attach (vacuum) Needle to Head
- lower the Head to Z=0mm
- move Head around such that the center of the Calibration Sheet is under
	the Needle
- lift the Head to Z=20mm
- remove the Needle again
- in Frontend go to "Camera Adjustment"
- press button "Start CAL" (or "Reset CAL")
- Camera image should get automatically calibrated
- once the Camera image has successfully been calibrated:
	- zoom into the image a little bit
	- use the Offset adjustments to center the Calibration Sheet to the center of the grid
- repeat process for second camera

Once both Camera images have been calibrated:

- select "CAM BOTH"
- align the individual Camera images such that they match up properly

## Head Adjustment - Camera Image Offsets per Z-position

- in Frontend go to "Head Adjustment"
- select "CAM BOTH"
- lower the Head to Z=10mm
- use the "Dynamic Camera Image X/Y-axis offset" adjustments to center the Camera Images
- lower the Head to Z=0mm
- again center the images
- lift the Head to Z=30mm
- again center the images
- lift the Head in increments of 10mm on the Z-axis and center the images for each step

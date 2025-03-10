#
# by TS, Aug 2023, Apr 2023, Mar 2025
#
# https://docs.opencv.org/4.x/db/d05/tutorial_config_reference.html
#

VAR_MYNAME="$(basename "$0")"

#

[[ -z "${COMPILE_WITH_GUI}" ]] && {
	echo "${VAR_MYNAME}: Missing COMPILE_WITH_GUI. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "${COMPILE_WITH_FFMPEG}" ]] && {
	echo "${VAR_MYNAME}: Missing COMPILE_WITH_FFMPEG. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "${COMPILE_WITH_GSTREAMER}" ]] && {
	echo "${VAR_MYNAME}: Missing COMPILE_WITH_GSTREAMER. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "${COMPILE_WITH_JAVA}" ]] && {
	echo "${VAR_MYNAME}: Missing COMPILE_WITH_JAVA. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "${NEED_LIBCAMERA}" ]] && {
	echo "${VAR_MYNAME}: Missing NEED_LIBCAMERA. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "${COMPILE_STATIC_LIBS}" ]] && {
	echo "${VAR_MYNAME}: Missing COMPILE_STATIC_LIBS. Aborting." >>/dev/stderr
	exit 1
}

LCFG_OPENCV_RELEASE="opencv-4.11.0"

#

# Outputs Operating System name
#
# @return int EXITCODE
function getOsName() {
	case "${OSTYPE}" in
		linux*)
			echo -n "linux"
			;;
		darwin*)
			echo -n "macos"
			;;
		*)
			echo "${VAR_MYNAME}: Error: getOsName(): Unknown OSTYPE '${OSTYPE}'" >>/dev/stderr
			return 1
			;;
	esac
	return 0
}

getOsName >/dev/null || exit 1

LVAR_OSNAME="$(getOsName)"

#

if [ ! -f "${LCFG_OPENCV_RELEASE}.tar.gz" ]; then
	echo "${VAR_MYNAME}: You'll need to download '${LCFG_OPENCV_RELEASE}.tar.gz' from 'https://github.com/opencv/opencv/releases' first." >>/dev/stderr
	exit 1
fi

case "${LVAR_OSNAME}" in
	linux)
		echo "${VAR_MYNAME}: running 'apt-get update'"
		sudo apt-get update || exit 1
		echo "${VAR_MYNAME}: running 'apt-get install [...]'"
		sudo apt-get install -y \
				g++ \
				cmake \
				make \
				git \
				pkg-config \
				libgnutls28-dev \
				libv4l-dev \
				|| exit 1
		;;
	macos)
		#echo "${VAR_MYNAME}: running 'brew install [...]'"
		#brew install \
		#		cmake \
		#		|| exit 1
		;;
	*)
		echo "${VAR_MYNAME}: Error: Unknown OSTYPE '$OSTYPE'" >>/dev/stderr
		exit 1
		;;
esac

if [ ! -d "${LCFG_OPENCV_RELEASE}" ]; then
	tar xf "${LCFG_OPENCV_RELEASE}.tar.gz" || exit 1
fi

if [ -d "build" ]; then
	cd build || exit 1
	case "${LVAR_OSNAME}" in
		linux)
			echo "${VAR_MYNAME}: running 'make uninstall'"
			sudo make uninstall
			;;
		macos)
			;;
	esac
	cd .. || exit 1
fi

test -d build && rm -fr build
mkdir build || exit 1
cd build || exit 1

TMP_OPT_HIGHGUI="OFF"
if [ "${COMPILE_WITH_GUI}" = "true" ]; then
	TMP_OPT_HIGHGUI="ON"

	case "${LVAR_OSNAME}" in
		linux)
			echo "${VAR_MYNAME}: running 'apt-get install [...]'"
			sudo apt-get install -y \
					libgtk2.0-dev \
					|| exit 1
			;;
		macos)
			;;
	esac
fi

TMP_OPT_FFMPEG="OFF"
if [ "${COMPILE_WITH_FFMPEG}" = "true" ]; then
	TMP_OPT_FFMPEG="ON"

	case "${LVAR_OSNAME}" in
		linux)
			echo "${VAR_MYNAME}: running 'apt-get install [...]'"
			# libavresample-dev doesn't seem to exist anymore. Replacing it with libswresample-dev
			# libavresample4 doesn't seem to exist anymore. Replacing it with libswresample4
			sudo apt-get install -y \
					libavcodec-dev \
					libavformat-dev \
					libavutil-dev \
					libswscale-dev \
					libswresample-dev \
					libswresample4 \
					libjpeg-dev \
					libopenjp2-7-dev \
					libpng-dev \
					|| exit 1
			;;
		macos)
			echo "${VAR_MYNAME}: running 'brew install [...]'"
			brew install \
					ffmpeg \
					|| exit 1
			;;
	esac
fi

TMP_OPT_GSTREAMER="OFF"
if [ "${COMPILE_WITH_GSTREAMER}" = "true" ]; then
	TMP_OPT_GSTREAMER="ON"

	case "${LVAR_OSNAME}" in
		linux)
			echo "${VAR_MYNAME}: running 'apt-get install [...]'"
			sudo apt-get install -y \
					libgstreamer1.0-dev \
					libgstreamer-plugins-base1.0-dev \
					libgstreamer-plugins-bad1.0-dev \
					gstreamer1.0-plugins-base \
					gstreamer1.0-plugins-base-apps \
					gstreamer1.0-plugins-good \
					gstreamer1.0-plugins-bad \
					gstreamer1.0-plugins-ugly \
					gstreamer1.0-libav \
					gstreamer1.0-tools \
					gstreamer1.0-alsa \
					gstreamer1.0-gl \
					gstreamer1.0-pulseaudio \
					|| exit 1
			if [ "${COMPILE_WITH_GUI}" = "true" ]; then
				echo "${VAR_MYNAME}: running 'apt-get install [...]'"
				sudo apt-get install -y \
						gstreamer1.0-x \
						gstreamer1.0-gtk3 \
						gstreamer1.0-qt5 \
						|| exit 1
			fi
			;;
		macos)
			;;
	esac
fi

TMP_OPT_JAVA="OFF"
if [ "${COMPILE_WITH_JAVA}" = "true" ]; then
	TMP_OPT_JAVA="ON"

	case "${LVAR_OSNAME}" in
		linux)
			echo "${VAR_MYNAME}: running 'apt-get install [...]'"
			sudo apt-get install -y \
					ant \
					|| exit 1
			;;
		macos)
			echo "${VAR_MYNAME}: running 'brew install [...]'"
			brew install \
					ant \
					|| exit 1
			;;
	esac
fi

if [ "${NEED_LIBCAMERA}" = "true" ]; then
	case "${LVAR_OSNAME}" in
		linux)
			echo "${VAR_MYNAME}: running 'apt-get install [...]'"
			sudo apt-get install -y \
					libcamera-dev \
					|| exit 1
			;;
		macos)
			;;
	esac
fi

TMP_OPT_SHARED="ON"
if [ "${COMPILE_STATIC_LIBS}" = "true" ]; then
	TMP_OPT_SHARED="OFF"
fi

LVAR_BIN_CMAKE=""
if command -v cmake >/dev/null; then
	LVAR_BIN_CMAKE="cmake"
else
	LTMP_OS="$(getOsName)"
	if [ "${LTMP_OS}" = "macos" -a -x "${HOME}/Applications/CLion.app/Contents/bin/cmake/mac/x64/bin/cmake" ]; then
		LVAR_BIN_CMAKE="${HOME}/Applications/CLion.app/Contents/bin/cmake/mac/x64/bin/cmake"
	elif [ "${LTMP_OS}" = "linux" ]; then
		TMP_FIND_FN="tmp.find-$$.tmp"
		test -f "${TMP_FIND_FN}" && rm "${TMP_FIND_FN}"
		find /opt -maxdepth 1 -type d -name "clion-*" | sort -r > "${TMP_FIND_FN}"
		while IFS= read -r TMP_DN; do
			LVAR_BIN_CMAKE="$(find "${TMP_DN}" -type f -name "cmake" -perm -u=x 2>/dev/null)"
			break
		done < "${TMP_FIND_FN}"
		rm "${TMP_FIND_FN}"
	fi
fi

if [ -z "${LVAR_BIN_CMAKE}" ]; then
	echo "${VAR_MYNAME}: Couldn't find cmake. Aborting." >>/dev/stderr
	exit 1
fi

echo "${VAR_MYNAME}: running 'cmake [...]'"
${LVAR_BIN_CMAKE} \
		-D WITH_LIBV4L=ON \
		-D WITH_V4L=ON \
		-D WITH_FFMPEG=${TMP_OPT_FFMPEG} \
		-D WITH_GSTREAMER=${TMP_OPT_GSTREAMER} \
		-D WITH_MSMF=OFF \
		-D WITH_DSHOW=OFF \
		-D WITH_AVFOUNDATION=OFF \
		-D WITH_1394=OFF \
		-D WITH_ANDROID_MEDIANDK=OFF \
		-D VIDEOIO_ENABLE_PLUGINS=OFF \
		-DBUILD_TESTS=OFF \
		-DBUILD_PERF_TESTS=OFF \
		-DBUILD_SHARED_LIBS=${TMP_OPT_SHARED} \
		-DBUILD_opencv_calib3d=ON \
		-DBUILD_opencv_dnn=OFF \
		-DBUILD_opencv_highgui=${TMP_OPT_HIGHGUI} \
		-DBUILD_opencv_java=${TMP_OPT_JAVA} \
		-DBUILD_opencv_js=OFF \
		-DBUILD_opencv_ml=OFF \
		-DBUILD_opencv_objc=OFF \
		-DBUILD_opencv_python=OFF \
		-DOPENCV_FORCE_LIBATOMIC_COMPILER_CHECK=1 \
		"../${LCFG_OPENCV_RELEASE}" || exit 1

#

echo "${VAR_MYNAME}: running 'make [...]'"
make -j4 || exit 1

echo
case "${LVAR_OSNAME}" in
	linux)
		echo "${VAR_MYNAME}: running 'make install'"
		sudo make install
		;;
	macos)
		echo "${VAR_MYNAME}: Not installing OpenCV in system"
		;;
esac

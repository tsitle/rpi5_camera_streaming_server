#
# by TS, Aug 2023, Apr 2023, Mar 2025
#
# https://docs.opencv.org/4.x/db/d05/tutorial_config_reference.html
#

VAR_MYNAME="$(basename "$0")"

#

[[ -z "${CFG_COMPILE_WITH_GUI}" ]] && {
	echo "${VAR_MYNAME}: Missing CFG_COMPILE_WITH_GUI. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "${CFG_COMPILE_WITH_FFMPEG}" ]] && {
	echo "${VAR_MYNAME}: Missing CFG_COMPILE_WITH_FFMPEG. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "${CFG_COMPILE_WITH_GSTREAMER}" ]] && {
	echo "${VAR_MYNAME}: Missing CFG_COMPILE_WITH_GSTREAMER. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "${CFG_COMPILE_WITH_PYTHON3}" ]] && {
	echo "${VAR_MYNAME}: Missing CFG_COMPILE_WITH_PYTHON3. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "${CFG_COMPILE_WITH_JAVA}" ]] && {
	echo "${VAR_MYNAME}: Missing CFG_COMPILE_WITH_JAVA. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "${CFG_NEED_LIBCAMERA}" ]] && {
	echo "${VAR_MYNAME}: Missing CFG_NEED_LIBCAMERA. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "${CFG_COMPILE_STATIC_LIBS}" ]] && {
	echo "${VAR_MYNAME}: Missing CFG_COMPILE_STATIC_LIBS. Aborting." >>/dev/stderr
	exit 1
}
[[ -z "${CFG_INSTALL_TO_SYSTEM}" ]] && {
	echo "${VAR_MYNAME}: Missing CFG_INSTALL_TO_SYSTEM. Aborting." >>/dev/stderr
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

LVAR_IS_DEBIAN=false
LVAR_IS_REDHAT=false
LVAR_LX_PKGMAN=""
if [ "${LVAR_OSNAME}" = "linux" ]; then
	command -v apt-get >/dev/null 2>&1 && LVAR_IS_DEBIAN=true
	if [ "${LVAR_IS_DEBIAN}" = "true" ]; then
		LVAR_LX_PKGMAN="apt-get"
	else
		command -v dnf >/dev/null 2>&1 && LVAR_IS_REDHAT=true
		if [ "${LVAR_IS_REDHAT}" = "true" ]; then
			LVAR_LX_PKGMAN="dnf"
		else
			echo "${VAR_MYNAME}: Error: Cannot detect Linux variant" >>/dev/stderr
			exit 1
		fi
	fi
fi

#

if [ ! -f "${LCFG_OPENCV_RELEASE}.tar.gz" ]; then
	echo "${VAR_MYNAME}: You'll need to download '${LCFG_OPENCV_RELEASE}.tar.gz' from 'https://github.com/opencv/opencv/releases' first." >>/dev/stderr
	exit 1
fi

case "${LVAR_OSNAME}" in
	linux)
		if [ "${LVAR_IS_DEBIAN}" = "true" ]; then
			echo "${VAR_MYNAME}: running '${LVAR_LX_PKGMAN} update'"
			sudo ${LVAR_LX_PKGMAN} update || exit 1
		else
			# not running 'dnf update' here because it would try to update
			# already installed packages
			echo -n
		fi
		echo "${VAR_MYNAME}: running '${LVAR_LX_PKGMAN} install [...]'"
		if [ "${LVAR_IS_DEBIAN}" = "true" ]; then
			TMP_PACKS="libgnutls28-dev libv4l-dev"
		else
			TMP_PACKS="gnutls-devel libv4l-devel"
		fi
		sudo ${LVAR_LX_PKGMAN} install -y \
				g++ \
				cmake \
				make \
				git \
				pkg-config \
				${TMP_PACKS} \
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

test -d build && rm -fr build
mkdir build && cd build

TMP_OPT_HIGHGUI="OFF"
if [ "${CFG_COMPILE_WITH_GUI}" = "true" ]; then
	TMP_OPT_HIGHGUI="ON"

	case "${LVAR_OSNAME}" in
		linux)
			echo "${VAR_MYNAME}: running '${LVAR_LX_PKGMAN} install [...]'"
			if [ "${LVAR_IS_DEBIAN}" = "true" ]; then
				TMP_PACKS="libgtk2.0-dev"
			else
				TMP_PACKS="gtk2-devel"
			fi
			sudo ${LVAR_LX_PKGMAN} install -y \
					${TMP_PACKS} \
					|| exit 1
			;;
		macos)
			;;
	esac
fi

TMP_OPT_FFMPEG="OFF"
if [ "${CFG_COMPILE_WITH_FFMPEG}" = "true" ]; then
	TMP_OPT_FFMPEG="ON"

	case "${LVAR_OSNAME}" in
		linux)
			echo "${VAR_MYNAME}: running '${LVAR_LX_PKGMAN} install [...]'"
			# libavresample-dev doesn't seem to exist anymore. Replacing it with libswresample-dev
			# libavresample4 doesn't seem to exist anymore. Replacing it with libswresample4
			if [ "${LVAR_IS_DEBIAN}" = "true" ]; then
				TMP_PACKS="libavcodec-dev"
				TMP_PACKS+=" libavformat-dev"
				TMP_PACKS+=" libavutil-dev"
				TMP_PACKS+=" libswscale-dev"
				TMP_PACKS+=" libswresample-dev"
				TMP_PACKS+=" libswresample4"
				TMP_PACKS+=" libjpeg-dev"
				TMP_PACKS+=" libopenjp2-7-dev"
				TMP_PACKS+=" libpng-dev"
			else
				TMP_PACKS="libavcodec-free-devel"
				TMP_PACKS+=" libavformat-free-devel"
				TMP_PACKS+=" libavutil-free-devel"
				TMP_PACKS+=" libswscale-free-devel"
				TMP_PACKS+=" libswresample-free-devel"
				#TMP_PACKS+=" "  # libswresample4 does not exist for Rocky Linux 9.5
				TMP_PACKS+=" libjpeg-turbo-devel"
				TMP_PACKS+=" openjpeg2-devel"
				TMP_PACKS+=" libpng-devel"
			fi
			sudo ${LVAR_LX_PKGMAN} install -y \
					${TMP_PACKS} \
					|| exit 1
			;;
		macos)
			if ! command -v ffmpeg >/dev/null 2>&1; then
				echo "${VAR_MYNAME}: running 'brew install [...]'"
				brew install \
						ffmpeg \
						|| exit 1
			fi
			;;
	esac
fi

TMP_OPT_GSTREAMER="OFF"
if [ "${CFG_COMPILE_WITH_GSTREAMER}" = "true" ]; then
	TMP_OPT_GSTREAMER="ON"

	case "${LVAR_OSNAME}" in
		linux)
			echo "${VAR_MYNAME}: running '${LVAR_LX_PKGMAN} install [...]'"
			if [ "${LVAR_IS_DEBIAN}" = "true" ]; then
				TMP_PACKS="libgstreamer1.0-dev"
				TMP_PACKS+=" libgstreamer-plugins-base1.0-dev"
				TMP_PACKS+=" libgstreamer-plugins-bad1.0-dev"
				TMP_PACKS+=" gstreamer1.0-plugins-base"
				TMP_PACKS+=" gstreamer1.0-plugins-base-apps"
				TMP_PACKS+=" gstreamer1.0-plugins-good"
				TMP_PACKS+=" gstreamer1.0-plugins-bad"
				TMP_PACKS+=" gstreamer1.0-plugins-ugly"
				TMP_PACKS+=" gstreamer1.0-libav"
				TMP_PACKS+=" gstreamer1.0-tools"
				TMP_PACKS+=" gstreamer1.0-alsa"
				TMP_PACKS+=" gstreamer1.0-gl"
				TMP_PACKS+=" gstreamer1.0-pulseaudio"
			else
				TMP_PACKS="gstreamer1-devel"
				TMP_PACKS+=" gstreamer1-plugins-base-devel"
				TMP_PACKS+=" gstreamer1-plugins-bad-free-devel"
				TMP_PACKS+=" gstreamer1-plugins-base"
				TMP_PACKS+=" gstreamer1-plugins-base-tools"
				TMP_PACKS+=" gstreamer1-plugins-good"
				TMP_PACKS+=" gstreamer1-plugins-bad"
				TMP_PACKS+=" gstreamer1-plugins-ugly"
				TMP_PACKS+=" gstreamer1-libav"
				#TMP_PACKS+=" "  # gstreamer1.0-tools does not exist for Rocky Linux 9.5
				#TMP_PACKS+=" "  # gstreamer1.0-alsa does not exist for Rocky Linux 9.5
				#TMP_PACKS+=" "  # gstreamer1.0-gl does not exist for Rocky Linux 9.5
				#TMP_PACKS+=" "  # gstreamer1.0-pulseaudio does not exist for Rocky Linux 9.5
			fi
			sudo ${LVAR_LX_PKGMAN} install -y \
					${TMP_PACKS} \
					|| exit 1
			if [ "${CFG_COMPILE_WITH_GUI}" = "true" ] && [ "${LVAR_IS_DEBIAN}" = "true" ]; then
				echo "${VAR_MYNAME}: running '${LVAR_LX_PKGMAN} install [...]'"
				sudo ${LVAR_LX_PKGMAN} install -y \
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

TMP_OPT_PY3="OFF"
if [ "${CFG_COMPILE_WITH_PYTHON3}" = "true" ]; then
	TMP_OPT_PY3="ON"
fi

TMP_OPT_JAVA="OFF"
if [ "${CFG_COMPILE_WITH_JAVA}" = "true" ]; then
	TMP_OPT_JAVA="ON"

	if ! command -v ant >/dev/null 2>&1; then
		case "${LVAR_OSNAME}" in
			linux)
				echo "${VAR_MYNAME}: running '${LVAR_LX_PKGMAN} install [...]'"
				sudo ${LVAR_LX_PKGMAN} install -y \
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
fi

if [ "${CFG_NEED_LIBCAMERA}" = "true" ]; then
	case "${LVAR_OSNAME}" in
		linux)
			echo "${VAR_MYNAME}: running '${LVAR_LX_PKGMAN} install [...]'"
			if [ "${LVAR_IS_DEBIAN}" != "true" ]; then
				echo "${VAR_MYNAME}: Error: libcamera-dev should only be installed on Debian" >>/dev/stderr
				exit 1
			fi
			sudo ${LVAR_LX_PKGMAN} install -y \
					libcamera-dev \
					|| exit 1
			;;
		macos)
			;;
	esac
fi

TMP_OPT_SHARED="ON"
if [ "${CFG_COMPILE_STATIC_LIBS}" = "true" ]; then
	TMP_OPT_SHARED="OFF"
fi

TMP_OPT_V4L="OFF"
if [ "${LVAR_OSNAME}" = "linux" ]; then
	TMP_OPT_V4L="ON"
fi

LVAR_BIN_CMAKE=""
if command -v cmake >/dev/null 2>&1; then
	LVAR_BIN_CMAKE="cmake"
else
	LTMP_OS="$(getOsName)"
	if [ "${LTMP_OS}" = "macos" ] && [ -x "${HOME}/Applications/CLion.app/Contents/bin/cmake/mac/x64/bin/cmake" ]; then
		LVAR_BIN_CMAKE="${HOME}/Applications/CLion.app/Contents/bin/cmake/mac/x64/bin/cmake"
	elif [ "${LTMP_OS}" = "macos" ]; then
		echo "${VAR_MYNAME}: running 'brew install [...]'"
		brew install \
				cmake \
				|| exit 1
		LVAR_BIN_CMAKE="$(command -v cmake)"
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

echo -e "\n${VAR_MYNAME}: running 'cmake [...]'"
${LVAR_BIN_CMAKE} \
		-DWITH_LIBV4L=${TMP_OPT_V4L} \
		-DWITH_V4L=${TMP_OPT_V4L} \
		-DWITH_FFMPEG=${TMP_OPT_FFMPEG} \
		-DWITH_GSTREAMER=${TMP_OPT_GSTREAMER} \
		-DWITH_OPENEXR=OFF \
		-DWITH_MSMF=OFF \
		-DWITH_DSHOW=OFF \
		-DWITH_AVFOUNDATION=OFF \
		-DWITH_1394=OFF \
		-DWITH_ANDROID_MEDIANDK=OFF \
		-DVIDEOIO_ENABLE_PLUGINS=OFF \
		-DBUILD_TESTS=OFF \
		-DBUILD_PERF_TESTS=OFF \
		-DBUILD_SHARED_LIBS=${TMP_OPT_SHARED} \
		-DBUILD_OPENEXR=OFF \
		-DBUILD_opencv_calib3d=ON \
		-DBUILD_opencv_dnn=OFF \
		-DBUILD_opencv_highgui=${TMP_OPT_HIGHGUI} \
		-DBUILD_opencv_java=${TMP_OPT_JAVA} \
		-DOPENCV_JAVA_TARGET_VERSION=1.8 \
		-DBUILD_opencv_js=OFF \
		-DBUILD_opencv_ml=OFF \
		-DBUILD_opencv_objc=OFF \
		-DBUILD_opencv_python2=OFF \
		-DBUILD_opencv_python3=${TMP_OPT_PY3} \
		-DOPENCV_FORCE_LIBATOMIC_COMPILER_CHECK=1 \
		"../${LCFG_OPENCV_RELEASE}" || exit 1

#

nproc_polyfill() {
	if [ "${LVAR_OSNAME}" = "linux" ]; then
		nproc --all
	elif [ "${LVAR_OSNAME}" = "macos" ] || [ "$(uname -s | grep -q BSD)" = "BSD" ]; then
		sysctl -n hw.ncpu
	else
		getconf _NPROCESSORS_ONLN  # glibc/coreutils fallback
	fi
}

echo -e "\n${VAR_MYNAME}: running 'make [...]'"
make -j"$(nproc_polyfill)" || exit 1

if [ "${CFG_INSTALL_TO_SYSTEM}" = "true" ]; then
	echo "${VAR_MYNAME}: running 'make install'"
	sudo make install || exit 1

	echo -e "\n${VAR_MYNAME}: (run '\$ sudo make uninstall' to uninstall it again)"
else
	echo -e "\n${VAR_MYNAME}: Not installing OpenCV into system"
fi

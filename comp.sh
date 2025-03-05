#!/usr/bin/env bash

VAR_MYNAME="$(basename "$0")"

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

#

if ! test -d build; then
	mkdir build
	cd build || exit 1
	${LVAR_BIN_CMAKE} -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ../src || exit 1
	cd ..
fi
cd build || exit 1
make || exit 1

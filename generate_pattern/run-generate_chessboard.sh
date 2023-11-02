#!/bin/bash

#
# by TS, Sep 2023
#
# Source for gen_pattern.py:
#   https://raw.githubusercontent.com/opencv/opencv/4.x/doc/pattern_tools/gen_pattern.py
#
# Source for svgfig.py:
#   https://raw.githubusercontent.com/opencv/opencv/4.x/doc/pattern_tools/svgfig.py
#

if [ ! -d ./venv ]; then
	echo -e "Creating VENV...\n"
	python3 -m venv venv || exit 1
	./venv/bin/python3 -m pip install -r requirements.txt || exit 1
fi

function genCirleboard() {
	test -f pattern-circleboard.svg && rm pattern-circleboard.svg
	./venv/bin/python3 \
			gen_pattern.py \
				-o pattern-circleboard.svg \
				--rows 7 \
				--columns 5 \
				--type circles \
				--square_size 15
}

function genAcirleboard() {
	test -f pattern-acircleboard.svg && rm pattern-acircleboard.svg
	./venv/bin/python3 \
			gen_pattern.py \
				-o pattern-acircleboard.svg \
				--rows 7 \
				--columns 5 \
				--type acircles \
				--square_size 10 \
				--radius_rate 2
}

function genChessboard() {
	local CFG_ROWS=5
	local CFG_COLS=7
	local CFG_SQSZ=5

	TMP_STR_ROWS="$(printf "%02d" $((CFG_ROWS-1)))"
	TMP_STR_COLS="$(printf "%02d" $((CFG_COLS-1)))"
	TMP_STR_SQSZ="$(printf "%02d" $CFG_SQSZ)"

	local TMP_FN="pattern-chessboard-inner_${TMP_STR_ROWS}x${TMP_STR_COLS}_${TMP_STR_SQSZ}mm.svg"
	test -f "$TMP_FN" && rm "$TMP_FN"
	./venv/bin/python3 \
			gen_pattern.py \
				-o "$TMP_FN" \
				--rows $CFG_ROWS \
				--columns $CFG_COLS \
				--type checkerboard \
				--square_size $CFG_SQSZ
}

#genCirleboard
#genAcirleboard
genChessboard

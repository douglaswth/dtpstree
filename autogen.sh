#!/usr/bin/env bash
# DT PS Tree
#
# Douglas Thrift
#
# configure.ac

#  Copyright 2010 Douglas Thrift
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

set -e
shopt -s extglob
cd `dirname $0`

ac_m4s=(check_gnu_make.m4)
ac_dir=`aclocal --print-ac-dir`

for ac_m4 in ${ac_m4s[@]}; do
	if [[ $ac_dir/$ac_m4 -nt $ac_m4 ]]; then
		install -m 644 $ac_dir/$ac_m4 .
		rm -fv aclocal.m4
	fi
done

aclocal

for ac_m4 in ${ac_m4s[@]}; do
	if ! grep -q "^# ${ac_m4//./\\.}$" aclocal.m4; then
		echo -e "# $ac_m4\n$(<$ac_m4)" >> aclocal.m4
	fi
done

automake -acf || true
autoconf

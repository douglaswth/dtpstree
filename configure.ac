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

AC_PREREQ([2.62])
AC_INIT([DT PS Tree], [1.0.2], [douglas@douglasthrift.net], [dtpstree], [http://code.douglasthrift.net/trac/dtpstree])
AC_CONFIG_SRCDIR([dtpstree.cpp])
AC_CONFIG_AUX_DIR([build-aux])
CHECK_GNU_MAKE
AC_SUBST([GMAKE], [$_cv_gnu_make_command])
AS_IF([test "x$GMAKE" = x], [AC_MSG_ERROR([You need GNU make])])
AC_PROG_INSTALL
AC_PATH_PROG([HELP2MAN], [help2man], [:])
AC_MSG_CHECKING([for realpath])
AC_PATH_PROGS_FEATURE_CHECK([REALPATH], [realpath readlink greadlink gnureadlink],
	[AS_CASE([$ac_path_REALPATH], [*realpath],
		[ac_cv_path_REALPATH=$ac_path_REALPATH
		ac_path_REALPATH_found=:],
		[AS_IF([$ac_path_REALPATH -f . >/dev/null 2>&1],
			[ac_cv_path_REALPATH="$ac_path_REALPATH -f"
			ac_path_REALPATH_found=:]
		)]
	)]
)
AS_IF([test "x$ac_cv_path_REALPATH" != x],
	[AC_SUBST([REALPATH], [$ac_cv_path_REALPATH])
	AC_MSG_RESULT([$REALPATH])],
	[AC_SUBST([REALPATH], [:])
	AC_MSG_RESULT([no])]
)
AC_MSG_CHECKING([for BSD tar with xz support])
AC_PATH_PROGS_FEATURE_CHECK([BSDTAR], [tar bsdtar],
	[AS_IF([test "x`$ac_path_BSDTAR --version 2>/dev/null | grep '^bsdtar '`" != x],
		[AS_IF([$ac_path_BSDTAR -cJf - /dev/null >/dev/null 2>&1],
			[ac_cv_path_BSDTAR=$ac_path_BSDTAR
			ac_path_BSDTAR_found=:]
		)]
	)]
)
AS_IF([test "x$ac_cv_path_BSDTAR" != x],
	[AC_SUBST([BSDTAR], [$ac_cv_path_BSDTAR])
	AC_MSG_RESULT([$BSDTAR])],
	[AC_SUBST([BSDTAR], [:])
	AC_MSG_RESULT([no])]
)
AC_LANG([C++])
AC_DEFUN([DT_MSG_TERMCAP], [AC_MSG_ERROR([You need terminfo or termcap])])
AC_DEFUN([DT_CHECK_TERMCAP], [AC_CHECK_HEADERS([termcap.h])
	AS_CASE([$ac_cv_header_termcap_h], [no], [DT_MSG_TERMCAP],
		[AC_SEARCH_LIBS([tgetent], [termcap termlib], [], [DT_MSG_TERMCAP])]
	)]
)
AC_CHECK_HEADERS([ncurses/curses.h curses.h], [break])
AS_CASE([$ac_cv_header_curses_h$ac_cv_header_ncurses_curses_h], [*yes*],
	[AC_CHECK_HEADERS([ncurses/term.h term.h], [break])
	AS_CASE([$ac_cv_header_term_h$ac_cv_header_ncurses_term_h], [*yes*],
		[AC_SEARCH_LIBS([setupterm], [ncurses curses terminfo tinfo], [],
			[DT_CHECK_TERMCAP]
		)], [DT_CHECK_TERMCAP]
	)], [DT_CHECK_TERMCAP]
)
AC_DEFUN([DT_MSG_KVM], [AC_MSG_ERROR([You need libkvm])])
AC_CHECK_HEADER([kvm.h], [], [DT_MSG_KVM])
AC_SEARCH_LIBS([kvm_openfiles], [kvm], [], [DT_MSG_KVM])
AC_CONFIG_FILES([GNUmakefile])
AC_OUTPUT

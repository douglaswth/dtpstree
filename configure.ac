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
AC_SUBST([GNUMAKE], [$_cv_gnu_make_command])
AS_IF([test "x$GNUMAKE" = x], [AC_MSG_ERROR([You need GNU make])])
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
AC_DEFUN([DT_CHECK_TERMCAP], [AC_CHECK_HEADERS([termcap.h ncurses/termcap.h])
	AS_CASE([$ac_cv_header_termcap_h$ac_cv_header_ncurses_termcap_h], [*yes*],
		[AC_SEARCH_LIBS([tgetent],
			[termcap termlib terminfo tinfo ncurses pdcurses curses], [],
			[DT_MSG_TERMCAP]
		)], [DT_MSG_TERMCAP]
	)]
)
AC_DEFUN([DT_CHECK_TERMINFO],
	[AC_CHECK_HEADERS([curses.h ncurses/ncurses.h],
	[break])
	AS_CASE([$ac_cv_header_curses_h$ac_cv_header_ncurses_ncurses_h], [*yes*],
		[AC_CHECK_HEADERS([term.h ncurses/term.h], [break])
		AS_CASE([$ac_cv_header_term_h$ac_cv_header_ncurses_term_h], [*yes*],
			[AC_SEARCH_LIBS([setupterm],
				[terminfo tinfo ncurses pdcurses curses], [], [DT_CHECK_TERMCAP]
			)], [DT_CHECK_TERMCAP]
		)], [DT_CHECK_TERMCAP]
	)]
)
AC_ARG_ENABLE([terminfo],
	[AS_HELP_STRING([--disable-terminfo],
		[do not try to use the terminfo interface]
	)], [AS_CASE([$enableval], [no], [DT_CHECK_TERMCAP], [DT_CHECK_TERMINFO])],
	[DT_CHECK_TERMINFO]
)
AC_DEFUN([DT_MSG_BSD], [AC_MSG_ERROR([You need libbsd])])
AC_CHECK_HEADER([vis.h], [], [DT_MSG_BSD])
AC_SEARCH_LIBS([strvis], [bsd], [], [DT_MSG_BSD])
AC_DEFUN([DT_MSG_KVM], [AC_MSG_ERROR([You need libkvm])])
AC_CHECK_HEADER([kvm.h], [], [DT_MSG_KVM])
AC_CHECK_HEADERS_ONCE([kinfo.h])
AC_SEARCH_LIBS([kvm_openfiles], [kvm], [], [DT_MSG_KVM])
AC_DEFUN([DT_INCLUDES_KVM], [#ifdef HAVE_KINFO_H
#include <kinfo.h>
#endif
#include <kvm.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
AC_INCLUDES_DEFAULT])
AC_DEFUN([DT_CHECK_MEMBERS_KVM],
	[m4_foreach_w([dt_member_suffix], [pid ppid ruid comm],
		[AC_CHECK_MEMBER([struct ]$1[.]$2[_]dt_member_suffix,
			[AC_DEFINE([HAVE_STRUCT_KINFO_PROCX_]m4_toupper($2)[_]m4_toupper(dt_member_suffix))],
			[AC_CHECK_MEMBER([struct ]$1[.]$3[_]dt_member_suffix,
				[AC_DEFINE([HAVE_STRUCT_KINFO_PROCX_]m4_toupper($3)[_]m4_toupper(dt_member_suffix))],
				[AC_CHECK_MEMBER([struct ]$1[.]$4[_]dt_member_suffix,
					[AC_DEFINE([HAVE_STRUCT_KINFO_PROCX_]m4_toupper($4)[_]m4_toupper(dt_member_suffix))],
					[DT_MSG_KVM], [DT_INCLUDES_KVM]
				)], [DT_INCLUDES_KVM]
			)], [DT_INCLUDES_KVM]
		)]
	)]
)
AC_CHECK_TYPE([struct kinfo_proc2],
	[AC_DEFINE([HAVE_STRUCT_KINFO_PROC2])
	AC_CHECK_FUNC([kvm_getargv2], [], [DT_MSG_KVM])
	AC_CHECK_FUNC([kvm_getproc2], [], [DT_MSG_KVM])
	DT_CHECK_MEMBERS_KVM([kinfo_proc2], [p], [ki], [kp])],
	[AC_CHECK_TYPE([struct kinfo_proc],
		[AC_DEFINE([HAVE_STRUCT_KINFO_PROC])
		AC_CHECK_FUNC([kvm_getargv], [], [DT_MSG_KVM])
		AC_CHECK_FUNC([kvm_getprocs], [], [DT_MSG_KVM])
		DT_CHECK_MEMBERS_KVM([kinfo_proc], [ki], [kp], [p])],
		[DT_MSG_KVM], [DT_INCLUDES_KVM]
	)], [DT_INCLUDES_KVM]
)
AC_CHECK_DECLS([KERN_PROC_PROC, KERN_PROC_KTHREAD, KERN_PROC_ALL], [], [],
	[DT_INCLUDES_KVM]
)
AS_CASE([$ac_cv_have_decl_KERN_PROC_PROC$ac_cv_have_decl_KERN_PROC_KTHREAD$ac_cv_have_decl_KERN_PROC_ALL],
	[*yes*], [], [DT_MSG_KVM]
)
AS_CASE([$GNUMAKE], [make], [], [AC_CONFIG_FILES([makefile])])
AC_CONFIG_FILES([GNUmakefile])
AC_OUTPUT

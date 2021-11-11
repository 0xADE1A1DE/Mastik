#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([mastik], [0.1], [yval@cs.adelaide.edu.au])
AC_CONFIG_SRCDIR([src/symbol.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_HEADERS([demo/config.h])

# Checks for programs.
AC_PROG_CC

AC_PROG_CC_C99

AC_ARG_ENABLE(symbols, [AS_HELP_STRING([--disable-symbols], [do not use symbols in binaries])])
AC_ARG_ENABLE(debug-symbols, [AS_HELP_STRING([--disable-debug-symbols], [do not use debugging symbols in binaries])])
AC_ARG_ENABLE(doubloon, [AS_HELP_STRING([--disable-doubloon], [do not install doubloon])])
AC_ARG_ENABLE(experimental, [AS_HELP_STRING([--enable-experimental], [enable experimental features])])



# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h strings.h unistd.h sched.h sys/prctl.h])

AC_PATH_PROG(UNAME, uname)
AC_CHECK_PROGS(PYTHON, [python3 python])
AC_PATH_PROG(PYTHON, [${PYTHON}])

BUILD_OS="unknown"
if test x"$UNAME" != x; then
  BUILD_OS=`$UNAME`
fi

DOUBLOON_HAVE_DWARF="False"
if test "$enable_symbols" != "no"; then
  # We first identify a Mac using the mach-o headers.  Failing that, try Linux files.
  AC_CHECK_HEADERS([mach-o/loader.h])
  if test x"$ac_cv_header_mach_o_loader_h" = x"yes"; then
    SYMBOL_SRCS=symbol_mach.c 
    AC_DEFINE([HAVE_SYMBOLS], [1], [Have support for parsing loader symbol table])
  else
    AC_CHECK_HEADERS([bfd.h], [], [AC_ERROR([bfd.h is required for symbols support.])])
    AC_CHECK_LIB([bfd], [bfd_init], [], [AC_ERROR([A working libbfd is required for symbols support])])
    AC_CHECK_LIB([elf], [elf_version], [], [AC_ERROR([A working libelf is required for symbols support])])

    SYMBOL_SRCS=symbol_bfd.c
    AC_DEFINE([HAVE_SYMBOLS], [1], [Have support for parsing loader symbol table])

    MYINCLUDE="#include <bfd.h>"
    AC_COMPILE_IFELSE(
	 [AC_LANG_PROGRAM([[$MYINCLUDE]],
			  [[bfd_section_vma((bfd*)0,(asection*)0);]])],
       AC_DEFINE([HAVE_LEGACY_BFD], [1], [Have legacy bfd.h]))


    if test "$enable_debug_symbols" != "no"; then 
      AC_CHECK_LIB([dwarf], [dwarf_init], [], [AC_ERROR([A working libdwarf is required for debug-symbols support])])
      AC_CHECK_HEADERS([libdwarf.h libdwarf/libdwarf.h], [mastik_have_dwarf="yes"])
      if test x"$mastik_have_dwarf" = x"yes" ; then
	AC_DEFINE([HAVE_DEBUG_SYMBOLS], [1], [Have support for debug symbols])
	AC_DEFINE([HAVE_DWARF], [1], [Have support for dwarf])
	DOUBLOON_HAVE_DWARF="True"
      else
	AC_ERROR([libdwarf.h is required for debug-symbols support])
      fi
    fi 
  fi
fi

case "$enable_experimental" in
  yes) DOUBLOON_EXPERIMENTAL="True";;
  *)   DOUBLOON_EXPERIMENTAL="False";;
esac

if test x"${BUILD_OS}" = x"Darwin"; then
  DOUBLOON_TEXTBOX_HEIGHT=22
else
  DOUBLOON_TEXTBOX_HEIGHT=30
fi

PYVER=unknown
if test x"$PYTHON" != x; then
  case "`${PYTHON} -V`" in
    "Python 2."*) PYVER=2;;
    "Python 3."*) PYVER=3;;
  esac
fi

TOOLS=""
if test x"$enable_doubloon" != x"no"; then 
  if test x"${PYVER}" = "x3"; then
    TOOLS+=" Doubloon"
    DOUBLOON=yes
    missingmodules=""
    for module in wx numpy paramiko matplotlib tqdm; do
      ${PYTHON} -c "import ${module}" >/dev/null 2>&1
      if test x"$?" != x"0"; then
	missingmodules+=" $module"
      fi
    done
    if test x"$missingmodules" != x""; then
      AC_WARN([Doubloon needs these missing modules: $missingmodules])
    fi
  else
    AC_WARN([Cannot find Python 3.  Not installing Doubloon])
  fi
fi






# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([bzero memset munmap strchr strdup strtoul strtoull uname])
AC_CHECK_FUNCS([mmap64 sched_setaffinity])


if test x"$ac_cv_prog_cc_c99" != x"no"; then
  CFLAGS+="  $ac_cv_prog_cc_c99"
fi


AC_SUBST(SYMBOL_SRCS, ["$SYMBOL_SRCS"])
AC_SUBST(DOUBLOON_TEXTBOX_HEIGHT)
AC_SUBST(DOUBLOON_HAVE_DWARF)
AC_SUBST(DOUBLOON_EXPERIMENTAL)
AC_SUBST(TOOLS)
AC_SUBST(PYTHON)

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([mastik/Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([bin/Makefile])
AC_CONFIG_FILES([demo/Makefile])
AC_CONFIG_FILES([tests/Makefile])
AC_CONFIG_FILES([tools/Makefile])
if test x"$DOUBLOON" = x"yes"; then
  AC_CONFIG_FILES([tools/Doubloon/Makefile])
fi
AC_OUTPUT

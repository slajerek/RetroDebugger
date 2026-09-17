dnl vim: set et ts=4 sw=4 sts=4:
dnl
dnl \brief  Compiler check macros for VICE
dnl
dnl A few macros to simplify checking compiler and linker features.
dnl Incomplete and unused at the moment for 'code review'. The idea is to add
dnl VICE_CXXFLAGS_[REQUEST|REQUIRE]() as well, and perhaps linker checks,
dnl should we require those.
dnl
dnl \author Bas Wassink <b.wassink@ziggo.nl>


dnl VICE_CFLAG_REQUEST(flag, [flags_var_name=VICE_CFLAGS])
dnl
dnl Check if the C compiler supports `flag` and add it when suppported
dnl
dnl \param[in]  flag            compiler flag to check
dnl \param[in]  flags_var_name  name of flags variable to update (optional,
dnl                             defaults to VICE_CFLAGS)
dnl
dnl \todo   Use 'local' variables (pushdef/popdef) to avoid name clashes in
dnl         other m4 or configure.ac code
dnl
AC_DEFUN([VICE_CFLAG_REQUEST],
[
    dnl Handle optional 'flags_var_name' argument
    flags_var_name=m4_default($2, [VICE_CFLAGS])
    AS_VAR_COPY([old_var_CFLAGS], [$flags_var_name])

    AC_LANG_PUSH([C])
    AC_MSG_CHECKING([if the C compiler supports $1])

    dnl We need -Werror to make AC_TRY_COMPILE error out with Clang, and we
    dnl need it to be before any -Werror=foo flags to make gcc 9.2+ error out,
    dnl otherwise gcc 9.2 will complain but still return 0.
    dnl We need -Wno-strict-prototypes to avoid barfing on 'main()'
    old_CFLAGS="$CFLAGS"
    CFLAGS="-Werror $old_var_CFLAGS $1 -Wno-strict-prototypes"

    dnl Try compiling a minimal piece of code to test the requested flag
    AC_TRY_COMPILE(
        [],
        [int the_answer = 42; return the_answer;],
        [AC_MSG_RESULT([yes])
         AS_VAR_SET([$flags_var_name], ["$old_var_CFLAGS $1"])],
        [AC_MSG_RESULT([no])]
    )
    CFLAGS="$old_CFLAGS"
    AC_LANG_POP([C])
])


dnl VICE_CFLAG_REQUIRE(flag)
dnl
dnl Check if the compiler supports `flag` and error out when not suppported
dnl
AC_DEFUN([VICE_CFLAG_REQUIRE],
[
    AC_LANG_PUSH([C])
    AC_MSG_CHECKING([if the C compiler supports $1])
    old_CFLAGS="$CFLAGS"
    dnl We need -Werror to make AC_TRY_COMPILE error out with Clang
    CFLAGS="-Werror $VICE_CFLAGS $1 -Wno-strict-prototypes"
    AC_TRY_COMPILE(
        [],
        [int poop = 42; return poop;],
        [AC_MSG_RESULT([yes])
         VICE_CFLAGS="$VICE_CFLAGS $1"],
        [AC_MSG_ERROR([no, $1 is required])]
    )
    CFLAGS="$old_CFLAGS"
    AC_LANG_POP([C])
])


dnl VICE_CXXFLAG_REQUEST(flag, [flags_var_name=VICE_CXXFLAGS])
dnl
dnl Check if the C++ compiler supports `flag` and add it when suppported
dnl
dnl \param[in]  flag            compiler flag to check
dnl \param[in]  flags_var_name  name of flags variable to update (optional,
dnl                             defaults to VICE_CXXFLAGS)
dnl
dnl \todo   Use 'local' variables (pushdef/popdef) to avoid name clashes in
dnl         other m4 or configure.ac code
dnl
AC_DEFUN([VICE_CXXFLAG_REQUEST],
[
    dnl Handle optional 'flags_var_name' argument
    flags_var_name=m4_default($2, [VICE_CXXFLAGS])
    AS_VAR_COPY([old_var_CXXFLAGS], [$flags_var_name])

    AC_LANG_PUSH([C++])
    AC_MSG_CHECKING([if the C++ compiler supports $1])

    dnl We need -Werror to make AC_TRY_COMPILE error out with Clang, and we
    dnl need it to be before any -Werror=foo flags to make gcc 9.2+ error out,
    dnl otherwise gcc 9.2 will complain but still return 0.
    dnl We need -Wno-strict-prototypes to avoid barfing on 'main()'
    old_CXXFLAGS="$CXXFLAGS"
    CXXFLAGS="-Werror $old_var_CXXFLAGS $1"

    dnl Try compiling a minimal piece of code to test the requested flag
    AC_TRY_COMPILE(
        [],
        [int the_answer = 42; return the_answer;],
        [AC_MSG_RESULT([yes])
         AS_VAR_SET([$flags_var_name], ["$old_var_CXXFLAGS $1"])],
        [AC_MSG_RESULT([no])]
    )
    CXXFLAGS="$old_CXXFLAGS"
    AC_LANG_POP([C++])
])



#
# init the vice_arg_enable_list and vice_arg_with_list vars
#
# VICE_ARG_INIT()
#
# Written by Marco van den Heuvel.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# VICE_ARG_INIT
# -------------
AC_DEFUN([VICE_ARG_INIT],
[
vice_arg_enable_list="dependency-tracking silent-rules"
vice_arg_with_list=""
])# VICE_ARG_INIT

#
# expand the AC_ARG_ENABLE macro to handle adding the item name to a list
#
# VICE_ARG_ENABLE_LIST(FEATURE, HELP-STRING, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
#
# Written by Marco van den Heuvel.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# VICE_ARG_ENABLE_LIST
# --------------------
AC_DEFUN([VICE_ARG_ENABLE_LIST],
[
AC_ARG_ENABLE($1, [$2], [$3], [$4])
vice_arg_enable_list="[$]vice_arg_enable_list $1"
])# VICE_ARG_ENABLE_LIST

#
# expand the AC_ARG_WITH macro to handle adding the item name to a list
#
# VICE_ARG_WITH_LIST(FEATURE, HELP-STRING, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
#
# Written by Marco van den Heuvel.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# VICE_ARG_WITH_LIST
# ------------------
AC_DEFUN([VICE_ARG_WITH_LIST],
[
AC_ARG_WITH($1, [$2], [$3], [$4])
vice_arg_with_list="[$]vice_arg_with_list $1"
])# VICE_ARG_WITH_LIST

#
# compare the elements in the arguments list to the elements in the
# valid enable/with arguments list, if any of the arguments is invalid
# an AC_MSG_ERROR() will be called
#
# Written by Marco van den Heuvel.
#
# VICE_ARG_LIST_CHECK()
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# VICE_ARG_LIST_CHECK
#
# Arguments:    $1  options passed to configure
#               $2  "yes" or "no" depending on --enable-option-checking
# -------------------
AC_DEFUN([VICE_ARG_LIST_CHECK],
[

dnl --enable-*, --disable-*, --with-* and --without-* sanity checks
for argcheck in $1
do
  argcheck=`echo "$argcheck" | sed 's/=.*//'`
  argvalid=yes
  AS_CASE([[$]argcheck],
      [--enable-option-checking], [argvalid=yes],
      [--enable-*], [
        argvalid=no
        for i in [$]vice_arg_enable_list
        do
          AS_IF([test x"[$]argcheck" = x"--enable-[$]i"], [argvalid=yes])
        done],
      [--disable-option-checking], [argvalid=yes],
      [--disable-*], [
        argvalid=no
        for i in [$]vice_arg_enable_list
        do
          AS_IF([test x"[$]argcheck" = x"--disable-[$]i"], [argvalid=yes])
        done],
      [--with-*], [
        argvalid=no
        for i in [$]vice_arg_with_list
        do
          AS_IF([test x"[$]argcheck" = x"--with-[$]i"], [argvalid=yes])
        done],
      [--without-*], [
        argvalid=no
        for i in [$]vice_arg_with_list
        do
          AS_IF([test x"[$]argcheck" = x"--without-[$]i"], [argvalid=yes])
        done],
      [])
  AS_IF([test x"$2" = "xyes"],
    AS_IF([test x"[$]argvalid" = "xno"],
        [AC_MSG_ERROR([invalid option: [$]argcheck])]))
  AS_IF([test x"$2" = "xno"],
    AS_IF([test x"[$]argvalid" = "xno"],
        [AC_MSG_WARN([invalid option: [$]argcheck])]))
done
])# VICE_ARG_LIST_CHECK

AN_IDENTIFIER([ssize_t], [AC_TYPE_SSIZE_T])
AC_DEFUN([AC_TYPE_SSIZE_T], [AC_CHECK_TYPE(ssize_t, int)])

#
# Ensure that at most one of the arguments given is in the target list
#
# Written by Michael C. Martin.
#
# VICE_ARG_LIST_AT_MOST_ONE([options],[group name])
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# VICE_ARG_LIST_AT_MOST_ONE
# -------------------------
AC_DEFUN([VICE_ARG_LIST_AT_MOST_ONE], [
vice_arg_list_amo_1=none
vice_arg_list_amo_2=none

for vice_arg_list_command_option in [$]@
do
  for vice_arg_list_entry in $1; do
    AS_IF([test x"$vice_arg_list_command_option" = x"$vice_arg_list_entry"],
          [AS_IF([test x"$vice_arg_list_amo_1" = "xnone"],
                 [vice_arg_list_amo_1=$vice_arg_list_entry],
                 [vice_arg_list_amo_2=$vice_arg_list_entry])])
  done
done
AS_IF([test x"$vice_arg_list_amo_2" != "xnone"],
      [AC_MSG_ERROR([conflicting $2 options: $vice_arg_list_amo_1 $vice_arg_list_amo_2])])
])#VICE_ARG_LIST_AT_MOST_ONE

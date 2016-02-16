dnl $Id$
dnl config.m4 for extension firephp

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(firephp, for firephp support,
dnl Make sure that the comment is aligned:
dnl [  --with-firephp             Include firephp support])

dnl Otherwise use enable:

 PHP_ARG_ENABLE(firephp, whether to enable firephp support,
 Make sure that the comment is aligned:
 [  --enable-firephp           Enable firephp support])

if test "$PHP_FIREPHP" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-firephp -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/firephp.h"  # you most likely want to change this
  dnl if test -r $PHP_FIREPHP/$SEARCH_FOR; then # path given as parameter
  dnl   FIREPHP_DIR=$PHP_FIREPHP
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for firephp files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       FIREPHP_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$FIREPHP_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the firephp distribution])
  dnl fi

  dnl # --with-firephp -> add include path
  dnl PHP_ADD_INCLUDE($FIREPHP_DIR/include)

  dnl # --with-firephp -> check for lib and symbol presence
  dnl LIBNAME=firephp # you may want to change this
  dnl LIBSYMBOL=firephp # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $FIREPHP_DIR/$PHP_LIBDIR, FIREPHP_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_FIREPHPLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong firephp lib version or lib not found])
  dnl ],[
  dnl   -L$FIREPHP_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(FIREPHP_SHARED_LIBADD)

  PHP_NEW_EXTENSION(firephp, firephp.c, $ext_shared)
fi

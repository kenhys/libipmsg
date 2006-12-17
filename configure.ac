#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(libipmsg, 0.1.1)
AC_CONFIG_SRCDIR([src/ipmsg.h])

# Checks for programs.
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h unistd.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_PROG_GCC_TRADITIONAL
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_UTIME_NULL
AC_CHECK_FUNCS([inet_ntoa memset mkdir select socket strerror strtoul strtoull utime])

# Checks for libraries.
AC_ARG_WITH(openssl,
		AC_HELP_STRING([--with-openssl],[Encryption support @<:@default=yes@:>@]), ,[with_openssl=yes])
if test "x$with_openssl" = xyes; then
	have_evp_open_init=yes
	have_evp_open_update=yes
	have_evp_open_final=yes
	have_evp_encrypt_init=yes
	have_evp_encrypt_update=yes
	have_evp_encrypt_final=yes
	have_evp_cipher_ctx_init=yes
	have_evp_cipher_ctx_set_key_length=yes
	have_evp_pkey_set1_rsa=yes
	echo "checking for OpenSSL EVP api..."
	AC_CHECK_LIB(ssl,EVP_OpenInit,LIBSSL="-lssl -lcrypto", have_evp_open_init=no)
	#EVP_DecryptUpdate = EVP_OpenUpdate
	AC_CHECK_LIB(ssl,EVP_DecryptUpdate,LIBSSL="-lssl -lcrypto", have_evp_open_update=no)
	AC_CHECK_LIB(ssl,EVP_OpenFinal,LIBSSL="-lssl -lcrypto", have_evp_open_final=no)
	AC_CHECK_LIB(ssl,EVP_EncryptInit,LIBSSL="-lssl -lcrypto", have_evp_encrypt_init=no)
	AC_CHECK_LIB(ssl,EVP_EncryptUpdate,LIBSSL="-lssl -lcrypto", have_evp_encrypt_update=no)
	AC_CHECK_LIB(ssl,EVP_EncryptFinal,LIBSSL="-lssl -lcrypto", have_evp_encrypt_final=no)
	AC_CHECK_LIB(ssl,EVP_CIPHER_CTX_init,LIBSSL="-lssl -lcrypto", have_evp_cipher_ctx_init=no)
	AC_CHECK_LIB(ssl,EVP_CIPHER_CTX_set_key_length,LIBSSL="-lssl -lcrypto", have_evp_cipher_ctx_set_key_length=no)
	AC_CHECK_LIB(ssl,EVP_PKEY_set1_RSA,LIBSSL="-lssl -lcrypto", have_evp_pkey_set1_rsa=no)
	if test "x$have_evp_open_init" = xyes &&
	   test "x$have_evp_open_update" = xyes &&
	   test "x$have_evp_open_final" = xyes &&
	   test "x$have_evp_open_init" = xyes &&
	   test "x$have_evp_open_update" = xyes &&
	   test "x$have_evp_open_final" = xyes &&
	   test "x$have_evp_cipher_ctx_init" = xyes &&
	   test "x$have_evp_cipher_ctx_set_key_length" = xyes &&
	   test "x$have_evp_pkey_set1_rsa" = xyes; then
		AC_DEFINE(HAVE_OPENSSL, 1, [Encryption support])
		echo Encryption support enabled.
	else
		AC_MSG_ERROR([--with-openssl was given, but test for libssl failed])
	fi
else
	echo libipmsg was not support encryption.
fi

AC_ARG_WITH(pthread,
		AC_HELP_STRING([--with-pthread],[Encryption support @<:@default=yes@:>@]), ,[with_pthread=yes])
if test "x$with_pthread" = xyes; then
	echo "checking for pthread..."
	have_pthread_create=yes
	have_pthread_detach=yes
	have_pthread_mutex_init=yes
	have_pthread_mutex_lock=yes
	have_pthread_mutex_unlock=yes
	AC_CHECK_LIB(pthread,pthread_create,LIBPTHREAD="-lpthread", have_pthread_create=no)
	AC_CHECK_LIB(pthread,pthread_detach,LIBPTHREAD="-lpthread", have_pthread_detach=no)
	AC_CHECK_LIB(pthread,pthread_mutex_init,LIBPTHREAD="-lpthread", have_pthread_mutex_init=no)
	AC_CHECK_LIB(pthread,pthread_mutex_lock,LIBPTHREAD="-lpthread", have_pthread_mutex_lock=no)
	AC_CHECK_LIB(pthread,pthread_mutex_unlock,LIBPTHREAD="-lpthread", have_pthread_mutex_unlock=no)
	if test "x$have_pthread_create" = xyes &&
	   test "x$have_pthread_detach" = xyes &&
	   test "x$have_pthread_mutex_init" = xyes &&
	   test "x$have_pthread_mutex_lock" = xyes &&
	   test "x$have_pthread_mutex_unlock" = xyes ;then 
		AC_DEFINE(HAVE_PTHREAD, 1, [pthread support])
		echo POSIX thread support enabled.
	else
		AC_MSG_ERROR([--with-pthread was given, but test for libpthread failed])
	fi
else
	echo libipmsg was not support POSIX thread.
fi

AC_ARG_ENABLE(debug,
		AC_HELP_STRING([--enable-debug],[Debug message support @<:@default=no@:>@]), ,[enable_debug=no])
if test "x$enable_debug" = xno; then
	CXXFLAGS="-DNDEBUG"
	echo Debug support disabled.
else
	CXXFLAGS="-g -DDEBUG -DINFO -Werror -Wall"
	CXXFLAGS="-g -DDEBUG -DINFO -Werror -Wall -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -Wswitch-enum"
#	CXXFLAGS="-g -DDEBUG -DINFO -Werror -Wall -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -Wswitch-enum -Weffc++"
#	CXXFLAGS="$CXXFLAGS -Wextra -Wstrict-aliasing=2 -Woverload-virtual"
#	CXXFLAGS="$CXXFLAGS -W"
	echo Debug support enabled.
fi

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile])
AC_OUTPUT

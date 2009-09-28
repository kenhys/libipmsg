#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(libipmsg, 0.1.8)
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
AC_CHECK_FUNCS([inet_ntoa memset mkdir select socket strerror strtoul strtoull utime, getifaddrs])

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
	AC_CHECK_LIB(crypto,EVP_OpenInit,LIBSSL="$LDFLAGS -lssl -lcrypto", have_evp_open_init=no)
	#EVP_DecryptUpdate = EVP_OpenUpdate
	AC_CHECK_LIB(crypto,EVP_DecryptUpdate,LIBSSL="$LDFLAGS -lssl -lcrypto", have_evp_open_update=no)
	AC_CHECK_LIB(crypto,EVP_OpenFinal,LIBSSL="$LDFLAGS -lssl -lcrypto", have_evp_open_final=no)
	AC_CHECK_LIB(crypto,EVP_EncryptInit,LIBSSL="$LDFLAGS -lssl -lcrypto", have_evp_encrypt_init=no)
	AC_CHECK_LIB(crypto,EVP_EncryptUpdate,LIBSSL="$LDFLAGS -lssl -lcrypto", have_evp_encrypt_update=no)
	AC_CHECK_LIB(crypto,EVP_EncryptFinal,LIBSSL="$LDFLAGS -lssl -lcrypto", have_evp_encrypt_final=no)
	AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,LIBSSL="$LDFLAGS -lssl -lcrypto", have_evp_cipher_ctx_init=no)
	AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_set_key_length,LIBSSL="$LDFLAGS -lssl -lcrypto", have_evp_cipher_ctx_set_key_length=no)
	AC_CHECK_LIB(crypto,EVP_PKEY_set1_RSA,LIBSSL="$LDFLAGS -lssl -lcrypto", have_evp_pkey_set1_rsa=no)
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
		LIBS="$LIBS -lssl -lcrypto"
		echo Encryption support enabled.
	else
		AC_MSG_ERROR([--with-openssl was given, but test for libssl failed])
	fi
else
	echo libipmsg was not support encryption.
fi

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
	LIBS="$LIBS -lpthread"
	echo POSIX thread support enabled.
else
	AC_MSG_ERROR([libipmsg required pthread library, but test for libpthread failed])
fi

AC_ARG_ENABLE(ipv4,
		AC_HELP_STRING([--enable-ipv4],[IPv4 support @<:@default=yes@:>@]), ,[enable_ipv4=yes])
if test "x$enable_ipv4" = xno; then
	AC_DEFINE(DISABLE_IPV4, 1, [ipv4 support])
	echo IPv4 support disabled.
else
	AC_DEFINE(ENABLE_IPV4, 1, [ipv4 support])
	echo IPv4 support enabled.
fi

AC_ARG_ENABLE(ipv6,
		AC_HELP_STRING([--enable-ipv6],[IPv6 support @<:@default=yes@:>@]), ,[enable_ipv6=yes])
if test "x$enable_ipv6" = xno; then
	AC_DEFINE(DISABLE_IPV6, 1, [ipv6 support])
	echo IPv6 support disabled.
else
	AC_DEFINE(ENABLE_IPV6, 1, [ipv6 support])
	echo IPv6 support enabled.
fi

if test "x$enable_ipv4" = xno &&
   test "x$enable_ipv6" = xno; then
	AC_MSG_ERROR([libipmsg required --disable-ipv4 and --disable-ipv6, so enable protocol nothing])
fi

AC_ARG_ENABLE(debug,
		AC_HELP_STRING([--enable-debug],[Debug message support @<:@default=no@:>@]), ,[enable_debug=no])
if test "x$enable_debug" = xno; then
	CXXFLAGS="$CXXFLAGS -DNDEBUG"
	echo Debug support disabled.
else
	CXXFLAGS="$CXXFLAGS -g -DDEBUG -DINFO -Werror -Wall -Wformat=2 -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wfloat-equal -Wpointer-arith -Wswitch-enum"
	echo Debug support enabled.
fi

AC_ARG_ENABLE(lockdebug,
		AC_HELP_STRING([--enable-lockdebug],[Debug lock trace message support, needs --enable-debug @<:@default=no@:>@]), ,[enable_lockdebug=no])
if test "x$enable_lockdebug" = xno; then
	echo Debug lock trace support disabled.
else
	if test "x$enable_debug" = xno; then
		AC_MSG_ERROR([--enable-lockdebug was given, but no specified --enable-debug option])
	else
		CXXFLAGS="$CXXFLAGS -DLOCK_DEBUG"
		echo Debug lock trace support enabled.
	fi
fi

AC_ARG_ENABLE(trace,
		AC_HELP_STRING([--enable-trace],[Debug trace message support, needs --enable-debug @<:@default=no@:>@]), ,[enable_trace=no])
if test "x$enable_trace" = xno; then
	echo Debug trace support disabled.
else
	if test "x$enable_debug" = xno; then
		AC_MSG_ERROR([--enable-trace was given, but no specified --enable-debug option])
	else
		CXXFLAGS="$CXXFLAGS -DDEBUG_TRACE"
		echo Debug trace support enabled.
	fi
fi

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile])
AC_OUTPUT

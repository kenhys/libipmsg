#!/usr/bin/bash
CC="/opt/SUNWspro/bin/cc" CXX="/opt/SUNWspro/bin/CC" CFLAGS="-I/usr/sfw/include -I/opt/fsw4sun/include -D_POSIX_PTHREAD_SEMANTICS" CXXFLAGS="-I/usr/sfw/include -I/opt/fsw4sun/include -D_POSIX_PTHREAD_SEMANTICS" LDFLAGS="-L/opt/fsw4sun/lib -L/opt/fsw4snu/ssl/lib -L/usr/sfw/lib" ./configure --prefix=/opt/fsw4sun

lib_LTLIBRARIES = libipmsg.la
libipmsg_la_SOURCES = IpMessenger.cpp IpMessengerImpl.h IpMessengerImpl.cpp ipmsg.h HostList.cpp AttachFile.cpp RecievedMessage.cpp SentMessage.cpp PrivateUtils.cpp Crypto.cpp Network.cpp
libipmsgincludedir = $(includedir)
libipmsginclude_HEADERS = IpMessenger.h

######################
#  LDFLAGSの例
######################
#  更新履歴                         -version-info  ファイル名
#                                 (current:age:rev) 
# --------------------------------------------------------------------
#  初期状態                         0:0:0           libipmsg.so.0.0.0
#  些細なバグを修正                 0:1:0           libipmsg.so.0.0.1
#  些細なバグを修正                 0:2:0           libipmsg.so.0.0.2
#  新規のインターフェースを追加     1:0:1           libipmsg.so.0.1.0
#  新規のインターフェースを追加     2:0:2           libipmsg.so.0.2.0
#  些細なバグを修正                 2:1:2           libipmsg.so.0.2.1
#  既存のインターフェースを変更     3:0:2           libipmsg.so.1.2.0
#  既存のインターフェースを削除     4:0:0           libipmsg.so.4.0.0
#  些細なバグを修正                 4:1:0           libipmsg.so.4.0.1
#  新規のインターフェースを追加     5:0:1           libipmsg.so.4.1.0
#  既存のインターフェースを変更     6:0:1           libipmsg.so.5.1.0
#  既存のインターフェースを削除     7:0:0           libipmsg.so.7.0.0

#0.1.5
libipmsg_la_LDFLAGS = -version-info 1:5:1

%define name libipmsg
%define version 0.1.2
%define release 0vl0

Summary: 	Ip Messenger Library
Summary(ja):Ipメッセンジャーライブラリ
Name: 		%{name}
Version: 	%{version}
Release: 	%{release}
Source0: 	%{name}-%{version}.tar.gz
License: 	LGPL
Group:		System Environment/Libraries
BuildRoot: 	%{_tmppath}/%{name}-buildroot
Requires: 	openssl
BuildRequires: 	openssl-devel

%description
Ip Messenger Library is comunicate with The IpMessenger(famous windows application)

%description -l ja
Ip Messenger ライブラリはIpMessengerと会話します(Windows上の有名なアプリケーション)

%package devel
Summary: 	Ip Messenger Library Development kit
Summary(ja):Ipメッセンジャーライブラリ開発キット
Group:		System Environment/Libraries
Requires: 	openssl-devel
%description devel
The Ip Messenger Library Development kit

%description devel -l ja
Ip Messenger ライブラリの開発キット

%prep
%setup -q
%configure --prefix=/usr

%build
make

%install
# cleanup
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%doc ChangeLog README TODO AUTHORS COPYING
%{_prefix}/lib/libipmsg.so*

%files devel
%{_prefix}/include/IpMessenger.h
%{_prefix}/lib/libipmsg.*a

%changelog
* Mon Jan 29 2006 Kuninobu Niki <nikikuni@yahoo.co.jp> 0.1.2-0vl0
- New upstream release.

* Fri Dec 29 2006 Kuninobu Niki <nikikuni@yahoo.co.jp> 0.1.1-0vl0
- New upstream release.

* Sat Nov 18 2006 Kuninobu Niki <nikikuni@yahoo.co.jp> 0.1.0-0vl0
- Initial version.

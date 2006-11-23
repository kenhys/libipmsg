%define name libipmsg
%define version 0.1.0
%define release 1

Summary: 	Ip Messenger Library
Summary(ja):Ip��å��󥸥㡼�饤�֥��
Name: 		%{name}
Version: 	%{version}
Release: 	%{release}
Source0: 	%{name}-%{version}.tar.gz
License: 	LGPL
Group:		System Environment/Libraries
BuildRoot: 	%{_tmppath}/%{name}-buildroot
Requires: 	kdelibs, kdebase, qt
BuildRequires: 	kdelibs-devel, kdebase-devel, qt-devel

%description
Ip Messenger Library is comunicate with The IpMessenger(famous windows application)

%description -l ja
Ip Messenger �饤�֥���IpMessenger�Ȳ��ä��ޤ�(Windows���ͭ̾�ʥ��ץꥱ�������)

%package devel
Summary: 	Ip Messenger Library Development kit
Summary(ja):Ip��å��󥸥㡼�饤�֥�곫ȯ���å�
Group:		System Environment/Libraries
Requires: 	kdelibs-devel, kdebase-devel, qt-devel
%description devel
The Ip Messenger Library Development kit

%description -l ja
Ip Messenger �饤�֥��γ�ȯ���å�

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
* Sat Nov 18 2006 Kuninobu Niki <nikikuni@yahoo.co.jp> 0.1.0
- Initial version

Name:           mbpfan
Version:        2.0.2
Release:        1%{?dist}
Summary:        Daemon to control fan speed on apple devices

Group:          Package
License:        GPLv3
URL:            https://github.com/dgraziotin/mbpfan
Source0:        mbpfan-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-root
BuildArch:      x86_64


%description
mbpfan is a daemon that uses input from coretemp module and sets the fan speed using the applesmc module.
This enhanced version assumes any number of processors and fans (max. 10).

*  It only uses the temperatures from the processors as input.
*  It requires coretemp and applesmc kernel modules to be loaded.
*  It requires root use
*  It daemonizes or stays in foreground
*  Verbose mode for both syslog and stdout
*  Users can configure it using the file /etc/mbpfan.conf

%global debug_package %{nil}

%prep
%setup -q -n %{name}-%{version}

%build
make all

%install
make install DESTDIR=$RPM_BUILD_ROOT
install -m644 mbpfan.service $RPM_BUILD_ROOT/lib/systemd/system/

%clean
rm -rf $RPM_BUILD_ROOT

%files
/usr/sbin/mbpfan
/etc/mbpfan.conf
/lib/systemd/system/mbpfan.service
/usr/share/man/man8/mbpfan.8.gz
/usr/share/doc/mbpfan/README.md

%post
systemctl enable mbpfan.service
systemctl start mbpfan.service

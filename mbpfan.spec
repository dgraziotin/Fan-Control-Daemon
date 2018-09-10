Name:           mbpfan
URL:            https://github.com/dgraziotin/mbpfan
License:        GPLv3
Group:          System Environment/Daemons
Version:        %{SOURCE_VERSION}
Release:        3
Summary:        A simple daemon to control fan speed on all MacBook/MacBook Pros (probably all Apple computers) for Linux 3.x.x and 4.x.x
Source:         v%{version}.tar.gz

%description
This is an enhanced version of Allan McRae mbpfan

mbpfan is a daemon that uses input from coretemp module and sets the fan speed using the applesmc module. This enhanced version assumes any number of processors and fans (max. 10).

It only uses the temperatures from the processors as input.
It requires coretemp and applesmc kernel modules to be loaded.
It requires root use
It daemonizes or stays in foreground
Verbose mode for both syslog and stdout
Users can configure it using the file /etc/mbpfan.conf

%prep
%setup -q -n %{name}-%{version}

%build
make

%install
install -D -m755 bin/mbpfan $RPM_BUILD_ROOT/usr/sbin/mbpfan
install -D -m644 mbpfan.conf $RPM_BUILD_ROOT/etc/mbpfan.conf
install -D -m644 mbpfan.service $RPM_BUILD_ROOT/usr/lib/systemd/system/mbpfan.service

%clean
rm -rf $RPM_BUILD_ROOT

%post
%systemd_post mbpfan.service
echo "mbpfan will auto detect sane values for min and max fan speeds."
echo  "If you want to customize these values please edit:"
echo "/etc/mbpfan.conf"
echo "To start the daemon now type:"
echo "systemctl start mbpfan"
echo "To run also at boot, type:"
echo "systemctl enable mbpfan"

%preun
%systemd_preun mbpfan.service

%postun
%systemd_postun_with_restart mbpfan.service

%files
%defattr (-,root,root)
%doc AUTHORS README.md
/usr/sbin/mbpfan
%config /etc/mbpfan.conf
/usr/lib/systemd/system/mbpfan.service

%changelog
* Mon Sep 10 2018 Michele Codutti <codutti@gmail.com> - 2.0.2-3
- Removed autoconfig with suggested procedure because has been integrated on mbpfan.

* Sun Aug 19 2018 Michele Codutti <codutti@gmail.com> - 2.0.2-2
- Autoconfig with suggested procedure.
- Initial packaging

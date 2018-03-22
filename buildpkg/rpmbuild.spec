
%global __os_install_post %{nil}


Summary: SysUp Daemon for UPS Monitoring
Name: %{name}
Version: %{version}
Release: %{release}
Prefix: /fsapp
Group: Applications/System
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}
Packager: Dongeon kim
Vendor: SysUp Inc.
License: Copyright SysUp 2017-
#Url: www.gabia.com
#Requires: freeipmi, dmidecode, OpenIPMI
# 자동 dependency 체크 제거 (패키지 디렉토리 안에 포함되 있기 때문에)
#AutoReqProv: no
# rpm binary strip disable
# strip enable시에는 pyinstaller로 만든 바이너리가 동작 안함
# %global __os_install_post %{nil}


%description
UPS 모니터링을 위한 서버데몬

%prep
rm -rf $RPM_BUILD_ROOT
rm -rf %{_builddir}/%{name}

%build
install -d -m 755 %{_builddir}/%{name}/buildpkg
install -d -m 755 %{_builddir}/%{name}/bin
install -d -m 755 %{_builddir}/%{name}/conf
install -d -m 755 %{_builddir}/%{name}/log
install -d -m 755 %{_builddir}/%{name}/lib
install -d -m 755 %{_builddir}/%{name}/scripts

cd %{extend_path}/src/fmsd
make clean all

install -m 755 %{extend_path}/src/fmsd/fmsd %{_builddir}/%{name}/bin/fmsd
install -m 755 %{extend_path}/conf/fmsd.conf %{_builddir}/%{name}/conf/fmsd.conf
install -m 755 %{extend_path}/scripts/fmsd.init %{_builddir}/%{name}/scripts/fmsd.init
install -m 755 %{extend_path}/lib/fmsd_lib.tar.gz %{_builddir}/%{name}/lib/fmsd_lib.tar.gz

cd %{_builddir}/%{name}/buildpkg

%install
install -d -m 755 $RPM_BUILD_ROOT%{prefix}/%{name}
install -d -m 755 $RPM_BUILD_ROOT%{prefix}/%{name}/bin
install -d -m 755 $RPM_BUILD_ROOT%{prefix}/%{name}/conf
install -d -m 755 $RPM_BUILD_ROOT%{prefix}/%{name}/log
install -d -m 755 $RPM_BUILD_ROOT%{prefix}/%{name}/lib
install -d -m 755 $RPM_BUILD_ROOT%{prefix}/%{name}/scripts

install -m 755 %{_builddir}/%{name}/bin/fmsd $RPM_BUILD_ROOT%{prefix}/%{name}/bin/fmsd
install -m 755 %{_builddir}/%{name}/conf/fmsd.conf $RPM_BUILD_ROOT%{prefix}/%{name}/conf/fmsd.conf
install -m 755 %{_builddir}/%{name}/scripts/fmsd.init $RPM_BUILD_ROOT%{prefix}/%{name}/scripts/fmsd.init
install -m 755 %{_builddir}/%{name}/lib/fmsd_lib.tar.gz $RPM_BUILD_ROOT%{prefix}/%{name}/lib/fmsd_lib.tar.gz

os=$(cat /etc/redhat-release | awk '{print $1}')

# shared library(libcom_err.so.2 등) prelink로 인한 rpm 설치시 cpio: MD5 sum mismatch 에러 문제
find $RPM_BUILD_ROOT%{prefix}/%{name} -type f -name lib* -exec /usr/sbin/prelink -u {} \;

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf %{_builddir}/%{name}

%files
%define _unpackaged_files_terminate_build 0
%attr(0644, root, root) %{prefix}/%{name}/lib/fmsd_lib.tar.gz
%{prefix}/%{name}


#%pre -p /bin/bash
##!/bin/bash
#TEXTDOMAIN=%{name}

%post -p /bin/bash
#!/bin/bash
cd %{prefix}/%{name}/lib
tar xfz fmsd_lib.tar.gz
ldconfig -n %{prefix}/%{name}/lib
rm -f fmsd_lib.tar.gz


# uninstall 전 실행.
%preun -p /bin/bash
#!/bin/bash
rm -rf %{prefix}/%{name}/lib/libhvaas*
rm -rf %{prefix}/%{name}/lib/libmysql*
rm -rf %{prefix}/%{name}/lib/libprofiler*
rm -rf %{prefix}/%{name}/lib/libtcmalloc*
rm -rf %{prefix}/%{name}/lib/libunwind*
rm -rf %{prefix}/%{name}/lib/pkgconfig
rm -rf %{prefix}/%{name}/lib/plugin

%changelog
* Thu Jun 15 2017 Dongeon Kim <terax2@naver.com>
- 첫 배포판 생성

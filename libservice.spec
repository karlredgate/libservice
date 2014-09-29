Summary: Redgate service library
Name: redgate-libservice
Version: 3.0
Release: 50267
Distribution: Redgate
Group: System Environment/Libraries
License: Proprietary
Vendor: Redgates.com
Packager: Karl Redgate <Karl.Redgate@gmail.com>
%define _topdir %(echo $PWD)/rpm
BuildRoot: %{_topdir}/BUILDROOT
%define Exports %(echo $PWD)/exports

%description
Library for service daemons.

%prep
%build

%install
tar -C %{Exports} -cf - . | (cd $RPM_BUILD_ROOT; tar xf -)

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,0755)
/usr/lib/
/usr/share/

%post
[ "$1" -gt 1 ] && {
    # echo "`date '+%%b %%e %%H:%%M:%%S'`: Upgrading service library"
    :
}

[ "$1" = 1 ] && {
    # echo "`date '+%%b %%e %%H:%%M:%%S'`: New install of service library"
    :
}

[ "$1" -gt 1 ] && {
    # echo "`date '+%%b %%e %%H:%%M:%%S'`: upgrading"
    :
}

%changelog
* Mon Sep 29 2014 Karl Redgate <redgates.com> [1.0]
- Initial release

# vim:autoindent
# vim:syntax=plain

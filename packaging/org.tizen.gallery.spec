Name:       org.tizen.gallery
#VCS_FROM:   profile/mobile/apps/native/gallery#06c3de7e77f13d8b680d9a78d34d31fcae2df3d9
#RS_Ver:    20160711_2 
Summary:    org.tizen.gallery UX
Version:    1.0.0
Release:    1
Group:      Applications
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz

ExcludeArch:  aarch64 x86_64
BuildRequires:  pkgconfig(libtzplatform-config)
Requires(post):  /usr/bin/tpk-backend

%define internal_name org.tizen.gallery
%define preload_tpk_path %{TZ_SYS_RO_APP}/.preload-tpk 

%define build_mode %{nil}

%ifarch i386 i486 i586 i686 x86_64
%define target i386
%else
%ifarch arm armv7l aarch64
%define target arm
%else
%define target noarch
%endif
%endif

%description
profile/mobile/apps/native/gallery#06c3de7e77f13d8b680d9a78d34d31fcae2df3d9
This is a container package which have preload TPK files

%prep
%setup -q

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/%{preload_tpk_path}
install %{internal_name}-%{version}-%{target}%{build_mode}.tpk %{buildroot}/%{preload_tpk_path}/

%post

%files
%defattr(-,root,root,-)
%{preload_tpk_path}/*

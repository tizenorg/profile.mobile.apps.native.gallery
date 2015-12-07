#%define _optdir	/opt/usr
#%define _usrdir	/usr
#%define _appdir	%{_usrdir}/apps
#%define _appdatadir	%{_optdir}/apps

Name:       org.tizen.gallery
Summary:    org.tizen.gallery UX
Version:    1.3.21
Release:    1
Group:      Applications
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz

%if "%{?tizen_profile_name}" == "wearable" || "%{?tizen_profile_name}" == "tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif

BuildRequires:  cmake
BuildRequires:  gettext-tools
BuildRequires:  edje-tools
BuildRequires:  prelink
BuildRequires:  libicu-devel

BuildRequires: pkgconfig(appsvc)
BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(efl-extension)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(storage)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-appfw-widget-application)
BuildRequires: capi-appfw-widget-application-devel
BuildRequires: pkgconfig(capi-content-media-content)
BuildRequires: pkgconfig(capi-system-system-settings)
BuildRequires: pkgconfig(widget_service)
BuildRequires: pkgconfig(widget)
BuildRequires: pkgconfig(widget_provider)
BuildRequires: pkgconfig(widget_provider_app)
BuildRequires: pkgconfig(libtzplatform-config)

%description
Description: org.tizen.gallery UX

%prep
%setup -q

%build
echo %{TZ_SYS_RO_APP}
echo "---------------------"
echo %{TZ_SYS_RO_ICONS}
echo "---------------------"
%define _appdir	%{TZ_SYS_RO_APP}
%define _app_icon_dir             %{TZ_SYS_RO_ICONS}/default/small
%define _app_share_packages_dir   %{TZ_SYS_RO_PACKAGES}
%define _app_license_dir          %{TZ_SYS_SHARE}/license

%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif

%ifarch %{arm}
CXXFLAGS+=" -D_ARCH_ARM_ -mfpu=neon"
%endif

cmake . -DCMAKE_INSTALL_PREFIX=%{_appdir}/org.tizen.gallery  -DARCH=%{ARCH} -DCMAKE_APP_ICON_DIR=%{_app_icon_dir} -DCMAKE_APP_SHARE_PACKAGES_DIR=%{_app_share_packages_dir}

make %{?jobs:-j%jobs}

echo %{TZ_SYS_RO_APP}
echo "---------------------"
echo %{TZ_SYS_RO_ICONS}
echo "---------------------"

%make_install

mkdir -p %{buildroot}%{_app_license_dir}
cp LICENSE %{buildroot}%{_app_license_dir}/org.tizen.gallery

%files -n org.tizen.gallery
%manifest org.tizen.gallery.manifest
%defattr(-,root,root,-)
%{_appdir}/org.tizen.gallery/bin/*
%{_appdir}/org.tizen.gallery/res/locale/*
%{TZ_SYS_RO_ICONS}/default/small/org.tizen.gallery.png
%{TZ_SYS_RO_ICONS}/default/small/preview_gallery_4x4.png
%{_appdir}/org.tizen.gallery/res/images/*
%{_appdir}/org.tizen.gallery/res/edje/*
#%{_appdatadir}/org.tizen.gallery/data
%{TZ_SYS_RO_PACKAGES}/org.tizen.gallery.xml
%doc %{TZ_SYS_SHARE}/license/org.tizen.gallery


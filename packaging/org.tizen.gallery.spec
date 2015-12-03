%define _optdir	/opt/usr
%define _usrdir	/usr
%define _appdir	%{_usrdir}/apps
%define _appdatadir	%{_optdir}/apps

%define _datadir /opt%{_ugdir}/data
%define _sharedir /opt/usr/media/.iv
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

%description
Description: org.tizen.gallery UX

%prep
%setup -q

%build

%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif

%ifarch %{arm}
CXXFLAGS+=" -D_ARCH_ARM_ -mfpu=neon"
%endif

cmake . -DCMAKE_INSTALL_PREFIX=%{_appdir}/org.tizen.gallery  -DCMAKE_DATA_DIR=%{_datadir} -DARCH=%{ARCH}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
if [ ! -d %{buildroot}/opt/usr/apps/org.tizen.gallery/data ]
then
        mkdir -p %{buildroot}/opt/usr/apps/org.tizen.gallery/data
fi

%make_install

mkdir -p %{buildroot}/usr/share/license
mkdir -p %{buildroot}%{_sharedir}
cp LICENSE %{buildroot}/usr/share/license/org.tizen.gallery

%post
chown -R 5000:5000 %{_appdatadir}/org.tizen.gallery/data
%postun

%files -n org.tizen.gallery
%manifest org.tizen.gallery.manifest
%defattr(-,root,root,-)
%{_appdir}/org.tizen.gallery/bin/*
%{_appdir}/org.tizen.gallery/res/locale/*
/usr/share/icons/default/small/org.tizen.gallery.png
/usr/share/icons/default/small/preview_gallery_4x4.png
%{_appdir}/org.tizen.gallery/res/images/*
%{_appdir}/org.tizen.gallery/res/edje/*
%{_appdatadir}/org.tizen.gallery/data
/usr/share/packages/org.tizen.gallery.xml
/usr/share/miregex/*
/usr/share/license/org.tizen.gallery


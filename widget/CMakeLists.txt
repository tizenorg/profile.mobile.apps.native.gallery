SET(GALLERY_WIDGET gallery-widget)

SET(SRCS
    src/gl_widget_main.c
    src/gl_widget_create.c
)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

INCLUDE(FindPkgConfig)
pkg_check_modules(app_pkgs REQUIRED
    capi-appfw-application
    capi-appfw-widget-application
    capi-content-media-content
    elementary
    edje
    evas
    appcore-efl
    dlog
    widget_service
)

ADD_DEFINITIONS(${app_pkgs_CFLAGS})
FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" STREQUAL "arm")
    ADD_DEFINITIONS("-DTARGET")
    MESSAGE("add -DTARGET")
ENDIF("${ARCH}" STREQUAL "arm")

ADD_DEFINITIONS("-DLOG_TAG=\"WIDGET_APP\"")
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/include")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie -Wl,-z,noexecstack")

ADD_EXECUTABLE(${GALLERY_WIDGET} ${SRCS})
TARGET_LINK_LIBRARIES(${GALLERY_WIDGET} ${app_pkgs_LDFLAGS} "-pie -lm")

#edj compile

INSTALL(TARGETS ${GALLERY_WIDGET} DESTINATION ${BINDIR})

#install application HOME directory
INSTALL(DIRECTORY DESTINATION ${DATADIR})
ADD_SUBDIRECTORY(res)

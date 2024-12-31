AM_CFLAGS = -Wall \
            -Wundef \
            -Wstrict-prototypes \
            -Wno-trigraphs

ACLOCAL_AMFLAGS = -I m4

library_includedir = $(pkgincludedir)
library_include_HEADERS = ./inc/qcom_tee_api.h \
                          ./inc/qcom_tee_defs.h \
                          ./inc/credentials.h

c_sources = ./src/qcom_tee_api.c \
            ./src/objects/remote_obj.c \
            ./src/objects/root_obj.c \
            ./src/utils/credentials.c

lib_LTLIBRARIES := libqcomtee.la
libqcomtee_la_CC = @CC@
libqcomtee_la_SOURCES = $(c_sources)
libqcomtee_la_CFLAGS = \
                        $(AM_CFLAGS) \
                        -DOE \
                        -fPIC \
                        @UAPI_INC_PATH@ \
                        @QCBOR_INC_PATH@ \
                        -I./inc

libqcomtee_la_LDFLAGS = -shared -version-info 0:1:0 -lpthread @QCBOR_LIB_PATH@ -lqcbor

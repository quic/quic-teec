AM_CFLAGS = -Wall \
            -Wundef \
            -Wstrict-prototypes \
            -Wno-trigraphs

ACLOCAL_AMFLAGS = -I m4

h_sources = qcom_tee_client.h

c_sources = qcom_tee_client.c

bin_PROGRAMS = qcomtee_client
qcomtee_client_CC = @CC@
qcomtee_client_SOURCES = $(c_sources) $(h_sources)
qcomtee_client_CFLAGS = \
                          $(AM_CFLAGS) \
                          -DOE \
                          -I$(top_srcdir)/libqcomtee/inc

qcomtee_client_LDFLAGS = @QCBOR_LIB_PATH@ -lqcbor
qcomtee_client_LDADD = $(top_srcdir)/libqcomtee/libqcomtee.la

find_path(QCBOR_INCLUDE_DIR
        NAMES qcbor/qcbor.h
        HINTS ${QCBOR_DIR_HINT}
        PATH_SUFFIXES include
)

find_library(QCBOR_LIBRARY
        NAMES qcbor
        HINTS ${QCBOR_DIR_HINT}
        PATH_SUFFIXES lib
)

if(QCBOR_INCLUDE_DIR AND QCBOR_LIBRARY)
        set(QCBOR_FOUND TRUE)
        set(QCBOR_LIBRARIES ${QCBOR_LIBRARY})
        set(QCBOR_INCLUDE_DIRS ${QCBOR_INCLUDE_DIR})
else()
        set(QCBOR_FOUND FALSE)
endif()

mark_as_advanced(QCBOR_INCLUDE_DIR QCBOR_LIBRARY)

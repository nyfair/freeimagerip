SET(SRC ${3RDPARTY_DIR}/openjpeg)

INCLUDE(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
CHECK_INCLUDE_FILE("strings.h"      HAVE_STRINGS_H)
CHECK_INCLUDE_FILE("inttypes.h"     HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE("memory.h"       HAVE_MEMORY_H)
CHECK_INCLUDE_FILE("stdint.h"       HAVE_STDINT_H)
CHECK_INCLUDE_FILE("stdlib.h"       HAVE_STDLIB_H)
CHECK_INCLUDE_FILE("string.h"       HAVE_STRING_H)
CHECK_INCLUDE_FILE("sys/stat.h"     HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE("sys/types.h"    HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE("unistd.h"       HAVE_UNISTD_H)

CONFIGURE_FILE(${SRC}/opj_config.h.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/opj_config.h)

SET(SOURCE_FILES
	${SRC}/bio.c
	${SRC}/cio.c
	${SRC}/dwt.c
	${SRC}/event.c
	${SRC}/image.c
	${SRC}/j2k.c
	${SRC}/j2k_lib.c
	${SRC}/jp2.c
	${SRC}/jpt.c
	${SRC}/mct.c
	${SRC}/mqc.c
	${SRC}/openjpeg.c
	${SRC}/pi.c
	${SRC}/raw.c
	${SRC}/t1.c
	${SRC}/t2.c
	${SRC}/tcd.c
	${SRC}/tgt.c
	${SRC}/cidx_manager.c
	${SRC}/phix_manager.c
	${SRC}/ppix_manager.c
	${SRC}/thix_manager.c
	${SRC}/tpix_manager.c
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_DEFINITIONS(-DOPJ_STATIC)

ADD_LIBRARY(openjpeg STATIC ${SOURCE_FILES})

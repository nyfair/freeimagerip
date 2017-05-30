SET(SRC ${3RDPARTY_DIR}/zlib)

CONFIGURE_FILE(${SRC}/zconf.h.cmakein ${CMAKE_CURRENT_BINARY_DIR}/zconf.h)

SET(SOURCE_FILES
	${SRC}/adler32.c
	${SRC}/compress.c
	${SRC}/crc32.c
	${SRC}/deflate.c
	${SRC}/infback.c
	${SRC}/inffast.c
	${SRC}/inflate.c
	${SRC}/inftrees.c
	${SRC}/trees.c
	${SRC}/uncompr.c
	${SRC}/zutil.c
)

INCLUDE_DIRECTORIES(${SRC} ${CMAKE_CURRENT_BINARY_DIR})

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_LIBRARY(zlib STATIC ${SOURCE_FILES})

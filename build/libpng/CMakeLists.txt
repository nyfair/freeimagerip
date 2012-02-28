SET(SRC ${3RDPARTY_DIR}/libpng)

SET(SOURCE_FILES
	${SRC}/png.c
	${SRC}/pngerror.c
	${SRC}/pngget.c
	${SRC}/pngmem.c
	${SRC}/pngpread.c
	${SRC}/pngread.c
	${SRC}/pngrio.c
	${SRC}/pngrtran.c
	${SRC}/pngrutil.c
	${SRC}/pngset.c
	${SRC}/pngtrans.c
	${SRC}/pngwio.c
	${SRC}/pngwrite.c
	${SRC}/pngwtran.c
	${SRC}/pngwutil.c
)

INCLUDE_DIRECTORIES(
	${3RDPARTY_DIR}/zlib/
)

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_LIBRARY(libpng STATIC ${SOURCE_FILES})

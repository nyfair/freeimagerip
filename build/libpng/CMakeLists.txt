SET(SRC ${3RDPARTY_DIR}/LibPNG)

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
	${3RDPARTY_DIR}/Zlib/
)

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_LIBRARY(LibPNG STATIC ${SOURCE_FILES})

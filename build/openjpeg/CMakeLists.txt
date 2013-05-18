SET(SRC ${3RDPARTY_DIR}/openjpeg)

SET(SOURCE_FILES
	${SRC}/bio.c
	${SRC}/cio.c
	${SRC}/dwt.c
	${SRC}/event.c
	${SRC}/function_list.c
	${SRC}/image.c
	${SRC}/invert.c
	${SRC}/j2k.c
	${SRC}/jp2.c
	${SRC}/mct.c
	${SRC}/mqc.c
	${SRC}/openjpeg.c
	${SRC}/opj_clock.c
	${SRC}/pi.c
	${SRC}/raw.c
	${SRC}/t1.c
	${SRC}/t2.c
	${SRC}/tcd.c
	${SRC}/tgt.c
)

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_DEFINITIONS(-DOPJ_STATIC)

ADD_LIBRARY(openjpeg STATIC ${SOURCE_FILES})

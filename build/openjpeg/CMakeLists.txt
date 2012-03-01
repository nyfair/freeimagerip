SET(SRC ${3RDPARTY_DIR}/openjpeg)

SET(SOURCE_FILES
	${SRC}/bio.c
	${SRC}/cidx_manager.c
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
	${SRC}/phix_manager.c
	${SRC}/pi.c
	${SRC}/ppix_manager.c
	${SRC}/raw.c
	${SRC}/t1.c
	${SRC}/t2.c
	${SRC}/tcd.c
	${SRC}/tgt.c
	${SRC}/thix_manager.c
	${SRC}/tpix_manager.c
)

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_DEFINITIONS(-DOPJ_STATIC)

ADD_LIBRARY(openjpeg STATIC ${SOURCE_FILES})

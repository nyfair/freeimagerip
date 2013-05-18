SET(SRC ${3RDPARTY_DIR}/libjxr)

SET(SOURCE_FILES
	${SRC}/image/decode/decode.c
	${SRC}/image/decode/JXRTranscode.c
	${SRC}/image/decode/postprocess.c
	${SRC}/image/decode/segdec.c
	${SRC}/image/decode/strdec.c
	${SRC}/image/decode/strdec_x86.c
	${SRC}/image/decode/strInvTransform.c
	${SRC}/image/decode/strPredQuantDec.c
	${SRC}/image/encode/encode.c
	${SRC}/image/encode/segenc.c
	${SRC}/image/encode/strenc.c
	${SRC}/image/encode/strenc_x86.c
	${SRC}/image/encode/strFwdTransform.c
	${SRC}/image/encode/strPredQuantEnc.c
	${SRC}/image/sys/adapthuff.c
	${SRC}/image/sys/image.c
	${SRC}/image/sys/perfTimerANSI.c
	${SRC}/image/sys/strcodec.c
	${SRC}/image/sys/strPredQuant.c
	${SRC}/image/sys/strTransform.c
	${SRC}/jxrgluelib/JXRGlue.c
	${SRC}/jxrgluelib/JXRGlueJxr.c
	${SRC}/jxrgluelib/JXRGluePFC.c
	${SRC}/jxrgluelib/JXRMeta.c
)

INCLUDE_DIRECTORIES(
	${SRC}/common/include/
	${SRC}/image/sys/
	${SRC}/image/x86/
	${SRC}/jxrgluelib/
)

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_DEFINITIONS(-DENABLE_OPTIMIZATIONS -DDISABLE_PERF_MEASUREMENT)

ADD_LIBRARY(libjxr STATIC ${SOURCE_FILES})

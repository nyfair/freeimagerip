SET(SRC ${3RDPARTY_DIR}/HDPhoto)

SET(SOURCE_FILES
	${SRC}/image/decode/decode.c
	${SRC}/image/decode/huffman.c
	${SRC}/image/decode/postprocess.c
	${SRC}/image/decode/segdec.c
	${SRC}/image/decode/strdec.c
	${SRC}/image/decode/strInvTransform.c
	${SRC}/image/decode/strPredQuantDec.c
	${SRC}/image/decode/WMPTranscode.c
	${SRC}/image/encode/encode.c
	${SRC}/image/encode/segenc.c
	${SRC}/image/encode/strenc.c
	${SRC}/image/encode/strFwdTransform.c
	${SRC}/image/encode/strPredQuantEnc.c
	${SRC}/image/sys/adapthuff.c
	${SRC}/image/sys/image.c
	${SRC}/image/sys/strcodec.c
	${SRC}/image/sys/strPredQuant.c
	${SRC}/image/sys/strTransform.c
	${SRC}/systems/tools/wmpgluelib/WMPGlue.c
	${SRC}/systems/tools/wmpgluelib/WMPGluePFC.c
	${SRC}/systems/tools/wmpgluelib/WMPGlueWmp.c
	${SRC}/systems/tools/wmpmetalib/WMPMeta.c
)

IF(NOT MSVC)
	ADD_DEFINITIONS(-D__ANSI__)
	SET(SOURCE_FILES
		${SOURCE_FILES} ${SRC}/nonvc_helper.c)
ENDIF()

INCLUDE_DIRECTORIES(
	${SRC}/common/include/
	${SRC}/image/sys/
	${SRC}/image/x86/
	${SRC}/systems/tools/wmpgluelib/
	${SRC}/systems/tools/wmpmetalib/
)

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_DEFINITIONS(-DSTREAMING_PACKET_IO
	-DX86OPT_HUFFMAN -DX86OPT_PREBUILT_TABLE
	-DX86OPT_QUANT -DX86OPT_INLINE)

ADD_LIBRARY(HDPhoto STATIC ${SOURCE_FILES})

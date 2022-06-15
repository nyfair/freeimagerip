SET(SRC ${3RDPARTY_DIR}/mozjpeg)

INCLUDE(CheckTypeSize)
CHECK_TYPE_SIZE("size_t" SIZE_T)
SET(JPEG_LIB_VERSION 62)
SET(BITS_IN_JSAMPLE 8)
SET(MEM_SRCDST_SUPPORTED 1)
IF(WIN32)
	CONFIGURE_FILE(${SRC}/win/jconfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/jconfig.h)
ELSE()
	CONFIGURE_FILE(${SRC}/jconfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/jconfig.h)
ENDIF()
CONFIGURE_FILE(${SRC}/jconfigint.h.in ${CMAKE_CURRENT_BINARY_DIR}/jconfigint.h)
CONFIGURE_FILE(${SRC}/jversion.h.in ${CMAKE_CURRENT_BINARY_DIR}/jversion.h)

SET(SOURCE_FILES
	${SRC}/jcapimin.c
	${SRC}/jcapistd.c
	${SRC}/jccoefct.c
	${SRC}/jccolor.c
	${SRC}/jcdctmgr.c
	${SRC}/jchuff.c
	${SRC}/jcext.c
	${SRC}/jcicc.c
	${SRC}/jcinit.c
	${SRC}/jcmainct.c
	${SRC}/jcmarker.c
	${SRC}/jcmaster.c
	${SRC}/jcomapi.c
	${SRC}/jcparam.c
	${SRC}/jcphuff.c
	${SRC}/jcprepct.c
	${SRC}/jcsample.c
	${SRC}/jctrans.c
	${SRC}/jdapimin.c
	${SRC}/jdapistd.c
	${SRC}/jdatadst.c
	${SRC}/jdatasrc.c
	${SRC}/jdcoefct.c
	${SRC}/jdcolor.c
	${SRC}/jddctmgr.c
	${SRC}/jdhuff.c
	${SRC}/jdicc.c
	${SRC}/jdinput.c
	${SRC}/jdmainct.c
	${SRC}/jdmarker.c
	${SRC}/jdmaster.c
	${SRC}/jdmerge.c
	${SRC}/jdphuff.c
	${SRC}/jdpostct.c
	${SRC}/jdsample.c
	${SRC}/jdtrans.c
	${SRC}/jerror.c
	${SRC}/jfdctflt.c
	${SRC}/jfdctfst.c
	${SRC}/jfdctint.c
	${SRC}/jidctflt.c
	${SRC}/jidctfst.c
	${SRC}/jidctint.c
	${SRC}/jidctred.c
	${SRC}/jquant1.c
	${SRC}/jquant2.c
	${SRC}/jutils.c
	${SRC}/jmemmgr.c
	${SRC}/jmemnobs.c
	${SRC}/transupp.c
)

ADD_DEFINITIONS(-DWITH_SIMD)

SET(NAFLAGS -I${SRC}/simd/nasm)

IF(MSVC)
	SET(NAFLAGS ${NAFLAGS} -DMSVC)
	SET(SIMDEXT .obj)
ELSE()
	SET(SIMDEXT .o)
ENDIF()

IF(X64)
	SET(SIMD_PATH x86_64)
  SET(SIMD_BASENAMES jsimdcpu.asm jfdctflt-sse.asm
    jccolor-sse2.asm jcgray-sse2.asm jchuff-sse2.asm
    jcphuff-sse2.asm jcsample-sse2.asm jdcolor-sse2.asm
    jdmerge-sse2.asm jdsample-sse2.asm jfdctfst-sse2.asm
    jfdctint-sse2.asm jidctflt-sse2.asm jidctfst-sse2.asm
    jidctint-sse2.asm jidctred-sse2.asm jquantf-sse2.asm
    jquanti-sse2.asm
    jccolor-avx2.asm jcgray-avx2.asm jcsample-avx2.asm
    jdcolor-avx2.asm jdmerge-avx2.asm jdsample-avx2.asm
    jfdctint-avx2.asm jidctint-avx2.asm jquanti-avx2.asm)
	IF(WIN32)
		SET(NAFLAGS ${NAFLAGS} -fwin64 -DWIN64)
	ELSEIF(APPLE)
		SET(NAFLAGS ${NAFLAGS} -fmacho64 -DMACHO)
	ELSE()
		SET(NAFLAGS ${NAFLAGS} -felf64 -DELF)
	ENDIF()
	SET(NAFLAGS ${NAFLAGS} -D__x86_64__ -I${SRC}/simd/x86_64)
ELSE()
	SET(SIMD_PATH i386)
	SET(SIMD_BASENAMES jsimdcpu.asm jfdctflt-3dn.asm
    jidctflt-3dn.asm jquant-3dn.asm
    jccolor-mmx.asm jcgray-mmx.asm jcsample-mmx.asm
    jdcolor-mmx.asm jdmerge-mmx.asm jdsample-mmx.asm
    jfdctfst-mmx.asm jfdctint-mmx.asm jidctfst-mmx.asm
    jidctint-mmx.asm jidctred-mmx.asm jquant-mmx.asm
    jfdctflt-sse.asm jidctflt-sse.asm jquant-sse.asm
    jccolor-sse2.asm jcgray-sse2.asm jchuff-sse2.asm
    jcphuff-sse2.asm jcsample-sse2.asm jdcolor-sse2.asm
    jdmerge-sse2.asm jdsample-sse2.asm jfdctfst-sse2.asm
    jfdctint-sse2.asm jidctflt-sse2.asm jidctfst-sse2.asm
    jidctint-sse2.asm jidctred-sse2.asm jquantf-sse2.asm
    jquanti-sse2.asm
    jccolor-avx2.asm jcgray-avx2.asm jcsample-avx2.asm
    jdcolor-avx2.asm jdmerge-avx2.asm jdsample-avx2.asm
    jfdctint-avx2.asm jidctint-avx2.asm jquanti-avx2.asm)
	IF(WIN32)
		SET(NAFLAGS ${NAFLAGS} -fwin32 -DWIN32)
	ELSE()
		SET(NAFLAGS ${NAFLAGS} -felf -DELF)
	ENDIF()
	SET(NAFLAGS ${NAFLAGS} -I${SRC}/simd/i386)
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

FILE(GLOB INC_FILES nasm/*.inc)

FOREACH(FILE ${SIMD_BASENAMES})
	SET(DEPFILE "")
	SET(SIMD_SRC ${SRC}/simd/${SIMD_PATH}/${FILE})
	IF(${FILE} MATCHES jccolor)
		SET(DEPFILE ${SIMD_SRC})
		STRING(REGEX REPLACE "jccolor" "jccolext" DEPFILE ${DEPFILE})
	ENDIF()
	IF(${FILE} MATCHES jcgray)
		SET(DEPFILE ${SIMD_SRC})
		STRING(REGEX REPLACE "jcgray" "jcgryext" DEPFILE ${DEPFILE})
	ENDIF()
	IF(${FILE} MATCHES jdcolor)
		SET(DEPFILE ${SIMD_SRC})
		STRING(REGEX REPLACE "jdcolor" "jdcolext" DEPFILE ${DEPFILE})
	ENDIF()
	IF(${FILE} MATCHES jdmerge)
		SET(DEPFILE ${SIMD_SRC})
		STRING(REGEX REPLACE "jdmerge" "jdmrgext" DEPFILE ${DEPFILE})
	ENDIF()
	SET(SIMD_OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE}.obj)
	ADD_CUSTOM_COMMAND(OUTPUT ${SIMD_OBJ}
		DEPENDS ${SIMD_SRC} ${DEPFILE} ${INC_FILES}
		COMMAND nasm ${NAFLAGS} ${SIMD_SRC} -o${SIMD_OBJ})
	SET(SIMD_OBJS ${SIMD_OBJS} ${SIMD_OBJ})
ENDFOREACH()

ADD_LIBRARY(mozjpeg STATIC ${SOURCE_FILES} ${SIMD_OBJS} ${SRC}/simd/${SIMD_PATH}/jsimd.c)

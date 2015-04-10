SET(SRC ${3RDPARTY_DIR}/libjpeg-turbo)

SET(JPEG_LIB_VERSION 62)
SET(DLL_VERSION ${JPEG_LIB_VERSION})
SET(FULLVERSION ${DLL_VERSION}.0.0)

SET(BITS_IN_JSAMPLE 8)
CONFIGURE_FILE(${SRC}/win/jconfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/jconfig.h)
CONFIGURE_FILE(${SRC}/win/jconfigint.h.in ${CMAKE_CURRENT_BINARY_DIR}/jconfigint.h)

SET(SOURCE_FILES
	${SRC}/jcapimin.c
	${SRC}/jcapistd.c
	${SRC}/jccoefct.c
	${SRC}/jccolor.c
	${SRC}/jcdctmgr.c
	${SRC}/jchuff.c
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

SET(NAFLAGS -I${SRC}/simd/ -I${SRC}/win)

IF(MSVC)
	SET(NAFLAGS ${NAFLAGS} -DMSVC)
	SET(SIMDEXT .obj)
ELSE()
	SET(SIMDEXT .o)
ENDIF()

IF(X64)
	SET(SOURCE_FILES ${SOURCE_FILES} ${SRC}/simd/jsimd_x86_64.c)
	SET(SIMD_BASENAMES jfdctflt-sse-64 jccolor-sse2-64 jcgray-sse2-64
		jcsample-sse2-64 jdcolor-sse2-64 jdmerge-sse2-64 jdsample-sse2-64
		jfdctfst-sse2-64 jfdctint-sse2-64 jidctflt-sse2-64 jidctfst-sse2-64
		jidctint-sse2-64 jidctred-sse2-64 jquantf-sse2-64 jquanti-sse2-64)
	IF(WIN32)
		SET(NAFLAGS ${NAFLAGS} -fwin64 -DWIN64 -D__x86_64__)
	ELSE()
		SET(NAFLAGS ${NAFLAGS} -felf64 -DELF -D__x86_64__)
	ENDIF()
ELSE()
	SET(SOURCE_FILES ${SOURCE_FILES} ${SRC}/simd/jsimd_i386.c)
	SET(SIMD_BASENAMES jsimdcpu jfdctflt-3dn jidctflt-3dn jquant-3dn jccolor-mmx
		jcgray-mmx jcsample-mmx jdcolor-mmx jdmerge-mmx jdsample-mmx jfdctfst-mmx
		jfdctint-mmx jidctfst-mmx jidctint-mmx jidctred-mmx jquant-mmx jfdctflt-sse
		jidctflt-sse jquant-sse jccolor-sse2 jcgray-sse2 jcsample-sse2 jdcolor-sse2
		jdmerge-sse2 jdsample-sse2 jfdctfst-sse2 jfdctint-sse2 jidctflt-sse2
		jidctfst-sse2 jidctint-sse2 jidctred-sse2 jquantf-sse2 jquanti-sse2)
	IF(WIN32)
		SET(NAFLAGS ${NAFLAGS} -fwin32 -DWIN32)
	ELSE()
		SET(NAFLAGS ${NAFLAGS} -felf -DELF)
	ENDIF()
ENDIF()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

FILE(GLOB INC_FILES *.inc)

FOREACH(FILE ${SIMD_BASENAMES})
	SET(DEPFILE "")
	SET(SIMD_SRC ${SRC}/simd/${FILE}.asm)
	IF(${FILE} MATCHES jccolor)
		SET(DEPFILE ${SIMD_SRC})
		STRING(REGEX REPLACE "jccolor" "jccolext" DEPFILE ${DEPFILE})
	ENDIF()
	IF(${FILE} MATCHES jdcolor)
		SET(DEPFILE ${SIMD_SRC})
		STRING(REGEX REPLACE "jdcolor" "jdcolext" DEPFILE ${DEPFILE})
	ENDIF()
	IF(${FILE} MATCHES jdmerge)
		SET(DEPFILE ${SIMD_SRC})
		STRING(REGEX REPLACE "jdmerge" "jdmrgext" DEPFILE ${DEPFILE})
	ENDIF()
	IF(${FILE} MATCHES jcgray)
		SET(DEPFILE ${SIMD_SRC})
		STRING(REGEX REPLACE "jcgray" "jcgryext" DEPFILE ${DEPFILE})
	ENDIF()
	SET(SIMD_OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE}.obj)
	ADD_CUSTOM_COMMAND(OUTPUT ${SIMD_OBJ}
    		DEPENDS ${SIMD_SRC} ${DEPFILE} ${INC_FILES}
    		COMMAND yasm ${NAFLAGS} ${SIMD_SRC} -o${SIMD_OBJ})
	SET(SIMD_OBJS ${SIMD_OBJS} ${SIMD_OBJ})
ENDFOREACH()

ADD_DEFINITIONS(-DWITH_SIMD)

ADD_LIBRARY(libjpeg-turbo STATIC ${SOURCE_FILES} ${SIMD_OBJS})

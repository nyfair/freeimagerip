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

IF(MSVC)
	ENABLE_LANGUAGE(ASM_MASM)
	IF(X64)
		SET(ZLIB_ASMS
			${SRC}/contrib/masmx64/gvmat64.asm
			${SRC}/contrib/masmx64/inffasx64.asm
			${SRC}/contrib/masmx64/inffas8664.c
		)
	ELSE()
    SET(ZLIB_ASMS
			${SRC}/contrib/masmx86/match686.asm
			${SRC}/contrib/masmx86/inffas32.asm
		)
	ENDIF()
	ADD_DEFINITIONS(-DASMV -DASMINF)
ELSE()
	IF(X64)
		SET(ZLIB_ASMS ${SRC}/contrib/amd64/amd64-match.S)
  ELSE()
		SET(ZLIB_ASMS ${SRC}/contrib/asm686/match.S)
  ENDIF()
	ADD_DEFINITIONS(-DASMV)
		SET_SOURCE_FILES_PROPERTIES(${ZLIB_ASMS} PROPERTIES LANGUAGE C COMPILE_FLAGS -DNO_UNDERLINE)
ENDIF()

INCLUDE_DIRECTORIES(${SRC} ${CMAKE_CURRENT_BINARY_DIR})

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_LIBRARY(zlib STATIC ${SOURCE_FILES} ${ZLIB_ASMS})

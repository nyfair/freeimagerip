SET(SRC ${3RDPARTY_DIR}/Zlib)

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

INCLUDE_DIRECTORIES(${SRC}/)

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

IF(MSVC)
	IF(CMAKE_CL_64)
		ADD_CUSTOM_COMMAND(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/inffasx64.obj
			COMMAND ml64 /coff /c ${SRC}/contrib/masmx64/inffasx64.asm
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gvmat64.obj
			COMMAND ml64 /coff /c ${SRC}/contrib/masmx64/gvmat64.asm
		)
		SET(ASM_FILES
			${CMAKE_CURRENT_BINARY_DIR}/inffasx64.obj
			${CMAKE_CURRENT_BINARY_DIR}/gvmat64.obj
		)
		SET(SOURCE_FILES ${SOURCE_FILES} ${SRC}/contrib/masmx64/inffas8664.c)
	ELSE()
		ADD_CUSTOM_COMMAND(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/inffas32.obj
			COMMAND ml /coff /c ${SRC}/contrib/masmx86/inffas32.asm
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/match686.obj
			COMMAND ml /coff /c ${SRC}/contrib/masmx86/match686.asm
		)
		SET(ASM_FILES
			${CMAKE_CURRENT_BINARY_DIR}/inffas32.obj
			${CMAKE_CURRENT_BINARY_DIR}/match686.obj
		)
	ENDIF()
ELSE()
	SET(SOURCE_FILES ${SOURCE_FILES} ${SRC}/contrib/inflate86/inffas86.c)
	IF(X64)
		ADD_CUSTOM_COMMAND(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gvmat64.o
			COMMAND ${CMAKE_C_COMPILER} -c -DASMV ${SRC}/contrib/gcc_gvmat64/gvmat64.S
		)
		SET(ASM_FILES ${CMAKE_CURRENT_BINARY_DIR}/gvmat64.o)
	ELSE()
		ADD_CUSTOM_COMMAND(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/match.o
			COMMAND ${CMAKE_C_COMPILER} -c -DASMV ${SRC}/contrib/asm686/match.S
		)
		SET(ASM_FILES ${CMAKE_CURRENT_BINARY_DIR}/match.o)
	ENDIF()
ENDIF()

ADD_DEFINITIONS(-DASMV -DASMINF)

ADD_LIBRARY(Zlib STATIC ${SOURCE_FILES} ${ASM_FILES})

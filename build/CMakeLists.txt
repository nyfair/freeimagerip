PROJECT(FreeImage)

# ASM file handling was fixed in cmake2.8.4
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.4)

IF(MSVC)
# More MSVC specific compilation flags
	ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4996 /wd4819 /wd4244")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996 /wd4819 /wd4244")
ENDIF()

# Determine Platform
IF(MSVC AND CMAKE_CL_64)
	SET(X64 1)
ELSEIF(CMAKE_SIZEOF_VOID_P MATCHES 8)
	SET(X64 1)
ENDIF()
IF(NOT MSVC AND X64)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
ENDIF()

# Set postfix
SET(CMAKE_DEBUG_POSTFIX "_d" CACHE STRING "add a postfix, usually d on windows")
SET(CMAKE_RELWITHDEBINFO_POSTFIX "_rd" CACHE STRING "add a postfix, usually empty on windows")
SET(CMAKE_MINSIZEREL_POSTFIX "_s" CACHE STRING "add a postfix, usually empty on windows")

# Output directories
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_LIBDIR})
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BINDIR})
IF(WIN32)
	SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BINDIR})
ELSE()
	SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_LIBDIR})
ENDIF()

# Install directories
SET(INSTALL_INCDIR include)
SET(INSTALL_BINDIR bin)
SET(INSTALL_ARCHIVEDIR lib)
IF(WIN32)
	SET(INSTALL_LIBDIR bin)
ELSE()
	SET(INSTALL_LIBDIR lib)
ENDIF()

# Project directories
FIND_PATH(PROJECT_DIR FreeImage.h PATHS ../Source/)
SET(3RDPARTY_DIR ${PROJECT_DIR}/../3rdparty/)

# FreeImage core
SET(SOURCE_FILES
	${PROJECT_DIR}/FreeImage/BitmapAccess.cpp
	${PROJECT_DIR}/FreeImage/ColorLookup.cpp
	${PROJECT_DIR}/FreeImage/Conversion.cpp
	${PROJECT_DIR}/FreeImage/Conversion16_555.cpp
	${PROJECT_DIR}/FreeImage/Conversion16_565.cpp
	${PROJECT_DIR}/FreeImage/Conversion24.cpp
	${PROJECT_DIR}/FreeImage/Conversion32.cpp
	${PROJECT_DIR}/FreeImage/Conversion4.cpp
	${PROJECT_DIR}/FreeImage/Conversion8.cpp
	${PROJECT_DIR}/FreeImage/ConversionFloat.cpp
	${PROJECT_DIR}/FreeImage/ConversionRGB16.cpp
	${PROJECT_DIR}/FreeImage/ConversionRGBF.cpp
	${PROJECT_DIR}/FreeImage/ConversionType.cpp
	${PROJECT_DIR}/FreeImage/ConversionUINT16.cpp
	${PROJECT_DIR}/FreeImage/FreeImage.cpp
	${PROJECT_DIR}/FreeImage/FreeImageIO.cpp
	${PROJECT_DIR}/FreeImage/GetType.cpp
	${PROJECT_DIR}/FreeImage/Halftoning.cpp
	${PROJECT_DIR}/FreeImage/MemoryIO.cpp
	${PROJECT_DIR}/FreeImage/PixelAccess.cpp
	${PROJECT_DIR}/FreeImage/Plugin.cpp
	${PROJECT_DIR}/FreeImage/PluginBMP.cpp
	${PROJECT_DIR}/FreeImage/PluginGIF.cpp
	${PROJECT_DIR}/FreeImage/PluginHDR.cpp
	${PROJECT_DIR}/FreeImage/PluginICO.cpp
	${PROJECT_DIR}/FreeImage/PluginJPEG.cpp
	${PROJECT_DIR}/FreeImage/PluginJXR.cpp
	${PROJECT_DIR}/FreeImage/PluginPNG.cpp
	${PROJECT_DIR}/FreeImage/PluginPSD.cpp
	${PROJECT_DIR}/FreeImage/PluginTARGA.cpp
	${PROJECT_DIR}/FreeImage/PluginWEBP.cpp
	${PROJECT_DIR}/FreeImage/PSDParser.cpp
	${PROJECT_DIR}/FreeImage/tmoColorConvert.cpp
	${PROJECT_DIR}/FreeImage/tmoDrago03.cpp
	${PROJECT_DIR}/FreeImage/tmoFattal02.cpp
	${PROJECT_DIR}/FreeImage/tmoReinhard05.cpp
	${PROJECT_DIR}/FreeImage/ToneMapping.cpp
	${PROJECT_DIR}/FreeImage/NNQuantizer.cpp
	${PROJECT_DIR}/FreeImage/WuQuantizer.cpp
	${PROJECT_DIR}/FreeImageToolkit/Background.cpp
	${PROJECT_DIR}/FreeImageToolkit/BSplineRotate.cpp
	${PROJECT_DIR}/FreeImageToolkit/Channels.cpp
	${PROJECT_DIR}/FreeImageToolkit/ClassicRotate.cpp
	${PROJECT_DIR}/FreeImageToolkit/Colors.cpp
	${PROJECT_DIR}/FreeImageToolkit/CopyPaste.cpp
	${PROJECT_DIR}/FreeImageToolkit/Display.cpp
	${PROJECT_DIR}/FreeImageToolkit/Flip.cpp
	${PROJECT_DIR}/FreeImageToolkit/JPEGTransform.cpp
	${PROJECT_DIR}/FreeImageToolkit/MultigridPoissonSolver.cpp
	${PROJECT_DIR}/FreeImageToolkit/Rescale.cpp
	${PROJECT_DIR}/FreeImageToolkit/Resize.cpp
)

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_DEFINITIONS(-DFREEIMAGE_EXPORTS)
ADD_LIBRARY(freeimage SHARED ${SOURCE_FILES})

ADD_DEFINITIONS(-DOPJ_STATIC -DLIBRAW_NODLL)
IF(NOT MSVC)
	ADD_DEFINITIONS(-D__ANSI__)
ENDIF()

OPTION(USE_BUILTIN_LIBJPEG8-TURBO "Use built-in jpeg-turbo library" ON)
OPTION(USE_BUILTIN_LIBPNG "Use built-in png library" ON)
OPTION(USE_BUILTIN_LIBWEBP "Use built-in webp library" ON)

ADD_SUBDIRECTORY(HDPhoto)
ADD_DEPENDENCIES(freeimage HDPhoto)
INCLUDE_DIRECTORIES(
	${PROJECT_DIR}/
	${3RDPARTY_DIR}/HDPhoto/common/include/
	${3RDPARTY_DIR}/HDPhoto/image/sys/
	${3RDPARTY_DIR}/HDPhoto/image/x86/
	${3RDPARTY_DIR}/HDPhoto/Systems/tools/WMPGlueLib/
	${3RDPARTY_DIR}/HDPhoto/Systems/tools/WMPMetaLib/
	${3RDPARTY_DIR}/LibJPEG-Turbo/
	${3RDPARTY_DIR}/LibPNG/
	${3RDPARTY_DIR}/Zlib/
	${3RDPARTY_DIR}/LibWebP/src/
)
TARGET_LINK_LIBRARIES(freeimage HDPhoto)
IF(USE_BUILTIN_LIBJPEG8-TURBO)
	ADD_SUBDIRECTORY(LibJPEG-Turbo)
	ADD_DEPENDENCIES(freeimage LibJPEG)
	TARGET_LINK_LIBRARIES(freeimage LibJPEG)
ELSE()
	FIND_PACKAGE(JPEG REQUIRED)
	TARGET_LINK_LIBRARIES(freeimage ${JPEG_LIBRARY})
ENDIF()
IF(USE_BUILTIN_LIBPNG)
	ADD_SUBDIRECTORY(LibPNG)
	ADD_SUBDIRECTORY(Zlib)
	ADD_DEPENDENCIES(freeimage LibPNG Zlib)
	TARGET_LINK_LIBRARIES(freeimage LibPNG Zlib)
ELSE()
	FIND_PACKAGE(PNG REQUIRED)
	TARGET_LINK_LIBRARIES(freeimage ${PNG_LIBRARY})
ENDIF()
IF(USE_BUILTIN_LIBWEBP)
	ADD_SUBDIRECTORY(LibWebP)
	ADD_DEPENDENCIES(freeimage LibWebP)
	TARGET_LINK_LIBRARIES(freeimage LibWebP)
ELSE()
	FIND_PACKAGE(WEBP REQUIRED)
	TARGET_LINK_LIBRARIES(freeimage ${WEBP_LIBRARY})
ENDIF()


INSTALL(TARGETS freeimage
	RUNTIME DESTINATION ${INSTALL_BINDIR}
	LIBRARY DESTINATION ${INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR}
)

INSTALL(FILES
	${PROJECT_DIR}/FreeImage.h
	DESTINATION ${INSTALL_INCDIR}
)

IF(MSVC)
# Susie Plugin
	SET(CMAKE_LINK_DEF_FILE_FLAG "/DEF:${PROJECT_DIR}/susiespi.def")
	ADD_LIBRARY(SUSIE SHARED ${PROJECT_DIR}/susie.cc ${PROJECT_DIR}/susiespi.def)
	ADD_DEPENDENCIES(SUSIE freeimage)
	TARGET_LINK_LIBRARIES(SUSIE freeimage)
	SET_TARGET_PROPERTIES(SUSIE PROPERTIES OUTPUT_NAME "iffi" SUFFIX ".spi")
ENDIF()

ADD_EXECUTABLE(convtest ${PROJECT_DIR}/Test/conv.cc)
ADD_DEPENDENCIES(convtest freeimage)
TARGET_LINK_LIBRARIES(convtest freeimage)

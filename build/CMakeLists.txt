PROJECT(FreeImage)

# ASM file handling was fixed in cmake2.8.4
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.4)

IF(MSVC)
# More MSVC specific compilation flags
	ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
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
FIND_PATH(PROJECT_DIR FreeImage.h PATHS ../src/)
SET(3RDPARTY_DIR ${PROJECT_DIR}/../3rdparty)
SET(PLUGIN_DIR ${PROJECT_DIR}/../plugin)

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
	${PROJECT_DIR}/FreeImage/J2KHelper.cpp
	${PROJECT_DIR}/FreeImage/MemoryIO.cpp
	${PROJECT_DIR}/FreeImage/PixelAccess.cpp
	${PROJECT_DIR}/FreeImage/Plugin.cpp
	${PROJECT_DIR}/FreeImage/PluginBMP.cpp
	${PROJECT_DIR}/FreeImage/PluginDDS.cpp
	${PROJECT_DIR}/FreeImage/PluginGIF.cpp
	${PROJECT_DIR}/FreeImage/PluginHDR.cpp
	${PROJECT_DIR}/FreeImage/PluginICO.cpp
	${PROJECT_DIR}/FreeImage/PluginJP2.cpp
	${PROJECT_DIR}/FreeImage/PluginJPEG.cpp
	${PROJECT_DIR}/FreeImage/PluginJXR.cpp
	${PROJECT_DIR}/FreeImage/PluginPNG.cpp
	${PROJECT_DIR}/FreeImage/PluginPSD.cpp
	${PROJECT_DIR}/FreeImage/PluginTARGA.cpp
	${PROJECT_DIR}/FreeImage/PluginTIFF.cpp
	${PROJECT_DIR}/FreeImage/PluginWEBP.cpp
	${PROJECT_DIR}/FreeImage/PSDParser.cpp
	${PROJECT_DIR}/FreeImage/TIFFLogLuv.cpp
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

ADD_DEFINITIONS(-DFREEIMAGE_EXPORTS -DOPJ_STATIC)
ADD_LIBRARY(freeimage SHARED ${SOURCE_FILES})

IF(NOT MSVC)
	ADD_DEFINITIONS(-D__ANSI__)
ENDIF()

IF(MSVC)
	OPTION(SUSIE_PLUGIN "Build Susie plugin" OFF)
ENDIF()
OPTION(QT_PLUGIN "Build QT plugin" OFF)
OPTION(CONV_TEST "Build a simple convert test program" OFF)
ADD_SUBDIRECTORY(plugin)

ADD_SUBDIRECTORY(libjpeg-turbo)
ADD_SUBDIRECTORY(libjxr)
ADD_SUBDIRECTORY(libpng)
ADD_SUBDIRECTORY(libtiff)
ADD_SUBDIRECTORY(libwebp)
ADD_SUBDIRECTORY(openjpeg)
ADD_SUBDIRECTORY(zlib)

ADD_DEPENDENCIES(freeimage libjpeg-turbo libjxr libpng libtiff libwebp openjpeg zlib)
TARGET_LINK_LIBRARIES(freeimage libjpeg-turbo libjxr libpng libtiff libwebp openjpeg zlib)
INCLUDE_DIRECTORIES(
	${PROJECT_DIR}/
	${3RDPARTY_DIR}/libjpeg-turbo/
	${3RDPARTY_DIR}/libpng/
	${3RDPARTY_DIR}/libjxr/common/include/
	${3RDPARTY_DIR}/libjxr/image/sys/
	${3RDPARTY_DIR}/libjxr/image/x86/
	${3RDPARTY_DIR}/libjxr/Systems/tools/WMPGlueLib/
	${3RDPARTY_DIR}/libjxr/Systems/tools/WMPMetaLib/
	${3RDPARTY_DIR}/libtiff/
	${3RDPARTY_DIR}/libwebp/src/
	${3RDPARTY_DIR}/openjpeg/
	${3RDPARTY_DIR}/zlib/
	${CMAKE_BINARY_DIR}/libjpeg-turbo/
	${CMAKE_BINARY_DIR}/libpng/
	${CMAKE_BINARY_DIR}/openjpeg/
)

INSTALL(TARGETS freeimage
	RUNTIME DESTINATION ${INSTALL_BINDIR}
	LIBRARY DESTINATION ${INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR}
)

INSTALL(FILES
	${PROJECT_DIR}/FreeImage.h
	DESTINATION ${INSTALL_INCDIR}
)

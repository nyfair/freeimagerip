SET(SRC ${3RDPARTY_DIR}/libbpg)

SET(SOURCE_FILES
	${SRC}/jctvc_glue.cpp
	${SRC}/jctvc/TAppEncCfg.cpp
	${SRC}/jctvc/TAppEncTop.cpp
	${SRC}/jctvc/program_options_lite.cpp
	${SRC}/jctvc/TLibEncoder/SyntaxElementWriter.cpp
	${SRC}/jctvc/TLibEncoder/TEncSbac.cpp
	${SRC}/jctvc/TLibEncoder/TEncBinCoderCABACCounter.cpp
	${SRC}/jctvc/TLibEncoder/TEncGOP.cpp
	${SRC}/jctvc/TLibEncoder/TEncSampleAdaptiveOffset.cpp
	${SRC}/jctvc/TLibEncoder/TEncBinCoderCABAC.cpp
	${SRC}/jctvc/TLibEncoder/TEncEntropy.cpp
	${SRC}/jctvc/TLibEncoder/TEncTop.cpp
	${SRC}/jctvc/TLibEncoder/SEIEncoder.cpp
	${SRC}/jctvc/TLibEncoder/SEIwrite.cpp
	${SRC}/jctvc/TLibEncoder/TEncPic.cpp
	${SRC}/jctvc/TLibEncoder/TEncRateCtrl.cpp
	${SRC}/jctvc/TLibEncoder/WeightPredAnalysis.cpp
	${SRC}/jctvc/TLibEncoder/TEncSlice.cpp
	${SRC}/jctvc/TLibEncoder/TEncCu.cpp
	${SRC}/jctvc/TLibEncoder/NALwrite.cpp
	${SRC}/jctvc/TLibEncoder/TEncCavlc.cpp
	${SRC}/jctvc/TLibEncoder/TEncSearch.cpp
	${SRC}/jctvc/TLibEncoder/TEncPreanalyzer.cpp
	${SRC}/jctvc/TLibVideoIO/TVideoIOYuv.cpp
	${SRC}/jctvc/TLibCommon/TComWeightPrediction.cpp
	${SRC}/jctvc/TLibCommon/TComLoopFilter.cpp
	${SRC}/jctvc/TLibCommon/TComBitStream.cpp
	${SRC}/jctvc/TLibCommon/TComMotionInfo.cpp
	${SRC}/jctvc/TLibCommon/TComSlice.cpp
	${SRC}/jctvc/TLibCommon/ContextModel3DBuffer.cpp
	${SRC}/jctvc/TLibCommon/TComPic.cpp
	${SRC}/jctvc/TLibCommon/TComRdCostWeightPrediction.cpp
	${SRC}/jctvc/TLibCommon/TComTU.cpp
	${SRC}/jctvc/TLibCommon/TComPicSym.cpp
	${SRC}/jctvc/TLibCommon/TComPicYuv.cpp
	${SRC}/jctvc/TLibCommon/TComYuv.cpp
	${SRC}/jctvc/TLibCommon/TComTrQuant.cpp
	${SRC}/jctvc/TLibCommon/TComInterpolationFilter.cpp
	${SRC}/jctvc/TLibCommon/ContextModel.cpp
	${SRC}/jctvc/TLibCommon/TComSampleAdaptiveOffset.cpp
	${SRC}/jctvc/TLibCommon/SEI.cpp
	${SRC}/jctvc/TLibCommon/TComPrediction.cpp
	${SRC}/jctvc/TLibCommon/TComDataCU.cpp
	${SRC}/jctvc/TLibCommon/TComChromaFormat.cpp
	${SRC}/jctvc/TLibCommon/Debug.cpp
	${SRC}/jctvc/TLibCommon/TComRom.cpp
	${SRC}/jctvc/TLibCommon/TComPicYuvMD5.cpp
	${SRC}/jctvc/TLibCommon/TComRdCost.cpp
	${SRC}/jctvc/TLibCommon/TComPattern.cpp
	${SRC}/jctvc/TLibCommon/TComCABACTables.cpp
	${SRC}/jctvc/libmd5/libmd5.c
	${SRC}/libbpg.c
	${SRC}/libavcodec/hevc_cabac.c
	${SRC}/libavcodec/hevc_filter.c
	${SRC}/libavcodec/hevc.c
	${SRC}/libavcodec/hevcpred.c
	${SRC}/libavcodec/hevc_refs.c
	${SRC}/libavcodec/hevcdsp.c
	${SRC}/libavcodec/hevc_mvs.c
	${SRC}/libavcodec/hevc_ps.c
	${SRC}/libavcodec/hevc_sei.c
	${SRC}/libavcodec/utils.c
	${SRC}/libavcodec/cabac.c
	${SRC}/libavcodec/golomb.c
	${SRC}/libavcodec/videodsp.c
	${SRC}/libavutil/mem.c
	${SRC}/libavutil/buffer.c
	${SRC}/libavutil/log2_tab.c
	${SRC}/libavutil/frame.c
	${SRC}/libavutil/pixdesc.c
	${SRC}/libavutil/md5.c
)

IF(MSVC)
	SET(SOURCE_FILES ${SOURCE_FILES} ${SRC}/getopt/getopt.cpp)
	INCLUDE_DIRECTORIES(${SRC}/getopt)
ENDIF()

INCLUDE_DIRECTORIES(${SRC} ${SRC}/jctvc
	${3RDPARTY_DIR}/libjpeg-turbo
	${3RDPARTY_DIR}/libpng
	${CMAKE_BINARY_DIR}/libjpeg-turbo
	${CMAKE_BINARY_DIR}/libpng
)

ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_REENTRANT -DHAVE_AV_CONFIG_H -DUSE_VAR_BIT_DEPTH -DUSE_PRED -DENCODE_MAIN -DUSE_JCTVC)

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES} ${SRC}/bpgenc.cpp)

ADD_LIBRARY(libbpg STATIC ${SOURCE_FILES} ${SRC}/bpgenc.cpp)

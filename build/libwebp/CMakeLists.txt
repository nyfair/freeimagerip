SET(SRC ${3RDPARTY_DIR}/libwebp/src)

SET(SOURCE_FILES
	${SRC}/dec/alpha.c
	${SRC}/dec/buffer.c
	${SRC}/dec/frame.c
	${SRC}/dec/idec.c
	${SRC}/dec/io.c
	${SRC}/dec/layer.c
	${SRC}/dec/quant.c
	${SRC}/dec/tree.c
	${SRC}/dec/vp8.c
	${SRC}/dec/vp8l.c
	${SRC}/dec/webp.c
	${SRC}/dsp/cpu.c
	${SRC}/dsp/dec.c
	${SRC}/dsp/dec_sse2.c
	${SRC}/dsp/enc.c
	${SRC}/dsp/enc_sse2.c
	${SRC}/dsp/lossless.c
	${SRC}/dsp/upsampling.c
	${SRC}/dsp/upsampling_sse2.c
	${SRC}/dsp/yuv.c
	${SRC}/demux/demux.c
	${SRC}/enc/alpha.c
	${SRC}/enc/analysis.c
	${SRC}/enc/backward_references.c
	${SRC}/enc/config.c
	${SRC}/enc/cost.c
	${SRC}/enc/filter.c
	${SRC}/enc/frame.c
	${SRC}/enc/histogram.c
	${SRC}/enc/iterator.c
	${SRC}/enc/layer.c
	${SRC}/enc/picture.c
	${SRC}/enc/quant.c
	${SRC}/enc/syntax.c
	${SRC}/enc/tree.c
	${SRC}/enc/vp8l.c
	${SRC}/enc/webpenc.c
	${SRC}/enc/token.c
	${SRC}/mux/muxedit.c
	${SRC}/mux/muxinternal.c
	${SRC}/mux/muxread.c
	${SRC}/utils/alpha_processing.c
	${SRC}/utils/bit_reader.c
	${SRC}/utils/bit_writer.c
	${SRC}/utils/color_cache.c
	${SRC}/utils/filters.c
	${SRC}/utils/huffman.c
	${SRC}/utils/huffman_encode.c
	${SRC}/utils/quant_levels.c
	${SRC}/utils/quant_levels_dec.c
	${SRC}/utils/random.c
	${SRC}/utils/rescaler.c
	${SRC}/utils/thread.c
	${SRC}/utils/utils.c
)

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_DEFINITIONS(-DWEBP_EXPERIMENTAL_FEATURES)

ADD_LIBRARY(libwebp STATIC ${SOURCE_FILES})

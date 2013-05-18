SET(SRC ${3RDPARTY_DIR}/libwebp/src)

SET(SOURCE_FILES
	${SRC}/dec/dec_alpha.c
	${SRC}/dec/dec_buffer.c
	${SRC}/dec/dec_frame.c
	${SRC}/dec/dec_idec.c
	${SRC}/dec/dec_io.c
	${SRC}/dec/dec_layer.c
	${SRC}/dec/dec_quant.c
	${SRC}/dec/dec_tree.c
	${SRC}/dec/dec_vp8.c
	${SRC}/dec/dec_vp8l.c
	${SRC}/dec/dec_webp.c
	${SRC}/dsp/cpu.c
	${SRC}/dsp/dec.c
	${SRC}/dsp/dec_sse2.c
	${SRC}/dsp/enc.c
	${SRC}/dsp/enc_sse2.c
	${SRC}/dsp/lossless.c
	${SRC}/dsp/upsampling.c
	${SRC}/dsp/upsampling_sse2.c
	${SRC}/dsp/yuv.c
	${SRC}/enc/enc_alpha.c
	${SRC}/enc/enc_analysis.c
	${SRC}/enc/enc_backward_references.c
	${SRC}/enc/enc_config.c
	${SRC}/enc/enc_cost.c
	${SRC}/enc/enc_filter.c
	${SRC}/enc/enc_frame.c
	${SRC}/enc/enc_histogram.c
	${SRC}/enc/enc_iterator.c
	${SRC}/enc/enc_layer.c
	${SRC}/enc/enc_picture.c
	${SRC}/enc/enc_quant.c
	${SRC}/enc/enc_syntax.c
	${SRC}/enc/enc_tree.c
	${SRC}/enc/enc_vp8l.c
	${SRC}/enc/enc_webpenc.c
	${SRC}/enc/token.c
	${SRC}/utils/bit_reader.c
	${SRC}/utils/bit_writer.c
	${SRC}/utils/color_cache.c
	${SRC}/utils/filters.c
	${SRC}/utils/huffman.c
	${SRC}/utils/huffman_encode.c
	${SRC}/utils/quant_levels.c
	${SRC}/utils/quant_levels_dec.c
	${SRC}/utils/rescaler.c
	${SRC}/utils/thread.c
	${SRC}/utils/utils.c
)

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_DEFINITIONS(-DWEBP_EXPERIMENTAL_FEATURES)

ADD_LIBRARY(libwebp STATIC ${SOURCE_FILES})

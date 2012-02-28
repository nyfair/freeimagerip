SET(SRC ${3RDPARTY_DIR}/LibWebP/src)

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
	${SRC}/dec/webp.c
	${SRC}/dsp/cpu.c
	${SRC}/dsp/dec.c
	${SRC}/dsp/dec_sse2.c
	${SRC}/dsp/enc.c
	${SRC}/dsp/enc_sse2.c
	${SRC}/dsp/upsampling.c
	${SRC}/dsp/upsampling_sse2.c
	${SRC}/dsp/yuv.c
	${SRC}/enc/alpha.c
	${SRC}/enc/analysis.c
	${SRC}/enc/config.c
	${SRC}/enc/cost.c
	${SRC}/enc/filter.c
	${SRC}/enc/frame.c
	${SRC}/enc/iterator.c
	${SRC}/enc/layer.c
	${SRC}/enc/picture.c
	${SRC}/enc/quant.c
	${SRC}/enc/syntax.c
	${SRC}/enc/tree.c
	${SRC}/enc/webpenc.c
	${SRC}/utils/bit_reader.c
	${SRC}/utils/bit_writer.c
	${SRC}/utils/thread.c
)

INCLUDE_DIRECTORIES(
	${SRC}/
)

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_LIBRARY(LibWebP STATIC ${SOURCE_FILES})

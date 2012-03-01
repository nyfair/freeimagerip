SET(SRC ${3RDPARTY_DIR}/libtiff)

SET(SOURCE_FILES
	${SRC}/tif_aux.c
	${SRC}/tif_close.c
	${SRC}/tif_codec.c
	${SRC}/tif_color.c
	${SRC}/tif_compress.c
	${SRC}/tif_dir.c
	${SRC}/tif_dirinfo.c
	${SRC}/tif_dirread.c
	${SRC}/tif_dirwrite.c
	${SRC}/tif_dumpmode.c
	${SRC}/tif_error.c
	${SRC}/tif_extension.c
	${SRC}/tif_fax3.c
	${SRC}/tif_fax3sm.c
	${SRC}/tif_flush.c
	${SRC}/tif_getimage.c
	${SRC}/tif_jpeg.c
	${SRC}/tif_luv.c
	${SRC}/tif_lzma.c
	${SRC}/tif_lzw.c
	${SRC}/tif_next.c
	${SRC}/tif_ojpeg.c
	${SRC}/tif_open.c
	${SRC}/tif_packbits.c
	${SRC}/tif_pixarlog.c
	${SRC}/tif_predict.c
	${SRC}/tif_print.c
	${SRC}/tif_read.c
	${SRC}/tif_stream.cxx
	${SRC}/tif_strip.c
	${SRC}/tif_swab.c
	${SRC}/tif_thunder.c
	${SRC}/tif_tile.c
	${SRC}/tif_version.c
	${SRC}/tif_warning.c
	${SRC}/tif_write.c
	${SRC}/tif_zip.c
)

INCLUDE_DIRECTORIES(
	${3RDPARTY_DIR}/libjpeg-turbo/
)

SOURCE_GROUP("Source Files" FILES ${SOURCE_FILES})

ADD_LIBRARY(libtiff STATIC ${SOURCE_FILES})

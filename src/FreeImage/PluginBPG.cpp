// ==========================================================
// BPG Loader
//
// Design and implementation by
// - nyfair (nyfair2012@gmail.com)
// ==========================================================

#include "FreeImage.h"
#include "Utilities.h"

extern "C" {
#include "libbpg.h"
}

// ==========================================================
// Plugin Interface
// ==========================================================

static int s_format_id;

// ==========================================================
// Plugin Implementation
// ==========================================================

static const char * DLL_CALLCONV
Format() {
	return "BPG";
}

static const char * DLL_CALLCONV
Description() {
	return "Better Portable Graphics";
}

static const char * DLL_CALLCONV
Extension() {
	return "bpg";
}

static const char * DLL_CALLCONV
RegExpr() {
	return NULL;
}

static const char * DLL_CALLCONV
MimeType() {
	return "image/bpg";
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO *io, fi_handle handle) {
	BYTE bpg_signature[4] = { 0x42, 0x50, 0x47, 0xfb };
	BYTE signature[4] = { 0, 0, 0, 0 };

	io->read_proc(&signature, 1, 4, handle);

	return (memcmp(bpg_signature, signature, 4) == 0);
}

static BOOL DLL_CALLCONV
SupportsExportDepth(int depth) {
	return (
		(depth == 24) ||
		(depth == 32)
	);
}

static BOOL DLL_CALLCONV 
SupportsExportType(FREE_IMAGE_TYPE type) {
	return (type == FIT_BITMAP) ? TRUE : FALSE;
}

static BOOL DLL_CALLCONV
SupportsNoPixels() {
	return TRUE;
}

static FIBITMAP * DLL_CALLCONV
Load(FreeImageIO *io, fi_handle handle, int flags, void *data) {
	if(handle) {
		FIBITMAP *dib = NULL;

		try {
			// remember the start offset
			long start_offset = io->tell_proc(handle);

			// remember end-of-file (used for RLE cache)
			io->seek_proc(handle, 0, SEEK_END);
			long eof = io->tell_proc(handle);
			io->seek_proc(handle, start_offset, SEEK_SET);
			int size = eof - start_offset;

			BPGImageInfo img_info_s, *img_info = &img_info_s;
			BPGDecoderContext *img;
			img = bpg_decoder_open();
			if(bpg_decoder_decode(img, (uint8_t *)handle, size) < 0 || bpg_decoder_get_info(img, img_info) < 0) {
				bpg_decoder_close(img);
				return FALSE;
			}

			uint32_t width = img_info->width, height = img_info->height;
			if(img_info->has_alpha) {
				if(bpg_decoder_start(img, BPG_OUTPUT_FORMAT_RGBA32) < 0 ) {
					bpg_decoder_close(img);
					return FALSE;
				}
				dib = FreeImage_Allocate(width, height, 32);
			} else {
				if(bpg_decoder_start(img, BPG_OUTPUT_FORMAT_RGB24) < 0 ) {
					bpg_decoder_close(img);
					return FALSE;
				}
				dib = FreeImage_Allocate(width, height, 24);
			}
			BYTE* curLine;
			for(int y = 0; y < height; y++) {
				curLine = FreeImage_GetScanLine(dib, height);
				if(bpg_decoder_get_line(img, curLine) < 0)
					break;
			}
			return dib;
		} catch (const char *message) {
				FreeImage_OutputMessageProc(s_format_id, message);
				return FALSE;
		}
	}
	return FALSE;
}

static BOOL DLL_CALLCONV
Save(FreeImageIO *io, FIBITMAP *dib, fi_handle handle, int flags, void *data) {
	return FALSE;
}

// ==========================================================
//	 Init
// ==========================================================

void DLL_CALLCONV
InitBPG(Plugin *plugin, int format_id) {
	s_format_id = format_id;

	plugin->format_proc = Format;
	plugin->description_proc = Description;
	plugin->extension_proc = Extension;
	plugin->regexpr_proc = RegExpr;
	plugin->open_proc = NULL;
	plugin->close_proc = NULL;
	plugin->load_proc = Load;
	plugin->save_proc = Save;
	plugin->validate_proc = Validate;
	plugin->mime_proc = MimeType;
	plugin->supports_export_bpp_proc = SupportsExportDepth;
	plugin->supports_export_type_proc = SupportsExportType;
}
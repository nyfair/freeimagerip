// ==========================================================
// BPG Loader
//
// Design and implementation by
// - nyfair (nyfair2012@gmail.com)
// ==========================================================
#include "FreeImage.h"
#include "Utilities.h"

#include "bpgenc.h"

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
		long start_offset = io->tell_proc(handle);
		io->seek_proc(handle, 0, SEEK_END);
		long eof = io->tell_proc(handle);
		io->seek_proc(handle, start_offset, SEEK_SET);
		int size = eof - start_offset;

		BPGImageInfo img_info_s, *img_info = &img_info_s;
		BPGDecoderContext *img;
		img = bpg_decoder_open();
		uint8_t *raw_data = (uint8_t *) malloc(size);
		io->read_proc(raw_data, 1, size, handle);
		if(bpg_decoder_decode(img, raw_data, size) < 0 || bpg_decoder_get_info(img, img_info) < 0) {
			bpg_decoder_close(img);
			return FALSE;
		}

		uint32_t width = img_info->width, height = img_info->height;
		int bpp, i;
		if(img_info->has_alpha) {
			i = bpg_decoder_start(img, BPG_OUTPUT_FORMAT_RGBA32);
			bpp = 32;
		} else {
			i = bpg_decoder_start(img, BPG_OUTPUT_FORMAT_RGB24);
			bpp = 24;
		}
		if(i==0) {
			dib = FreeImage_Allocate(width, height, bpp);
			BYTE* curLine;
			for(int y = 0; y < height; y++) {
				curLine = FreeImage_GetScanLine(dib, height-1-y);
				if(bpg_decoder_get_line(img, curLine))
					break;
			}
		}
		SwapRedBlue32(dib);
		bpg_decoder_close(img);
		free(raw_data);
		return dib;
	}
	return FALSE;
}

static BOOL DLL_CALLCONV
Save(FreeImageIO *io, FIBITMAP *dib, fi_handle handle, int flags, void *data) {
	if(handle) {
		const char *tmpdir = getenv("TEMP");
		if (tmpdir == 0)
			tmpdir = "/tmp";
		char swap_png[256], swap_bpg[256];
		snprintf(swap_png, 256, "%s/__tmp.png", tmpdir);
		snprintf(swap_bpg, 256, "%s/__tmp.bpg", tmpdir);
		FreeImage_Save(FIF_PNG, dib, swap_png, PNG_NO_COMPRESSION);

		int argc;
		char *argv[ARGV_MAX + 1];
		void* outbuf;
		argc = 0;
		add_opt(&argc, argv, swap_png);
		add_opt(&argc, argv, "-o");
		add_opt(&argc, argv, swap_bpg);
		add_opt(&argc, argv, "-m");
		add_opt(&argc, argv, "9");	// max compression level

		if ((flags & BPG_LOSSLESS) == BPG_LOSSLESS) {
			add_opt(&argc, argv, "-lossless");
		} else {
			char *chroma, *bt, *bit;
			if ((flags & BPG_SUBSAMPLING_420) == BPG_SUBSAMPLING_420) {
				chroma = "420";
			} else if ((flags & BPG_SUBSAMPLING_422) == BPG_SUBSAMPLING_422) {
				chroma = "422";
			} else {
				chroma = "444";
			}
			add_opt(&argc, argv, "-f");
			add_opt(&argc, argv, chroma);
			if ((flags & BPG_COLOR_601) == BPG_COLOR_601) {
				bt = "ycbcr";
			} else if ((flags & BPG_COLOR_709) == BPG_COLOR_709) {
				bt = "ycbcr_bt709";
			} else {
				bt = "ycbcr_bt2020";
			}
			add_opt(&argc, argv, "-c");
			add_opt(&argc, argv, bt);
			if ((flags & BPG_BITDEPTH_8) == BPG_BITDEPTH_8) {
				bit = "8";
			} else if ((flags & BPG_BITDEPTH_10) == BPG_BITDEPTH_10) {
				bit = "10";
			} else {
				bit = "12";
			}
			add_opt(&argc, argv, "-b");
			add_opt(&argc, argv, bit);
			int q = flags & 0xff;
			if (q == 0) {
				q = 20;
			} else {
				q = 100 - q;
				if (q < 0 || q > 51)
					q = 51;
			}
			char qp[2];
			sprintf(qp,"%d",q); 
			add_opt(&argc, argv, "-q");
			add_opt(&argc, argv, qp);
		}

		if (encode_main(argc, argv) == 0) {
			FILE *f = fopen(swap_bpg, "rb");
			fseek(f, 0, SEEK_END);
			int size = ftell(f);
			outbuf = malloc(size);
			fseek(f, 0, SEEK_SET);
			fread(outbuf, 1, size, f);
			io->write_proc(outbuf, 1, size, handle);
			free(outbuf);
			return TRUE;
		}
		return FALSE;
	}
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
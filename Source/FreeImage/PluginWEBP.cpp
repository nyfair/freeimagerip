// ==========================================================
// Google WebP Image Loader
//
// Design and implementation by
// - nyfair (nyfair2012@gmail.com)
//
// COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
// OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
// THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
// OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
// CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
// THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
// SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
// PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
// THIS DISCLAIMER.
//
// Use at your own risk!
// ==========================================================

#include "FreeImage.h"
#include "Utilities.h"
#include "webp/decode.h"
#include "webp/encode.h"

// ==========================================================
// Plugin Interface
// ==========================================================

static int s_format_id;

// ----------------------------------------------------------
// Helper Function
int offset;
static int WebPWriter(const uint8_t* data, size_t data_size,
					const WebPPicture* pic) {
  BYTE *ptr = (BYTE*) pic->custom_ptr;
  ptr += offset;
  memcpy(ptr, data, data_size);
  offset += data_size;
  return 1;
}

// ==========================================================
// Plugin Implementation
// ==========================================================

static const char * DLL_CALLCONV
Format() {
	return "WebP";
}

static const char * DLL_CALLCONV
Description() {
	return "Google VP8 Codec Image";
}

static const char * DLL_CALLCONV
Extension() {
	return "webp";
}

static const char * DLL_CALLCONV
RegExpr() {
	return NULL;
}

static const char * DLL_CALLCONV
MimeType() {
	return "image/freeimage-webp";
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO *io, fi_handle handle) {
	BYTE riff_signature[4] = { 0x52, 0x49, 0x46, 0x46 };
	BYTE webp_signature[4] = { 0x57, 0x45, 0x42, 0x50 };
	BYTE signature[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	io->read_proc(signature, 1, 12, handle);

	return (memcmp(riff_signature, signature, 4) == 0
		&& memcmp(webp_signature, signature + 8, 4) == 0);
}

static BOOL DLL_CALLCONV
SupportsExportDepth(int depth) {
	return (depth == 24 || depth == 32);
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
		BYTE *raw_data;

		try {
			// remember the start offset
			long start_offset = io->tell_proc(handle);

			// remember end-of-file (used for RLE cache)
			io->seek_proc(handle, 0, SEEK_END);
			long eof = io->tell_proc(handle);
			io->seek_proc(handle, start_offset, SEEK_SET);
			int size = eof - start_offset;
			raw_data = (BYTE *) malloc(size);
			io->read_proc(raw_data, 1, size, handle);

			WebPDecoderConfig config;
			WebPDecBuffer* const output_buffer = &config.output;
			WebPBitstreamFeatures* const bitstream = &config.input;
			WebPInitDecoderConfig(&config);
			config.options.use_threads = 1;

			VP8StatusCode status = WebPGetFeatures(raw_data, size, bitstream);
			if(status != VP8_STATUS_OK) return NULL;
			output_buffer->colorspace = bitstream->has_alpha ? MODE_BGRA : MODE_BGR;
			status = WebPDecode(raw_data, size, &config);
			if(status != VP8_STATUS_OK) return NULL;
			BYTE *bitmap = output_buffer->u.RGBA.rgba;
			int width = output_buffer->width;
			int height = output_buffer->height;

			if(!bitstream->has_alpha) {
				dib = FreeImage_Allocate(width, height, 24);
				RGBTRIPLE *rgb;
				for(int y = 0; y < height; y++) {
					rgb = (RGBTRIPLE *) FreeImage_GetScanLine(dib, height-1-y);
					for(int x = 0; x < width; x++) {
						rgb[x].rgbtBlue		= bitmap[0];
						rgb[x].rgbtGreen	= bitmap[1];
						rgb[x].rgbtRed		= bitmap[2];
						bitmap += 3;
					}
				}
			} else {
				dib = FreeImage_Allocate(width, height, 32);
				RGBQUAD *rgba;
				for(int y = 0; y < height; y++) {
					rgba = (RGBQUAD *) FreeImage_GetScanLine(dib, height-1-y);
					for(int x = 0; x < width; x++) {
						rgba[x].rgbBlue			= bitmap[0];
						rgba[x].rgbGreen		= bitmap[1];
						rgba[x].rgbRed			= bitmap[2];
						rgba[x].rgbReserved = bitmap[3];
						bitmap += 4;
					}
				}
			}
			WebPFreeDecBuffer(output_buffer);
			return dib;
		} catch (const char *message) {
			FreeImage_OutputMessageProc(s_format_id, message);
			return NULL;
		}
	}
	return NULL;
}

static BOOL DLL_CALLCONV
Save(FreeImageIO *io, FIBITMAP *dib, fi_handle handle, int flags, void *data) {
	if((dib != NULL) && (handle != NULL)) {
		try {
			int width = FreeImage_GetWidth(dib);
			int height = FreeImage_GetHeight(dib);

			WebPPicture picture;
			WebPConfig config;
			WebPPictureInit(&picture);
			WebPConfigInit(&config);

			picture.width = width;
			picture.height = height;
			config.method = 6;
			if((flags & 0x7F) == 0) {
				config.quality = 90;
			} else {
				config.quality = flags & 0x7F;
				if(config.quality > 100) config.quality = 100;
			}
			BYTE *bitmap, *bitmap_ptr, *encode_cache;

			//if((flags & WEBP_SUBSAMPLING_444) == WEBP_SUBSAMPLING_444) {
			//	picture.colorspace = WEBP_YUV444;
			//} else if((flags & WEBP_SUBSAMPLING_422) == WEBP_SUBSAMPLING_422) {
			//	picture.colorspace = WEBP_YUV422;
			//} else {
			//	picture.colorspace = WEBP_YUV420;
			//}

			int pixel_depth = FreeImage_GetBPP(dib);
			switch(pixel_depth) {
				case 24:
					encode_cache = (BYTE *) malloc(width*height*3);
					bitmap = (BYTE *) malloc(width*height*3);
					bitmap_ptr = bitmap;
					RGBTRIPLE *rgb;
					for(int y = 0; y < height; y++) {
						rgb = (RGBTRIPLE *) FreeImage_GetScanLine(dib, height-1-y);
						for(int x = 0; x < width; x++) {
							bitmap[0] = rgb[x].rgbtBlue;
							bitmap[1] = rgb[x].rgbtGreen;
							bitmap[2] = rgb[x].rgbtRed;
							bitmap += 3;
						}
					}
					WebPPictureImportBGR(&picture, bitmap_ptr, width*3);
					break;
				case 32:
					encode_cache = (BYTE *) malloc(width*height*4);
					bitmap = (BYTE *) malloc(width*height*4);
					bitmap_ptr = bitmap;
					RGBQUAD *rgba;
					for(int y = 0; y < height; y++) {
						rgba = (RGBQUAD *) FreeImage_GetScanLine(dib, height-1-y);
						for(int x = 0; x < width; x++) {
							bitmap[0] = rgba[x].rgbBlue;
							bitmap[1] = rgba[x].rgbGreen;
							bitmap[2] = rgba[x].rgbRed;
							bitmap[3] = rgba[x].rgbReserved;
							bitmap += 4;
						}
					}
					WebPPictureImportBGRA(&picture, bitmap_ptr, width*4);
					break;
				default:
					return FALSE;
			}

			picture.custom_ptr = encode_cache;
			picture.writer = WebPWriter;
			offset = 0;
			if(!WebPEncode(&config, &picture)) return FALSE;
			io->write_proc(encode_cache, 1, offset, handle);
			free(picture.extra_info);
			WebPPictureFree(&picture);
			return TRUE;
		} catch (const char* text) {
			FreeImage_OutputMessageProc(s_format_id, text);
		}
	}
	return FALSE;
}

// ==========================================================
//	 Init
// ==========================================================

void DLL_CALLCONV
InitWEBP(Plugin *plugin, int format_id) {
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
	plugin->supports_icc_profiles_proc = NULL;
	plugin->supports_no_pixels_proc = SupportsNoPixels;
}

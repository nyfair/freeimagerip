// ==========================================================
// JPEG XL Image Loader
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

#include <jxl/decode.h>
#include <jxl/encode.h>
#include <jxl/thread_parallel_runner.h>
#include <jxl/thread_parallel_runner_cxx.h>

// ==========================================================
// Plugin Interface
// ==========================================================

static int s_format_id;

// ==========================================================
// Plugin Implementation
// ==========================================================

static const char * DLL_CALLCONV
Format() {
	return "JPEG-XL";
}

static const char * DLL_CALLCONV
Description() {
	return "JPEG XL";
}

static const char * DLL_CALLCONV
Extension() {
	return "jxl";
}

static const char * DLL_CALLCONV
RegExpr() {
	return NULL;
}

static const char * DLL_CALLCONV
MimeType() {
	return "image/jxl";
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO *io, fi_handle handle) {
	uint8_t signature[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	io->read_proc(&signature, 1, 12, handle);
	JxlSignature sig = JxlSignatureCheck(signature, 12);
	return sig > 1;
}

static BOOL DLL_CALLCONV
SupportsExportDepth(int depth) {
	return (
		(depth == 8) ||
		(depth == 16) ||
		(depth == 24) ||
		(depth == 32)
	);
}

static BOOL DLL_CALLCONV 
SupportsExportType(FREE_IMAGE_TYPE type) {
	return (
		(type == FIT_RGB16) ||
		(type == FIT_RGBA16) ||
		(type == FIT_RGBF) ||
		(type == FIT_RGBAF) ||
		(type == FIT_BITMAP)
	);
}

static BOOL DLL_CALLCONV
SupportsNoPixels() {
	return TRUE;
}

static FIBITMAP * DLL_CALLCONV
Load(FreeImageIO *io, fi_handle handle, int flags, void *data) {
	if (handle) {
		FIBITMAP *dib = NULL;
		uint8_t *raw_data;
		uint8_t *bitmap;
		uint8_t *buf;
		JxlDecoder *dec;
		void *runner;

		try {
			long start_offset = io->tell_proc(handle);
			io->seek_proc(handle, 0, SEEK_END);
			long eof = io->tell_proc(handle);
			io->seek_proc(handle, start_offset, SEEK_SET);
			int size = eof - start_offset;

			dec = JxlDecoderCreate(nullptr);
			runner = JxlThreadParallelRunnerCreate(nullptr, JxlThreadParallelRunnerDefaultNumWorkerThreads());
			if (JxlDecoderSetParallelRunner(dec, JxlThreadParallelRunner, runner) != JXL_DEC_SUCCESS) {
				throw "Couldn't Initialize JxlDecoder Runner";
			}
			if (JxlDecoderSubscribeEvents(dec, JXL_DEC_BASIC_INFO | JXL_DEC_FULL_IMAGE) != JXL_DEC_SUCCESS) {
				throw "Couldn't Subscribe JxlDecoder";
			}
			raw_data = (uint8_t*)malloc(size * sizeof(uint8_t));
			io->read_proc(raw_data, 1, size, handle);
			if (JxlDecoderSetInput(dec, raw_data, size) != JXL_DEC_SUCCESS) {
				throw "JxlDecoder Set Input Failed";
			}
			JxlDecoderStatus status;
			JxlBasicInfo info;
			JxlPixelFormat format = {3, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};
			size_t pixels_size;

			while ((status = JxlDecoderProcessInput(dec)) != JXL_DEC_FULL_IMAGE) {
				switch (status) {
					case JXL_DEC_BASIC_INFO:
						if (JxlDecoderGetBasicInfo(dec, &info) != JXL_DEC_SUCCESS) {
							throw "Couldn't Decode JXL Basic Info";
						}
						format.num_channels = ((info.num_color_channels >= 3) ? 3 : 1) + (info.alpha_bits > 0);
						format.data_type = (info.bits_per_sample == 32 ? JXL_TYPE_FLOAT
							: info.bits_per_sample > 8 ? JXL_TYPE_UINT16 : JXL_TYPE_UINT8);
						break;
					case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
						if (JxlDecoderImageOutBufferSize(dec, &format, &pixels_size) != JXL_DEC_SUCCESS ) {
							throw "Couldn't Specify JXL Decode Buffer";
						}
						buf = (uint8_t*)malloc(pixels_size * sizeof(uint8_t));
						bitmap = buf;
						if (JxlDecoderSetImageOutBuffer(dec, &format, bitmap, pixels_size) != JXL_DEC_SUCCESS) {
							throw "Couldn't Decode JXL Pixel Data";
						}
						break;
					case JXL_DEC_NEED_MORE_INPUT:
					case JXL_DEC_ERROR:
						throw "JXL Decode Error";
				}
			}

			size_t bpp = (info.bits_per_sample == 32 ? 32 : info.bits_per_sample > 8 ? 16 : 8) * format.num_channels;
			switch (bpp) {
				case 24:
					dib = FreeImage_Allocate(info.xsize, info.ysize, bpp);
					RGBTRIPLE *rgb;
					for (int y = 0; y < info.ysize; y++) {
						rgb = (RGBTRIPLE *) FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						for (int x = 0; x < info.xsize; x++) {
							rgb[x].rgbtRed	= bitmap[0];
							rgb[x].rgbtGreen= bitmap[1];
							rgb[x].rgbtBlue	= bitmap[2];
							bitmap += 3;
						}
					}
					break;
				case 32:
					dib = FreeImage_Allocate(info.xsize, info.ysize, bpp);
					RGBQUAD *rgba;
					for (int y = 0; y < info.ysize; y++) {
						rgba = (RGBQUAD *) FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						for (int x = 0; x < info.xsize; x++) {
							rgba[x].rgbRed		= bitmap[0];
							rgba[x].rgbGreen	= bitmap[1];
							rgba[x].rgbBlue		= bitmap[2];
							rgba[x].rgbReserved	= bitmap[3];
							bitmap += 4;
						}
					}
					break;
				case 48:
					dib = FreeImage_AllocateT(FIT_RGB16, info.xsize, info.ysize);
					FIRGB16 *rgb16;
					for (int y = 0; y < info.ysize; y++) {
						rgb16 = (FIRGB16 *) FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						for (int x = 0; x < info.xsize; x++) {
							rgb16[x].red	= ((short *)(bitmap))[0];
							rgb16[x].green	= ((short *)(bitmap))[1];
							rgb16[x].blue	= ((short *)(bitmap))[2];
							bitmap += 6;
						}
					}
					break;
				case 64:
					dib = FreeImage_AllocateT(FIT_RGBA16, info.xsize, info.ysize);
					FIRGBA16 *rgba16;
					for (int y = 0; y < info.ysize; y++) {
						rgba16 = (FIRGBA16 *) FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						for (int x = 0; x < info.xsize; x++) {
							rgba16[x].red	= ((short *)(bitmap))[0];
							rgba16[x].green	= ((short *)(bitmap))[1];
							rgba16[x].blue	= ((short *)(bitmap))[2];
							rgba16[x].alpha	= ((short *)(bitmap))[3];
							bitmap += 8;
						}
					}
					break;
				case 96:
					dib = FreeImage_AllocateT(FIT_RGBAF, info.xsize, info.ysize);
					FIRGBF *rgbf;
					for (int y = 0; y < info.ysize; y++) {
						rgbf = (FIRGBF *) FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						for (int x = 0; x < info.xsize; x++) {
							rgba16[x].red	= ((short *)(bitmap))[0];
							rgba16[x].green	= ((short *)(bitmap))[1];
							rgba16[x].blue	= ((short *)(bitmap))[2];
							bitmap += 12;
						}
					}
					break;
				case 128:
					dib = FreeImage_AllocateT(FIT_RGBAF, info.xsize, info.ysize);
					FIRGBAF *rgbaf;
					for (int y = 0; y < info.ysize; y++) {
						rgbaf = (FIRGBAF *) FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						for (int x = 0; x < info.xsize; x++) {
							rgbaf[x].red	= ((float *)(bitmap))[0];
							rgbaf[x].green	= ((float *)(bitmap))[1];
							rgbaf[x].blue	= ((float *)(bitmap))[2];
							rgbaf[x].alpha	= ((float *)(bitmap))[3];
							bitmap += 16;
						}
					}
					break;
				case 8:
				case 16:
					dib = FreeImage_Allocate(info.xsize, info.ysize, bpp);
					for (int y = 0; y < info.ysize; y++) {
						BYTE *line = FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						memcpy(line, bitmap, info.xsize * bpp / 8);
						bitmap += info.xsize  * bpp / 8;
					}
					break;
				default:
					return FALSE;
			}

			free(raw_data);
			free(buf);
			JxlDecoderDestroy(dec);
			JxlThreadParallelRunnerDestroy(runner);
			return dib;
		} catch (const char *message) {
			free(raw_data);
			free(buf);
			if (dec) {
				JxlDecoderDestroy(dec);
			}
			if (runner) {
				JxlThreadParallelRunnerDestroy(runner);
			}
			FreeImage_OutputMessageProc(s_format_id, message);
			return FALSE;
		}
	}
	return FALSE;
}

static BOOL DLL_CALLCONV
Save(FreeImageIO *io, FIBITMAP *dib, fi_handle handle, int flags, void *data) {
	if ((dib != NULL) && (handle != NULL)) {
		if (flags == 0) {
			flags = 90;
		}
		JxlEncoder *enc;
		void *runner;
		size_t bitmap_size;
		void *bitmap;
		void *bitmap_ptr;
		uint8_t *buf;

		try {
			enc = JxlEncoderCreate(nullptr);
			runner = JxlThreadParallelRunnerCreate(nullptr, JxlThreadParallelRunnerDefaultNumWorkerThreads());
			if (JxlEncoderSetParallelRunner(enc, JxlThreadParallelRunner, runner) != JXL_DEC_SUCCESS) {
				throw "Couldn't Initialize JxlEncoder Runner";
			}
			JxlEncoderFrameSettings *opts = JxlEncoderFrameSettingsCreate(enc, NULL);
			if (!opts) {
				throw "Couldn't Create JxlEncoder Frame Settings";
			}

			JxlBasicInfo info;
			JxlEncoderInitBasicInfo(&info);
			JxlPixelFormat format = {3, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};
			info.xsize = FreeImage_GetWidth(dib);
			info.ysize = FreeImage_GetHeight(dib);
			int pixel_depth = FreeImage_GetBPP(dib);

			switch (pixel_depth) {
				case 24:
					bitmap_size = info.xsize * info.ysize * 3;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					RGBTRIPLE *rgb;
					for (int y = 0; y < info.ysize; y++) {
						rgb = (RGBTRIPLE *) FreeImage_GetScanLine(dib, info.ysize-1-y);
						for (int x = 0; x < info.xsize; x++) {
							((BYTE *)bitmap)[0] = rgb[x].rgbtRed;
							((BYTE *)bitmap)[1] = rgb[x].rgbtGreen;
							((BYTE *)bitmap)[2] = rgb[x].rgbtBlue;
							bitmap = (BYTE *)bitmap + 3;
						}
					}
					break;
				case 32:
					info.alpha_bits = 8;
					info.num_extra_channels = 1;
					format.num_channels = 4;
					bitmap_size = info.xsize * info.ysize * 4;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					RGBQUAD *rgba;
					for (int y = 0; y < info.ysize; y++) {
						rgba = (RGBQUAD *) FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						for (int x = 0; x < info.xsize; x++) {
							((BYTE *)bitmap)[0] = rgba[x].rgbRed;
							((BYTE *)bitmap)[1] = rgba[x].rgbGreen;
							((BYTE *)bitmap)[2] = rgba[x].rgbBlue;
							((BYTE *)bitmap)[3] = rgba[x].rgbReserved;
							bitmap = (BYTE *)bitmap + 4;
						}
					}
					break;
				case 48:
					if ((flags & JXL_LOSSLESS) == JXL_LOSSLESS) {
						info.bits_per_sample = 16;
					} else {
						info.bits_per_sample = 12;
					}
					format.data_type = JXL_TYPE_UINT16;
					bitmap_size = info.xsize * info.ysize * 6;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					FIRGB16 *rgb16;
					for (int y = 0; y < info.ysize; y++) {
						rgb16 = (FIRGB16 *) FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						for (int x = 0; x < info.xsize; x++) {
							((short *)bitmap)[0] = rgb16[x].red;
							((short *)bitmap)[1] = rgb16[x].green;
							((short *)bitmap)[2] = rgb16[x].blue;
							bitmap = (BYTE *)bitmap + 6;
						}
					}
					break;
				case 64:
					if ((flags & JXL_LOSSLESS) == JXL_LOSSLESS) {
						info.bits_per_sample = 16;
						info.alpha_bits = 16;
					} else {
						info.bits_per_sample = 12;
						info.alpha_bits = 12;
					}
					info.num_extra_channels = 1;
					format.num_channels = 4;
					format.data_type = JXL_TYPE_UINT16;
					bitmap_size = info.xsize * info.ysize * 8;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					FIRGBA16 *rgba16;
					for (int y = 0; y < info.ysize; y++) {
						rgba16 = (FIRGBA16 *) FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						for (int x = 0; x < info.xsize; x++) {
							((short *)bitmap)[0] = rgba16[x].red;
							((short *)bitmap)[1] = rgba16[x].green;
							((short *)bitmap)[2] = rgba16[x].blue;
							((short *)bitmap)[3] = rgba16[x].alpha;
							bitmap = (BYTE *)bitmap + 8;
						}
					}
					break;
				case 96:
					info.bits_per_sample = 32;
					format.data_type = JXL_TYPE_FLOAT;
					bitmap_size = info.xsize * info.ysize * 12;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					FIRGBF *rgbf;
					for (int y = 0; y < info.ysize; y++) {
						rgbf = (FIRGBF *) FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						for (int x = 0; x < info.xsize; x++) {
							((float *)bitmap)[0] = rgbf[x].red;
							((float *)bitmap)[1] = rgbf[x].green;
							((float *)bitmap)[2] = rgbf[x].blue;
							bitmap = (BYTE *)bitmap + 12;
						}
					}
					break;
				case 128:
					info.bits_per_sample = 32;
					info.alpha_bits = 32;
					info.num_extra_channels = 1;
					format.num_channels = 4;
					format.data_type = JXL_TYPE_FLOAT;
					bitmap_size = info.xsize * info.ysize * 16;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					FIRGBAF *rgbaf;
					for (int y = 0; y < info.ysize; y++) {
						rgbaf = (FIRGBAF *) FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						for (int x = 0; x < info.xsize; x++) {
							((float *)bitmap)[0] = rgbaf[x].red;
							((float *)bitmap)[1] = rgbaf[x].green;
							((float *)bitmap)[2] = rgbaf[x].blue;
							((float *)bitmap)[3] = rgbaf[x].alpha;
							bitmap = (BYTE *)bitmap + 16;
						}
					}
					break;
				case 8:
					info.num_color_channels = 1;
					format.num_channels = 1;
					bitmap_size = info.xsize * info.ysize;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					for (int y = 0; y < info.ysize; y++) {
						BYTE *line = FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						memcpy(bitmap, line, info.xsize);
						bitmap = (BYTE *)bitmap + info.xsize;
					}
					break;
				case 16:
					info.num_color_channels = 1;
					info.alpha_bits = 8;
					info.num_extra_channels = 1;
					format.num_channels = 2;
					bitmap_size = info.xsize * info.ysize * 2;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					for (int y = 0; y < info.ysize; y++) {
						BYTE *line = FreeImage_GetScanLine(dib, info.ysize - 1 - y);
						memcpy(bitmap, line, info.xsize * 2);
						bitmap = (BYTE *)bitmap + info.xsize * 2;
					}
					break;
				default:
					return FALSE;
			}
			if ((flags & JXL_BITDEPTH_10) == JXL_BITDEPTH_10) {
				info.bits_per_sample = 10;
				format.data_type = JXL_TYPE_UINT16;
				if (info.num_extra_channels > 0) {
					info.alpha_bits = 10;
				}
			}
			if ((flags & JXL_BITDEPTH_12) == JXL_BITDEPTH_12) {
				info.bits_per_sample = 12;
				format.data_type = JXL_TYPE_UINT16;
				if (info.num_extra_channels > 0) {
					info.alpha_bits = 12;
				}
			}
			if ((flags & JXL_BITDEPTH_16) == JXL_BITDEPTH_16) {
				info.bits_per_sample = 16;
				format.data_type = JXL_TYPE_UINT16;
				if (info.num_extra_channels > 0) {
					info.alpha_bits = 16;
				}
			}
			if ((flags & JXL_BITDEPTH_32) == JXL_BITDEPTH_32) {
				info.bits_per_sample = 32;
				format.data_type = JXL_TYPE_FLOAT;
				if (info.num_extra_channels > 0) {
					info.alpha_bits = 32;
				}
			}

			int quality = flags & 0xff;
			if (quality >= JXL_LOSSLESS) {
				info.uses_original_profile = JXL_TRUE;
				if (JxlEncoderSetFrameLossless(opts, JXL_TRUE) != JXL_ENC_SUCCESS) {
					throw "Couldn't Set Encode JXL Lossless";
				}
			} else {
				float distance = 15.0;
				if (quality >= 90) {
					distance = (100.0 - quality) * 0.1;
				} else if (quality >= 30) {
					distance = 0.1 + (100.0 - quality) * 0.09;
				} else if (quality > 0) {
					distance = 15.0 + (59.0 * quality - 4350.0) * quality / 9000.0;
				}
				if (JxlEncoderSetFrameDistance(opts, distance) != JXL_ENC_SUCCESS) {
					throw "Couldn't Set Encode Distance";
				}
			}
			if (JxlEncoderFrameSettingsSetOption(opts, JXL_ENC_FRAME_SETTING_EFFORT, 9) != JXL_ENC_SUCCESS) {
				throw "Couldn't Set Encode Effort";
			}
			if (JxlEncoderSetBasicInfo(enc, &info) != JXL_ENC_SUCCESS) {
				throw "Couldn't Set Encode Info";
			}
			JxlColorEncoding color;
			JxlColorEncodingSetToSRGB(&color, JXL_FALSE);
			if (JxlEncoderSetColorEncoding(enc, &color) != JXL_ENC_SUCCESS) {
				throw "Couldn't Set Encode Color";
			}

			if (JxlEncoderAddImageFrame(opts, &format, bitmap_ptr, bitmap_size) != JXL_ENC_SUCCESS) {
				throw "Couldn't Encode JXL Frame";
			}
			JxlEncoderCloseInput(enc);
			JxlEncoderStatus res;
			size_t buf_size = bitmap_size / 8;
			buf = (uint8_t*)malloc(buf_size);
			uint8_t *next_out = buf;
			size_t avail_out = buf_size;
			while ((res = JxlEncoderProcessOutput(enc, &next_out, &avail_out)) != JXL_ENC_SUCCESS) {
				if (res == JXL_ENC_NEED_MORE_OUTPUT) {
					if (next_out == buf) {
						throw "Encoding stalled";
					}
					if (io->write_proc((void*)buf, 1, buf_size - avail_out, handle) != buf_size - avail_out) {
						FreeImage_OutputMessageProc(s_format_id, "Failed to write jxl output file");
						throw (1);
					}
					next_out = buf;
					avail_out = buf_size;
				}
				else {
					throw "Error during encoding";
				}
			}
			if (io->write_proc((void*)buf, 1, buf_size - avail_out, handle) != buf_size - avail_out) {
				FreeImage_OutputMessageProc(s_format_id, "Failed to write jxl output file");
				throw (1);
			}
			free(bitmap_ptr);
			free(buf);
			JxlEncoderDestroy(enc);
			JxlThreadParallelRunnerDestroy(runner);
			return TRUE;
		} catch (const char* text) {
			free(bitmap_ptr);
			free(buf);
			if (enc) {
				JxlEncoderDestroy(enc);
			}
			if (runner) {
				JxlThreadParallelRunnerDestroy(runner);
			}
			FreeImage_OutputMessageProc(s_format_id, text);
		}
	}
	return FALSE;
}

// ==========================================================
//	 Init
// ==========================================================

void DLL_CALLCONV
InitJXL(Plugin *plugin, int format_id) {
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

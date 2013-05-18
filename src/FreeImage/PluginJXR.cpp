// ==========================================================
// Microsoft HD Photo Loader
//
// Design and implementation by
// - nyfair (nyfair2012@gmail.com)
//
// Note: this file is not under the FreeImage license
// the license can be found at http://en.wikipedia.org/wiki/HD_Photo
//
// Use at your own risk!
// ==========================================================

#include "FreeImage.h"
#include "Utilities.h"

#include "JXRGlue.h"

// ==========================================================
// Plugin Interface
// ==========================================================

static int s_format_id;

// ==========================================================
// Plugin Implementation
// ==========================================================

static const char * DLL_CALLCONV
Format() {
	return "JPEG-XR";
}

static const char * DLL_CALLCONV
Description() {
	return "Microsoft HD Photo";
}

static const char * DLL_CALLCONV
Extension() {
	return "wdp,hdp,jxr";
}

static const char * DLL_CALLCONV
RegExpr() {
	return NULL;
}

static const char * DLL_CALLCONV
MimeType() {
	return "image/vnd.ms-photo";
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO *io, fi_handle handle) {
	BYTE jxr_signature[3] = { 0x49, 0x49, 0xbc };
	BYTE signature[3] = { 0, 0, 0 };

	io->read_proc(&signature, 1, 3, handle);

	return (memcmp(jxr_signature, signature, 3) == 0);
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
			int width, height;
			float rX, rY;
			GUID format;
			U8 *bitmap;

			raw_data = (U8 *) malloc(size);
			struct WMPStream* pDecodeStream = NULL;
			PKImageDecode* pDecoder = NULL;
			PKImageDecode_Create_WMP(&pDecoder);
			io->read_proc(raw_data, 1, size, handle);
			CreateWS_Memory(&pDecodeStream, raw_data, size);
			pDecoder->Initialize(pDecoder, pDecodeStream);
			
			PKImageDecode_GetPixelFormat(pDecoder, &format);
			PKImageDecode_GetSize(pDecoder, &width, &height);
			PKImageDecode_GetResolution(pDecoder, &rX, &rY);
			FreeImage_SetDotsPerMeterX(dib, (unsigned)(rX/0.0254));
			FreeImage_SetDotsPerMeterY(dib, (unsigned)(rY/0.0254));
			
			PKRect rect = {0, 0, width, height};
			U32 cbStride = 0;
			PKPixelInfo pixelInfo;
			pixelInfo.pGUIDPixFmt = &format;
			PixelFormatLookup(&pixelInfo, LOOKUP_FORWARD);
			cbStride = (((pixelInfo.cbitUnit + 7) >> 3) * width);
			
			switch(pixelInfo.cbitUnit) {
				case 24:
					bitmap = (U8 *) malloc(width*height*3);
					pDecoder->Copy(pDecoder, &rect, bitmap, cbStride);
					dib = FreeImage_Allocate(width, height, pixelInfo.cbitUnit);
					RGBTRIPLE *rgb;
					for(int y = 0; y < height; y++) {
						rgb = (RGBTRIPLE *) FreeImage_GetScanLine(dib, height-1-y);
						for(int x = 0; x < width; x++) {
							rgb[x].rgbtBlue	= bitmap[0];
							rgb[x].rgbtGreen= bitmap[1];
							rgb[x].rgbtRed	= bitmap[2];
							bitmap += 3;
						}
					}
					break;
				case 32:
					bitmap = (U8 *) malloc(width*height*4);
					pDecoder->Copy(pDecoder, &rect, bitmap, cbStride);
					dib = FreeImage_Allocate(width, height, pixelInfo.cbitUnit);
					RGBQUAD *rgba;
					for(int y = 0; y < height; y++) {
						rgba = (RGBQUAD *) FreeImage_GetScanLine(dib, height-1-y);
						for(int x = 0; x < width; x++) {
							rgba[x].rgbBlue		= bitmap[0];
							rgba[x].rgbGreen	= bitmap[1];
							rgba[x].rgbRed		= bitmap[2];
							rgba[x].rgbReserved	= bitmap[3];
							bitmap += 4;
						}
					}
					break;
				case 48:
					bitmap = (U8 *) malloc(width*height*6);
					pDecoder->Copy(pDecoder, &rect, bitmap, cbStride);
					dib = FreeImage_AllocateHeaderT(FALSE, FIT_RGB16, width, height);
					FIRGB16 *rgb16;
					for(int y = 0; y < height; y++) {
						rgb16 = (FIRGB16 *) FreeImage_GetScanLine(dib, height-1-y);
						for(int x = 0; x < width; x++) {
							rgb16[x].red	= ((short *)(bitmap))[0];
							rgb16[x].green	= ((short *)(bitmap))[1];
							rgb16[x].blue	= ((short *)(bitmap))[2];
							bitmap += 6;
						}
					}
					break;
				case 64:
					bitmap = (U8 *) malloc(width*height*8);
					pDecoder->Copy(pDecoder, &rect, bitmap, cbStride);
					dib = FreeImage_AllocateHeaderT(FALSE, FIT_RGBA16, width, height);
					FIRGBA16 *rgba16;
					for(int y = 0; y < height; y++) {
						rgba16 = (FIRGBA16 *) FreeImage_GetScanLine(dib, height-1-y);
						for(int x = 0; x < width; x++) {
							rgba16[x].red	= ((short *)(bitmap))[0];
							rgba16[x].green	= ((short *)(bitmap))[1];
							rgba16[x].blue	= ((short *)(bitmap))[2];
							rgba16[x].alpha	= ((short *)(bitmap))[3];
							bitmap += 8;
						}
					}
					break;
				case 128:
					bitmap = (U8 *) malloc(width*height*16);
					pDecoder->Copy(pDecoder, &rect, bitmap, cbStride);
					if (format == GUID_PKPixelFormat128bppRGBFloat) {
						dib = FreeImage_AllocateHeaderT(FALSE, FIT_RGBF, width, height);
						FIRGBF *rgbf;
						for(int y = 0; y < height; y++) {
							rgbf = (FIRGBF *) FreeImage_GetScanLine(dib, height-1-y);
							for(int x = 0; x < width; x++) {
								rgbf[x].red	= ((float *)(bitmap))[0];
								rgbf[x].green	= ((float *)(bitmap))[1];
								rgbf[x].blue	= ((float *)(bitmap))[2];
								bitmap += 16;
							}
						}
					} else if (format == GUID_PKPixelFormat128bppRGBAFloat) {
						dib = FreeImage_AllocateHeaderT(FALSE, FIT_RGBAF, width, height);
						FIRGBAF *rgbaf;
						for(int y = 0; y < height; y++) {
							rgbaf = (FIRGBAF *) FreeImage_GetScanLine(dib, height-1-y);
							for(int x = 0; x < width; x++) {
								rgbaf[x].red	= ((float *)(bitmap))[0];
								rgbaf[x].green	= ((float *)(bitmap))[1];
								rgbaf[x].blue	= ((float *)(bitmap))[2];
								rgbaf[x].alpha	= ((float *)(bitmap))[3];
								bitmap += 16;
							}
						}
					} else return FALSE;
					break;
				case 8:
					bitmap = (U8 *) malloc(width*height);
					pDecoder->Copy(pDecoder, &rect, bitmap, cbStride);
					dib = FreeImage_Allocate(width, height, pixelInfo.cbitUnit);
					for(int y = 0; y < height; y++) {
						BYTE *line = FreeImage_GetScanLine(dib, height-1-y);
						memcpy(line, bitmap, width);
						bitmap += width;
					}
					break;
				default:
					return FALSE;
			}

			pDecoder->Release(&pDecoder);
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
	struct WMPStream *pEncodeStream = NULL;
	PKImageEncode *pEncoder = NULL;

	if((dib != NULL) && (handle != NULL)) {
		if(flags == 0) flags = 80;
		try {
			int width	= FreeImage_GetWidth(dib);
			int height = FreeImage_GetHeight(dib);
			// JPEG-XR's decoder doesn't support too small image although it may be valid when encodes
			if(width < 16 || height < 16) return FALSE;
			bool hasAlpha = false;
			PKPixelFormatGUID format;
			size_t bitmap_size;
			int pixel_depth = FreeImage_GetBPP(dib);

			void *bitmap;
			void *bitmap_ptr = NULL;
			switch(pixel_depth) {
				case 24:
					format = GUID_PKPixelFormat24bppBGR;
					bitmap_size = width * height * 3;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					RGBTRIPLE *rgb;
					for(int y = 0; y < height; y++) {
						rgb = (RGBTRIPLE *) FreeImage_GetScanLine(dib, height-1-y);
						for(int x = 0; x < width; x++) {
							((BYTE *)bitmap)[0] = rgb[x].rgbtBlue;
							((BYTE *)bitmap)[1] = rgb[x].rgbtGreen;
							((BYTE *)bitmap)[2] = rgb[x].rgbtRed;
							bitmap = (BYTE *)bitmap + 3;
						}
					}
					break;
				case 32:
					format = GUID_PKPixelFormat32bppBGRA;
					hasAlpha = true;
					bitmap_size = width * height * 4;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					RGBQUAD *rgba;
					for(int y = 0; y < height; y++) {
						rgba = (RGBQUAD *) FreeImage_GetScanLine(dib, height-1-y);
						for(int x = 0; x < width; x++) {
							((BYTE *)bitmap)[0] = rgba[x].rgbBlue;
							((BYTE *)bitmap)[1] = rgba[x].rgbGreen;
							((BYTE *)bitmap)[2] = rgba[x].rgbRed;
							((BYTE *)bitmap)[3] = rgba[x].rgbReserved;
							bitmap = (BYTE *)bitmap + 4;
						}
					}
					break;
				case 48:
					format = GUID_PKPixelFormat48bppRGB;
					bitmap_size = width * height * 6;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					FIRGB16 *rgb16;
					for(int y = 0; y < height; y++) {
						rgb16 = (FIRGB16 *) FreeImage_GetScanLine(dib, height-1-y);
						for(int x = 0; x < width; x++) {
							((short *)bitmap)[0] = rgb16[x].red;
							((short *)bitmap)[1] = rgb16[x].green;
							((short *)bitmap)[2] = rgb16[x].blue;
							bitmap = (BYTE *)bitmap + 6;
						}
					}
					break;
				case 64:
					format = GUID_PKPixelFormat64bppRGBA;
					hasAlpha = true;
					bitmap_size = width * height * 8;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					FIRGBA16 *rgba16;
					for(int y = 0; y < height; y++) {
						rgba16 = (FIRGBA16 *) FreeImage_GetScanLine(dib, height-1-y);
						for(int x = 0; x < width; x++) {
							((short *)bitmap)[0] = rgba16[x].red;
							((short *)bitmap)[1] = rgba16[x].green;
							((short *)bitmap)[2] = rgba16[x].blue;
							((short *)bitmap)[3] = rgba16[x].alpha;
							bitmap = (BYTE *)bitmap + 8;
						}
					}
					break;
				case 96:
					// 96bppRGBFloat doesn't supported
					format = GUID_PKPixelFormat128bppRGBFloat;
					bitmap_size = width * height * 16;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					FIRGBF *rgbf;
					for(int y = 0; y < height; y++) {
						rgbf = (FIRGBF *) FreeImage_GetScanLine(dib, height-1-y);
						for(int x = 0; x < width; x++) {
							((float *)bitmap)[0] = rgbf[x].red;
							((float *)bitmap)[1] = rgbf[x].green;
							((float *)bitmap)[2] = rgbf[x].blue;
							bitmap = (BYTE *)bitmap + 16;
						}
					}
					break;
				case 128:
					format = GUID_PKPixelFormat128bppRGBAFloat;
					hasAlpha = true;
					bitmap_size = width * height * 16;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					FIRGBAF *rgbaf;
					for(int y = 0; y < height; y++) {
						rgbaf = (FIRGBAF *) FreeImage_GetScanLine(dib, height-1-y);
						for(int x = 0; x < width; x++) {
							((float *)bitmap)[0] = rgbaf[x].red;
							((float *)bitmap)[1] = rgbaf[x].green;
							((float *)bitmap)[2] = rgbaf[x].blue;
							((float *)bitmap)[3] = rgbaf[x].alpha;
							bitmap = (BYTE *)bitmap + 16;
						}
					}
					break;
				case 8:
					format = GUID_PKPixelFormat8bppGray;
					bitmap_size = width * height;
					bitmap = malloc(bitmap_size);
					bitmap_ptr = bitmap;
					for(int y = 0; y < height; y++) {
						BYTE *line = FreeImage_GetScanLine(dib, height-1-y);
						memcpy(bitmap, line, width);
						bitmap = (BYTE *)bitmap + width;
					}
					break;
				default:
					return FALSE;
			}

		U32 cbStride = 0;
		PKPixelInfo pixelInfo;
		PKImageEncode_Create_WMP(&pEncoder);
		BYTE *encode_cache = (BYTE *) malloc(bitmap_size);
		CreateWS_Memory(&pEncodeStream, (U8*)encode_cache, bitmap_size);

		if((flags & JXR_SUBSAMPLING_420) == JXR_SUBSAMPLING_420) {
			pEncoder->WMP.wmiSCP.cfColorFormat = YUV_420;
		} else if((flags & JXR_SUBSAMPLING_422) == JXR_SUBSAMPLING_422) {
			pEncoder->WMP.wmiSCP.cfColorFormat = YUV_422;
		} else {
			pEncoder->WMP.wmiSCP.cfColorFormat = YUV_444;
		}
		
		pEncoder->WMP.wmiSCP.bdBitDepth = BD_LONG;
		pEncoder->WMP.wmiSCP.olOverlap = OL_ONE;
		
		pEncoder->WMP.wmiSCP.uiDefaultQPIndex = (flags & 0xFF) >= 100 ? 0 : 100 - (flags & 0xFF);
		if(hasAlpha) {
			pEncoder->WMP.wmiSCP.uAlphaMode = 2;
			int alphaq = ((flags & 0xFF000) >> 12);
			if(alphaq == 0) alphaq = pEncoder->WMP.wmiSCP.uiDefaultQPIndex;
			pEncoder->WMP.wmiSCP_Alpha.uiDefaultQPIndex = alphaq;
		}

		pixelInfo.pGUIDPixFmt = &format;
		PixelFormatLookup(&pixelInfo, LOOKUP_FORWARD);
		cbStride = (((pixelInfo.cbitUnit + 7) >> 3) * width);

		pEncoder->Initialize(pEncoder, pEncodeStream, 
							&pEncoder->WMP.wmiSCP, sizeof(pEncoder->WMP.wmiSCP));
		pEncoder->SetPixelFormat(pEncoder, format);
		pEncoder->SetSize(pEncoder, width, height);
		unsigned x = FreeImage_GetDotsPerMeterX(dib);
		unsigned y = FreeImage_GetDotsPerMeterY(dib);
		pEncoder->SetResolution(pEncoder, x*(float)0.0254, y*(float)0.0254);
		pEncoder->WritePixels(pEncoder, height, (U8 *) bitmap_ptr, cbStride);
		int image_size = pEncoder->WMP.nCbImage + pEncoder->WMP.nCbAlpha
							+ pEncoder->WMP.nOffImage;
		io->write_proc(encode_cache, 1, image_size, handle);

		pEncoder->Release(&pEncoder);
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
InitJXR(Plugin *plugin, int format_id) {
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

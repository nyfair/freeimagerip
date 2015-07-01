#include <windows.h>
#include <string>
#include "FreeImage.h"
#include "susie.h"

FREE_IMAGE_FORMAT fmt;

int WINAPI GetPluginInfo(int infono, LPSTR buf, int buflen) {
	if(infono < 0 || infono >= (sizeof(pluginfo) / sizeof(char *)))
		return FALSE;
	if(infono == 2) {
		int count = FreeImage_GetFIFCount(), i;
		std::string ext = "";
		for(i = 0; i < count-1; i++) {
			ext.append("*.").append(FreeImage_GetFIFExtensionList
								((FREE_IMAGE_FORMAT)i)).append(";");
		}
		ext.append("*.").append(FreeImage_GetFIFExtensionList
							((FREE_IMAGE_FORMAT)(count-1)));
		while(i = ext.find(",")) {
			if(i < 0) break;
			ext.replace(i, 1, ";*.", 3);
		}
		lstrcpyn(buf, ext.data(), buflen);
	} else {
		lstrcpyn(buf, pluginfo[infono], buflen);
	}
	return lstrlen(buf);
}

int WINAPI IsSupported(LPSTR filename, DWORD dw) {
	fmt = FreeImage_GetFIFFromFilename(filename);
	return fmt != FIF_UNKNOWN;
}

int WINAPI IsSupportedW(LPWSTR filename, DWORD dw) {
	fmt = FreeImage_GetFIFFromFilenameU(filename);
	return fmt != FIF_UNKNOWN;
}

int WINAPI GetPictureInfo(LPSTR buf, long len,
									unsigned flag, PictureInfo *lpInfo) {
	return SPI_ALL_RIGHT;
}

int WINAPI GetPictureInfoW(LPWSTR buf, long len,
									unsigned flag, PictureInfo *lpInfo) {
	return SPI_ALL_RIGHT;
}

int GetPictureEx(FIBITMAP* dib, HANDLE *pHBInfo, HANDLE *pHBm,
	SPI_PROGRESS lpPrgressCallback, long lData) {
	if (lpPrgressCallback != NULL)
		if (lpPrgressCallback(0, 1, lData))
		return SPI_ABORT;
	int ret = SPI_ALL_RIGHT;

	unsigned width = FreeImage_GetWidth(dib);
	unsigned height = FreeImage_GetHeight(dib);
	unsigned bpp_real = FreeImage_GetBPP(dib);
	unsigned bpp = bpp_real > 32 ? (bpp_real % 48 ? 32 : 24) : bpp_real;
	unsigned factor, line_size;
	if (bpp <= 8) {
		line_size = FreeImage_GetPitch(dib);
	} else {
		factor = bpp >> 3;
		line_size = (factor*width + 3)&~3;
	}
	unsigned remain = line_size - factor*width;
	unsigned bitmap_size = line_size * height;

	if(bpp_real <= 8) {
		*pHBInfo = LocalAlloc(LMEM_MOVEABLE, infosize + (sizeof(RGBQUAD) << bpp_real));
	} else {
		*pHBInfo = LocalAlloc(LMEM_MOVEABLE, infosize);
	}
	*pHBm = LocalAlloc(LMEM_MOVEABLE, bitmap_size);
	BITMAPINFO *pinfo = (BITMAPINFO *)LocalLock(*pHBInfo);
	BYTE *bitmap = (BYTE *)LocalLock(*pHBm);

	if(*pHBInfo == NULL || *pHBm == NULL) {
		if(*pHBInfo != NULL) LocalFree(*pHBInfo);
		if(*pHBm != NULL) LocalFree(*pHBm);
		return SPI_NO_MEMORY;
	}

	BITMAPINFO *info = FreeImage_GetInfo(dib);
	pinfo->bmiHeader = info->bmiHeader;
	if(bpp_real <= 8) {
		memcpy(pinfo->bmiColors, info->bmiColors, sizeof(RGBQUAD) << bpp_real);
	}

	switch(bpp_real) {
		case 1:
		case 4:
		case 8:
		case 16:
		case 24:
		case 32:
			FreeImage_ConvertToRawBits(bitmap, dib, line_size, bpp_real, 0, 0, 0, 0);
			break;
		case 48:
			pinfo->bmiHeader.biBitCount = 24;
			FIRGB16 *rgb16;
			for(unsigned y = height-1; y; y--) {
				rgb16 = (FIRGB16 *) FreeImage_GetScanLine(dib, height-1-y);
				for(unsigned x = 0; x < width; x++) {
					((BYTE *)bitmap)[0] = (BYTE)(rgb16[x].blue >> 8);
					((BYTE *)bitmap)[1] = (BYTE)(rgb16[x].green >> 8);
					((BYTE *)bitmap)[2] = (BYTE)(rgb16[x].red >> 8);
					bitmap = (BYTE *)bitmap + 3;
				}
				bitmap = (BYTE *)bitmap + remain;
			}
			break;
		case 64:
			pinfo->bmiHeader.biBitCount = 32;
			FIRGBA16 *rgba16;
			for(unsigned y = height-1; y; y--) {
				rgba16 = (FIRGBA16 *) FreeImage_GetScanLine(dib, height-1-y);
				for(unsigned x = 0; x < width; x++) {
					((BYTE *)bitmap)[0] = (BYTE)(rgba16[x].blue >> 8);
					((BYTE *)bitmap)[1] = (BYTE)(rgba16[x].green >> 8);
					((BYTE *)bitmap)[2] = (BYTE)(rgba16[x].red >> 8);
					((BYTE *)bitmap)[3] = (BYTE)(rgba16[x].alpha >> 8);
					bitmap = (BYTE *)bitmap + 4;
				}
			}
			break;
		case 96:
			pinfo->bmiHeader.biBitCount = 24;
			FIRGBF *rgbf;
			for(unsigned y = height-1; y; y--) {
				rgbf = (FIRGBF *) FreeImage_GetScanLine(dib, height-1-y);
				for(unsigned x = 0; x < width; x++) {
					((BYTE *)bitmap)[0] = (BYTE)(rgbf[x].blue * 256);
					((BYTE *)bitmap)[1] = (BYTE)(rgbf[x].green * 256);
					((BYTE *)bitmap)[2] = (BYTE)(rgbf[x].red * 256);
					bitmap = (BYTE *)bitmap + 3;
				}
				bitmap = (BYTE *)bitmap + remain;
			}
			break;
		case 128:
			pinfo->bmiHeader.biBitCount = 32;
			FIRGBAF *rgbaf;
			for(unsigned y = height-1; y; y--) {
				rgbaf = (FIRGBAF *) FreeImage_GetScanLine(dib, height-1-y);
				for(unsigned x = 0; x < width; x++) {
					((BYTE *)bitmap)[0] = (BYTE)(rgbaf[x].blue * 256);
					((BYTE *)bitmap)[1] = (BYTE)(rgbaf[x].green * 256);
					((BYTE *)bitmap)[2] = (BYTE)(rgbaf[x].red * 256);
					((BYTE *)bitmap)[3] = (BYTE)(rgbaf[x].alpha * 256);
					bitmap = (BYTE *)bitmap + 4;
				}
			}
			break;
		default:
			return SPI_OTHER_ERROR;
	}

	LocalUnlock(*pHBInfo);
	LocalUnlock(*pHBm);
	if(lpPrgressCallback != NULL)
		if(lpPrgressCallback(1, 1, lData))
			return SPI_ABORT;
	FreeImage_Unload(dib);
	return ret;
}

int WINAPI GetPicture(LPSTR buf, long len, unsigned flag,
						HANDLE *pHBInfo, HANDLE *pHBm,
						SPI_PROGRESS lpPrgressCallback, long lData) {
	FIBITMAP* dib;
	if((flag & 7) == 0) {
	/* buf is filename */
		fmt = FreeImage_GetFileType(buf);
		dib = FreeImage_Load(fmt, buf);
	} else {
	/* buf is memory */
		FIMEMORY *mem = FreeImage_OpenMemory((BYTE*) buf, len); 
		fmt = FreeImage_GetFileTypeFromMemory(mem);
		dib = FreeImage_LoadFromMemory(fmt, mem);
		FreeImage_CloseMemory(mem);
	}

	return GetPictureEx(dib, pHBInfo, pHBm, lpPrgressCallback, lData);
}

int WINAPI GetPictureW(LPWSTR buf, long len, unsigned flag,
						HANDLE *pHBInfo, HANDLE *pHBm,
						SPI_PROGRESS lpPrgressCallback, long lData) {
	FIBITMAP* dib;
	if((flag & 7) == 0) {
	/* buf is filename */
		fmt = FreeImage_GetFIFFromFilenameU(buf);
		dib = FreeImage_LoadU(fmt, buf);
	} else {
	/* buf is memory */
		FIMEMORY *mem = FreeImage_OpenMemory((BYTE*) buf, len); 
		fmt = FreeImage_GetFileTypeFromMemory(mem);
		dib = FreeImage_LoadFromMemory(fmt, mem);
		FreeImage_CloseMemory(mem);
	}

	return GetPictureEx(dib, pHBInfo, pHBm, lpPrgressCallback, lData);
}

int WINAPI GetPreview(LPSTR buf, long len, unsigned flag,
						HANDLE *pHBInfo, HANDLE *pHBm,
						SPI_PROGRESS lpPrgressCallback, long lData) {
	return SPI_NO_FUNCTION;
}

int WINAPI GetPreviewW(LPWSTR buf, long len, unsigned flag,
						HANDLE *pHBInfo, HANDLE *pHBm,
						SPI_PROGRESS lpPrgressCallback, long lData) {
	return SPI_NO_FUNCTION;
}

#include <windows.h>
#include <string>
#include "FreeImage.h"
#include "susie.h"

int WINAPI GetPluginInfo(int infono, LPSTR buf, int buflen) {
	int count = FreeImage_GetFIFCount(), i;
	std::string ext = "";
	for(i = 0; i < count-2; i++) {
		ext.append("*.").append(FreeImage_GetFIFExtensionList
							((FREE_IMAGE_FORMAT)i)).append(";");
	}
	ext.append("*.").append(FreeImage_GetFIFExtensionList
						((FREE_IMAGE_FORMAT)(count-1)));
	while(i = ext.find(",")) {
		if(i < 0) break;
		ext.replace(i, 1, ";*.", 3);
	}
	pluginfo[2] = ext.data();
	
	if(infono < 0 || infono >= (sizeof(pluginfo) / sizeof(char *)))
		return FALSE;
	lstrcpyn(buf, pluginfo[infono], buflen);
	return lstrlen(buf);
}

int WINAPI IsSupported(LPSTR filename, DWORD dw) {
	FREE_IMAGE_FORMAT fmt = FreeImage_GetFIFFromFilename(filename);
	if(fmt != FIF_UNKNOWN) return TRUE;

	FIMEMORY mem = FIMEMORY();
	mem.data = &dw;
	fmt = FreeImage_GetFileTypeFromMemory(&mem);
	FreeImage_CloseMemory(&mem);
	return fmt != FIF_UNKNOWN;
}

int DLL_API WINAPI GetPictureInfo(LPSTR buf, long len,
																	unsigned int flag, PictureInfo *lpInfo) {
	/**
	int ret = SPI_ALL_RIGHT;
	FIBITMAP* dib;
	FREE_IMAGE_FORMAT fmt;
	
	if((flag & 7) == 0) {
		fmt = FreeImage_GetFIFFromFilename(buf);
		dib = FreeImage_Load(fmt, buf);
	} else {
		FIMEMORY mem = FIMEMORY();
		mem.data = &buf;
		fmt = FreeImage_GetFileTypeFromMemory(&mem);
		dib = FreeImage_LoadFromMemory(fmt, &mem);
		FreeImage_CloseMemory(&mem);
	}

	lpInfo->left = 0;
	lpInfo->top = 0;
	lpInfo->width	= FreeImage_GetWidth(dib);
	lpInfo->height = FreeImage_GetHeight(dib);
	lpInfo->x_density	= floor(FreeImage_GetDotsPerMeterX(dib)*0.0254+0.5);
	lpInfo->y_density	= floor(FreeImage_GetDotsPerMeterY(dib)*0.0254+0.5);
	lpInfo->colorDepth	 = FreeImage_GetBPP(dib);
	lpInfo->hInfo	= NULL;
	FreeImage_Unload(dib);
	return ret;
	*/
	return SPI_ALL_RIGHT;
}

int WINAPI GetPicture(LPSTR buf, long len, unsigned int flag,
											HANDLE *pHBInfo, HANDLE *pHBm,
											SPI_PROGRESS lpPrgressCallback, long lData) {
	if(lpPrgressCallback != NULL)
		if(lpPrgressCallback(0, 1, lData))
			return SPI_ABORT;
	int ret = SPI_ALL_RIGHT;
	FIBITMAP* dib;
	FREE_IMAGE_FORMAT fmt;

	if((flag & 7) == 0) {
	/* buf is filename */
		fmt = FreeImage_GetFIFFromFilename(buf);
		dib = FreeImage_Load(fmt, buf);
	} else {
	/* buf is memory */
		FIMEMORY mem = FIMEMORY();
		mem.data = &buf;
		fmt = FreeImage_GetFileTypeFromMemory(&mem);
		dib = FreeImage_LoadFromMemory(fmt, &mem);
		FreeImage_CloseMemory(&mem);
	}

	unsigned int width = FreeImage_GetWidth(dib);
	unsigned int height = FreeImage_GetHeight(dib);
	unsigned int bpp_real = FreeImage_GetBPP(dib);
	unsigned int bpp = bpp_real>32 ? (bpp_real%48 ? 32 : 24) : bpp_real;
	unsigned int line_size = (((bpp>>3)*width)+3)&~3;
	unsigned int remain = line_size - (bpp>>3)*width;
	unsigned int bitmap_size = line_size * height;

	if(bpp <= 8) {
		*pHBInfo = LocalAlloc(LMEM_MOVEABLE, infosize + (sizeof(RGBQUAD) << bpp));
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
	if(bpp <= 8) {
		memcpy(pinfo->bmiColors, info->bmiColors, sizeof(RGBQUAD) << bpp);
	}

	switch(bpp_real) {
		case 24:
			RGBTRIPLE *rgb;
			for(int y = height-1; y; y--) {
				rgb = (RGBTRIPLE *) FreeImage_GetScanLine(dib, height-1-y);
				for(int x = 0; x < width; x++) {
					((BYTE *)bitmap)[0] = rgb[x].rgbtBlue;
					((BYTE *)bitmap)[1] = rgb[x].rgbtGreen;
					((BYTE *)bitmap)[2] = rgb[x].rgbtRed;
					bitmap = (BYTE *)bitmap + 3;
				}
				bitmap = (BYTE *)bitmap + remain;
			}
			break;
		case 32:
			RGBQUAD *rgba;
			for(int y = height-1; y; y--) {
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
		case 8:
			for(int y = height-1; y; y--) {
				BYTE *line = FreeImage_GetScanLine(dib, height-1-y);
				memcpy(bitmap, line, width);
				bitmap = (BYTE *)bitmap + line_size;
			}
			break;
		case 48:
			pinfo->bmiHeader.biBitCount = 24;
			FIRGB16 *rgb16;
			for(int y = height-1; y; y--) {
				rgb16 = (FIRGB16 *) FreeImage_GetScanLine(dib, height-1-y);
				for(int x = 0; x < width; x++) {
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
			for(int y = height-1; y; y--) {
				rgba16 = (FIRGBA16 *) FreeImage_GetScanLine(dib, height-1-y);
				for(int x = 0; x < width; x++) {
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
			for(int y = height-1; y; y--) {
				rgbf = (FIRGBF *) FreeImage_GetScanLine(dib, height-1-y);
				for(int x = 0; x < width; x++) {
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
			for(int y = height-1; y; y--) {
				rgbf = (FIRGBF *) FreeImage_GetScanLine(dib, height-1-y);
				for(int x = 0; x < width; x++) {
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


int WINAPI GetPreview(LPSTR buf, long len, unsigned int flag,
											HANDLE *pHBInfo, HANDLE *pHBm,
											SPI_PROGRESS lpPrgressCallback, long lData) {
	return SPI_NO_FUNCTION;
}

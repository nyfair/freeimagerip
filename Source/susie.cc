#include <windows.h>
#include <math.h>
#include "FreeImage.h"
#include "susie.h"

FREE_IMAGE_FORMAT fmt;
FIBITMAP* dib;

int WINAPI GetPluginInfo(int infono, LPSTR buf, int buflen) {
	if(infono < 0 || infono >= (sizeof(pluginfo) / sizeof(char *))) 
		return FALSE;
	lstrcpyn(buf, pluginfo[infono], buflen);
	return lstrlen(buf);
}

int WINAPI IsSupported(LPSTR filename, DWORD dw) {
	fmt = FreeImage_GetFIFFromFilename(filename);
	if(fmt != FIF_UNKNOWN) return TRUE;

	FIMEMORY mem = FIMEMORY();
	mem.data = &dw;
	fmt = FreeImage_GetFileTypeFromMemory(&mem);
	FreeImage_CloseMemory(&mem);
	return fmt != FIF_UNKNOWN;
}

int DLL_API WINAPI GetPictureInfo(LPSTR buf, long len, 
																	unsigned int flag, PictureInfo *lpInfo) {
	int ret = SPI_ALL_RIGHT;
	
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
}

int WINAPI GetPicture(LPSTR buf, long len, unsigned int flag,
											HANDLE *pHBInfo, HANDLE *pHBm,
											SPI_PROGRESS lpPrgressCallback, long lData) {
	if(lpPrgressCallback != NULL)
		if(lpPrgressCallback(0, 1, lData))
			return SPI_ABORT;
	int ret = SPI_ALL_RIGHT;

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

	int width = FreeImage_GetWidth(dib);
	int height = FreeImage_GetHeight(dib);
	int bpp = FreeImage_GetBPP(dib) >> 3;
	unsigned int bitmap_size = bpp * width * height;
	*pHBInfo = LocalAlloc(LMEM_MOVEABLE, infosize);
	*pHBm = LocalAlloc(LMEM_MOVEABLE, bitmap_size);
	BITMAPINFO *pinfo = (BITMAPINFO *)LocalLock(*pHBInfo);
	BYTE *bitmap = (BYTE *)LocalLock(*pHBm);

	if(*pHBInfo == NULL || *pHBm == NULL) {
		if(*pHBInfo != NULL) LocalFree(*pHBInfo);
		if(*pHBm != NULL) LocalFree(*pHBm);
		return SPI_NO_MEMORY;
	}

	pinfo->bmiHeader.biSize = infosize;
	pinfo->bmiHeader.biWidth	 = width;
	pinfo->bmiHeader.biHeight = height;
	pinfo->bmiHeader.biPlanes = 1;
	pinfo->bmiHeader.biBitCount = bpp << 3;
	pinfo->bmiHeader.biCompression	 = BI_RGB;
	pinfo->bmiHeader.biSizeImage	 = 0;
	pinfo->bmiHeader.biXPelsPerMeter = FreeImage_GetDotsPerMeterX(dib);
	pinfo->bmiHeader.biYPelsPerMeter	 = FreeImage_GetDotsPerMeterY(dib);
	pinfo->bmiHeader.biClrUsed	 = 0;
	pinfo->bmiHeader.biClrImportant = 0;

	switch(bpp) {
		case 3:
			RGBTRIPLE *rgb;
			for(int y = height-1; y; y--) {
				rgb = (RGBTRIPLE *) FreeImage_GetScanLine(dib, height-1-y);
				for(int x = 0; x < width; x++) {
					((BYTE *)bitmap)[0] = rgb[x].rgbtBlue;
					((BYTE *)bitmap)[1] = rgb[x].rgbtGreen;
					((BYTE *)bitmap)[2] = rgb[x].rgbtRed;
					bitmap = (BYTE *)bitmap + 3;
				}
			}
			break;
		case 4:
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
		case 1:
			for(int y = height-1; y; y--) {
				BYTE *line = FreeImage_GetScanLine(dib, height-1-y);
				memcpy(bitmap, line, width);
				bitmap = (BYTE *)bitmap + width;
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

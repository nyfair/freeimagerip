#include <stdio.h>
#include "FreeImage.h"

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage : convert <input file name> <output file name>\n");
		return 0;
	}
	FREE_IMAGE_FORMAT fifin = FreeImage_GetFileType(argv[1]);
	if(fifin == FIF_UNKNOWN)
		fifin = FreeImage_GetFIFFromFilename(argv[1]);
	FREE_IMAGE_FORMAT fifout = FreeImage_GetFIFFromFilename(argv[2]);
	FIBITMAP *dib = FreeImage_Load(fifin, argv[1]);
	FreeImage_Save(fifout, dib, argv[2]);
	FreeImage_Unload(dib);
	return 0;
}

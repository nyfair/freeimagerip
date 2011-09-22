#include <stdlib.h>
#include "FreeImage.h"

int main(int argc, char *argv[]) {
	FIBITMAP *src = FreeImage_Load(FreeImage_GetFIFFromFilename(argv[1]), argv[1], 0);
	FreeImage_Save(FreeImage_GetFIFFromFilename(argv[2]), src, argv[2], atoi(argv[3]));
	FreeImage_Unload(src);
	return 0;
}

// ==========================================================
// TIFF Loader and Writer
//
// Design and implementation by 
// - Floris van den Berg (flvdberg@wxs.nl)
// - Hervé Drolon (drolon@infonie.fr)
// - Markus Loibl (markus.loibl@epost.de)
// - Luca Piergentili (l.pierge@terra.es)
// - Detlev Vendt (detlev.vendt@brillit.de)
// - Mihail Naydenov (mnaydenov@users.sourceforge.net)
//
// This file is part of FreeImage 3
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

#ifdef _MSC_VER 
#pragma warning (disable : 4786) // identifier was truncated to 'number' characters
#endif

#ifdef unix
#undef unix
#endif
#ifdef __unix
#undef __unix
#endif

#include "FreeImage.h"
#include "Utilities.h"
#include "tiffiop.h"

#include "FreeImageIO.h"
#include "PSDParser.h"

// --------------------------------------------------------------------------
//   LogLuv conversion functions interface (see TIFFLogLuv.cpp)
// --------------------------------------------------------------------------
void tiff_ConvertLineXYZToRGB(BYTE *target, BYTE *source, double stonits, int width_in_pixels);
void tiff_ConvertLineRGBToXYZ(BYTE *target, BYTE *source, int width_in_pixels);

// ----------------------------------------------------------

/** Supported loading methods */
typedef enum {
	LoadAsRBGA			= 0, 
	LoadAsCMYK			= 1, 
	LoadAs8BitTrns		= 2, 
	LoadAsGenericStrip	= 3, 
	LoadAsTiled			= 4,
	LoadAsLogLuv		= 5
} TIFFLoadMethod;

// ----------------------------------------------------------
//   local prototypes
// ----------------------------------------------------------

static tmsize_t _tiffReadProc(thandle_t handle, void* buf, tmsize_t size);
static tmsize_t _tiffWriteProc(thandle_t handle, void* buf, tmsize_t size);
static toff_t _tiffSeekProc(thandle_t handle, toff_t off, int whence);
static int _tiffCloseProc(thandle_t fd);
static int _tiffMapProc(thandle_t fd, void** pbase, toff_t* psize);
static void _tiffUnmapProc(thandle_t fd, void* base, toff_t size);

static uint16_t CheckColormap(int n, uint16_t* r, uint16_t* g, uint16_t* b);
static uint16_t GetPhotometric(FIBITMAP *dib);

static void ReadResolution(TIFF *tiff, FIBITMAP *dib);
static void WriteResolution(TIFF *tiff, FIBITMAP *dib);

static void ReadPalette(TIFF *tiff, uint16_t photometric, uint16_t bitspersample, FIBITMAP *dib);

static FIBITMAP* CreateImageType(BOOL header_only, FREE_IMAGE_TYPE fit, int width, int height, uint16_t bitspersample, uint16_t samplesperpixel);
static FREE_IMAGE_TYPE ReadImageType(TIFF *tiff, uint16_t bitspersample, uint16_t samplesperpixel);
static void WriteImageType(TIFF *tiff, FREE_IMAGE_TYPE fit);

static void WriteCompression(TIFF *tiff, uint16_t bitspersample, uint16_t samplesperpixel, uint16_t photometric, int flags);

static TIFFLoadMethod FindLoadMethod(TIFF *tif, uint16_t photometric, uint16_t bitspersample, uint16_t samplesperpixel, FREE_IMAGE_TYPE image_type, int flags);

// ==========================================================
// Plugin Interface
// ==========================================================

static int s_format_id;

typedef struct {
	//! FreeImage IO functions
    FreeImageIO *io;
	//! FreeImage handle
	fi_handle handle;
	//! LibTIFF handle
	TIFF *tif;
	//! Count the number of thumbnails already read (used to avoid recursion on loading)
	unsigned thumbnailCount;
} fi_TIFFIO;

// ----------------------------------------------------------
//   libtiff interface 
// ----------------------------------------------------------

static tmsize_t 
_tiffReadProc(thandle_t handle, void *buf, tmsize_t size) {
	fi_TIFFIO *fio = (fi_TIFFIO*)handle;
	return fio->io->read_proc(buf, (unsigned)size, 1, fio->handle) * size;
}

static tmsize_t
_tiffWriteProc(thandle_t handle, void *buf, tmsize_t size) {
	fi_TIFFIO *fio = (fi_TIFFIO*)handle;
	return fio->io->write_proc(buf, (unsigned)size, 1, fio->handle) * size;
}

static toff_t
_tiffSeekProc(thandle_t handle, toff_t off, int whence) {
	fi_TIFFIO *fio = (fi_TIFFIO*)handle;
	fio->io->seek_proc(fio->handle, (long)off, whence);
	return fio->io->tell_proc(fio->handle);
}

static int
_tiffCloseProc(thandle_t fd) {
	return 0;
}

#include <sys/stat.h>

static toff_t
_tiffSizeProc(thandle_t handle) {
    fi_TIFFIO *fio = (fi_TIFFIO*)handle;
    long currPos = fio->io->tell_proc(fio->handle);
    fio->io->seek_proc(fio->handle, 0, SEEK_END);
    long fileSize = fio->io->tell_proc(fio->handle);
    fio->io->seek_proc(fio->handle, currPos, SEEK_SET);
    return fileSize;
}

static int
_tiffMapProc(thandle_t, void** base, toff_t* size) {
	return 0;
}

static void
_tiffUnmapProc(thandle_t, void* base, toff_t size) {
}

/**
Open a TIFF file descriptor for reading or writing
@param handle File handle
@param name Name of the file handle
@param mode Specifies if the file is to be opened for reading ("r") or writing ("w")
*/
TIFF *
TIFFFdOpen(thandle_t handle, const char *name, const char *mode) {
	// Open the file; the callback will set everything up
	TIFF *tif = TIFFClientOpen(name, mode, handle,
	    _tiffReadProc, _tiffWriteProc, _tiffSeekProc, _tiffCloseProc,
	    _tiffSizeProc, _tiffMapProc, _tiffUnmapProc);

	return tif;
}

/**
Open a TIFF file for reading or writing
@param name
@param mode
*/
TIFF*
TIFFOpen(const char* name, const char* mode) {
	return 0;
}

// ----------------------------------------------------------
//   TIFF library FreeImage-specific routines.
// ----------------------------------------------------------

void*
_TIFFmalloc(tmsize_t s) {
	return malloc(s);
}

void* 
_TIFFcalloc(tmsize_t nmemb, tmsize_t siz) {
	if (nmemb == 0 || siz == 0) {
		return ((void *)NULL);
	}
	return calloc((size_t)nmemb, (size_t)siz);
}

void
_TIFFfree(void *p) {
	free(p);
}

void*
_TIFFrealloc(void* p, tmsize_t s) {
	return realloc(p, s);
}

void
_TIFFmemset(void* p, int v, tmsize_t c) {
	memset(p, v, (size_t) c);
}

void
_TIFFmemcpy(void* d, const void* s, tmsize_t c) {
	memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const void* p1, const void* p2, tmsize_t c) {
	return (memcmp(p1, p2, (size_t) c));
}

// ----------------------------------------------------------
//   in FreeImage warnings and errors are disabled
// ----------------------------------------------------------

static void
msdosWarningHandler(const char* module, const char* fmt, va_list ap) {
}

TIFFErrorHandler _TIFFwarningHandler = msdosWarningHandler;

static void
msdosErrorHandler(const char* module, const char* fmt, va_list ap) {
	
	// use this for diagnostic only (do not use otherwise, even in DEBUG mode)
	/*
	if (module != NULL) {
		char msg[1024];
		vsprintf(msg, fmt, ap);
		FreeImage_OutputMessageProc(s_format_id, "%s: %s", module, msg);
	}
	*/
}

TIFFErrorHandler _TIFFerrorHandler = msdosErrorHandler;

// ----------------------------------------------------------

#define CVT(x)      (((x) * 255L) / ((1L<<16)-1))
#define	SCALE(x)	(((x)*((1L<<16)-1))/255)

// ==========================================================
// Internal functions
// ==========================================================

static uint16_t
CheckColormap(int n, uint16_t* r, uint16_t* g, uint16_t* b) {
    while (n-- > 0) {
        if (*r++ >= 256 || *g++ >= 256 || *b++ >= 256) {
			return 16;
		}
	}

    return 8;
}

/**
Get the TIFFTAG_PHOTOMETRIC value from the dib
*/
static uint16_t
GetPhotometric(FIBITMAP *dib) {
	FREE_IMAGE_COLOR_TYPE color_type = FreeImage_GetColorType(dib);
	switch(color_type) {
		case FIC_MINISWHITE:	// min value is white
			return PHOTOMETRIC_MINISWHITE;
		case FIC_MINISBLACK:	// min value is black
			return PHOTOMETRIC_MINISBLACK;
		case FIC_PALETTE:		// color map indexed
			return PHOTOMETRIC_PALETTE;
		case FIC_RGB:			// RGB color model
		case FIC_RGBALPHA:		// RGB color model with alpha channel
			return PHOTOMETRIC_RGB;
		default:
			return PHOTOMETRIC_MINISBLACK;
	}
}

/**
Get the resolution from the TIFF and fill the dib with universal units
*/
static void 
ReadResolution(TIFF *tiff, FIBITMAP *dib) {
	float fResX = 300.0;
	float fResY = 300.0;
	uint16_t resUnit = RESUNIT_INCH;

	TIFFGetField(tiff, TIFFTAG_RESOLUTIONUNIT, &resUnit);
	TIFFGetField(tiff, TIFFTAG_XRESOLUTION, &fResX);
	TIFFGetField(tiff, TIFFTAG_YRESOLUTION, &fResY);
	
	// If we don't have a valid resolution unit and valid resolution is specified then assume inch
	if (resUnit == RESUNIT_NONE && fResX > 0.0 && fResY > 0.0) {
		resUnit = RESUNIT_INCH;
	}
	if (resUnit == RESUNIT_INCH) {
		FreeImage_SetDotsPerMeterX(dib, (unsigned) (fResX/0.0254000 + 0.5));
		FreeImage_SetDotsPerMeterY(dib, (unsigned) (fResY/0.0254000 + 0.5));
	} else if(resUnit == RESUNIT_CENTIMETER) {
		FreeImage_SetDotsPerMeterX(dib, (unsigned) (fResX*100.0 + 0.5));
		FreeImage_SetDotsPerMeterY(dib, (unsigned) (fResY*100.0 + 0.5));
	}
}

/**
Set the resolution to the TIFF using english units
*/
static void 
WriteResolution(TIFF *tiff, FIBITMAP *dib) {
	double res;

	TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

	res = (unsigned long) (0.5 + 0.0254 * FreeImage_GetDotsPerMeterX(dib));
	TIFFSetField(tiff, TIFFTAG_XRESOLUTION, res);

	res = (unsigned long) (0.5 + 0.0254 * FreeImage_GetDotsPerMeterY(dib));
	TIFFSetField(tiff, TIFFTAG_YRESOLUTION, res);
}

/**
Fill the dib palette according to the TIFF photometric
*/
static void 
ReadPalette(TIFF *tiff, uint16_t photometric, uint16_t bitspersample, FIBITMAP *dib) {
	RGBQUAD *pal = FreeImage_GetPalette(dib);

	switch(photometric) {
		case PHOTOMETRIC_MINISBLACK:	// bitmap and greyscale image types
		case PHOTOMETRIC_MINISWHITE:
			// Monochrome image

			if (bitspersample == 1) {
				if (photometric == PHOTOMETRIC_MINISWHITE) {
					pal[0].rgbRed = pal[0].rgbGreen = pal[0].rgbBlue = 255;
					pal[1].rgbRed = pal[1].rgbGreen = pal[1].rgbBlue = 0;
				} else {
					pal[0].rgbRed = pal[0].rgbGreen = pal[0].rgbBlue = 0;
					pal[1].rgbRed = pal[1].rgbGreen = pal[1].rgbBlue = 255;
				}

			} else if ((bitspersample == 4) ||(bitspersample == 8)) {
				// need to build the scale for greyscale images
				int ncolors = FreeImage_GetColorsUsed(dib);

				if (photometric == PHOTOMETRIC_MINISBLACK) {
					for (int i = 0; i < ncolors; i++) {
						pal[i].rgbRed	=
						pal[i].rgbGreen =
						pal[i].rgbBlue	= (BYTE)(i*(255/(ncolors-1)));
					}
				} else {
					for (int i = 0; i < ncolors; i++) {
						pal[i].rgbRed	=
						pal[i].rgbGreen =
						pal[i].rgbBlue	= (BYTE)(255-i*(255/(ncolors-1)));
					}
				}
			}

			break;

		case PHOTOMETRIC_PALETTE:	// color map indexed
			uint16_t *red;
			uint16_t *green;
			uint16_t *blue;
			
			TIFFGetField(tiff, TIFFTAG_COLORMAP, &red, &green, &blue); 

			// load the palette in the DIB

			if (CheckColormap(1<<bitspersample, red, green, blue) == 16) {
				for (int i = (1 << bitspersample) - 1; i >= 0; i--) {
					pal[i].rgbRed =(BYTE) CVT(red[i]);
					pal[i].rgbGreen = (BYTE) CVT(green[i]);
					pal[i].rgbBlue = (BYTE) CVT(blue[i]);           
				}
			} else {
				for (int i = (1 << bitspersample) - 1; i >= 0; i--) {
					pal[i].rgbRed = (BYTE) red[i];
					pal[i].rgbGreen = (BYTE) green[i];
					pal[i].rgbBlue = (BYTE) blue[i];        
				}
			}

			break;
	}
}

/** 
Allocate a FIBITMAP
@param header_only If TRUE, allocate a 'header only' FIBITMAP, otherwise allocate a full FIBITMAP
@param fit Image type
@param width Image width in pixels
@param height Image height in pixels
@param bitspersample # bits per sample
@param samplesperpixel # samples per pixel
@return Returns the allocated image if successful, returns NULL otherwise
*/
static FIBITMAP* 
CreateImageType(BOOL header_only, FREE_IMAGE_TYPE fit, int width, int height, uint16_t bitspersample, uint16_t samplesperpixel) {
	FIBITMAP *dib = NULL;

	if((width < 0) || (height < 0)) {
		// check for malicious images
		return NULL;
	}

	int bpp = bitspersample * samplesperpixel;

	if(fit == FIT_BITMAP) {
		// standard bitmap type 

		if(bpp == 16) {
			
			if((samplesperpixel == 2) && (bitspersample == 8)) {
				// 8-bit indexed + 8-bit alpha channel -> convert to 8-bit transparent
				dib = FreeImage_AllocateHeader(header_only, width, height, 8);
			} else {
				// 16-bit RGB -> expect it to be 565
				dib = FreeImage_AllocateHeader(header_only, width, height, bpp, FI16_565_RED_MASK, FI16_565_GREEN_MASK, FI16_565_BLUE_MASK);
			}
			
		}
		else if (bpp <= 32) {
			dib = FreeImage_AllocateHeader(header_only, width, height, bpp, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
		}

	} else {
		// other bitmap types
		
		dib = FreeImage_AllocateHeaderT(header_only, fit, width, height, bpp);
	}

	return dib;
}

/** 
Read the TIFFTAG_SAMPLEFORMAT tag and convert to FREE_IMAGE_TYPE
@param tiff LibTIFF TIFF Handle
@param bitspersample # bit per sample
@param samplesperpixel # samples per pixel
@return Returns the image type as a FREE_IMAGE_TYPE value
*/
static FREE_IMAGE_TYPE 
ReadImageType(TIFF *tiff, uint16_t bitspersample, uint16_t samplesperpixel) {
	uint16_t sampleformat = 0;
	FREE_IMAGE_TYPE fit = FIT_BITMAP ; 

	uint16_t bpp = bitspersample * samplesperpixel;

	// try the sampleformat tag
    if(TIFFGetField(tiff, TIFFTAG_SAMPLEFORMAT, &sampleformat)) {

        switch (sampleformat) {
			case SAMPLEFORMAT_UINT:
				switch (bpp) {
					case 1:
					case 4:
					case 8:
					case 24:
						fit = FIT_BITMAP;
						break;
					case 16:
						// 8-bit + alpha or 16-bit greyscale
						if(samplesperpixel == 2) {
							fit = FIT_BITMAP;
						} else {
							fit = FIT_UINT16;
						}
						break;
					case 32:
						if(samplesperpixel == 4) {
							fit = FIT_BITMAP;
						} else {
							fit = FIT_UINT32;
						}
						break;
					case 48:
						if(samplesperpixel == 3) {
							fit = FIT_RGB16;
						}
						break;
					case 64:
						if(samplesperpixel == 4) {
							fit = FIT_RGBA16;
						}
						break;
				}
				break;

			case SAMPLEFORMAT_INT:
				switch (bpp) {
					case 16:
						if(samplesperpixel == 3) {
							fit = FIT_BITMAP;
						} else {
							fit = FIT_INT16;
						}
						break;
					case 32:
						fit = FIT_INT32;
						break;
				}
				break;

			case SAMPLEFORMAT_IEEEFP:
				switch (bpp) {
					case 32:
						fit = FIT_FLOAT;
						break;
					case 48:
						// 3 x half float => convert to RGBF
						if((samplesperpixel == 3) && (bitspersample == 16)) {
							fit = FIT_RGBF;
						}
						break;
					case 64:
						if(samplesperpixel == 2) {
							fit = FIT_FLOAT;
						} else {
							fit = FIT_DOUBLE;
						}
						break;
					case 96:
						fit = FIT_RGBF;
						break;
					default:
						if(bpp >= 128) {
							fit = FIT_RGBAF;
						}
					break;
				}
				break;
			case SAMPLEFORMAT_COMPLEXIEEEFP:
				switch (bpp) {
					case 64:
						break;
					case 128:
						fit = FIT_COMPLEX;
						break;
				}
				break;

			}
    }
	// no sampleformat tag : assume SAMPLEFORMAT_UINT
	else {
		if(samplesperpixel == 1) {
			switch (bpp) {
				case 16:
					fit = FIT_UINT16;
					break;
					
				case 32:
					fit = FIT_UINT32;
					break;
			}
		}
		else if(samplesperpixel == 3) {
			if(bpp == 48) fit = FIT_RGB16;
		}
		else if(samplesperpixel >= 4) { 
			if(bitspersample == 16) {
				fit = FIT_RGBA16;
			}
		}

	}

    return fit;
}

/** 
Convert FREE_IMAGE_TYPE and write TIFFTAG_SAMPLEFORMAT
@param tiff LibTIFF TIFF Handle
@param fit Image type as a FREE_IMAGE_TYPE value
*/
static void 
WriteImageType(TIFF *tiff, FREE_IMAGE_TYPE fit) {
	switch(fit) {
		case FIT_BITMAP:	// standard image: 1-, 4-, 8-, 16-, 24-, 32-bit
		case FIT_UINT16:	// array of unsigned short	: unsigned 16-bit
		case FIT_UINT32:	// array of unsigned long	: unsigned 32-bit
		case FIT_RGB16:		// 48-bit RGB image			: 3 x 16-bit
		case FIT_RGBA16:	// 64-bit RGBA image		: 4 x 16-bit
			TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
			break;

		case FIT_INT16:		// array of short	: signed 16-bit
		case FIT_INT32:		// array of long	: signed 32-bit
			TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_INT);
			break;

		case FIT_FLOAT:		// array of float	: 32-bit
		case FIT_DOUBLE:	// array of double	: 64-bit
		case FIT_RGBF:		// 96-bit RGB float image	: 3 x 32-bit IEEE floating point
		case FIT_RGBAF:		// 128-bit RGBA float image	: 4 x 32-bit IEEE floating point
			TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
			break;

		case FIT_COMPLEX:	// array of COMPLEX : 2 x 64-bit
			TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_COMPLEXIEEEFP);
			break;
	}
}

/**
Select the compression algorithm
@param tiff LibTIFF TIFF Handle
@param bitspersample
@param samplesperpixel
@param photometric
@param flags
*/
static void 
WriteCompression(TIFF *tiff, uint16_t bitspersample, uint16_t samplesperpixel, uint16_t photometric, int flags) {
	uint16_t compression = COMPRESSION_LZW;
	uint16_t bitsperpixel = bitspersample * samplesperpixel;

	if (photometric == PHOTOMETRIC_LOGLUV) {
		compression = COMPRESSION_SGILOG;
	}
	else if ((flags & TIFF_PACKBITS) == TIFF_PACKBITS) {
		compression = COMPRESSION_PACKBITS;
	}
	else if ((flags & TIFF_DEFLATE) == TIFF_DEFLATE) {
		// compression = COMPRESSION_DEFLATE is obsolete, writers should always use COMPRESSION_ADOBE_DEFLATE value
		compression = COMPRESSION_ADOBE_DEFLATE;
	}
	else if ((flags & TIFF_ADOBE_DEFLATE) == TIFF_ADOBE_DEFLATE) {
		compression = COMPRESSION_ADOBE_DEFLATE;
	}
	else if ((flags & TIFF_NONE) == TIFF_NONE) {
		compression = COMPRESSION_NONE;
	}
	else if ((bitsperpixel == 1) && ((flags & TIFF_CCITTFAX3) == TIFF_CCITTFAX3)) {
		compression = COMPRESSION_CCITTFAX3;
	}
	else if ((bitsperpixel == 1) && ((flags & TIFF_CCITTFAX4) == TIFF_CCITTFAX4)) {
		compression = COMPRESSION_CCITTFAX4;
	}
	else if ((flags & TIFF_LZW) == TIFF_LZW) {
		compression = COMPRESSION_LZW;
	}
	else if ((flags & TIFF_JPEG) == TIFF_JPEG) {
		if (((bitsperpixel == 8) && (photometric != PHOTOMETRIC_PALETTE)) || (bitsperpixel == 24)) {
			compression = COMPRESSION_JPEG;
			// RowsPerStrip must be multiple of 8 for JPEG
			uint32_t rowsperstrip = (uint32_t)-1;
			rowsperstrip = TIFFDefaultStripSize(tiff, rowsperstrip);
			rowsperstrip = rowsperstrip + (8 - (rowsperstrip % 8));
			// overwrite previous RowsPerStrip
			TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
		}
		else {
			// default to LZW
			compression = COMPRESSION_LZW;
		}
	}
	else {
		// default compression scheme

		switch (bitsperpixel) {
		case 1:
			compression = COMPRESSION_CCITTFAX4;
			break;

		case 4:
		case 8:
		case 16:
		case 24:
		case 32:
			compression = COMPRESSION_LZW;
			break;
		case 48:
		case 64:
		case 96:
		case 128:
			compression = COMPRESSION_LZW;
			break;

		default:
			compression = COMPRESSION_NONE;
			break;
		}
	}

	TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression);

	if ((compression == COMPRESSION_LZW) || (compression == COMPRESSION_ADOBE_DEFLATE)) {
		// This option is only meaningful with LZW or ADOBE DEFLATE compression: a predictor value of 2 
		// causes each scanline of the output image to undergo horizontal differencing 
		// before it is encoded; a value of 1 forces each scanline to be encoded without differencing.

		// Found on LibTIFF mailing list : 
		// LZW without differencing works well for 1-bit images, 4-bit grayscale images, 
		// and many palette-color images. But natural 24-bit color images and some 8-bit 
		// grayscale images do much better with differencing.

		// set the default predictor value
		TIFFSetField(tiff, TIFFTAG_PREDICTOR, PREDICTOR_NONE);

		if ((bitspersample == 8) || (bitspersample == 16)) {
			if ((bitsperpixel >= 8) && (photometric != PHOTOMETRIC_PALETTE)) {
				TIFFSetField(tiff, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
			}
		}
		else if ((photometric == PHOTOMETRIC_RGB) && (bitspersample == 32) && ((samplesperpixel == 3) || (samplesperpixel == 4))) {
			// this is a RGB or RGBA float image, set a floating point predictor
			TIFFSetField(tiff, TIFFTAG_PREDICTOR, PREDICTOR_FLOATINGPOINT);
		}
	}
	else if ((compression == COMPRESSION_CCITTFAX3) || (compression == COMPRESSION_CCITTFAX4)) {
		uint32_t imageLength = 0;
		TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &imageLength);
		// overwrite previous RowsPerStrip
		TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, imageLength);

		if (compression == COMPRESSION_CCITTFAX3) {
			// try to be compliant with the TIFF Class F specification
			// that documents the TIFF tags specific to FAX applications
			// see http://palimpsest.stanford.edu/bytopic/imaging/std/tiff-f.html
			uint32_t group3options = GROUP3OPT_2DENCODING | GROUP3OPT_FILLBITS;
			TIFFSetField(tiff, TIFFTAG_GROUP3OPTIONS, group3options);	// 2d-encoded, has aligned EOL
			TIFFSetField(tiff, TIFFTAG_FILLORDER, FILLORDER_LSB2MSB);	// lsb-to-msb fillorder
		}
	}
}

// ==========================================================
// Plugin Implementation
// ==========================================================

static const char * DLL_CALLCONV
Format() {
	return "TIFF";
}

static const char * DLL_CALLCONV
Description() {
	return "Tagged Image File Format";
}

static const char * DLL_CALLCONV
Extension() {
	return "tif,tiff";
}

static const char * DLL_CALLCONV
RegExpr() {
	return "^[MI][MI][\\x01*][\\x01*]";
}

static const char * DLL_CALLCONV
MimeType() {
	return "image/tiff";
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO *io, fi_handle handle) {	
	// TIFF signatures
	static const BYTE tiff_C_II[] = { 0x49, 0x49, 0x2A, 0x00 };	// Classic TIFF, little-endian
	static const BYTE tiff_C_MM[] = { 0x4D, 0x4D, 0x00, 0x2A };	// Classic TIFF, big-endian
	static const BYTE tiff_B_II[] = { 0x49, 0x49, 0x2B, 0x00 };	// Big TIFF, little-endian
	static const BYTE tiff_B_MM[] = { 0x4D, 0x4D, 0x00, 0x2B };	// Big TIFF, big-endian

	// many camera raw files use a TIFF signature ...
	// ... try to exclude any TIFF if it is a raw file

	// Canon (CR2), little-endian byte order signature
	static const BYTE CR2_II[] = { 0x49, 0x49, 0x2A, 0x00, 0x10, 0x00, 0x00, 0x00, 0x43, 0x52, 0x02, 0x00 };

	BYTE signature[16] = { 0 };

	if (io->read_proc(signature, sizeof(BYTE), 16, handle) != 16) {
		return FALSE;
	}

	if (memcmp(tiff_C_II, signature, 4) == 0) {
		if (memcmp(CR2_II, signature, 12) == 0) {
			return FALSE;
		}
		return TRUE;
	}
	if (memcmp(tiff_C_MM, signature, 4) == 0) {
		return TRUE;
	}
	if (memcmp(tiff_B_II, signature, 4) == 0) {
		return TRUE;
	}
	if (memcmp(tiff_B_MM, signature, 4) == 0) {
		return TRUE;
	}

	return FALSE;
}

static BOOL DLL_CALLCONV
SupportsExportDepth(int depth) {
	return (
			(depth == 1)  ||
			(depth == 4)  ||
			(depth == 8)  ||
			(depth == 24) ||
			(depth == 32)
		);
}

static BOOL DLL_CALLCONV 
SupportsExportType(FREE_IMAGE_TYPE type) {
	return (
		(type == FIT_BITMAP)  ||
		(type == FIT_UINT16)  ||
		(type == FIT_INT16)   ||
		(type == FIT_UINT32)  ||
		(type == FIT_INT32)   ||
		(type == FIT_FLOAT)   ||
		(type == FIT_DOUBLE)  ||
		(type == FIT_COMPLEX) || 
		(type == FIT_RGB16)   || 
		(type == FIT_RGBA16)  || 
		(type == FIT_RGBF)    ||
		(type == FIT_RGBAF)
	);
}

static BOOL DLL_CALLCONV
SupportsNoPixels() {
	return TRUE;
} 

// ----------------------------------------------------------

static void * DLL_CALLCONV
Open(FreeImageIO *io, fi_handle handle, BOOL read) {
	// wrapper for TIFF I/O
	fi_TIFFIO *fio = (fi_TIFFIO*)malloc(sizeof(fi_TIFFIO));
	if (!fio) {
		return NULL;
	}
	fio->io = io;
	fio->handle = handle;

	if (read) {
		fio->tif = TIFFFdOpen((thandle_t)fio, "", "r");
	} else {
		// mode = "w"	: write Classic TIFF
		// mode = "w8"	: write Big TIFF
		fio->tif = TIFFFdOpen((thandle_t)fio, "", "w");
	}
	if(fio->tif == NULL) {
		free(fio);
		FreeImage_OutputMessageProc(s_format_id, "Error while opening TIFF: data is invalid");
		return NULL;
	}
	return fio;
}

static void DLL_CALLCONV
Close(FreeImageIO *io, fi_handle handle, void *data) {
	if(data) {
		fi_TIFFIO *fio = (fi_TIFFIO*)data;
		TIFFClose(fio->tif);
		free(fio);
	}
}

// ----------------------------------------------------------

/**
check for uncommon bitspersample values (e.g. 10, 12, ...)
@param photometric TIFFTAG_PHOTOMETRIC tiff tag
@param bitspersample TIFFTAG_BITSPERSAMPLE tiff tag
@param samplesperpixel TIFFTAG_SAMPLESPERPIXEL tiff tag
@return Returns FALSE if a uncommon bit-depth is encountered, returns TRUE otherwise
*/
static BOOL 
IsValidBitsPerSample(uint16_t photometric, uint16_t bitspersample, uint16_t samplesperpixel) {
	// get the pixel depth in bits
	const uint16_t pixel_depth = bitspersample * samplesperpixel;

	// check for a supported pixel depth
	switch (pixel_depth) {
		case 1:
		case 4:
		case 8:
		case 16:
		case 24:
		case 32:
		case 48:
		case 64:
		case 96:
		case 128:
			// OK, go on
			break;
		default:
			// unsupported pixel depth
			return FALSE;
	}

	switch(bitspersample) {
		case 1:
		case 4:
			if((photometric == PHOTOMETRIC_MINISWHITE) || (photometric == PHOTOMETRIC_MINISBLACK) || (photometric == PHOTOMETRIC_PALETTE)) { 
				return TRUE;
			} else {
				return FALSE;
			}
			break;
		case 8:
			return TRUE;
		case 16:
			if(photometric != PHOTOMETRIC_PALETTE) { 
				return TRUE;
			} else {
				return FALSE;
			}
			break;
		case 32:
			if((photometric == PHOTOMETRIC_MINISWHITE) || (photometric == PHOTOMETRIC_MINISBLACK) || (photometric == PHOTOMETRIC_LOGLUV)) { 
				return TRUE;
			} else if((photometric == PHOTOMETRIC_RGB) && (samplesperpixel == 3) || (samplesperpixel == 4)) {
				// RGB[A]F
				return TRUE;
			} else {
				return FALSE;
			}
			break;
		case 64:
		case 128:
			if(photometric == PHOTOMETRIC_MINISBLACK) { 
				return TRUE;
			} else {
				return FALSE;
			}
			break;
		default:
			return FALSE;
	}
	
	return FALSE;
}

static TIFFLoadMethod  
FindLoadMethod(TIFF *tif, FREE_IMAGE_TYPE image_type, int flags) {
	uint16_t bitspersample	= (uint16_t)-1;
	uint16_t samplesperpixel	= (uint16_t)-1;
	uint16_t photometric		= (uint16_t)-1;
	uint16_t planar_config	= (uint16_t)-1;

	TIFFLoadMethod loadMethod = LoadAsGenericStrip;

	TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
	TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
	TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
	TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planar_config);

	BOOL bIsTiled = (TIFFIsTiled(tif) == 0) ? FALSE:TRUE;

	switch(photometric) {
		// convert to 24 or 32 bits RGB if the image is full color
		case PHOTOMETRIC_RGB:
			if((image_type == FIT_RGB16) || (image_type == FIT_RGBA16)) {
				// load 48-bit RGB and 64-bit RGBA without conversion 
				loadMethod = LoadAsGenericStrip;
			} 
			break;
		case PHOTOMETRIC_YCBCR:
		case PHOTOMETRIC_CIELAB:
		case PHOTOMETRIC_ICCLAB:
		case PHOTOMETRIC_ITULAB:
			loadMethod = LoadAsRBGA;
			break;
		case PHOTOMETRIC_LOGLUV:
			loadMethod = LoadAsLogLuv;
			break;
		case PHOTOMETRIC_SEPARATED:
			// if image is PHOTOMETRIC_SEPARATED _and_ comes with an ICC profile, 
			// then the image should preserve its original (CMYK) colour model and 
			// should be read as CMYK (to keep the match of pixel and profile and 
			// to avoid multiple conversions. Conversion can be done by changing 
			// the profile from it's original CMYK to an RGB profile with an 
			// apropriate color management system. Works with non-tiled TIFFs.
			if(!bIsTiled) {
				loadMethod = LoadAsCMYK;
			}
			break;
		case PHOTOMETRIC_MINISWHITE:
		case PHOTOMETRIC_MINISBLACK:
		case PHOTOMETRIC_PALETTE:
			// When samplesperpixel = 2 and bitspersample = 8, set the image as a
			// 8-bit indexed image + 8-bit alpha layer image
			// and convert to a 8-bit image with a transparency table
			if((samplesperpixel > 1) && (bitspersample == 8)) {
				loadMethod = LoadAs8BitTrns;
			} else {
				loadMethod = LoadAsGenericStrip;
			}
			break;
		default:
			loadMethod = LoadAsGenericStrip;
			break;
	}

	if((loadMethod == LoadAsGenericStrip) && bIsTiled) {
		loadMethod = LoadAsTiled;
	}

	return loadMethod;
}

// --------------------------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImageIO *io, fi_handle handle, int flags, void *data) {
	if (!handle || !data ) {
		return NULL;
	}
	
	TIFF   *tif = NULL;
	uint32_t height = 0; 
	uint32_t width = 0; 
	uint16_t bitspersample = 1;
	uint16_t samplesperpixel = 1;
	uint32_t rowsperstrip = (uint32_t)-1;  
	uint16_t photometric = PHOTOMETRIC_MINISWHITE;
	uint16_t compression = (uint16_t)-1;
	uint16_t planar_config;

	FIBITMAP *dib = NULL;
	uint32_t iccSize = 0;		// ICC profile length
	void *iccBuf = NULL;	// ICC profile data		

	const BOOL header_only = (flags & FIF_LOAD_NOPIXELS) == FIF_LOAD_NOPIXELS;
	
	try {	
		fi_TIFFIO *fio = (fi_TIFFIO*)data;
		tif = fio->tif;

		// first, get the photometric, the compression and basic metadata
		// ---------------------------------------------------------------------------------

		TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
		TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression);

		// check for HDR formats
		// ---------------------------------------------------------------------------------

		if(photometric == PHOTOMETRIC_LOGLUV) {
			// check the compression
			if(compression != COMPRESSION_SGILOG && compression != COMPRESSION_SGILOG24) {
				throw "Only support SGILOG compressed LogLuv data";
			}
			// set decoder to output in IEEE 32-bit float XYZ values
			TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT);
		}

		// ---------------------------------------------------------------------------------

		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
		TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
		TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
		TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);   			
		TIFFGetField(tif, TIFFTAG_ICCPROFILE, &iccSize, &iccBuf);
		TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planar_config);

		// check for unsupported formats
		// ---------------------------------------------------------------------------------

		if(IsValidBitsPerSample(photometric, bitspersample, samplesperpixel) == FALSE) {
			FreeImage_OutputMessageProc(s_format_id, 
				"Unable to handle this format: bitspersample = %d, samplesperpixel = %d, photometric = %d", 
				(int)bitspersample, (int)samplesperpixel, (int)photometric);
			throw (char*)NULL;
		}

		// ---------------------------------------------------------------------------------

		// get image data type

		FREE_IMAGE_TYPE image_type = ReadImageType(tif, bitspersample, samplesperpixel);

		// get the most appropriate loading method

		TIFFLoadMethod loadMethod = FindLoadMethod(tif, image_type, flags);

		// ---------------------------------------------------------------------------------

		if(loadMethod == LoadAsRBGA) {
			// ---------------------------------------------------------------------------------
			// RGB[A] loading using the TIFFReadRGBAImage() API
			// ---------------------------------------------------------------------------------

			BOOL has_alpha = FALSE;   

			// Read the whole image into one big RGBA buffer and then 
			// convert it to a DIB. This is using the traditional
			// TIFFReadRGBAImage() API that we trust.
			
			uint32_t *raster = NULL;

			if(!header_only) {

				raster = (uint32_t*)_TIFFmalloc(width * height * sizeof(uint32_t));
				if (raster == NULL) {
					throw FI_MSG_ERROR_MEMORY;
				}

				// read the image in one chunk into an RGBA array

				if (!TIFFReadRGBAImage(tif, width, height, raster, 1)) {
					_TIFFfree(raster);
					throw FI_MSG_ERROR_UNSUPPORTED_FORMAT;
				}
			}
			// TIFFReadRGBAImage always deliveres 3 or 4 samples per pixel images
			// (RGB or RGBA, see below). Cut-off possibly present channels (additional 
			// alpha channels) from e.g. Photoshop. Any CMYK(A..) is now treated as RGB,
			// any additional alpha channel on RGB(AA..) is lost on conversion to RGB(A)

			if(samplesperpixel > 4) { // TODO Write to Extra Channels
				FreeImage_OutputMessageProc(s_format_id, "Warning: %d additional alpha channel(s) ignored", samplesperpixel-4);
				samplesperpixel = 4;
			}

			// create a new DIB (take care of different samples-per-pixel in case 
			// of converted CMYK image (RGB conversion is on sample per pixel less)

			if (photometric == PHOTOMETRIC_SEPARATED && samplesperpixel == 4) {
				samplesperpixel = 3;
			}

			dib = CreateImageType(header_only, image_type, width, height, bitspersample, samplesperpixel);
			if (dib == NULL) {
				// free the raster pointer and output an error if allocation failed
				if(raster) {
					_TIFFfree(raster);
				}
				throw FI_MSG_ERROR_DIB_MEMORY;
			}
			
			// fill in the resolution (english or universal)

			ReadResolution(tif, dib);

			if(!header_only) {

				// read the raster lines and save them in the DIB
				// with RGB mode, we have to change the order of the 3 samples RGB
				// We use macros for extracting components from the packed ABGR 
				// form returned by TIFFReadRGBAImage.

				uint32_t *row = &raster[0];

				if (samplesperpixel == 4) {
					// 32-bit RGBA
					for (uint32_t y = 0; y < height; y++) {
						BYTE *bits = FreeImage_GetScanLine(dib, y);
						for (uint32_t x = 0; x < width; x++) {
							bits[FI_RGBA_BLUE]	= (BYTE)TIFFGetB(row[x]);
							bits[FI_RGBA_GREEN] = (BYTE)TIFFGetG(row[x]);
							bits[FI_RGBA_RED]	= (BYTE)TIFFGetR(row[x]);
							bits[FI_RGBA_ALPHA] = (BYTE)TIFFGetA(row[x]);

							if (bits[FI_RGBA_ALPHA] != 0) {
								has_alpha = TRUE;
							}

							bits += 4;
						}
						row += width;
					}
				} else {
					// 24-bit RGB
					for (uint32_t y = 0; y < height; y++) {
						BYTE *bits = FreeImage_GetScanLine(dib, y);
						for (uint32_t x = 0; x < width; x++) {
							bits[FI_RGBA_BLUE]	= (BYTE)TIFFGetB(row[x]);
							bits[FI_RGBA_GREEN] = (BYTE)TIFFGetG(row[x]);
							bits[FI_RGBA_RED]	= (BYTE)TIFFGetR(row[x]);

							bits += 3;
						}
						row += width;
					}
				}

				_TIFFfree(raster);
			}
			
			// ### Not correct when header only
			FreeImage_SetTransparent(dib, has_alpha);

		} else if(loadMethod == LoadAs8BitTrns) {
			// ---------------------------------------------------------------------------------
			// 8-bit + 8-bit alpha layer loading
			// ---------------------------------------------------------------------------------
			const uint16_t dst_spp = 2;

			// create a new 8-bit DIB
			dib = CreateImageType(header_only, image_type, width, height, bitspersample, MIN(dst_spp, samplesperpixel));
			if (dib == NULL) {
				throw FI_MSG_ERROR_MEMORY;
			}

			// fill in the resolution (english or universal)

			ReadResolution(tif, dib);

			// set up the colormap based on photometric	

			ReadPalette(tif, photometric, bitspersample, dib);

			// calculate the line + pitch (separate for scr & dest)

			const tmsize_t src_line = TIFFScanlineSize(tif);
			// here, the pitch is 2x less than the original as we only keep the first layer				
			int dst_pitch = FreeImage_GetPitch(dib);

			// transparency table for 8-bit + 8-bit alpha images

			BYTE trns[256]; 
			// clear the transparency table
			memset(trns, 0xFF, 256 * sizeof(BYTE));

			// In the tiff file the lines are saved from up to down 
			// In a DIB the lines must be saved from down to up

			BYTE *bits = FreeImage_GetScanLine(dib, height - 1);

			// read the tiff lines and save them in the DIB

			if(planar_config == PLANARCONFIG_CONTIG && !header_only) {

				const tmsize_t bufsz = TIFFStripSize(tif) * sizeof(BYTE);
				BYTE *buf = (BYTE*)malloc(bufsz);
				if(buf == NULL) {
					throw FI_MSG_ERROR_MEMORY;
				}

				const uint32_t src_width = static_cast<uint32_t>(src_line / samplesperpixel);
				for (uint32_t y = 0; y < height; y += rowsperstrip) {
					const int32_t nrow = (y + rowsperstrip > height ? height - y : rowsperstrip);

					if (nrow * src_width * dst_spp * sizeof(BYTE) > static_cast<size_t>(bufsz)) {
						free(buf);
						throw FI_MSG_ERROR_CORRUPTED_IMAGE;
					}

					if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 0), buf, nrow * src_line) == -1) {
						free(buf);
						throw FI_MSG_ERROR_PARSING;
					}
					for (int l = 0; l < nrow; l++) {
						BYTE *p = bits;
						const BYTE *b = buf + l * src_line;

						for(uint32_t x = 0; x < src_width; x++) {
							// copy the 8-bit layer
							*p = b[0];
							// convert the 8-bit alpha layer to a trns table
							trns[ b[0] ] = b[1];

							p++;
							b += dst_spp;
						} 
						bits -= dst_pitch;
					}
				}

				free(buf);
			}
			else if(planar_config == PLANARCONFIG_SEPARATE && !header_only) {
				tmsize_t stripsize = TIFFStripSize(tif) * sizeof(BYTE);
				BYTE *buf = (BYTE*)malloc(2 * stripsize);
				if(buf == NULL) {
					throw FI_MSG_ERROR_MEMORY;
				}
				BYTE *grey = buf;
				BYTE *alpha = buf + stripsize;

				for (uint32_t y = 0; y < height; y += rowsperstrip) {
					int32_t nrow = (y + rowsperstrip > height ? height - y : rowsperstrip);

					if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 0), grey, nrow * src_line) == -1) {
						free(buf);
						throw FI_MSG_ERROR_PARSING;
					} 
					if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 1), alpha, nrow * src_line) == -1) {
						free(buf);
						throw FI_MSG_ERROR_PARSING;
					} 

					for (int l = 0; l < nrow; l++) {
						BYTE *p = bits;
						BYTE *g = grey + l * src_line;
						BYTE *a = alpha + l * src_line;

						for(uint32_t x = 0; x < (uint32_t)(src_line); x++) {
							// copy the 8-bit layer
							*p = g[0];
							// convert the 8-bit alpha layer to a trns table
							trns[ g[0] ] = a[0];

							p++;
							g++;
							a++;
						}
						bits -= dst_pitch;
					}
				}

				free(buf);

			}
			
			FreeImage_SetTransparencyTable(dib, &trns[0], 256);
			FreeImage_SetTransparent(dib, TRUE);

		} else if(loadMethod == LoadAsCMYK) {
			// ---------------------------------------------------------------------------------
			// CMYK loading
			// ---------------------------------------------------------------------------------

			// At this place, samplesperpixel could be > 4, esp. when a CMYK(A) format
			// is recognized. Where all other formats are handled straight-forward, this
			// format has to be handled special 

			BOOL isCMYKA = (photometric == PHOTOMETRIC_SEPARATED) && (samplesperpixel > 4);

			// We use a temp dib to store the alpha for the CMYKA to RGBA conversion
			// NOTE this is until we have Extra channels implementation.
			// Also then it will be possible to merge LoadAsCMYK with LoadAsGenericStrip
			
			FIBITMAP *alpha = NULL;
			unsigned alpha_pitch = 0;
			BYTE *alpha_bits = NULL;
			unsigned alpha_Bpp = 0;

			if(isCMYKA && !header_only) {
				if(bitspersample == 16) {
					alpha = FreeImage_AllocateT(FIT_UINT16, width, height);
				} else if (bitspersample == 8) {
					alpha = FreeImage_Allocate(width, height, 8);
				}
					
				if(!alpha) {
					FreeImage_OutputMessageProc(s_format_id, "Failed to allocate temporary alpha channel");
				} else {
					alpha_bits = FreeImage_GetScanLine(alpha, height - 1);
					alpha_pitch = FreeImage_GetPitch(alpha);
					alpha_Bpp = FreeImage_GetBPP(alpha) / 8;
				}
				
			}
			
			// create a new DIB
			const uint16_t chCount = MIN<uint16_t>(samplesperpixel, 4);
			dib = CreateImageType(header_only, image_type, width, height, bitspersample, chCount);
			if (dib == NULL) {
				FreeImage_Unload(alpha);
				throw FI_MSG_ERROR_MEMORY;
			}

			// fill in the resolution (english or universal)

			ReadResolution(tif, dib);

			if(!header_only) {

				// calculate the line + pitch (separate for scr & dest)

				const tmsize_t src_line = TIFFScanlineSize(tif);
				const tmsize_t dst_line = FreeImage_GetLine(dib);
				const unsigned dib_pitch = FreeImage_GetPitch(dib);
				const unsigned dibBpp = FreeImage_GetBPP(dib) / 8;
				const unsigned Bpc = dibBpp / chCount;
				const unsigned srcBpp = bitspersample * samplesperpixel / 8;

				assert(Bpc <= 2); //< CMYK is only BYTE or SHORT 
				
				// In the tiff file the lines are save from up to down 
				// In a DIB the lines must be saved from down to up

				BYTE *bits = FreeImage_GetScanLine(dib, height - 1);

				// read the tiff lines and save them in the DIB

				BYTE *buf = (BYTE*)malloc(TIFFStripSize(tif) * sizeof(BYTE));
				if(buf == NULL) {
					FreeImage_Unload(alpha);
					throw FI_MSG_ERROR_MEMORY;
				}

				if(planar_config == PLANARCONFIG_CONTIG) {
					
					// - loop for strip blocks -
					
					for (uint32_t y = 0; y < height; y += rowsperstrip) {
						const int32_t strips = (y + rowsperstrip > height ? height - y : rowsperstrip);

						if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 0), buf, strips * src_line) == -1) {
							free(buf);
							FreeImage_Unload(alpha);
							throw FI_MSG_ERROR_PARSING;
						} 
						
						// - loop for strips -
						
						if(src_line != dst_line) {
							// CMYKA+
							if(alpha) {
								for (int l = 0; l < strips; l++) {					
									for(BYTE *pixel = bits, *al_pixel = alpha_bits, *src_pixel =  buf + l * src_line; pixel < bits + dib_pitch; pixel += dibBpp, al_pixel += alpha_Bpp, src_pixel += srcBpp) {
										// copy pixel byte by byte
										BYTE b = 0;
										for( ; b < dibBpp; ++b) {
											pixel[b] =  src_pixel[b];
										}
										// TODO write the remaining bytes to extra channel(s)
										
										// HACK write the first alpha to a separate dib (assume BYTE or WORD)
										al_pixel[0] = src_pixel[b];
										if(Bpc > 1) {
											al_pixel[1] = src_pixel[b + 1];
										}
										
									}
									bits -= dib_pitch;
									alpha_bits -= alpha_pitch;
								}
							}
							else {
								// alpha/extra channels alloc failed
								for (int l = 0; l < strips; l++) {
									for(BYTE* pixel = bits, * src_pixel =  buf + l * src_line; pixel < bits + dst_line; pixel += dibBpp, src_pixel += srcBpp) {
										AssignPixel(pixel, src_pixel, dibBpp);
									}
									bits -= dib_pitch;
								}
							}
						}
						else { 
							// CMYK to CMYK
							for (int l = 0; l < strips; l++) {
								BYTE *b = buf + l * src_line;
								memcpy(bits, b, src_line);
								bits -= dib_pitch;
							}
						}

					} // height
				
				}
				else if(planar_config == PLANARCONFIG_SEPARATE) {

					BYTE *dib_strip = bits;
					BYTE *al_strip = alpha_bits;

					// - loop for strip blocks -
					
					for (uint32_t y = 0; y < height; y += rowsperstrip) {
						const int32_t strips = (y + rowsperstrip > height ? height - y : rowsperstrip);
						
						// - loop for channels (planes) -
						
						for(uint16_t sample = 0; sample < samplesperpixel; sample++) {
							
							if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, sample), buf, strips * src_line) == -1) {
								free(buf);
								FreeImage_Unload(alpha);
								throw FI_MSG_ERROR_PARSING;
							} 
									
							BYTE *dst_strip = dib_strip;
							unsigned dst_pitch = dib_pitch;
							uint16_t ch = sample;
							unsigned Bpp = dibBpp;

							if(sample >= chCount) {
								// TODO Write to Extra Channel
								
								// HACK redirect write to temp alpha
								if(alpha && sample == chCount) {

									dst_strip = al_strip;
									dst_pitch = alpha_pitch;

									ch = 0;
									Bpp = alpha_Bpp;
								}
								else {
									break; 
								}
							}
							
							const unsigned channelOffset = ch * Bpc;			
							
							// - loop for strips in block -
							
							BYTE *src_line_begin = buf;
							BYTE *dst_line_begin = dst_strip;
							for (int l = 0; l < strips; l++, src_line_begin += src_line, dst_line_begin -= dst_pitch ) {
								// - loop for pixels in strip -
								
								const BYTE* const src_line_end = src_line_begin + src_line;
								for (BYTE *src_bits = src_line_begin, * dst_bits = dst_line_begin; src_bits < src_line_end; src_bits += Bpc, dst_bits += Bpp) {
									AssignPixel(dst_bits + channelOffset, src_bits, Bpc);									
								} // line
								
							} // strips
															
						} // channels
							
						// done with a strip block, incr to the next
						dib_strip -= strips * dib_pitch;
						al_strip -= strips * alpha_pitch;
							
					} //< height
					
				}

				free(buf);
			
				ConvertCMYKtoRGBA(dib);
					
				// The ICC Profile is invalid, clear it
				iccSize = 0;
				iccBuf = NULL;
					
				if(isCMYKA) {
					// HACK until we have Extra channels. (ConvertCMYKtoRGBA will then do the work)
						
					FreeImage_SetChannel(dib, alpha, FICC_ALPHA);
					FreeImage_Unload(alpha);
					alpha = NULL;
				}
				else {
					FIBITMAP *t = RemoveAlphaChannel(dib);
					if(t) {
						FreeImage_Unload(dib);
						dib = t;
					}
					else {
						FreeImage_OutputMessageProc(s_format_id, "Cannot allocate memory for buffer. CMYK image converted to RGB + pending Alpha");
					}
				}
				
			} // !header_only
			
		} else if(loadMethod == LoadAsGenericStrip) {
			// ---------------------------------------------------------------------------------
			// Generic loading
			// ---------------------------------------------------------------------------------

			// create a new DIB
			const uint16_t chCount = MIN<uint16_t>(samplesperpixel, 4);
			dib = CreateImageType(header_only, image_type, width, height, bitspersample, chCount);
			if (dib == NULL) {
				throw FI_MSG_ERROR_MEMORY;
			}

			// fill in the resolution (english or universal)

			ReadResolution(tif, dib);

			// set up the colormap based on photometric	

			ReadPalette(tif, photometric, bitspersample, dib);
	
			if(!header_only) {
				// calculate the line + pitch (separate for scr & dest)

				const tmsize_t src_line = TIFFScanlineSize(tif);
				const tmsize_t dst_line = FreeImage_GetLine(dib);
				const unsigned dst_pitch = FreeImage_GetPitch(dib);
				const unsigned Bpp = FreeImage_GetBPP(dib) / 8;
				const unsigned srcBpp = bitspersample * samplesperpixel / 8;

				// In the tiff file the lines are save from up to down 
				// In a DIB the lines must be saved from down to up

				BYTE *bits = FreeImage_GetScanLine(dib, height - 1);

				// read the tiff lines and save them in the DIB

				BYTE *buf = (BYTE*)malloc(TIFFStripSize(tif) * sizeof(BYTE));
				if(buf == NULL) {
					throw FI_MSG_ERROR_MEMORY;
				}
				memset(buf, 0, TIFFStripSize(tif) * sizeof(BYTE));
				
				BOOL bThrowMessage = FALSE;
				
				if(planar_config == PLANARCONFIG_CONTIG) {

					for (uint32_t y = 0; y < height; y += rowsperstrip) {
						int32_t strips = (y + rowsperstrip > height ? height - y : rowsperstrip);

						if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 0), buf, strips * src_line) == -1) {
							// ignore errors as they can be frequent and not really valid errors, especially with fax images
							bThrowMessage = TRUE;							
							/*
							free(buf);
							throw FI_MSG_ERROR_PARSING;
							*/
						} 
						if(src_line == dst_line) {
							// channel count match
							for (int l = 0; l < strips; l++) {							
								memcpy(bits, buf + l * src_line, src_line);
								bits -= dst_pitch;
							}
						}
						else {
							for (int l = 0; l < strips; l++) {
								for(BYTE *pixel = bits, *src_pixel =  buf + l * src_line; pixel < bits + dst_pitch; pixel += Bpp, src_pixel += srcBpp) {
									AssignPixel(pixel, src_pixel, Bpp);
								}
								bits -= dst_pitch;
							}
						}
					}
				}
				else if(planar_config == PLANARCONFIG_SEPARATE) {
					
					const unsigned Bpc = bitspersample / 8;
					BYTE* dib_strip = bits;
					// - loop for strip blocks -
					
					for (uint32_t y = 0; y < height; y += rowsperstrip) {
						const int32_t strips = (y + rowsperstrip > height ? height - y : rowsperstrip);
						
						// - loop for channels (planes) -
						
						for(uint16_t sample = 0; sample < samplesperpixel; sample++) {
							
							if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, sample), buf, strips * src_line) == -1) {
								// ignore errors as they can be frequent and not really valid errors, especially with fax images
								bThrowMessage = TRUE;	
							} 
									
							if(sample >= chCount) {
								// TODO Write to Extra Channel
								break; 
							}
							
							const unsigned channelOffset = sample * Bpc;			
							
							// - loop for strips in block -
							
							BYTE* src_line_begin = buf;
							BYTE* dst_line_begin = dib_strip;
							for (int l = 0; l < strips; l++, src_line_begin += src_line, dst_line_begin -= dst_pitch ) {
									
								// - loop for pixels in strip -
								
								const BYTE* const src_line_end = src_line_begin + src_line;

								for (BYTE* src_bits = src_line_begin, * dst_bits = dst_line_begin; src_bits < src_line_end; src_bits += Bpc, dst_bits += Bpp) {
									// actually assigns channel
									AssignPixel(dst_bits + channelOffset, src_bits, Bpc); 
								} // line

							} // strips

						} // channels
							
						// done with a strip block, incr to the next
						dib_strip -= strips * dst_pitch;
							
					} // height

				}
				free(buf);
				
				if(bThrowMessage) {
					FreeImage_OutputMessageProc(s_format_id, "Warning: parsing error. Image may be incomplete or contain invalid data !");
				}
				
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
				SwapRedBlue32(dib);
#endif

			} // !header only
			
		} else if(loadMethod == LoadAsTiled) {
			// ---------------------------------------------------------------------------------
			// Tiled image loading
			// ---------------------------------------------------------------------------------

			uint32_t tileWidth, tileHeight;
			uint32_t src_line = 0;

			// create a new DIB
			dib = CreateImageType( header_only, image_type, width, height, bitspersample, samplesperpixel);
			if (dib == NULL) {
				throw FI_MSG_ERROR_MEMORY;
			}

			// fill in the resolution (english or universal)

			ReadResolution(tif, dib);

			// set up the colormap based on photometric	

			ReadPalette(tif, photometric, bitspersample, dib);

			// get the tile geometry
			if(!TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tileWidth) || !TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileHeight)) {
				throw "Invalid tiled TIFF image";
			}

			// read the tiff lines and save them in the DIB

			if(planar_config == PLANARCONFIG_CONTIG && !header_only) {
				
				// get the maximum number of bytes required to contain a tile
				tmsize_t tileSize = TIFFTileSize(tif);

				// allocate tile buffer
				BYTE *tileBuffer = (BYTE*)malloc(tileSize * sizeof(BYTE));
				if(tileBuffer == NULL) {
					throw FI_MSG_ERROR_MEMORY;
				}

				// calculate src line and dst pitch
				unsigned dst_pitch = FreeImage_GetPitch(dib);
				uint32_t tileRowSize = (uint32_t)TIFFTileRowSize(tif);
				uint32_t imageRowSize = (uint32_t)TIFFScanlineSize(tif);


				// In the tiff file the lines are saved from up to down 
				// In a DIB the lines must be saved from down to up

				BYTE *bits = FreeImage_GetScanLine(dib, height - 1);
				
				for (uint32_t y = 0; y < height; y += tileHeight) {						
					int32_t nrows = (y + tileHeight > height ? height - y : tileHeight);					

					for (uint32_t x = 0, rowSize = 0; x < width; x += tileWidth, rowSize += tileRowSize) {
						memset(tileBuffer, 0, tileSize);

						// read one tile
						if (TIFFReadTile(tif, tileBuffer, x, y, 0, 0) < 0) {
							free(tileBuffer);
							throw "Corrupted tiled TIFF file";
						}
						// convert to strip
						if(x + tileWidth > width) {
							src_line = imageRowSize - rowSize;
						} else {
							src_line = tileRowSize;
						}
						BYTE *src_bits = tileBuffer;
						BYTE *dst_bits = bits + rowSize;
						for(int k = 0; k < nrows; k++) {
							memcpy(dst_bits, src_bits, MIN(dst_pitch, src_line));
							src_bits += tileRowSize;
							dst_bits -= dst_pitch;
						}
					}

					bits -= nrows * dst_pitch;
				}

#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
				SwapRedBlue32(dib);
#endif
				free(tileBuffer);
			}
			else if(planar_config == PLANARCONFIG_SEPARATE) {
				throw "Separated tiled TIFF images are not supported"; 
			}


		} else if(loadMethod == LoadAsLogLuv) {
			// ---------------------------------------------------------------------------------
			// RGBF LogLuv compressed loading
			// ---------------------------------------------------------------------------------

			double	stonits;	// input conversion to nits
			if (!TIFFGetField(tif, TIFFTAG_STONITS, &stonits)) {
				stonits = 1;
			}
			
			// create a new DIB
			dib = CreateImageType(header_only, image_type, width, height, bitspersample, samplesperpixel);
			if (dib == NULL) {
				throw FI_MSG_ERROR_MEMORY;
			}

			// fill in the resolution (english or universal)

			ReadResolution(tif, dib);

			if(planar_config == PLANARCONFIG_CONTIG && !header_only) {
				// calculate the line + pitch (separate for scr & dest)

				tmsize_t src_line = TIFFScanlineSize(tif);
				int dst_pitch = FreeImage_GetPitch(dib);

				// In the tiff file the lines are save from up to down 
				// In a DIB the lines must be saved from down to up

				BYTE *bits = FreeImage_GetScanLine(dib, height - 1);

				// read the tiff lines and save them in the DIB

				BYTE *buf = (BYTE*)malloc(TIFFStripSize(tif) * sizeof(BYTE));
				if(buf == NULL) {
					throw FI_MSG_ERROR_MEMORY;
				}

				for (uint32_t y = 0; y < height; y += rowsperstrip) {
					int32_t nrow = (y + rowsperstrip > height ? height - y : rowsperstrip);

					if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 0), buf, nrow * src_line) == -1) {
						free(buf);
						throw FI_MSG_ERROR_PARSING;
					} 
					// convert from XYZ to RGB
					for (int l = 0; l < nrow; l++) {						
						tiff_ConvertLineXYZToRGB(bits, buf + l * src_line, stonits, width);
						bits -= dst_pitch;
					}
				}

				free(buf);
			}
			else if(planar_config == PLANARCONFIG_SEPARATE) {
				// this cannot happen according to the LogLuv specification
				throw "Unable to handle PLANARCONFIG_SEPARATE LogLuv images";
			}
		} else {
			// ---------------------------------------------------------------------------------
			// Unknown or unsupported format
			// ---------------------------------------------------------------------------------

			throw FI_MSG_ERROR_UNSUPPORTED_FORMAT;
		}
		
		return dib;

	} catch (const char *message) {			
		if(dib)	{
			FreeImage_Unload(dib);
		}
		if(message) {
			FreeImage_OutputMessageProc(s_format_id, message);
		}
		return NULL;
	}
  
}

// --------------------------------------------------------------------------

/**
Save a single image into a TIF

@param io FreeImage IO
@param dib The dib to be saved
@param handle FreeImage handle
@param page Page number
@param flags FreeImage TIFF save flag
@param data TIFF plugin context
@param ifd TIFF Image File Directory (0 means save image, > 0 && (page == -1) means save thumbnail)
@param ifdCount 1 if no thumbnail to save, 2 if image + thumbnail to save
@return Returns TRUE if successful, returns FALSE otherwise
*/
static BOOL 
SaveOneTIFF(FreeImageIO *io, FIBITMAP *dib, fi_handle handle, int flags, void *data) {
	if (!dib || !handle || !data) {
		return FALSE;
	} 
	
	try { 
		fi_TIFFIO *fio = (fi_TIFFIO*)data;
		TIFF *out = fio->tif;

		const FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(dib);

		const uint32_t width = FreeImage_GetWidth(dib);
		const uint32_t height = FreeImage_GetHeight(dib);
		const uint16_t bitsperpixel = (uint16_t)FreeImage_GetBPP(dib);

		// setup out-variables based on dib and flag options
		
		uint16_t bitspersample;
		uint16_t samplesperpixel;
		uint16_t photometric;

		if(image_type == FIT_BITMAP) {
			// standard image: 1-, 4-, 8-, 16-, 24-, 32-bit

			samplesperpixel = ((bitsperpixel == 24) ? 3 : ((bitsperpixel == 32) ? 4 : 1));
			bitspersample = bitsperpixel / samplesperpixel;
			photometric	= GetPhotometric(dib);

			if((bitsperpixel == 8) && FreeImage_IsTransparent(dib)) {
				// 8-bit transparent picture : convert later to 8-bit + 8-bit alpha
				samplesperpixel = 2;
				bitspersample = 8;
			}
			else if(bitsperpixel == 32) {
				// 32-bit images : check for CMYK or alpha transparency

				if(photometric == PHOTOMETRIC_RGB) {
					// transparency mask support
					uint16_t sampleinfo[1]; 
					// unassociated alpha data is transparency information
					sampleinfo[0] = EXTRASAMPLE_UNASSALPHA;
					TIFFSetField(out, TIFFTAG_EXTRASAMPLES, 1, sampleinfo);
				}
			}
		} else if(image_type == FIT_RGB16) {
			// 48-bit RGB

			samplesperpixel = 3;
			bitspersample = bitsperpixel / samplesperpixel;
			photometric	= PHOTOMETRIC_RGB;
		} else if(image_type == FIT_RGBA16) {
			// 64-bit RGBA

			samplesperpixel = 4;
			bitspersample = bitsperpixel / samplesperpixel;
			photometric	= PHOTOMETRIC_RGB;
			// transparency mask support
			uint16_t sampleinfo[1]; 
			// unassociated alpha data is transparency information
			sampleinfo[0] = EXTRASAMPLE_UNASSALPHA;
			TIFFSetField(out, TIFFTAG_EXTRASAMPLES, 1, sampleinfo);
		} else if(image_type == FIT_RGBF) {
			// 96-bit RGBF => store with a LogLuv encoding ?

			samplesperpixel = 3;
			bitspersample = bitsperpixel / samplesperpixel;
			// the library converts to and from floating-point XYZ CIE values
			if((flags & TIFF_LOGLUV) == TIFF_LOGLUV) {
				photometric	= PHOTOMETRIC_LOGLUV;
				TIFFSetField(out, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_FLOAT);
				// TIFFSetField(out, TIFFTAG_STONITS, 1.0);   // assume unknown 
			}
			else {
				// store with default compression (LZW) or with input compression flag
				photometric	= PHOTOMETRIC_RGB;
			}
			
		} else if (image_type == FIT_RGBAF) {
			// 128-bit RGBAF => store with default compression (LZW) or with input compression flag
			
			samplesperpixel = 4;
			bitspersample = bitsperpixel / samplesperpixel;
			photometric	= PHOTOMETRIC_RGB;
		} else {
			// special image type (int, long, double, ...)
			
			samplesperpixel = 1;
			bitspersample = bitsperpixel;
			photometric	= PHOTOMETRIC_MINISBLACK;
		}

		// set image data type

		WriteImageType(out, image_type);
		
		// handle standard width/height/bpp stuff

		TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
		TIFFSetField(out, TIFFTAG_IMAGELENGTH, height);
		TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
		TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bitspersample);
		TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photometric);
		TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);	// single image plane 
		TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		TIFFSetField(out, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
		TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, (uint32_t) -1)); 

		// handle metrics

		WriteResolution(out, dib);

		// palettes (image colormaps are automatically scaled to 16-bits)

		if (photometric == PHOTOMETRIC_PALETTE) {
			uint16_t *r, *g, *b;
			uint16_t nColors = (uint16_t)FreeImage_GetColorsUsed(dib);
			RGBQUAD *pal = FreeImage_GetPalette(dib);

			r = (uint16_t *) _TIFFmalloc(sizeof(uint16_t) * 3 * nColors);
			if(r == NULL) {
				throw FI_MSG_ERROR_MEMORY;
			}
			g = r + nColors;
			b = g + nColors;

			for (int i = nColors - 1; i >= 0; i--) {
				r[i] = SCALE((uint16_t)pal[i].rgbRed);
				g[i] = SCALE((uint16_t)pal[i].rgbGreen);
				b[i] = SCALE((uint16_t)pal[i].rgbBlue);
			}

			TIFFSetField(out, TIFFTAG_COLORMAP, r, g, b);

			_TIFFfree(r);
		}

		// compression tag

		WriteCompression(out, bitspersample, samplesperpixel, photometric, flags);

		// read the DIB lines from bottom to top
		// and save them in the TIF
		// -------------------------------------
		
		const uint32_t pitch = FreeImage_GetPitch(dib);

		if(image_type == FIT_BITMAP) {
			// standard bitmap type
		
			switch(bitsperpixel) {
				case 1 :
				case 4 :
				case 8 :
				{
					if((bitsperpixel == 8) && FreeImage_IsTransparent(dib)) {
						// 8-bit transparent picture : convert to 8-bit + 8-bit alpha

						// get the transparency table
						BYTE *trns = FreeImage_GetTransparencyTable(dib);

						BYTE *buffer = (BYTE *)malloc(2 * width * sizeof(BYTE));
						if(buffer == NULL) {
							throw FI_MSG_ERROR_MEMORY;
						}

						for (int y = height - 1; y >= 0; y--) {
							BYTE *bits = FreeImage_GetScanLine(dib, y);

							BYTE *p = bits, *b = buffer;

							for(uint32_t x = 0; x < width; x++) {
								// copy the 8-bit layer
								b[0] = *p;
								// convert the trns table to a 8-bit alpha layer
								b[1] = trns[ b[0] ];

								p++;
								b += samplesperpixel;
							}

							// write the scanline to disc

							TIFFWriteScanline(out, buffer, height - y - 1, 0);
						}

						free(buffer);
					}
					else {
						// other cases
						BYTE *buffer = (BYTE *)malloc(pitch * sizeof(BYTE));
						if(buffer == NULL) {
							throw FI_MSG_ERROR_MEMORY;
						}

						for (uint32_t y = 0; y < height; y++) {
							// get a copy of the scanline
							memcpy(buffer, FreeImage_GetScanLine(dib, height - y - 1), pitch);
							// write the scanline to disc
							TIFFWriteScanline(out, buffer, y, 0);
						}
						free(buffer);
					}

					break;
				}				

				case 24:
				case 32:
				{
					BYTE *buffer = (BYTE *)malloc(pitch * sizeof(BYTE));
					if(buffer == NULL) {
						throw FI_MSG_ERROR_MEMORY;
					}

					for (uint32_t y = 0; y < height; y++) {
						// get a copy of the scanline

						memcpy(buffer, FreeImage_GetScanLine(dib, height - y - 1), pitch);

#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
						if (photometric != PHOTOMETRIC_SEPARATED) {
							// TIFFs store color data RGB(A) instead of BGR(A)
		
							BYTE *pBuf = buffer;
		
							for (uint32_t x = 0; x < width; x++) {
								INPLACESWAP(pBuf[0], pBuf[2]);
								pBuf += samplesperpixel;
							}
						}
#endif
						// write the scanline to disc

						TIFFWriteScanline(out, buffer, y, 0);
					}

					free(buffer);

					break;
				}
			}//< switch (bitsperpixel)

		} else if(image_type == FIT_RGBF && (flags & TIFF_LOGLUV) == TIFF_LOGLUV) {
			// RGBF image => store as XYZ using a LogLuv encoding

			BYTE *buffer = (BYTE *)malloc(pitch * sizeof(BYTE));
			if(buffer == NULL) {
				throw FI_MSG_ERROR_MEMORY;
			}

			for (uint32_t y = 0; y < height; y++) {
				// get a copy of the scanline and convert from RGB to XYZ
				tiff_ConvertLineRGBToXYZ(buffer, FreeImage_GetScanLine(dib, height - y - 1), width);
				// write the scanline to disc
				TIFFWriteScanline(out, buffer, y, 0);
			}
			free(buffer);
		} else {
			// just dump the dib (tiff supports all dib types)
			
			BYTE *buffer = (BYTE *)malloc(pitch * sizeof(BYTE));
			if(buffer == NULL) {
				throw FI_MSG_ERROR_MEMORY;
			}
			
			for (uint32_t y = 0; y < height; y++) {
				// get a copy of the scanline
				memcpy(buffer, FreeImage_GetScanLine(dib, height - y - 1), pitch);
				// write the scanline to disc
				TIFFWriteScanline(out, buffer, y, 0);
			}
			free(buffer);
		}

		return TRUE;
		
	} catch(const char *text) {
		FreeImage_OutputMessageProc(s_format_id, text);
		return FALSE;
	} 
}

static BOOL DLL_CALLCONV
Save(FreeImageIO *io, FIBITMAP *dib, fi_handle handle, int flags, void *data) {
	BOOL bResult = FALSE;
	
	FIBITMAP *bitmap = dib;
	bResult = SaveOneTIFF(io, bitmap, handle, flags, data);
	if(!bResult) {
		return FALSE;
	}

	return bResult;
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitTIFF(Plugin *plugin, int format_id) {
	s_format_id = format_id;

	plugin->format_proc = Format;
	plugin->description_proc = Description;
	plugin->extension_proc = Extension;
	plugin->regexpr_proc = RegExpr;
	plugin->open_proc = Open;
	plugin->close_proc = Close;
	plugin->load_proc = Load;
	plugin->save_proc = Save;
	plugin->validate_proc = Validate;
	plugin->mime_proc = MimeType;
	plugin->supports_export_bpp_proc = SupportsExportDepth;
	plugin->supports_export_type_proc = SupportsExportType;
	plugin->supports_no_pixels_proc = SupportsNoPixels; 
}

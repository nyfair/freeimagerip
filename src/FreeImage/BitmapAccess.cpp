// ==========================================================
// FreeImage implementation
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Hervé Drolon (drolon@infonie.fr)
// - Detlev Vendt (detlev.vendt@brillit.de)
// - Petr Supina (psup@centrum.cz)
// - Carsten Klein (c.klein@datagis.com)
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

#include <stdlib.h>
#if defined(_WIN32) || defined(_WIN64) || defined(__MINGW32__)
#include <malloc.h>
#endif // _WIN32 || _WIN64 || __MINGW32__

#include "FreeImage.h"
#include "FreeImageIO.h"
#include "Utilities.h"
#include "MapIntrospector.h"

/**
Constants for the BITMAPINFOHEADER::biCompression field
BI_RGB:
The bitmap is in uncompressed red green blue (RGB) format that is not compressed and does not use color masks.
BI_BITFIELDS:
The bitmap is not compressed and the color table consists of three DWORD color masks that specify the red, green, and blue components, 
respectively, of each pixel. This is valid when used with 16 and 32-bits per pixel bitmaps.
*/
#ifndef _WINGDI_
#define BI_RGB       0L
#define BI_BITFIELDS 3L
#endif // _WINGDI_

// ----------------------------------------------------------
//  FIBITMAP definition
// ----------------------------------------------------------

/**
FreeImage header structure
*/
FI_STRUCT (FREEIMAGEHEADER) {
	/** data type - bitmap, array of long, double, complex, etc */
	FREE_IMAGE_TYPE type;

	/** background color used for RGB transparency */
	RGBQUAD bkgnd_color;

	/**@name transparency management */
	//@{
	/**
	why another table ? for easy transparency table retrieval !
	transparency could be stored in the palette, which is better
	overall, but it requires quite some changes and it will render
	FreeImage_GetTransparencyTable obsolete in its current form;
	*/
	BYTE transparent_table[256];
	/** number of transparent colors */
	int  transparency_count;
	/** TRUE if the image is transparent */
	BOOL transparent;
	//@}

	/** FALSE if the FIBITMAP only contains the header and no pixel data */
	BOOL has_pixels;

	/**@name external pixel buffer management */
	//@{
	/** pointer to user provided pixels, NULL otherwise */
	BYTE *external_bits;
	/** user provided pitch, 0 otherwise */
	unsigned external_pitch;
	//@}

	//BYTE filler[1];			 // fill to 32-bit alignment
};

// ----------------------------------------------------------
//  FREEIMAGERGBMASKS definition
// ----------------------------------------------------------

/**
RGB mask structure - mainly used for 16-bit RGB555 / RGB 565 FIBITMAP
*/
FI_STRUCT (FREEIMAGERGBMASKS) {
	unsigned red_mask;		//! bit layout of the red components
	unsigned green_mask;	//! bit layout of the green components
	unsigned blue_mask;		//! bit layout of the blue components
};

// ----------------------------------------------------------
//  Memory allocation on a specified alignment boundary
// ----------------------------------------------------------

#if (defined(_WIN32) || defined(_WIN64)) && !defined(__MINGW32__)

void* FreeImage_Aligned_Malloc(size_t amount, size_t alignment) {
	assert(alignment == FIBITMAP_ALIGNMENT);
	return _aligned_malloc(amount, alignment);
}

void FreeImage_Aligned_Free(void* mem) {
	_aligned_free(mem);
}

#elif defined (__MINGW32__)

void* FreeImage_Aligned_Malloc(size_t amount, size_t alignment) {
	assert(alignment == FIBITMAP_ALIGNMENT);
	return __mingw_aligned_malloc (amount, alignment);
}

void FreeImage_Aligned_Free(void* mem) {
	__mingw_aligned_free (mem);
}

#else

void* FreeImage_Aligned_Malloc(size_t amount, size_t alignment) {
	assert(alignment == FIBITMAP_ALIGNMENT);
	/*
	In some rare situations, the malloc routines can return misaligned memory. 
	The routine FreeImage_Aligned_Malloc allocates a bit more memory to do
	aligned writes.  Normally, it *should* allocate "alignment" extra memory and then writes
	one dword back the true pointer.  But if the memory manager returns a
	misaligned block that is less than a dword from the next alignment, 
	then the writing back one dword will corrupt memory.

	For example, suppose that alignment is 16 and malloc returns the address 0xFFFF.

	16 - 0xFFFF % 16 + 0xFFFF = 16 - 15 + 0xFFFF = 0x10000.

	Now, you subtract one dword from that and write and that will corrupt memory.

	That's why the code below allocates *two* alignments instead of one. 
	*/
	void* mem_real = malloc(amount + 2 * alignment);
	if(!mem_real) return NULL;
	char* mem_align = (char*)((unsigned long)(2 * alignment - (unsigned long)mem_real % (unsigned long)alignment) + (unsigned long)mem_real);
	*((long*)mem_align - 1) = (long)mem_real;
	return mem_align;
}

void FreeImage_Aligned_Free(void* mem) {
	free((void*)*((long*)mem - 1));
}

#endif // _WIN32 || _WIN64

// ----------------------------------------------------------
//  FIBITMAP memory management
// ----------------------------------------------------------

/**
Calculate the size of a FreeImage image. 
Align the palette and the pixels on a FIBITMAP_ALIGNMENT bytes alignment boundary.
This function includes a protection against malicious images, based on a KISS integer overflow detection mechanism. 

@param header_only If TRUE, calculate a 'header only' FIBITMAP size, otherwise calculate a full FIBITMAP size
@param width Image width
@param height Image height
@param bpp Number of bits-per-pixel
@param need_masks We only store the masks (and allocate memory for them) for 16-bit images of type FIT_BITMAP
@return Returns a size in BYTE units
@see FreeImage_AllocateBitmap
*/
static size_t 
FreeImage_GetInternalImageSize(BOOL header_only, unsigned width, unsigned height, unsigned bpp, BOOL need_masks) {
	size_t dib_size = sizeof(FREEIMAGEHEADER);
	dib_size += (dib_size % FIBITMAP_ALIGNMENT ? FIBITMAP_ALIGNMENT - dib_size % FIBITMAP_ALIGNMENT : 0);
	dib_size += FIBITMAP_ALIGNMENT - sizeof(BITMAPINFOHEADER) % FIBITMAP_ALIGNMENT;
	dib_size += sizeof(BITMAPINFOHEADER);
	// palette is aligned on a 16 bytes boundary
	dib_size += sizeof(RGBQUAD) * CalculateUsedPaletteEntries(bpp);
	// we both add palette size and masks size if need_masks is true, since CalculateUsedPaletteEntries
	// always returns 0 if need_masks is true (which is only true for 16 bit images).
	dib_size += need_masks ? sizeof(DWORD) * 3 : 0;
	dib_size += (dib_size % FIBITMAP_ALIGNMENT ? FIBITMAP_ALIGNMENT - dib_size % FIBITMAP_ALIGNMENT : 0);

	if(!header_only) {
		const size_t header_size = dib_size;

		// pixels are aligned on a 16 bytes boundary
		dib_size += (size_t)CalculatePitch(CalculateLine(width, bpp)) * (size_t)height;

		// check for possible malloc overflow using a KISS integer overflow detection mechanism
		{
			const double dPitch = floor( ((double)bpp * width + 31.0) / 32.0 ) * 4.0;
			const double dImageSize = (double)header_size + dPitch * height;
			if(dImageSize != (double)dib_size) {
				// here, we are sure to encounter a malloc overflow: try to avoid it ...
				return 0;
			}

			/*
			The following constant take into account the additionnal memory used by 
			aligned malloc functions as well as debug malloc functions. 
			It is supposed here that using a (8 * FIBITMAP_ALIGNMENT) risk margin will be enough
			for the target compiler. 
			*/
			const double FIBITMAP_MAX_MEMORY = (double)((size_t)-1) - 8 * FIBITMAP_ALIGNMENT;

			if(dImageSize > FIBITMAP_MAX_MEMORY) {
				// avoid possible overflow inside C allocation functions
				return 0;
			}
		}
	}

	return dib_size;
}

/**
Helper for 16-bit FIT_BITMAP
Returns a pointer to the bitmap's red-, green- and blue masks.
@param dib The bitmap to obtain masks from.
@return Returns a pointer to the bitmap's red-, green- and blue masks
or NULL, if no masks are present (e.g. for 24 bit images).
*/
static FREEIMAGERGBMASKS *
FreeImage_GetRGBMasks(FIBITMAP *dib) {
	return FreeImage_HasRGBMasks(dib) ? (FREEIMAGERGBMASKS *)(((BYTE *)FreeImage_GetInfoHeader(dib)) + sizeof(BITMAPINFOHEADER)) : NULL;
}

/**
Internal FIBITMAP allocation.

This function accepts (ext_bits, ext_pitch) arguments. If these are provided the FIBITMAP 
will be allocated as "header only", but bits and pitch will be stored within the FREEIMAGEHEADER 
and the resulting FIBITMAP will have pixels, i.e. HasPixels() will return TRUE.
- GetBits() and GetPitch return the correct values - either offsets or the stored values (user-provided bits and pitch).
- Clone() creates a new FIBITMAP with copy of the user pixel data.
- Unload's implementation does not need to change - it just release a "header only" dib.
Note that when using external data, the data does not need to have the same alignment as the default 4-byte alignment. 
This enables the possibility to access buffers with, for instance, stricter alignment,
like the ones used in low-level APIs like OpenCL or intrinsics.

@param header_only If TRUE, allocate a 'header only' FIBITMAP, otherwise allocate a full FIBITMAP
@param ext_bits Pointer to external user's pixel buffer if using wrapped buffer, NULL otherwise
@param ext_pitch Pointer to external user's pixel buffer pitch if using wrapped buffer, 0 otherwise
@param type Image type
@param width Image width
@param height Image height
@param bpp Number of bits per pixel
@param red_mask Image red mask 
@param green_mask Image green mask
@param blue_mask Image blue mask
@return Returns the allocated FIBITMAP if successful, returns NULL otherwise
*/
static FIBITMAP * 
FreeImage_AllocateBitmap(BOOL header_only, BYTE *ext_bits, unsigned ext_pitch, FREE_IMAGE_TYPE type, int width, int height, int bpp, unsigned red_mask, unsigned green_mask, unsigned blue_mask) {

	// check input variables
	width = abs(width);
	height = abs(height);
	if(!((width > 0) && (height > 0))) {
		return NULL;
	}
	if(ext_bits) {
		if(ext_pitch == 0) {
			return NULL;
		}
		assert(header_only == FALSE);
	}

	// we only store the masks (and allocate memory for them) for 16-bit images of type FIT_BITMAP
	BOOL need_masks = FALSE;

	// check pixel bit depth
	switch(type) {
		case FIT_BITMAP:
			switch(bpp) {
				case 1:
				case 4:
				case 8:
					break;
				case 16:
					need_masks = TRUE;
					break;
				case 24:
				case 32:
					break;
				default:
					bpp = 8;
					break;
			}
			break;
		case FIT_UINT16:
			bpp = 8 * sizeof(unsigned short);
			break;
		case FIT_INT16:
			bpp = 8 * sizeof(short);
			break;
		case FIT_UINT32:
			bpp = 8 * sizeof(DWORD);
			break;
		case FIT_INT32:
			bpp = 8 * sizeof(LONG);
			break;
		case FIT_FLOAT:
			bpp = 8 * sizeof(float);
			break;
		case FIT_DOUBLE:
			bpp = 8 * sizeof(double);
			break;
		case FIT_COMPLEX:
			bpp = 8 * sizeof(FICOMPLEX);
			break;
		case FIT_RGB16:
			bpp = 8 * sizeof(FIRGB16);
			break;
		case FIT_RGBA16:
			bpp = 8 * sizeof(FIRGBA16);
			break;
		case FIT_RGBF:
			bpp = 8 * sizeof(FIRGBF);
			break;
		case FIT_RGBAF:
			bpp = 8 * sizeof(FIRGBAF);
			break;
		default:
			return NULL;
	}

	FIBITMAP *bitmap = (FIBITMAP *)malloc(sizeof(FIBITMAP));

	if (bitmap != NULL) {

		// calculate the size of a FreeImage image
		// align the palette and the pixels on a FIBITMAP_ALIGNMENT bytes alignment boundary
		// palette is aligned on a 16 bytes boundary
		// pixels are aligned on a 16 bytes boundary

		// when using a user provided pixel buffer, force a 'header only' allocation

		size_t dib_size = FreeImage_GetInternalImageSize(header_only || ext_bits, width, height, bpp, need_masks);

		if(dib_size == 0) {
			// memory allocation will fail (probably a malloc overflow)
			free(bitmap);
			return NULL;
		}

		bitmap->data = (BYTE *)FreeImage_Aligned_Malloc(dib_size * sizeof(BYTE), FIBITMAP_ALIGNMENT);

		if (bitmap->data != NULL) {
			memset(bitmap->data, 0, dib_size);

			// write out the FREEIMAGEHEADER

			FREEIMAGEHEADER *fih = (FREEIMAGEHEADER *)bitmap->data;

			fih->type = type;

			memset(&fih->bkgnd_color, 0, sizeof(RGBQUAD));

			fih->transparent = FALSE;
			fih->transparency_count = 0;
			memset(fih->transparent_table, 0xff, 256);

			fih->has_pixels = header_only ? FALSE : TRUE;

			// write out the BITMAPINFOHEADER

			BITMAPINFOHEADER *bih   = FreeImage_GetInfoHeader(bitmap);
			bih->biSize             = sizeof(BITMAPINFOHEADER);
			bih->biWidth            = width;
			bih->biHeight           = height;
			bih->biPlanes           = 1;
			bih->biCompression      = need_masks ? BI_BITFIELDS : BI_RGB;
			bih->biBitCount         = (WORD)bpp;
			bih->biClrUsed          = CalculateUsedPaletteEntries(bpp);
			bih->biClrImportant     = bih->biClrUsed;
			bih->biXPelsPerMeter	= 2835;	// 72 dpi
			bih->biYPelsPerMeter	= 2835;	// 72 dpi

			if(bpp == 8) {
				// build a default greyscale palette (very useful for image processing)
				RGBQUAD *pal = FreeImage_GetPalette(bitmap);
				for(int i = 0; i < 256; i++) {
					pal[i].rgbRed	= (BYTE)i;
					pal[i].rgbGreen = (BYTE)i;
					pal[i].rgbBlue	= (BYTE)i;
				}
			}

			// just setting the masks (only if needed) just like the palette.
			if (need_masks) {
				FREEIMAGERGBMASKS *masks = FreeImage_GetRGBMasks(bitmap);
				masks->red_mask = red_mask;
				masks->green_mask = green_mask;
				masks->blue_mask = blue_mask;
			}

			return bitmap;
		}

		free(bitmap);
	}

	return NULL;
}

FIBITMAP * DLL_CALLCONV
FreeImage_AllocateHeaderForBits(BYTE *ext_bits, unsigned ext_pitch, FREE_IMAGE_TYPE type, int width, int height, int bpp, unsigned red_mask, unsigned green_mask, unsigned blue_mask) {
	return FreeImage_AllocateBitmap(FALSE, ext_bits, ext_pitch, type, width, height, bpp, red_mask, green_mask, blue_mask);
}

FIBITMAP * DLL_CALLCONV
FreeImage_AllocateHeaderT(BOOL header_only, FREE_IMAGE_TYPE type, int width, int height, int bpp, unsigned red_mask, unsigned green_mask, unsigned blue_mask) {
	return FreeImage_AllocateBitmap(header_only, NULL, 0, type, width, height, bpp, red_mask, green_mask, blue_mask);
}

FIBITMAP * DLL_CALLCONV
FreeImage_AllocateHeader(BOOL header_only, int width, int height, int bpp, unsigned red_mask, unsigned green_mask, unsigned blue_mask) {
	return FreeImage_AllocateBitmap(header_only, NULL, 0, FIT_BITMAP, width, height, bpp, red_mask, green_mask, blue_mask);
}

FIBITMAP * DLL_CALLCONV
FreeImage_Allocate(int width, int height, int bpp, unsigned red_mask, unsigned green_mask, unsigned blue_mask) {
	return FreeImage_AllocateBitmap(FALSE, NULL, 0, FIT_BITMAP, width, height, bpp, red_mask, green_mask, blue_mask);
}

FIBITMAP * DLL_CALLCONV
FreeImage_AllocateT(FREE_IMAGE_TYPE type, int width, int height, int bpp, unsigned red_mask, unsigned green_mask, unsigned blue_mask) {
	return FreeImage_AllocateBitmap(FALSE, NULL, 0, type, width, height, bpp, red_mask, green_mask, blue_mask);
}

void DLL_CALLCONV
FreeImage_Unload(FIBITMAP *dib) {
	if (NULL != dib) {	
		if (NULL != dib->data) {
			// delete bitmap ...
			FreeImage_Aligned_Free(dib->data);
		}

		free(dib);		// ... and the wrapper
	}
}

// ----------------------------------------------------------

FIBITMAP * DLL_CALLCONV
FreeImage_Clone(FIBITMAP *dib) {
	if(!dib) {
		return NULL;
	}

	FREE_IMAGE_TYPE type = FreeImage_GetImageType(dib);
	unsigned width	= FreeImage_GetWidth(dib);
	unsigned height	= FreeImage_GetHeight(dib);
	unsigned bpp	= FreeImage_GetBPP(dib);

	const BYTE *ext_bits = ((FREEIMAGEHEADER *)dib->data)->external_bits;
	
	// check for pixel availability ...
	BOOL header_only = FreeImage_HasPixels(dib) ? FALSE : TRUE;

	// check whether this image has masks defined ...
	BOOL need_masks = (bpp == 16 && type == FIT_BITMAP) ? TRUE : FALSE;

	// allocate a new dib
	FIBITMAP *new_dib = FreeImage_AllocateHeaderT(header_only, type, width, height, bpp,
			FreeImage_GetRedMask(dib), FreeImage_GetGreenMask(dib), FreeImage_GetBlueMask(dib));

	if (new_dib) {
		// calculate the size of the src image
		// align the palette and the pixels on a FIBITMAP_ALIGNMENT bytes alignment boundary
		// palette is aligned on a 16 bytes boundary
		// pixels are aligned on a 16 bytes boundary
		
		// when using a user provided pixel buffer, force a 'header only' calculation		

		size_t dib_size = FreeImage_GetInternalImageSize(header_only || ext_bits, width, height, bpp, need_masks);

		// copy the bitmap + internal pointers (remember to restore new_dib internal pointers later)
		memcpy(new_dib->data, dib->data, dib_size);

		// copy user provided pixel buffer (if any)
		if(ext_bits) {
			const unsigned pitch = FreeImage_GetPitch(dib);
			const unsigned linesize = FreeImage_GetLine(dib);
			for(unsigned y = 0; y < height; y++) {
				memcpy(FreeImage_GetScanLine(new_dib, y), ext_bits, linesize);
				ext_bits += pitch;
			}
		}

		return new_dib;
	}

	return NULL;
}

// ----------------------------------------------------------

BYTE * DLL_CALLCONV
FreeImage_GetBits(FIBITMAP *dib) {
	if(!FreeImage_HasPixels(dib)) {
		return NULL;
	}

	if(((FREEIMAGEHEADER *)dib->data)->external_bits) {
		return ((FREEIMAGEHEADER *)dib->data)->external_bits;
	}

	// returns the pixels aligned on a FIBITMAP_ALIGNMENT bytes alignment boundary
	size_t lp = (size_t)FreeImage_GetInfoHeader(dib);
	lp += sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * FreeImage_GetColorsUsed(dib);
	lp += FreeImage_HasRGBMasks(dib) ? sizeof(DWORD) * 3 : 0;
	lp += (lp % FIBITMAP_ALIGNMENT ? FIBITMAP_ALIGNMENT - lp % FIBITMAP_ALIGNMENT : 0);
	return (BYTE *)lp;
}

// ----------------------------------------------------------

FREE_IMAGE_COLOR_TYPE DLL_CALLCONV
FreeImage_GetColorType(FIBITMAP *dib) {
	RGBQUAD *rgb;

	const FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(dib);

	// special bitmap type
	if(image_type != FIT_BITMAP) {
		switch(image_type) {
			case FIT_UINT16:
				return FIC_MINISBLACK;
			break;
			case FIT_RGB16:
			case FIT_RGBF:
				return FIC_RGB;
			case FIT_RGBA16:
			case FIT_RGBAF:
				return FIC_RGBALPHA;
		}

		return FIC_MINISBLACK;
	}

	// standard image type
	switch (FreeImage_GetBPP(dib)) {
		case 1:
		{
			rgb = FreeImage_GetPalette(dib);

			if ((rgb->rgbRed == 0) && (rgb->rgbGreen == 0) && (rgb->rgbBlue == 0)) {
				rgb++;

				if ((rgb->rgbRed == 255) && (rgb->rgbGreen == 255) && (rgb->rgbBlue == 255)) {
					return FIC_MINISBLACK;
				}
			}

			if ((rgb->rgbRed == 255) && (rgb->rgbGreen == 255) && (rgb->rgbBlue == 255)) {
				rgb++;

				if ((rgb->rgbRed == 0) && (rgb->rgbGreen == 0) && (rgb->rgbBlue == 0)) {
					return FIC_MINISWHITE;
				}
			}

			return FIC_PALETTE;
		}

		case 4:
		case 8:	// Check if the DIB has a color or a greyscale palette
		{
			int ncolors = FreeImage_GetColorsUsed(dib);
		    int minisblack = 1;
			rgb = FreeImage_GetPalette(dib);

			for (int i = 0; i < ncolors; i++) {
				if ((rgb->rgbRed != rgb->rgbGreen) || (rgb->rgbRed != rgb->rgbBlue)) {
					return FIC_PALETTE;
				}

				// The DIB has a color palette if the greyscale isn't a linear ramp
				// Take care of reversed grey images
				if (rgb->rgbRed != i) {
					if ((ncolors-i-1) != rgb->rgbRed) {
						return FIC_PALETTE;
					} else {
						minisblack = 0;
					}
				}

				rgb++;
			}

			return minisblack ? FIC_MINISBLACK : FIC_MINISWHITE;
		}

		case 16:
		case 24:
			return FIC_RGB;

		case 32:
		{
			if( FreeImage_HasPixels(dib) ) {
				// check for fully opaque alpha layer
				for (unsigned y = 0; y < FreeImage_GetHeight(dib); y++) {
					rgb = (RGBQUAD *)FreeImage_GetScanLine(dib, y);

					for (unsigned x = 0; x < FreeImage_GetWidth(dib); x++) {
						if (rgb[x].rgbReserved != 0xFF) {
							return FIC_RGBALPHA;
						}
					}
				}
				return FIC_RGB;
			}

			return FIC_RGBALPHA;
		}
				
		default :
			return FIC_MINISBLACK;
	}
}

// ----------------------------------------------------------

FREE_IMAGE_TYPE DLL_CALLCONV 
FreeImage_GetImageType(FIBITMAP *dib) {
	return (dib != NULL) ? ((FREEIMAGEHEADER *)dib->data)->type : FIT_UNKNOWN;
}

// ----------------------------------------------------------

BOOL DLL_CALLCONV 
FreeImage_HasPixels(FIBITMAP *dib) {
	return (dib != NULL) ? ((FREEIMAGEHEADER *)dib->data)->has_pixels : FALSE;
}

// ----------------------------------------------------------

BOOL DLL_CALLCONV
FreeImage_HasRGBMasks(FIBITMAP *dib) {
	return dib && FreeImage_GetInfoHeader(dib)->biCompression == BI_BITFIELDS;
}

unsigned DLL_CALLCONV
FreeImage_GetRedMask(FIBITMAP *dib) {
	FREEIMAGERGBMASKS *masks = NULL;
	FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(dib);
	switch(image_type) {
		case FIT_BITMAP:
			// check for 16-bit RGB (565 or 555)
			masks = FreeImage_GetRGBMasks(dib);
			if (masks) {
				return masks->red_mask;
			}
			return FreeImage_GetBPP(dib) >= 24 ? FI_RGBA_RED_MASK : 0;
		default:
			return 0;
	}
}

unsigned DLL_CALLCONV
FreeImage_GetGreenMask(FIBITMAP *dib) {
	FREEIMAGERGBMASKS *masks = NULL;
	FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(dib);
	switch(image_type) {
		case FIT_BITMAP:
			// check for 16-bit RGB (565 or 555)
			masks = FreeImage_GetRGBMasks(dib);
			if (masks) {
				return masks->green_mask;
			}
			return FreeImage_GetBPP(dib) >= 24 ? FI_RGBA_GREEN_MASK : 0;
		default:
			return 0;
	}
}

unsigned DLL_CALLCONV
FreeImage_GetBlueMask(FIBITMAP *dib) {
	FREEIMAGERGBMASKS *masks = NULL;
	FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(dib);
	switch(image_type) {
		case FIT_BITMAP:
			// check for 16-bit RGB (565 or 555)
			masks = FreeImage_GetRGBMasks(dib);
			if (masks) {
				return masks->blue_mask;
			}
			return FreeImage_GetBPP(dib) >= 24 ? FI_RGBA_BLUE_MASK : 0;
		default:
			return 0;
	}
}

// ----------------------------------------------------------

BOOL DLL_CALLCONV
FreeImage_HasBackgroundColor(FIBITMAP *dib) {
	if(dib) {
		RGBQUAD *bkgnd_color = &((FREEIMAGEHEADER *)dib->data)->bkgnd_color;
		return (bkgnd_color->rgbReserved != 0) ? TRUE : FALSE;
	}
	return FALSE;
}

BOOL DLL_CALLCONV
FreeImage_GetBackgroundColor(FIBITMAP *dib, RGBQUAD *bkcolor) {
	if(dib && bkcolor) {
		if(FreeImage_HasBackgroundColor(dib)) {
			// get the background color
			RGBQUAD *bkgnd_color = &((FREEIMAGEHEADER *)dib->data)->bkgnd_color;
			memcpy(bkcolor, bkgnd_color, sizeof(RGBQUAD));
			// get the background index
			if(FreeImage_GetBPP(dib) == 8) {
				RGBQUAD *pal = FreeImage_GetPalette(dib);
				for(unsigned i = 0; i < FreeImage_GetColorsUsed(dib); i++) {
					if(bkgnd_color->rgbRed == pal[i].rgbRed) {
						if(bkgnd_color->rgbGreen == pal[i].rgbGreen) {
							if(bkgnd_color->rgbBlue == pal[i].rgbBlue) {
								bkcolor->rgbReserved = (BYTE)i;
								return TRUE;
							}
						}
					}
				}
			}

			bkcolor->rgbReserved = 0;

			return TRUE;
		}
	}

	return FALSE;
}

BOOL DLL_CALLCONV 
FreeImage_SetBackgroundColor(FIBITMAP *dib, RGBQUAD *bkcolor) {
	if(dib) {
		RGBQUAD *bkgnd_color = &((FREEIMAGEHEADER *)dib->data)->bkgnd_color;
		if(bkcolor) {
			// set the background color
			memcpy(bkgnd_color, bkcolor, sizeof(RGBQUAD));
			// enable the file background color
			bkgnd_color->rgbReserved = 1;
		} else {
			// clear and disable the file background color
			memset(bkgnd_color, 0, sizeof(RGBQUAD));
		}
		return TRUE;
	}

	return FALSE;
}

// ----------------------------------------------------------

BOOL DLL_CALLCONV
FreeImage_IsTransparent(FIBITMAP *dib) {
	if(dib) {
		FREE_IMAGE_TYPE image_type = FreeImage_GetImageType(dib);
		switch(image_type) {
			case FIT_BITMAP:
				if(FreeImage_GetBPP(dib) == 32) {
					if(FreeImage_GetColorType(dib) == FIC_RGBALPHA) {
						return TRUE;
					}
				} else {
					return ((FREEIMAGEHEADER *)dib->data)->transparent ? TRUE : FALSE;
				}
				break;
			case FIT_RGBA16:
			case FIT_RGBAF:
				return TRUE;
			default:
				break;
		}
	}
	return FALSE;
}

BYTE * DLL_CALLCONV
FreeImage_GetTransparencyTable(FIBITMAP *dib) {
	return dib ? ((FREEIMAGEHEADER *)dib->data)->transparent_table : NULL;
}

void DLL_CALLCONV
FreeImage_SetTransparent(FIBITMAP *dib, BOOL enabled) {
	if (dib) {
		if ((FreeImage_GetBPP(dib) <= 8) || (FreeImage_GetBPP(dib) == 32)) {
			((FREEIMAGEHEADER *)dib->data)->transparent = enabled;
		} else {
			((FREEIMAGEHEADER *)dib->data)->transparent = FALSE;
		}
	}
}

unsigned DLL_CALLCONV
FreeImage_GetTransparencyCount(FIBITMAP *dib) {
	return dib ? ((FREEIMAGEHEADER *)dib->data)->transparency_count : 0;
}

void DLL_CALLCONV
FreeImage_SetTransparencyTable(FIBITMAP *dib, BYTE *table, int count) {
	if (dib) {
		count = MAX(0, MIN(count, 256));
		if (FreeImage_GetBPP(dib) <= 8) {
			((FREEIMAGEHEADER *)dib->data)->transparent = (count > 0) ? TRUE : FALSE;
			((FREEIMAGEHEADER *)dib->data)->transparency_count = count;

			if (table) {
				memcpy(((FREEIMAGEHEADER *)dib->data)->transparent_table, table, count);
			} else {
				memset(((FREEIMAGEHEADER *)dib->data)->transparent_table, 0xff, count);
			}
		} 
	}
}

/** @brief Sets the index of the palette entry to be used as transparent color
 for the image specified. Does nothing on high color images. 
 
 This method sets the index of the palette entry to be used as single transparent
 color for the image specified. This works on palletised images only and does
 nothing for high color images.
 
 Although it is possible for palletised images to have more than one transparent
 color, this method sets the palette entry specified as the single transparent
 color for the image. All other colors will be set to be non-transparent by this
 method.
 
 As with FreeImage_SetTransparencyTable(), this method also sets the image's
 transparency property to TRUE (as it is set and obtained by
 FreeImage_SetTransparent() and FreeImage_IsTransparent() respectively) for
 palletised images.
 
 @param dib Input image, whose transparent color is to be set.
 @param index The index of the palette entry to be set as transparent color.
 */
void DLL_CALLCONV
FreeImage_SetTransparentIndex(FIBITMAP *dib, int index) {
	if (dib) {
		int count = FreeImage_GetColorsUsed(dib);
		if (count) {
			BYTE *new_tt = (BYTE *)malloc(count * sizeof(BYTE));
			memset(new_tt, 0xFF, count);
			if ((index >= 0) && (index < count)) {
				new_tt[index] = 0x00;
			}
			FreeImage_SetTransparencyTable(dib, new_tt, count);
			free(new_tt);
		}
	}
}

/** @brief Returns the palette entry used as transparent color for the image
 specified. Works for palletised images only and returns -1 for high color
 images or if the image has no color set to be transparent. 
 
 Although it is possible for palletised images to have more than one transparent
 color, this function always returns the index of the first palette entry, set
 to be transparent. 
 
 @param dib Input image, whose transparent color is to be returned.
 @return Returns the index of the palette entry used as transparent color for
 the image specified or -1 if there is no transparent color found (e.g. the image
 is a high color image).
 */
int DLL_CALLCONV
FreeImage_GetTransparentIndex(FIBITMAP *dib) {
	int count = FreeImage_GetTransparencyCount(dib);
	BYTE *tt = FreeImage_GetTransparencyTable(dib);
	for (int i = 0; i < count; i++) {
		if (tt[i] == 0) {
			return i;
		}
	}
	return -1;
}

// ----------------------------------------------------------

unsigned DLL_CALLCONV
FreeImage_GetWidth(FIBITMAP *dib) {
	return dib ? FreeImage_GetInfoHeader(dib)->biWidth : 0;
}

unsigned DLL_CALLCONV
FreeImage_GetHeight(FIBITMAP *dib) {
	return (dib) ? FreeImage_GetInfoHeader(dib)->biHeight : 0;
}

unsigned DLL_CALLCONV
FreeImage_GetBPP(FIBITMAP *dib) {
	return dib ? FreeImage_GetInfoHeader(dib)->biBitCount : 0;
}

unsigned DLL_CALLCONV
FreeImage_GetLine(FIBITMAP *dib) {
	return dib ? ((FreeImage_GetWidth(dib) * FreeImage_GetBPP(dib)) + 7) / 8 : 0;
}

unsigned DLL_CALLCONV
FreeImage_GetPitch(FIBITMAP *dib) {
	if(dib) {
		FREEIMAGEHEADER *fih = (FREEIMAGEHEADER *)dib->data;
		return fih->external_bits ? fih->external_pitch : (FreeImage_GetLine(dib) + 3 & ~3);
	}
	return 0;
}

unsigned DLL_CALLCONV
FreeImage_GetColorsUsed(FIBITMAP *dib) {
	return dib ? FreeImage_GetInfoHeader(dib)->biClrUsed : 0;
}

unsigned DLL_CALLCONV
FreeImage_GetDIBSize(FIBITMAP *dib) {
	return (dib) ? sizeof(BITMAPINFOHEADER) + (FreeImage_GetColorsUsed(dib) * sizeof(RGBQUAD)) + (FreeImage_GetPitch(dib) * FreeImage_GetHeight(dib)) : 0;
}

RGBQUAD * DLL_CALLCONV
FreeImage_GetPalette(FIBITMAP *dib) {
	return (dib && FreeImage_GetBPP(dib) < 16) ? (RGBQUAD *)(((BYTE *)FreeImage_GetInfoHeader(dib)) + sizeof(BITMAPINFOHEADER)) : NULL;
}

unsigned DLL_CALLCONV
FreeImage_GetDotsPerMeterX(FIBITMAP *dib) {
	return (dib) ? FreeImage_GetInfoHeader(dib)->biXPelsPerMeter : 0;
}

unsigned DLL_CALLCONV
FreeImage_GetDotsPerMeterY(FIBITMAP *dib) {
	return (dib) ? FreeImage_GetInfoHeader(dib)->biYPelsPerMeter : 0;
}

void DLL_CALLCONV
FreeImage_SetDotsPerMeterX(FIBITMAP *dib, unsigned res) {
	if(dib) {
		FreeImage_GetInfoHeader(dib)->biXPelsPerMeter = res;
	}
}

void DLL_CALLCONV
FreeImage_SetDotsPerMeterY(FIBITMAP *dib, unsigned res) {
	if(dib) {
		FreeImage_GetInfoHeader(dib)->biYPelsPerMeter = res;
	}
}

BITMAPINFOHEADER * DLL_CALLCONV
FreeImage_GetInfoHeader(FIBITMAP *dib) {
	if(!dib) {
		return NULL;
	}
	size_t lp = (size_t)dib->data + sizeof(FREEIMAGEHEADER);
	lp += (lp % FIBITMAP_ALIGNMENT ? FIBITMAP_ALIGNMENT - lp % FIBITMAP_ALIGNMENT : 0);
	lp += FIBITMAP_ALIGNMENT - sizeof(BITMAPINFOHEADER) % FIBITMAP_ALIGNMENT;
	return (BITMAPINFOHEADER *)lp;
}

BITMAPINFO * DLL_CALLCONV
FreeImage_GetInfo(FIBITMAP *dib) {
	return (BITMAPINFO *)FreeImage_GetInfoHeader(dib);
}

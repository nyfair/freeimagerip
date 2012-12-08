// ==========================================================
// Upsampling / downsampling routine
//
// Design and implementation by
// - Hervé Drolon (drolon@infonie.fr)
// - Carsten Klein (cklein05@users.sourceforge.net)
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

#include "Resize.h"

FIBITMAP * DLL_CALLCONV 
FreeImage_Rescale(FIBITMAP *src, int dst_width, int dst_height, FREE_IMAGE_FILTER filter) {
	FIBITMAP *dst = NULL;

	if (!FreeImage_HasPixels(src) || (dst_width <= 0) || (dst_height <= 0) || (FreeImage_GetWidth(src) <= 0) || (FreeImage_GetHeight(src) <= 0)) {
		return NULL;
	}

	// select the filter
	CGenericFilter *pFilter = NULL;
	switch (filter) {
		case FILTER_BOX:
			pFilter = new(std::nothrow) CBoxFilter();
			break;
		case FILTER_BICUBIC:
			pFilter = new(std::nothrow) CBicubicFilter();
			break;
		case FILTER_BILINEAR:
			pFilter = new(std::nothrow) CBilinearFilter();
			break;
		case FILTER_BSPLINE:
			pFilter = new(std::nothrow) CBSplineFilter();
			break;
		case FILTER_CATMULLROM:
			pFilter = new(std::nothrow) CCatmullRomFilter();
			break;
		case FILTER_LANCZOS3:
			pFilter = new(std::nothrow) CLanczos3Filter();
			break;
	}

	if (!pFilter) {
		return NULL;
	}

	CResizeEngine Engine(pFilter);

	dst = Engine.scale(src, dst_width, dst_height, 0, 0,
			FreeImage_GetWidth(src), FreeImage_GetHeight(src));

	delete pFilter;
	
	return dst;
}

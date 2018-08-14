// ==========================================================
// JPEG Loader and writer
// Based on code developed by The Independent JPEG Group
//
// Design and implementation by
// - Floris van den Berg (flvdberg@wxs.nl)
// - Jan L. Nauta (jln@magentammt.com)
// - Markus Loibl (markus.loibl@epost.de)
// - Karl-Heinz Bussian (khbussian@moss.de)
// - Hervé Drolon (drolon@infonie.fr)
// - Jascha Wetzel (jascha@mainia.de)
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

extern "C" {
#define XMD_H
#undef FAR
#include <setjmp.h>

#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"
}

#include "FreeImage.h"
#include "Utilities.h"


// ==========================================================
// Plugin Interface
// ==========================================================

static int s_format_id;

// ----------------------------------------------------------
//   Constant declarations
// ----------------------------------------------------------

#define INPUT_BUF_SIZE  4096	// choose an efficiently fread'able size 
#define OUTPUT_BUF_SIZE 4096    // choose an efficiently fwrite'able size

// ----------------------------------------------------------
//   Typedef declarations
// ----------------------------------------------------------

typedef struct tagErrorManager {
	/// "public" fields
	struct jpeg_error_mgr pub;
	/// for return to caller
	jmp_buf setjmp_buffer;
} ErrorManager;

typedef struct tagSourceManager {
	/// public fields
	struct jpeg_source_mgr pub;
	/// source stream
	fi_handle infile;
	FreeImageIO *m_io;
	/// start of buffer
	JOCTET * buffer;
	/// have we gotten any data yet ?
	boolean start_of_file;
} SourceManager;

typedef struct tagDestinationManager {
	/// public fields
	struct jpeg_destination_mgr pub;
	/// destination stream
	fi_handle outfile;
	FreeImageIO *m_io;
	/// start of buffer
	JOCTET * buffer;
} DestinationManager;

typedef SourceManager*		freeimage_src_ptr;
typedef DestinationManager* freeimage_dst_ptr;
typedef ErrorManager*		freeimage_error_ptr;

// ----------------------------------------------------------
//   Error handling
// ----------------------------------------------------------

/** Fatal errors (print message and exit) */
static inline void
JPEG_EXIT(j_common_ptr cinfo, int code) {
	freeimage_error_ptr error_ptr = (freeimage_error_ptr)cinfo->err;
	error_ptr->pub.msg_code = code;
	error_ptr->pub.error_exit(cinfo);
}

/** Nonfatal errors (we can keep going, but the data is probably corrupt) */
static inline void
JPEG_WARNING(j_common_ptr cinfo, int code) {
	freeimage_error_ptr error_ptr = (freeimage_error_ptr)cinfo->err;
	error_ptr->pub.msg_code = code;
	error_ptr->pub.emit_message(cinfo, -1);
}

/**
	Receives control for a fatal error.  Information sufficient to
	generate the error message has been stored in cinfo->err; call
	output_message to display it.  Control must NOT return to the caller;
	generally this routine will exit() or longjmp() somewhere.
*/
METHODDEF(void)
jpeg_error_exit (j_common_ptr cinfo) {
	freeimage_error_ptr error_ptr = (freeimage_error_ptr)cinfo->err;

	// always display the message
	error_ptr->pub.output_message(cinfo);

	// allow JPEG with unknown markers
	if(error_ptr->pub.msg_code != JERR_UNKNOWN_MARKER) {
	
		// let the memory manager delete any temp files before we die
		jpeg_destroy(cinfo);
		
		// return control to the setjmp point
		longjmp(error_ptr->setjmp_buffer, 1);		
	}
}

/**
	Actual output of any JPEG message.  Note that this method does not know
	how to generate a message, only where to send it.
*/
METHODDEF(void)
jpeg_output_message (j_common_ptr cinfo) {
	char buffer[JMSG_LENGTH_MAX];
	freeimage_error_ptr error_ptr = (freeimage_error_ptr)cinfo->err;

	// create the message
	error_ptr->pub.format_message(cinfo, buffer);
	// send it to user's message proc
	FreeImage_OutputMessageProc(s_format_id, buffer);
}

// ----------------------------------------------------------
//   Destination manager
// ----------------------------------------------------------

/**
	Initialize destination.  This is called by jpeg_start_compress()
	before any data is actually written. It must initialize
	next_output_byte and free_in_buffer. free_in_buffer must be
	initialized to a positive value.
*/
METHODDEF(void)
init_destination (j_compress_ptr cinfo) {
	freeimage_dst_ptr dest = (freeimage_dst_ptr) cinfo->dest;

	dest->buffer = (JOCTET *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  OUTPUT_BUF_SIZE * sizeof(JOCTET));

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

/**
	This is called whenever the buffer has filled (free_in_buffer
	reaches zero). In typical applications, it should write out the
	*entire* buffer (use the saved start address and buffer length;
	ignore the current state of next_output_byte and free_in_buffer).
	Then reset the pointer & count to the start of the buffer, and
	return TRUE indicating that the buffer has been dumped.
	free_in_buffer must be set to a positive value when TRUE is
	returned.  A FALSE return should only be used when I/O suspension is
	desired.
*/
METHODDEF(boolean)
empty_output_buffer (j_compress_ptr cinfo) {
	freeimage_dst_ptr dest = (freeimage_dst_ptr) cinfo->dest;

	if (dest->m_io->write_proc(dest->buffer, 1, OUTPUT_BUF_SIZE, dest->outfile) != OUTPUT_BUF_SIZE) {
		// let the memory manager delete any temp files before we die
		jpeg_destroy((j_common_ptr)cinfo);

		JPEG_EXIT((j_common_ptr)cinfo, JERR_FILE_WRITE);
	}

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

	return TRUE;
}

/**
	Terminate destination --- called by jpeg_finish_compress() after all
	data has been written.  In most applications, this must flush any
	data remaining in the buffer.  Use either next_output_byte or
	free_in_buffer to determine how much data is in the buffer.
*/
METHODDEF(void)
term_destination (j_compress_ptr cinfo) {
	freeimage_dst_ptr dest = (freeimage_dst_ptr) cinfo->dest;

	size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

	// write any data remaining in the buffer

	if (datacount > 0) {
		if (dest->m_io->write_proc(dest->buffer, 1, (unsigned int)datacount, dest->outfile) != datacount) {
			// let the memory manager delete any temp files before we die
			jpeg_destroy((j_common_ptr)cinfo);
			
			JPEG_EXIT((j_common_ptr)cinfo, JERR_FILE_WRITE);
		}
	}
}

// ----------------------------------------------------------
//   Source manager
// ----------------------------------------------------------

/**
	Initialize source.  This is called by jpeg_read_header() before any
	data is actually read. Unlike init_destination(), it may leave
	bytes_in_buffer set to 0 (in which case a fill_input_buffer() call
	will occur immediately).
*/
METHODDEF(void)
init_source (j_decompress_ptr cinfo) {
	freeimage_src_ptr src = (freeimage_src_ptr) cinfo->src;

	/* We reset the empty-input-file flag for each image,
 	 * but we don't clear the input buffer.
	 * This is correct behavior for reading a series of images from one source.
	*/

	src->start_of_file = TRUE;
}

/**
	This is called whenever bytes_in_buffer has reached zero and more
	data is wanted.  In typical applications, it should read fresh data
	into the buffer (ignoring the current state of next_input_byte and
	bytes_in_buffer), reset the pointer & count to the start of the
	buffer, and return TRUE indicating that the buffer has been reloaded.
	It is not necessary to fill the buffer entirely, only to obtain at
	least one more byte.  bytes_in_buffer MUST be set to a positive value
	if TRUE is returned.  A FALSE return should only be used when I/O
	suspension is desired.
*/
METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo) {
	freeimage_src_ptr src = (freeimage_src_ptr) cinfo->src;

	size_t nbytes = src->m_io->read_proc(src->buffer, 1, INPUT_BUF_SIZE, src->infile);

	if (nbytes <= 0) {
		if (src->start_of_file)	{
			// treat empty input file as fatal error

			// let the memory manager delete any temp files before we die
			jpeg_destroy((j_common_ptr)cinfo);

			JPEG_EXIT((j_common_ptr)cinfo, JERR_INPUT_EMPTY);
		}

		JPEG_WARNING((j_common_ptr)cinfo, JWRN_JPEG_EOF);

		/* Insert a fake EOI marker */

		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;

		nbytes = 2;
	}

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = FALSE;

	return TRUE;
}

/**
	Skip num_bytes worth of data.  The buffer pointer and count should
	be advanced over num_bytes input bytes, refilling the buffer as
	needed. This is used to skip over a potentially large amount of
	uninteresting data (such as an APPn marker). In some applications
	it may be possible to optimize away the reading of the skipped data,
	but it's not clear that being smart is worth much trouble; large
	skips are uncommon.  bytes_in_buffer may be zero on return.
	A zero or negative skip count should be treated as a no-op.
*/
METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes) {
	freeimage_src_ptr src = (freeimage_src_ptr) cinfo->src;

	/* Just a dumb implementation for now.  Could use fseek() except
     * it doesn't work on pipes.  Not clear that being smart is worth
	 * any trouble anyway --- large skips are infrequent.
	*/

	if (num_bytes > 0) {
		while (num_bytes > (long) src->pub.bytes_in_buffer) {
		  num_bytes -= (long) src->pub.bytes_in_buffer;

		  (void) fill_input_buffer(cinfo);

		  /* note we assume that fill_input_buffer will never return FALSE,
		   * so suspension need not be handled.
		   */
		}

		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

/**
	Terminate source --- called by jpeg_finish_decompress
	after all data has been read.  Often a no-op.

	NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
	application must deal with any cleanup that should happen even
	for error exit.
*/
METHODDEF(void)
term_source (j_decompress_ptr cinfo) {
  // no work necessary here
}

// ----------------------------------------------------------
//   Source manager & Destination manager setup
// ----------------------------------------------------------

/**
	Prepare for input from a stdio stream.
	The caller must have already opened the stream, and is responsible
	for closing it after finishing decompression.
*/
GLOBAL(void)
jpeg_freeimage_src (j_decompress_ptr cinfo, fi_handle infile, FreeImageIO *io) {
	freeimage_src_ptr src;

	// allocate memory for the buffer. is released automatically in the end

	if (cinfo->src == NULL) {
		cinfo->src = (struct jpeg_source_mgr *) (*cinfo->mem->alloc_small)
			((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(SourceManager));

		src = (freeimage_src_ptr) cinfo->src;

		src->buffer = (JOCTET *) (*cinfo->mem->alloc_small)
			((j_common_ptr) cinfo, JPOOL_PERMANENT, INPUT_BUF_SIZE * sizeof(JOCTET));
	}

	// initialize the jpeg pointer struct with pointers to functions

	src = (freeimage_src_ptr) cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; // use default method 
	src->pub.term_source = term_source;
	src->infile = infile;
	src->m_io = io;
	src->pub.bytes_in_buffer = 0;		// forces fill_input_buffer on first read 
	src->pub.next_input_byte = NULL;	// until buffer loaded 
}

/**
	Prepare for output to a stdio stream.
	The caller must have already opened the stream, and is responsible
	for closing it after finishing compression.
*/
GLOBAL(void)
jpeg_freeimage_dst (j_compress_ptr cinfo, fi_handle outfile, FreeImageIO *io) {
	freeimage_dst_ptr dest;

	if (cinfo->dest == NULL) {
		cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)
			((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(DestinationManager));
	}

	dest = (freeimage_dst_ptr) cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->outfile = outfile;
	dest->m_io = io;
}

// ----------------------------------------------------------
//   Special markers read functions
// ----------------------------------------------------------

/**
	Read JPEG_COM marker (comment)
*/
static BOOL 
jpeg_read_comment(FIBITMAP *dib, const BYTE *dataptr, unsigned int datalen) {
	size_t length = datalen;
	BYTE *profile = (BYTE*)dataptr;

	// read the comment
	char *value = (char*)malloc((length + 1) * sizeof(char));
	if(value == NULL) return FALSE;
	memcpy(value, profile, length);
	value[length] = '\0';

	free(value);

	return TRUE;
}


// ==========================================================
// Plugin Implementation
// ==========================================================

static const char * DLL_CALLCONV
Format() {
	return "JPEG";
}

static const char * DLL_CALLCONV
Description() {
	return "JPEG - JFIF Compliant";
}

static const char * DLL_CALLCONV
Extension() {
	return "jpg,jif,jpeg,jpe";
}

static const char * DLL_CALLCONV
RegExpr() {
	return "^\377\330\377";
}

static const char * DLL_CALLCONV
MimeType() {
	return "image/jpeg";
}

static BOOL DLL_CALLCONV
Validate(FreeImageIO *io, fi_handle handle) {
	BYTE jpeg_signature[] = { 0xFF, 0xD8 };
	BYTE signature[2] = { 0, 0 };

	io->read_proc(signature, 1, sizeof(jpeg_signature), handle);

	return (memcmp(jpeg_signature, signature, sizeof(jpeg_signature)) == 0);
}

static BOOL DLL_CALLCONV
SupportsExportDepth(int depth) {
	return (
			(depth == 8) ||
			(depth == 24)
		);
}

static BOOL DLL_CALLCONV 
SupportsExportType(FREE_IMAGE_TYPE type) {
	return (type == FIT_BITMAP) ? TRUE : FALSE;
}

static BOOL DLL_CALLCONV
SupportsNoPixels() {
	return TRUE;
}

// ----------------------------------------------------------

static FIBITMAP * DLL_CALLCONV
Load(FreeImageIO *io, fi_handle handle, int flags, void *data) {
	if (handle) {
		FIBITMAP *dib = NULL;

		BOOL header_only = (flags & FIF_LOAD_NOPIXELS) == FIF_LOAD_NOPIXELS;

		// set up the jpeglib structures

		struct jpeg_decompress_struct cinfo;
		ErrorManager fi_error_mgr;

		try {

			// step 1: allocate and initialize JPEG decompression object

			// we set up the normal JPEG error routines, then override error_exit & output_message
			cinfo.err = jpeg_std_error(&fi_error_mgr.pub);
			fi_error_mgr.pub.error_exit     = jpeg_error_exit;
			fi_error_mgr.pub.output_message = jpeg_output_message;
			
			// establish the setjmp return context for jpeg_error_exit to use
			if (setjmp(fi_error_mgr.setjmp_buffer)) {
				// If we get here, the JPEG code has signaled an error.
				// We need to clean up the JPEG object, close the input file, and return.
				jpeg_destroy_decompress(&cinfo);
				throw (const char*)NULL;
			}

			jpeg_create_decompress(&cinfo);

			// step 2a: specify data source (eg, a handle)

			jpeg_freeimage_src(&cinfo, handle, io);

			// step 2b: save special markers for later reading
			
			jpeg_save_markers(&cinfo, JPEG_COM, 0xFFFF);
			for(int m = 0; m < 16; m++) {
				jpeg_save_markers(&cinfo, JPEG_APP0 + m, 0xFFFF);
			}

			// step 3: read handle parameters with jpeg_read_header()

			jpeg_read_header(&cinfo, TRUE);

			// step 4: set parameters for decompression

			unsigned int scale_denom = 1;		// fraction by which to scale image
			int	requested_size = flags >> 16;	// requested user size in pixels
			if(requested_size > 0) {
				// the JPEG codec can perform x2, x4 or x8 scaling on loading
				// try to find the more appropriate scaling according to user's need
				double scale = MAX((double)cinfo.image_width, (double)cinfo.image_height) / (double)requested_size;
				if(scale >= 8) {
					scale_denom = 8;
				} else if(scale >= 4) {
					scale_denom = 4;
				} else if(scale >= 2) {
					scale_denom = 2;
				}
			}
			cinfo.scale_num = 1;
			cinfo.scale_denom = scale_denom;

			if ((flags & JPEG_ACCURATE) != JPEG_ACCURATE) {
				cinfo.dct_method          = JDCT_IFAST;
				cinfo.do_fancy_upsampling = FALSE;
			}

			// step 5a: start decompressor and calculate output width and height

			jpeg_start_decompress(&cinfo);

			// step 5b: allocate dib and init header

			if((cinfo.output_components == 4) && (cinfo.out_color_space == JCS_CMYK)) {
				// CMYK image
				// load as CMYK and convert to RGB
				dib = FreeImage_AllocateHeader(header_only, cinfo.output_width, cinfo.output_height, 24, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
				if(!dib) throw FI_MSG_ERROR_DIB_MEMORY;
			} else {
				// RGB or greyscale image
				dib = FreeImage_AllocateHeader(header_only, cinfo.output_width, cinfo.output_height, 8 * cinfo.output_components, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
				if(!dib) throw FI_MSG_ERROR_DIB_MEMORY;

				if (cinfo.output_components == 1) {
					// build a greyscale palette
					RGBQUAD *colors = FreeImage_GetPalette(dib);

					for (int i = 0; i < 256; i++) {
						colors[i].rgbRed   = (BYTE)i;
						colors[i].rgbGreen = (BYTE)i;
						colors[i].rgbBlue  = (BYTE)i;
					}
				}
			}

			// step 5c: handle metrices

			if (cinfo.density_unit == 1) {
				// dots/inch
				FreeImage_SetDotsPerMeterX(dib, (unsigned) (((float)cinfo.X_density) / 0.0254000 + 0.5));
				FreeImage_SetDotsPerMeterY(dib, (unsigned) (((float)cinfo.Y_density) / 0.0254000 + 0.5));
			} else if (cinfo.density_unit == 2) {
				// dots/cm
				FreeImage_SetDotsPerMeterX(dib, (unsigned) (cinfo.X_density * 100));
				FreeImage_SetDotsPerMeterY(dib, (unsigned) (cinfo.Y_density * 100));
			}
			
			// step 6: read special markers
			
			// --- header only mode => clean-up and return

			if (header_only) {
				// release JPEG decompression object
				jpeg_destroy_decompress(&cinfo);
				// return header data
				return dib;
			}

			// step 7a: while (scan lines remain to be read) jpeg_read_scanlines(...);

			if(cinfo.out_color_space == JCS_CMYK) {
				// convert from CMYK to RGB

				JSAMPARRAY buffer;		// output row buffer
				unsigned row_stride;	// physical row width in output buffer

				// JSAMPLEs per row in output buffer
				row_stride = cinfo.output_width * cinfo.output_components;
				// make a one-row-high sample array that will go away when done with image
				buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

				while (cinfo.output_scanline < cinfo.output_height) {
					JSAMPROW src = buffer[0];
					JSAMPROW dst = FreeImage_GetScanLine(dib, cinfo.output_height - cinfo.output_scanline - 1);

					jpeg_read_scanlines(&cinfo, buffer, 1);

					for(unsigned x = 0; x < cinfo.output_width; x++) {
						WORD K = (WORD)src[3];
						dst[FI_RGBA_RED]   = (BYTE)((K * src[0]) / 255);	// C -> R
						dst[FI_RGBA_GREEN] = (BYTE)((K * src[1]) / 255);	// M -> G
						dst[FI_RGBA_BLUE]  = (BYTE)((K * src[2]) / 255);	// Y -> B
						src += 4;
						dst += 3;
					}
				}
			} else {
				// normal case (RGB or greyscale image)

				while (cinfo.output_scanline < cinfo.output_height) {
					JSAMPROW dst = FreeImage_GetScanLine(dib, cinfo.output_height - cinfo.output_scanline - 1);

					jpeg_read_scanlines(&cinfo, &dst, 1);
				}

				// step 7b: swap red and blue components (see LibJPEG/jmorecfg.h: #define RGB_RED, ...)
				// The default behavior of the JPEG library is kept "as is" because LibTIFF uses 
				// LibJPEG "as is".

#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
				SwapRedBlue32(dib);
#endif
			}

			// step 8: finish decompression

			jpeg_finish_decompress(&cinfo);

			// step 9: release JPEG decompression object

			jpeg_destroy_decompress(&cinfo);

			// everything went well. return the loaded dib

			return dib;

		} catch (const char *text) {
			jpeg_destroy_decompress(&cinfo);
			if(NULL != dib) {
				FreeImage_Unload(dib);
			}
			if(NULL != text) {
				FreeImage_OutputMessageProc(s_format_id, text);
			}
		}
	}

	return NULL;
}

// ----------------------------------------------------------

static BOOL DLL_CALLCONV
Save(FreeImageIO *io, FIBITMAP *dib, fi_handle handle, int flags, void *data) {
	if ((dib) && (handle)) {
		try {
			if(flags == 0) flags = JPEG_QUALITYGOOD | JPEG_SUBSAMPLING_420 | JPEG_PROGRESSIVE;
			// Check dib format

			const char *sError = "only 24/32-bit highcolor or 8-bit greyscale/palette bitmaps can be saved as JPEG";

			FREE_IMAGE_COLOR_TYPE color_type = FreeImage_GetColorType(dib);
			WORD bpp = (WORD)FreeImage_GetBPP(dib);

			if ((bpp != 24) && (bpp != 32) && (bpp != 8)) {
				throw sError;
			}

			if(bpp == 8) {
				// allow grey, reverse grey and palette 
				if ((color_type != FIC_MINISBLACK) && (color_type != FIC_MINISWHITE) && (color_type != FIC_PALETTE)) {
					throw sError;
				}
			}

			FIBITMAP *rawdib = ((bpp == 32) ? FreeImage_ConvertTo24Bits(dib) : dib);

			struct jpeg_compress_struct cinfo;
			ErrorManager fi_error_mgr;

			// Step 1: allocate and initialize JPEG compression object

			// we set up the normal JPEG error routines, then override error_exit & output_message
			cinfo.err = jpeg_std_error(&fi_error_mgr.pub);
			fi_error_mgr.pub.error_exit     = jpeg_error_exit;
			fi_error_mgr.pub.output_message = jpeg_output_message;
			
			// establish the setjmp return context for jpeg_error_exit to use
			if (setjmp(fi_error_mgr.setjmp_buffer)) {
				// If we get here, the JPEG code has signaled an error.
				// We need to clean up the JPEG object, close the input file, and return.
				jpeg_destroy_compress(&cinfo);
				throw (const char*)NULL;
			}

			// Now we can initialize the JPEG compression object

			jpeg_create_compress(&cinfo);

			// Step 2: specify data destination (eg, a file)

			jpeg_freeimage_dst(&cinfo, handle, io);

			// Step 3: set parameters for compression 

			cinfo.image_width = FreeImage_GetWidth(rawdib);
			cinfo.image_height = FreeImage_GetHeight(rawdib);

			switch(color_type) {
				case FIC_MINISBLACK :
				case FIC_MINISWHITE :
					cinfo.in_color_space = JCS_GRAYSCALE;
					cinfo.input_components = 1;
					break;

				default :
					cinfo.in_color_space = JCS_RGB;
					cinfo.input_components = 3;
					break;
			}

			jpeg_set_defaults(&cinfo);

		    // progressive-JPEG support
			if((flags & JPEG_PROGRESSIVE) == JPEG_PROGRESSIVE) {
				jpeg_simple_progression(&cinfo);
			}
			
			// compute optimal Huffman coding tables for the image
			if((flags & JPEG_OPTIMIZE) == JPEG_OPTIMIZE) {
				cinfo.optimize_coding = TRUE;
			}

			// Set JFIF density parameters from the DIB data

			cinfo.X_density = (UINT16) (0.5 + 0.0254 * FreeImage_GetDotsPerMeterX(rawdib));
			cinfo.Y_density = (UINT16) (0.5 + 0.0254 * FreeImage_GetDotsPerMeterY(rawdib));
			cinfo.density_unit = 1;	// dots / inch

			// set subsampling options if required

			if(cinfo.in_color_space == JCS_RGB) {
				if((flags & JPEG_SUBSAMPLING_411) == JPEG_SUBSAMPLING_411) { 
					// 4:1:1 (4x1 1x1 1x1) - CrH 25% - CbH 25% - CrV 100% - CbV 100%
					// the horizontal color resolution is quartered
					cinfo.comp_info[0].h_samp_factor = 4;	// Y 
					cinfo.comp_info[0].v_samp_factor = 1; 
					cinfo.comp_info[1].h_samp_factor = 1;	// Cb 
					cinfo.comp_info[1].v_samp_factor = 1; 
					cinfo.comp_info[2].h_samp_factor = 1;	// Cr 
					cinfo.comp_info[2].v_samp_factor = 1; 
				} else if((flags & JPEG_SUBSAMPLING_420) == JPEG_SUBSAMPLING_420) {
					// 4:2:0 (2x2 1x1 1x1) - CrH 50% - CbH 50% - CrV 50% - CbV 50%
					// the chrominance resolution in both the horizontal and vertical directions is cut in half
					cinfo.comp_info[0].h_samp_factor = 2;	// Y
					cinfo.comp_info[0].v_samp_factor = 2; 
					cinfo.comp_info[1].h_samp_factor = 1;	// Cb
					cinfo.comp_info[1].v_samp_factor = 1; 
					cinfo.comp_info[2].h_samp_factor = 1;	// Cr
					cinfo.comp_info[2].v_samp_factor = 1; 
				} else if((flags & JPEG_SUBSAMPLING_422) == JPEG_SUBSAMPLING_422){ //2x1 (low) 
					// 4:2:2 (2x1 1x1 1x1) - CrH 50% - CbH 50% - CrV 100% - CbV 100%
					// half of the horizontal resolution in the chrominance is dropped (Cb & Cr), 
					// while the full resolution is retained in the vertical direction, with respect to the luminance
					cinfo.comp_info[0].h_samp_factor = 2;	// Y 
					cinfo.comp_info[0].v_samp_factor = 1; 
					cinfo.comp_info[1].h_samp_factor = 1;	// Cb 
					cinfo.comp_info[1].v_samp_factor = 1; 
					cinfo.comp_info[2].h_samp_factor = 1;	// Cr 
					cinfo.comp_info[2].v_samp_factor = 1; 
				} 
				else if((flags & JPEG_SUBSAMPLING_444) == JPEG_SUBSAMPLING_444){ //1x1 (no subsampling) 
					// 4:4:4 (1x1 1x1 1x1) - CrH 100% - CbH 100% - CrV 100% - CbV 100%
					// the resolution of chrominance information (Cb & Cr) is preserved 
					// at the same rate as the luminance (Y) information
					cinfo.comp_info[0].h_samp_factor = 1;	// Y 
					cinfo.comp_info[0].v_samp_factor = 1; 
					cinfo.comp_info[1].h_samp_factor = 1;	// Cb 
					cinfo.comp_info[1].v_samp_factor = 1; 
					cinfo.comp_info[2].h_samp_factor = 1;	// Cr 
					cinfo.comp_info[2].v_samp_factor = 1;  
				} 
			}

			// Step 4: set quality
			// the first 7 bits are reserved for low level quality settings
			// the other bits are high level (i.e. enum-ish)

			int quality;

			if ((flags & JPEG_QUALITYGOOD) == JPEG_QUALITYGOOD) {
				quality = 95;
			} else if ((flags & JPEG_QUALITYNORMAL) == JPEG_QUALITYNORMAL) {
				quality = 75;
			} else if ((flags & JPEG_QUALITYAVERAGE) == JPEG_QUALITYAVERAGE) {
				quality = 50;
			} else if ((flags & JPEG_QUALITYBAD) == JPEG_QUALITYBAD) {
				quality = 10;
			} else 	if ((flags & JPEG_QUALITYSUPERB) == JPEG_QUALITYSUPERB) {
				quality = 100;
			} else {
				quality = flags & 0x7F;
			}

			jpeg_set_quality(&cinfo, quality, TRUE); /* limit to baseline-JPEG values */

			// Step 5: Start compressor 

			jpeg_start_compress(&cinfo, TRUE);

			// Step 6: Write special markers
			
			//if ((flags & JPEG_BASELINE) !=  JPEG_BASELINE) {
			//	write_markers(&cinfo, dib);
			//}

			// Step 7: while (scan lines remain to be written) 

			if(color_type == FIC_RGB || color_type == FIC_RGBALPHA) {
				// 24-bit RGB image : need to swap red and blue channels
				unsigned pitch = FreeImage_GetPitch(rawdib);
				BYTE *target = (BYTE*)malloc(pitch * sizeof(BYTE));
				if (target == NULL) {
					throw FI_MSG_ERROR_MEMORY;
				}

				while (cinfo.next_scanline < cinfo.image_height) {
					// get a copy of the scanline
					memcpy(target, FreeImage_GetScanLine(rawdib, FreeImage_GetHeight(rawdib) - cinfo.next_scanline - 1), pitch);
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
					// swap R and B channels
					BYTE *target_p = target;
					for(unsigned x = 0; x < cinfo.image_width; x++) {
						INPLACESWAP(target_p[0], target_p[2]);
						target_p += 3;
					}
#endif
					// write the scanline
					jpeg_write_scanlines(&cinfo, &target, 1);
				}
				free(target);
			}
			else if(color_type == FIC_MINISBLACK) {
				// 8-bit standard greyscale images
				while (cinfo.next_scanline < cinfo.image_height) {
					JSAMPROW b = FreeImage_GetScanLine(rawdib, FreeImage_GetHeight(rawdib) - cinfo.next_scanline - 1);

					jpeg_write_scanlines(&cinfo, &b, 1);
				}
			}
			else if(color_type == FIC_PALETTE) {
				// 8-bit palettized images are converted to 24-bit images
				RGBQUAD *palette = FreeImage_GetPalette(rawdib);
				BYTE *target = (BYTE*)malloc(cinfo.image_width * 3);
				if (target == NULL) {
					throw FI_MSG_ERROR_MEMORY;
				}

				while (cinfo.next_scanline < cinfo.image_height) {
					BYTE *source = FreeImage_GetScanLine(rawdib, FreeImage_GetHeight(rawdib) - cinfo.next_scanline - 1);
					FreeImage_ConvertLine8To24(target, source, cinfo.image_width, palette);

#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
					// swap R and B channels
					BYTE *target_p = target;
					for(unsigned x = 0; x < cinfo.image_width; x++) {
						INPLACESWAP(target_p[0], target_p[2]);
						target_p += 3;
					}
#endif


					jpeg_write_scanlines(&cinfo, &target, 1);
				}

				free(target);
			}
			else if(color_type == FIC_MINISWHITE) {
				// reverse 8-bit greyscale image, so reverse grey value on the fly
				unsigned i;
				BYTE reverse[256];
				BYTE *target = (BYTE *)malloc(cinfo.image_width);
				if (target == NULL) {
					throw FI_MSG_ERROR_MEMORY;
				}

				for(i = 0; i < 256; i++) {
					reverse[i] = (BYTE)(255 - i);
				}

				while(cinfo.next_scanline < cinfo.image_height) {
					BYTE *source = FreeImage_GetScanLine(rawdib, FreeImage_GetHeight(rawdib) - cinfo.next_scanline - 1);
					for(i = 0; i < cinfo.image_width; i++) {
						target[i] = reverse[ source[i] ];
					}
					jpeg_write_scanlines(&cinfo, &target, 1);
				}

				free(target);
			}

			// Step 8: Finish compression 

			jpeg_finish_compress(&cinfo);

			// Step 9: release JPEG compression object 

			jpeg_destroy_compress(&cinfo);

			if(bpp == 32) FreeImage_Unload(rawdib);
			return TRUE;

		} catch (const char *text) {
			FreeImage_OutputMessageProc(s_format_id, text);
			return FALSE;
		} catch (FREE_IMAGE_FORMAT) {
			return FALSE;
		}
	}

	return FALSE;
}

// ==========================================================
//   Init
// ==========================================================

void DLL_CALLCONV
InitJPEG(Plugin *plugin, int format_id) {
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
	plugin->supports_no_pixels_proc = SupportsNoPixels;
}

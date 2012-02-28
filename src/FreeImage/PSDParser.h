// ==========================================================
// Photoshop Loader
//
// Design and implementation by
// - Hervé Drolon (drolon@infonie.fr)
// - Mihail Naydenov (mnaydenov@users.sourceforge.net)
//
// Based on LGPL code created and published by http://sourceforge.net/projects/elynx/
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

#ifndef PSDPARSER_H
#define PSDPARSER_H

/**
Table 2-12: File header section. 
The file header contains the basic properties of the image. 
*/
typedef struct psdHeader {
	BYTE Signature[4];	//! Always equal 8BPS, do not try to read the file if the signature does not match this value.
	BYTE Version[2];	//! Always equal 1, do not read file if the version does not match this value.
	char Reserved[6];	//! Must be zero.
	BYTE Channels[2];	//! Number of channels including any alpha channels, supported range is 1 to 24.
	BYTE Rows[4];		//! The height of the image in pixels. Supported range is 1 to 30,000.
	BYTE Columns[4];	//! The width of the image in pixels. Supported range is 1 to 30,000.
	BYTE Depth[2];		//! The number of bits per channel. Supported values are 1, 8, and 16.
	BYTE Mode[2];		//! Colour mode of the file, Bitmap=0, Grayscale=1, Indexed=2, RGB=3, CMYK=4, Multichannel=7, Duotone=8, Lab=9. 
} psdHeader;

/**
Table 2-12: HeaderInfo Color spaces
@see psdHeader
*/
class psdHeaderInfo {
public:
	short _Channels;	//! Numer of channels including any alpha channels, supported range is 1 to 24.
	int   _Height;		//! The height of the image in pixels. Supported range is 1 to 30,000.
	int   _Width;		//! The width of the image in pixels. Supported range is 1 to 30,000.
	short _BitsPerChannel;//! The number of bits per channel. Supported values are 1, 8, and 16.
	short _ColourMode;	//! Colour mode of the file, Bitmap=0, Grayscale=1, Indexed=2, RGB=3, CMYK=4, Multichannel=7, Duotone=8, Lab=9. 

public:
	psdHeaderInfo();
	~psdHeaderInfo();
	/**
	@return Returns the number of bytes read
	*/
	bool Read(FreeImageIO *io, fi_handle handle);
};

/**
Table 2-13 Color mode data section

Only indexed color and duotone have color mode data. For all other modes,
this section is just 4 bytes: the length field, which is set to zero.
For indexed color images, the length will be equal to 768, and the color data
will contain the color table for the image, in non-interleaved order.
For duotone images, the color data will contain the duotone specification,
the format of which is not documented. Other applications that read
Photoshop files can treat a duotone image as a grayscale image, and just
preserve the contents of the duotone information when reading and writing
the file.
*/
class psdColourModeData {
public:
	int _Length;			//! The length of the following color data
	BYTE * _plColourData;	//! The color data

public:
	psdColourModeData();
	~psdColourModeData();
	/**
	@return Returns the number of bytes read
	*/
	bool Read(FreeImageIO *io, fi_handle handle);
	bool FillPalette(FIBITMAP *dib);
};

/**
Table 2-1: Image resource block
NB: Resource data is padded to make size even
*/
class psdImageResource {
public:
	int     _Length;
	char    _OSType[4];	//! Photoshop always uses its signature, 8BIM
	short   _ID;		//! Unique identifier. Image resource IDs on page 8
	BYTE * _plName;		//! A pascal string, padded to make size even (a null name consists of two bytes of 0)
	int     _Size;		//! Actual size of resource data. This does not include the Type, ID, Name or Size fields.

public:
	psdImageResource();
	~psdImageResource();
	void Reset();
};

/**
Table A-6: ResolutionInfo structure
This structure contains information about the resolution of an image. It is
written as an image resource. See the Document file formats chapter for more
details.
*/
class psdResolutionInfo {
public:
	short _widthUnit;	//! Display width as 1=inches; 2=cm; 3=points; 4=picas; 5=columns.
	short _heightUnit;	//! Display height as 1=inches; 2=cm; 3=points; 4=picas; 5=columns.
	short _hRes;		//! Horizontal resolution in pixels per inch.
	short _vRes;		//! Vertical resolution in pixels per inch.
	int _hResUnit;		//! 1=display horizontal resolution in pixels per inch; 2=display horizontal resolution in pixels per cm.
	int _vResUnit;		//! 1=display vertical resolution in pixels per inch; 2=display vertical resolution in pixels per cm.

public:
	psdResolutionInfo();
	~psdResolutionInfo();	
	/**
	@return Returns the number of bytes read
	*/
	int Read(FreeImageIO *io, fi_handle handle);
	/**
	@param res_x [out] X resolution in pixels/meter
	@param res_y [out] Y resolution in pixels/meter
	*/
	void GetResolutionInfo(unsigned &res_x, unsigned &res_y);
};

// Obsolete - Photoshop 2.0
class psdResolutionInfo_v2 {
public:
	short _Channels;
	short _Rows;
	short _Columns;
	short _Depth;
	short _Mode;
	
public:
	psdResolutionInfo_v2();
	~psdResolutionInfo_v2();
	/**
	@return Returns the number of bytes read
	*/
	int Read(FreeImageIO *io, fi_handle handle);
};

/**
Table A-7: DisplayInfo Color spaces
This structure contains display information about each channel. It is written as an image resource.
*/
class psdDisplayInfo {
public:
	short _ColourSpace;
	short _Colour[4];
	short _Opacity;  //! 0..100
	BYTE _Kind;     //! selected = 0, protected = 1
	BYTE _padding;  //! should be zero
	
public:
	psdDisplayInfo();
	~psdDisplayInfo();
	/**
	@return Returns the number of bytes read
	*/
	int Read(FreeImageIO *io, fi_handle handle);
};

/**
PSD loader
*/
class psdParser {
private:
	psdHeaderInfo			_headerInfo;
	psdColourModeData		_colourModeData;
	psdResolutionInfo		_resolutionInfo;
	psdResolutionInfo_v2	_resolutionInfo_v2;
	psdDisplayInfo			_displayInfo;

	short _ColourCount;
	short _TransparentIndex;
	int _GlobalAngle;
	bool _bResolutionInfoFilled;
	bool _bResolutionInfoFilled_v2;
	bool _bDisplayInfoFilled;
	bool _bCopyright;

	int _fi_flags;
	int _fi_format_id;
	
private:
	/**	Actually ignore it */
	bool ReadLayerAndMaskInfoSection(FreeImageIO *io, fi_handle handle);
	FIBITMAP* ReadImageData(FreeImageIO *io, fi_handle handle);

public:
	psdParser();
	~psdParser();
	FIBITMAP* Load(FreeImageIO *io, fi_handle handle, int s_format_id, int flags=0);
	/** Also used by the TIFF plugin */
	bool ReadImageResources(FreeImageIO *io, fi_handle handle, LONG length=0);
};

#endif // PSDPARSER_H


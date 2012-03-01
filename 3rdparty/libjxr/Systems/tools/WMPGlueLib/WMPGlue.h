//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <WMPMeta.h>
#include <guiddef.h>

//================================================================
#define WMP_SDK_VERSION 0x0101
#define PK_SDK_VERSION 0x0101

#define sizeof2(array) (sizeof(array)/sizeof(*(array)))
#ifdef __ANSI__
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(b,a) ((a) > (b) ? (a) : (b))
#endif // __ANSI__

//================================================================
typedef struct tagPKRect
{
    I32 X;
    I32 Y;
    I32 Width;
    I32 Height;
} PKRect;

//================================================================
typedef U32 PKIID;

EXTERN_C const PKIID IID_PKImageScanEncode;
EXTERN_C const PKIID IID_PKImageFrameEncode;

EXTERN_C const PKIID IID_PKImageWmpEncode;
EXTERN_C const PKIID IID_PKImageBmpEncode;
EXTERN_C const PKIID IID_PKImagePnmEncode;
EXTERN_C const PKIID IID_PKImageTifEncode;

EXTERN_C const PKIID IID_PKImageWmpDecode;
EXTERN_C const PKIID IID_PKImageBmpDecode;
EXTERN_C const PKIID IID_PKImagePnmDecode;
EXTERN_C const PKIID IID_PKImageTifDecode;

//================================================================
typedef float Float;

typedef enum tagPKStreamFlags
{
    PKStreamOpenRead = 0x00000000UL,
    PKStreamOpenWrite = 0x00000001UL,
    PKStreamOpenReadWrite = 0x00000002UL,
    PKStreamNoLock = 0x00010000UL,
    PKStreamNoSeek = 0x00020000UL,
    PKStreamCompress = 0x00040000UL,
} PKStreamFlags;

/* Undefined formats */
#define GUID_PKPixelFormatUndefined GUID_PKPixelFormatDontCare

DEFINE_GUID(GUID_PKPixelFormatDontCare, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x00);

/* Indexed formats */
//DEFINE_GUID(GUID_PKPixelFormat1bppIndexed, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x01);
//DEFINE_GUID(GUID_PKPixelFormat2bppIndexed, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x02);
//DEFINE_GUID(GUID_PKPixelFormat4bppIndexed, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x03);
//DEFINE_GUID(GUID_PKPixelFormat8bppIndexed, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x04);

DEFINE_GUID(GUID_PKPixelFormatBlackWhite, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x05);
//DEFINE_GUID(GUID_PKPixelFormat2bppGray,   0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x06);
//DEFINE_GUID(GUID_PKPixelFormat4bppGray,   0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x07);
DEFINE_GUID(GUID_PKPixelFormat8bppGray,   0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x08);

/* sRGB formats (gamma is approx. 2.2) */
/* For a full definition, see the sRGB spec */

/* 16bpp formats */
DEFINE_GUID(GUID_PKPixelFormat16bppRGB555, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x09);
DEFINE_GUID(GUID_PKPixelFormat16bppRGB565, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0a);
DEFINE_GUID(GUID_PKPixelFormat16bppGray,   0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0b);

/* 24bpp formats */
DEFINE_GUID(GUID_PKPixelFormat24bppBGR, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0c);
DEFINE_GUID(GUID_PKPixelFormat24bppRGB, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0d);

/* 32bpp format */
DEFINE_GUID(GUID_PKPixelFormat32bppBGR,   0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0e);
DEFINE_GUID(GUID_PKPixelFormat32bppBGRA,  0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0f);
DEFINE_GUID(GUID_PKPixelFormat32bppPBGRA, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x10);
DEFINE_GUID(GUID_PKPixelFormat32bppGrayFloat,  0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x11);

/* 48bpp format */
DEFINE_GUID(GUID_PKPixelFormat48bppRGBFixedPoint, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x12);

/* scRGB formats. Gamma is 1.0 */
/* For a full definition, see the scRGB spec */

/* 16bpp format */
DEFINE_GUID(GUID_PKPixelFormat16bppGrayFixedPoint, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x13);

/* 32bpp format */
DEFINE_GUID(GUID_PKPixelFormat32bppRGB101010, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x14);

/* 48bpp format */
DEFINE_GUID(GUID_PKPixelFormat48bppRGB, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x15);

/* 64bpp format */
DEFINE_GUID(GUID_PKPixelFormat64bppRGBA,  0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x16);
DEFINE_GUID(GUID_PKPixelFormat64bppPRGBA, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x17);

/* 96bpp format */
DEFINE_GUID(GUID_PKPixelFormat96bppRGBFixedPoint, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x18);

 /* Floating point scRGB formats */
DEFINE_GUID(GUID_PKPixelFormat128bppRGBAFloat,  0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x19);
DEFINE_GUID(GUID_PKPixelFormat128bppPRGBAFloat, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x1a);
DEFINE_GUID(GUID_PKPixelFormat128bppRGBFloat,   0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x1b);

 /* CMYK formats. */
DEFINE_GUID(GUID_PKPixelFormat32bppCMYK, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x1c);

 /* Photon formats */
DEFINE_GUID(GUID_PKPixelFormat64bppRGBAFixedPoint, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x1d);
DEFINE_GUID(GUID_PKPixelFormat64bppRGBFixedPoint, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x40);
DEFINE_GUID(GUID_PKPixelFormat128bppRGBAFixedPoint, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x1e);
DEFINE_GUID(GUID_PKPixelFormat128bppRGBFixedPoint, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x41);

DEFINE_GUID(GUID_PKPixelFormat64bppRGBAHalf, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x3a);
DEFINE_GUID(GUID_PKPixelFormat64bppRGBHalf, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x42);
DEFINE_GUID(GUID_PKPixelFormat48bppRGBHalf, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x3b);

DEFINE_GUID(GUID_PKPixelFormat32bppRGBE, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x3d);

DEFINE_GUID(GUID_PKPixelFormat16bppGrayHalf, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x3e);
DEFINE_GUID(GUID_PKPixelFormat32bppGrayFixedPoint, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x3f);


/* More CMYK formats and n-Channel formats */
DEFINE_GUID(GUID_PKPixelFormat64bppCMYK, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x1f);

DEFINE_GUID(GUID_PKPixelFormat24bpp3Channels, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x20);
DEFINE_GUID(GUID_PKPixelFormat32bpp4Channels, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x21);
DEFINE_GUID(GUID_PKPixelFormat40bpp5Channels, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x22);
DEFINE_GUID(GUID_PKPixelFormat48bpp6Channels, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x23);
DEFINE_GUID(GUID_PKPixelFormat56bpp7Channels, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x24);
DEFINE_GUID(GUID_PKPixelFormat64bpp8Channels, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x25);

DEFINE_GUID(GUID_PKPixelFormat48bpp3Channels, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x26);
DEFINE_GUID(GUID_PKPixelFormat64bpp4Channels, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x27);
DEFINE_GUID(GUID_PKPixelFormat80bpp5Channels, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x28);
DEFINE_GUID(GUID_PKPixelFormat96bpp6Channels, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x29);
DEFINE_GUID(GUID_PKPixelFormat112bpp7Channels, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x2a);
DEFINE_GUID(GUID_PKPixelFormat128bpp8Channels, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x2b);

DEFINE_GUID(GUID_PKPixelFormat40bppCMYKAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x2c);
DEFINE_GUID(GUID_PKPixelFormat80bppCMYKAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x2d);

DEFINE_GUID(GUID_PKPixelFormat32bpp3ChannelsAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x2e);
DEFINE_GUID(GUID_PKPixelFormat40bpp4ChannelsAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x2f);
DEFINE_GUID(GUID_PKPixelFormat48bpp5ChannelsAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x30);
DEFINE_GUID(GUID_PKPixelFormat56bpp6ChannelsAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x31);
DEFINE_GUID(GUID_PKPixelFormat64bpp7ChannelsAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x32);
DEFINE_GUID(GUID_PKPixelFormat72bpp8ChannelsAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x33);

DEFINE_GUID(GUID_PKPixelFormat64bpp3ChannelsAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x34);
DEFINE_GUID(GUID_PKPixelFormat80bpp4ChannelsAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x35);
DEFINE_GUID(GUID_PKPixelFormat96bpp5ChannelsAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x36);
DEFINE_GUID(GUID_PKPixelFormat112bpp6ChannelsAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x37);
DEFINE_GUID(GUID_PKPixelFormat128bpp7ChannelsAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x38);
DEFINE_GUID(GUID_PKPixelFormat144bpp8ChannelsAlpha, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x39);

//YUV
DEFINE_GUID(GUID_PKPixelFormat12bppYUV420, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x43);
DEFINE_GUID(GUID_PKPixelFormat16bppYUV422, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x44);
DEFINE_GUID(GUID_PKPixelFormat24bppYUV444, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x45);

DEFINE_GUID(GUID_PKPixelFormat96bppRGB, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x46);
DEFINE_GUID(GUID_PKPixelFormat96bppRGBFloat, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x47);

DEFINE_GUID(GUID_PKPixelFormat32bppRGBA, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x48);
DEFINE_GUID(GUID_PKPixelFormat128bppRGBA, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x49);
//DEFINE_GUID(GUID_PKPixelFormat1bppGray, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x4a);
DEFINE_GUID(GUID_PKPixelFormat32bppGray, 0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x4b);

// PhotometricInterpretation
#define PK_PI_W0 0 // WhiteIsZero
#define PK_PI_B0 1 // BlackIsZero
#define PK_PI_RGB 2
#define PK_PI_RGBPalette 3
#define PK_PI_TransparencyMask 4
#define PK_PI_CMYK 5
#define PK_PI_YCbCr 6
#define PK_PI_CIELab 8

#define PK_PI_NCH  100
#define PK_PI_RGBE 101

#define PK_pixfmtNul           0x00000000
#define PK_pixfmtHasAlpha      0x00000010
#define PK_pixfmtPreMul        0x00000020
#define PK_pixfmtBGR           0x00000040
#define PK_pixfmtNeedConvert   0x80000000

#define LOOKUP_FORWARD         0
#define LOOKUP_BACKWARD_TIF    1

typedef unsigned long WMP_GRBIT;
typedef GUID PKPixelFormatGUID;

typedef struct tagPKPixelInfo
{
    const PKPixelFormatGUID* pGUIDPixFmt;

    size_t cChannel;
    COLORFORMAT cfColorFormat;
    BITDEPTH_BITS bdBitDepth;
    U32 cbitUnit;
    
    WMP_GRBIT grBit;

    // TIFF
    U32 uInterpretation;
    U32 uSamplePerPixel;
    U32 uBitsPerSample;
    U32 uSampleFormat;
} PKPixelInfo;

//================================================================
ERR PKAlloc(void** ppv, size_t cb);
ERR PKFree(void** ppv);

//----------------------------------------------------------------
//ERR GetPixelInfo(PKPixelFormatGUID enPixelFormat, const PKPixelInfo** ppPI);
ERR PixelFormatLookup(PKPixelInfo* pPI, U8 uLookupType);

ERR GetImageEncodeIID(const char* szExt, const PKIID** ppIID);
ERR GetImageDecodeIID(const char* szExt, const PKIID** ppIID);


//================================================================
#ifdef __ANSI__
#define PKFactory           struct tagPKFactory
#define PKCodecFactory      struct tagPKCodecFactory
#define PKImageDecode       struct tagPKImageDecode
#define PKImageEncode       struct tagPKImageEncode
#define PKFormatConverter   struct tagPKFormatConverter
#else // __ANSI__
typedef struct tagPKFactory PKFactory;
typedef struct tagPKCodecFactory PKCodecFactory;
typedef struct tagPKImageDecode PKImageDecode;
typedef struct tagPKImageEncode PKImageEncode;
typedef struct tagPKFormatConverter PKFormatConverter;
#endif // __ANSI__
//================================================================
typedef struct tagPKStream
{
    ERR (*InitializeFromFilename)(const char*, ULong);

    ERR (*Release)(void);

    FILE* fp;
} PKStream;


//================================================================
typedef struct tagPKFactory
{
    ERR (*CreateStream)(PKStream**);

    ERR (*CreateStreamFromFilename)(struct WMPStream**, const char*, const char*);
    ERR (*CreateStreamFromMemory)(struct WMPStream**, void*, size_t);

    ERR (*Release)(PKFactory**);
#ifdef __ANSI__
#undef PKFactory
#endif // __ANSI__
} PKFactory;

//----------------------------------------------------------------
ERR PKCreateFactory_CreateStream(PKStream** ppStream);
ERR PKCreateFactory_Release(PKFactory** ppFactory);

EXTERN_C ERR PKCreateFactory(PKFactory**, U32);


//================================================================
typedef struct tagPKCodecFactory
{
    ERR (*CreateCodec)(const PKIID*, void**);
    ERR (*CreateDecoderFromFile)(const char*, PKImageDecode**);
    ERR (*CreateFormatConverter)(PKFormatConverter**);

    ERR (*Release)(PKCodecFactory**);
#ifdef __ANSI__
#undef PKCodecFactory
#endif // __ANSI__
} PKCodecFactory;

//----------------------------------------------------------------
ERR PKCodecFactory_CreateCodec(const PKIID* iid, void** ppv);
ERR PKCreateCodecFactory_Release(PKCodecFactory** ppCFactory);

EXTERN_C ERR PKCreateCodecFactory(PKCodecFactory**, U32);

//================================================================
typedef struct tagPKImageEncode
{
    //ERR (*GetPixelFormat)(MILPixelFormat*));
    ERR (*Initialize)(PKImageEncode*, struct WMPStream*, void*, size_t);
    ERR (*Terminate)(PKImageEncode*);

    ERR (*SetPixelFormat)(PKImageEncode*, PKPixelFormatGUID);
    ERR (*SetSize)(PKImageEncode*, I32, I32);
    ERR (*SetResolution)(PKImageEncode*, Float, Float);

    ERR (*WritePixels)(PKImageEncode*, U32, U8*, U32);
    ERR (*WriteSource)(PKImageEncode*, PKFormatConverter*, PKRect*);

    ERR (*Transcode)(PKImageEncode*, PKImageDecode*, CWMTranscodingParam*);

    ERR (*CreateNewFrame)(PKImageEncode*, void*, size_t);

    ERR (*Release)(PKImageEncode**);

    struct WMPStream* pStream;
    size_t offStart;

    PKPixelFormatGUID guidPixFormat;

    U32 uWidth;
    U32 uHeight;
    U32 idxCurrentLine;

    Float fResX;
    Float fResY;

    U32 cFrame;

    Bool fHeaderDone;
    size_t offPixel;
    size_t cbPixel;

	Bool bWMP;//for the encoder in decoding

    union
    {
        struct
        {
            WmpDEMisc wmiDEMisc;
            CWMImageInfo wmiI;
            CWMIStrCodecParam wmiSCP;
            CTXSTRCODEC ctxSC;
            CWMImageInfo wmiI_Alpha;
            CWMIStrCodecParam wmiSCP_Alpha;
            CTXSTRCODEC ctxSC_Alpha;

            Bool bHasAlpha;
            Long nOffImage;
            Long nCbImage;
            Long nOffAlpha;
            Long nCbAlpha;
        } WMP;
    };
#ifdef __ANSI__
#undef PKImageEncode
#endif // __ANSI__
} PKImageEncode;

//----------------------------------------------------------------
ERR PKImageEncode_Create_WMP(PKImageEncode** ppIE);

ERR PKImageEncode_Initialize(PKImageEncode* pIE, struct WMPStream* pStream, void* pvParam, size_t cbParam);
ERR PKImageEncode_Terminate(PKImageEncode* pIE);
ERR PKImageEncode_SetPixelFormat(PKImageEncode* pIE, PKPixelFormatGUID enPixelFormat);
ERR PKImageEncode_SetSize(PKImageEncode* pIE, I32 iWidth, I32 iHeight);
ERR PKImageEncode_SetResolution(PKImageEncode* pIE, Float rX, Float rY);
ERR PKImageEncode_WritePixels(PKImageEncode* pIE, U32 cLine, U8* pbPixel, U32 cbStride);
ERR PKImageEncode_CreateNewFrame(PKImageEncode* pIE, void* pvParam, size_t cbParam);
ERR PKImageEncode_Release(PKImageEncode** ppIE);

ERR PKImageEncode_Create(PKImageEncode** ppIE);

//================================================================
typedef struct tagPKImageDecode
{
    ERR (*Initialize)(PKImageDecode*, struct WMPStream* pStream);

    ERR (*GetPixelFormat)(PKImageDecode*, PKPixelFormatGUID*);
    ERR (*GetSize)(PKImageDecode*, I32*, I32*);
    ERR (*GetResolution)(PKImageDecode*, Float*, Float*);

    ERR (*GetRawStream)(PKImageDecode*, struct WMPStream**);

    ERR (*Copy)(PKImageDecode*, const PKRect*, U8*, U32);

    ERR (*GetFrameCount)(PKImageDecode*, U32*);
    ERR (*SelectFrame)(PKImageDecode*, U32);

    ERR (*Release)(PKImageDecode**);

    struct WMPStream* pStream;
    Bool fStreamOwner;
    size_t offStart;
    
    PKPixelFormatGUID guidPixFormat;

    U32 uWidth;
    U32 uHeight;
    U32 idxCurrentLine;

    Float fResX;
    Float fResY;

    U32 cFrame;

    union
    {
        struct
        {
            WmpDEMisc wmiDEMisc;
            CWMImageInfo wmiI;
            CWMIStrCodecParam wmiSCP;
            CTXSTRCODEC ctxSC;
            CWMImageInfo wmiI_Alpha;
            CWMIStrCodecParam wmiSCP_Alpha;
            CTXSTRCODEC ctxSC_Alpha;

            Bool bHasAlpha;
            Long nOffImage;
            Long nCbImage;
            Long nOffAlpha;
            Long nCbAlpha;
            Bool bIgnoreOverlap;
        } WMP;
    };
#ifdef __ANSI__
#undef PKImageDecode
#endif // __ANSI__
} PKImageDecode;

//----------------------------------------------------------------
ERR PKImageDecode_Create_WMP(PKImageDecode** ppID);

ERR PKImageDecode_Initialize(PKImageDecode* pID, struct WMPStream* pStream);
ERR PKImageDecode_GetPixelFormat(PKImageDecode* pID, PKPixelFormatGUID* pPF);
ERR PKImageDecode_GetSize(PKImageDecode* pID, I32* piWidth, I32* piHeight);
ERR PKImageDecode_GetResolution(PKImageDecode* pID, Float* pfrX, Float* pfrY);
ERR PKImageDecode_Copy(PKImageDecode* pID, const PKRect* pRect, U8* pb, U32 cbStride);
ERR PKImageDecode_GetFrameCount(PKImageDecode* pID, U32* puCount);
ERR PKImageDecode_SelectFrame(PKImageDecode* pID, U32 uFrame);
ERR PKImageDecode_Release(PKImageDecode** ppID);

ERR PKImageDecode_Create(PKImageDecode** ppID);
ERR PKCodecFactory_CreateDecoderFromFile(const char* szFilename, PKImageDecode** ppDecoder);

//================================================================
typedef struct tagPKFormatConverter
{
    ERR (*Initialize)(PKFormatConverter*, PKImageDecode*, char *pExt, PKPixelFormatGUID);

    ERR (*GetPixelFormat)(PKFormatConverter*, PKPixelFormatGUID*);
    ERR (*GetSourcePixelFormat)(PKFormatConverter*, PKPixelFormatGUID*);
    ERR (*GetSize)(PKFormatConverter*, I32*, I32*);
    ERR (*GetResolution)(PKFormatConverter*, Float*, Float*);

    ERR (*Copy)(PKFormatConverter*, const PKRect*, U8*, U32);
    ERR (*Convert)(PKFormatConverter*, const PKRect*, U8*, U32);

    ERR (*Release)(PKFormatConverter**);

    PKImageDecode* pDecoder;
    PKPixelFormatGUID enPixelFormat;
#ifdef __ANSI__
#undef PKFormatConverter
#endif // __ANSI__
} PKFormatConverter;

//----------------------------------------------------------------
ERR PKImageEncode_Transcode(PKImageEncode* pIE, PKFormatConverter* pFC, PKRect* pRect);
ERR PKImageEncode_WriteSource(PKImageEncode* pIE, PKFormatConverter* pFC, PKRect* pRect);
ERR PKFormatConverter_Initialize(PKFormatConverter* pFC, PKImageDecode* pID, char *pExt, PKPixelFormatGUID enPF);
ERR PKFormatConverter_GetPixelFormat(PKFormatConverter* pFC, PKPixelFormatGUID* pPF);
ERR PKFormatConverter_GetSourcePixelFormat(PKFormatConverter* pFC, PKPixelFormatGUID* pPF);
ERR PKFormatConverter_GetSize(PKFormatConverter* pFC, I32* piWidth, I32* piHeight);
ERR PKFormatConverter_GetResolution(PKFormatConverter* pFC, Float* pfrX, Float* pfrY);
ERR PKFormatConverter_Copy(PKFormatConverter* pFC, const PKRect* pRect, U8* pb, U32 cbStride);
ERR PKFormatConverter_Convert(PKFormatConverter* pFC, const PKRect* pRect, U8* pb, U32 cbStride);
ERR PKFormatConverter_Release(PKFormatConverter** ppFC);

ERR PKCodecFactory_CreateFormatConverter(PKFormatConverter** ppFConverter);

#ifdef __cplusplus
} // extern "C"
#endif


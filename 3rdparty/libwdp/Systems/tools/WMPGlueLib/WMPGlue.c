//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
#include <stdlib.h>
#include <ctype.h>

#define INITGUID
#include <WMPGlue.h>

//================================================================
const PKIID IID_PKImageScanEncode = 1;
const PKIID IID_PKImageFrameEncode = 2;

const PKIID IID_PKImageWmpEncode = 101;
const PKIID IID_PKImageWmpDecode = 201;

//================================================================
// Misc supporting functions
//================================================================
ERR PKAlloc(void** ppv, size_t cb)
{
    *ppv = calloc(1, cb);
    return *ppv ? WMP_errSuccess : WMP_errOutOfMemory;
}


ERR PKFree(void** ppv)
{
    if (ppv)
    {
        free(*ppv);
        *ppv = NULL;
    }

    return WMP_errSuccess;
}

ERR PKAllocAligned(void** ppv, size_t cb, size_t iAlign)
{
    U8          *pOrigPtr;
    U8          *pReturnedPtr;
    size_t       iAlignmentCorrection;
    const size_t c_cbBlockSize = cb + sizeof(void*) + iAlign - 1;

    *ppv = NULL;
    pOrigPtr = calloc(1, c_cbBlockSize);
    if (NULL == ppv)
        return WMP_errOutOfMemory;

    iAlignmentCorrection = iAlign - ((size_t)pOrigPtr % iAlign);
    if (iAlignmentCorrection < sizeof(void*))
        // Alignment correction won't leave us enough space to store pOrigPtr - advance to next block
        iAlignmentCorrection += iAlign;

    assert(iAlignmentCorrection >= sizeof(void*)); // Alignment correction must have space for pOrigPtr
    assert(iAlignmentCorrection + cb <= c_cbBlockSize); // Don't exceed right edge of memory block

    pReturnedPtr = pOrigPtr + iAlignmentCorrection;
    *(void**)(pReturnedPtr - sizeof(void*)) = pOrigPtr;

    assert(0 == ((size_t)pReturnedPtr % iAlign)); // Are we in fact aligned?
    *ppv = pReturnedPtr;
    return WMP_errSuccess;
}

ERR PKFreeAligned(void** ppv)
{
    if (ppv && *ppv)
    {
        U8 **ppOrigPtr = (U8**)((U8*)(*ppv) - sizeof(void*));
        assert(*ppOrigPtr <= (U8*)ppOrigPtr); // Something's wrong if pOrigPtr points forward
        free(*ppOrigPtr);
        *ppv = NULL;
    }
    return WMP_errSuccess;
}



int PKStrnicmp(const char* s1, const char* s2, size_t c)
{
    for(; tolower(*s1) == tolower(*s2) && *s1 && *s2 && c; ++s1, ++s2, --c);
    return c ? *s1 - *s2 : 0;
}


//----------------------------------------------------------------
//ERR GetPixelInfo(PKPixelFormatGUID enPixelFormat, const PKPixelInfo** ppPI)
ERR PixelFormatLookup(PKPixelInfo* pPI, U8 uLookupType)
{
    ERR err = WMP_errSuccess;
    size_t i;

    static const PKPixelInfo pixelInfo[] =
    {
        {&GUID_PKPixelFormatDontCare, 1, Y_ONLY, BD_8, 8, PK_pixfmtNul, 0, 0, 0, 0},

        // Gray
        //{&GUID_PKPixelFormat2bppGray, 1, Y_ONLY, BD_8, 2, PK_pixfmtNul},
        //{&GUID_PKPixelFormat4bppGray, 1, Y_ONLY, BD_8, 4, PK_pixfmtNul},

        {&GUID_PKPixelFormatBlackWhite, 1, Y_ONLY, BD_1, 1, PK_pixfmtNul,               1, 1, 1, 1},//BlackIsZero is default for GUID_PKPixelFormatBlackWhite
        {&GUID_PKPixelFormatBlackWhite, 1, Y_ONLY, BD_1, 1, PK_pixfmtNul,               0, 1, 1, 1},//WhiteIsZero
        {&GUID_PKPixelFormat8bppGray, 1, Y_ONLY, BD_8, 8, PK_pixfmtNul,                 1, 1, 8, 1},
        {&GUID_PKPixelFormat16bppGray, 1, Y_ONLY, BD_16, 16, PK_pixfmtNul,              1, 1, 16, 1},
        {&GUID_PKPixelFormat16bppGrayFixedPoint, 1, Y_ONLY, BD_16S, 16, PK_pixfmtNul,   1, 1, 16, 2},
        {&GUID_PKPixelFormat16bppGrayHalf, 1, Y_ONLY, BD_16F, 16, PK_pixfmtNul,         1, 1, 16, 3},
        {&GUID_PKPixelFormat32bppGray, 1, Y_ONLY, BD_32, 32, PK_pixfmtNul,              1, 1, 32, 1},
        {&GUID_PKPixelFormat32bppGrayFixedPoint, 1, Y_ONLY, BD_32S, 32, PK_pixfmtNul,   1, 1, 32, 2},
        {&GUID_PKPixelFormat32bppGrayFloat, 1, Y_ONLY, BD_32F, 32, PK_pixfmtNul,        1, 1, 32, 3},

        // RGB
        {&GUID_PKPixelFormat24bppRGB, 3, CF_RGB, BD_8, 24, PK_pixfmtNul,                2, 3, 8, 1},
        {&GUID_PKPixelFormat24bppBGR, 3, CF_RGB, BD_8, 24, PK_pixfmtBGR,                2, 3, 8, 1},
        {&GUID_PKPixelFormat32bppBGR, 3, CF_RGB, BD_8, 32, PK_pixfmtBGR,                2, 4, 8, 1},
        {&GUID_PKPixelFormat48bppRGB, 3, CF_RGB, BD_16, 48, PK_pixfmtNul,               2, 3, 16, 1},
        {&GUID_PKPixelFormat48bppRGBFixedPoint, 3, CF_RGB, BD_16S, 48, PK_pixfmtNul,    2, 3, 16, 2},
        {&GUID_PKPixelFormat48bppRGBHalf, 3, CF_RGB, BD_16F, 48, PK_pixfmtNul,          2, 3, 16, 3},
        //{&GUID_PKPixelFormat64bppRGBFixedPoint, 3, CF_RGB, BD_16S, 64, PK_pixfmtNul,  2, 3, 16, 2},
        //{&GUID_PKPixelFormat64bppRGBHalf, 3, CF_RGB, BD_16F, 64, PK_pixfmtNul,        2, 3, 16, 3},
        {&GUID_PKPixelFormat96bppRGB, 3, CF_RGB, BD_32, 96, PK_pixfmtNul,               2, 3, 32, 1},
        {&GUID_PKPixelFormat96bppRGBFixedPoint, 3, CF_RGB, BD_32S, 96, PK_pixfmtNul,    2, 3, 32, 2},
        //{&GUID_PKPixelFormat96bppRGBFloat, 3, CF_RGB, BD_32F, 96, PK_pixfmtNul,         2, 3, 32, 3},
        //{&GUID_PKPixelFormat128bppRGBFixedPoint, 3, CF_RGB, BD_32S, 128, PK_pixfmtNul,2, 3, 32, 2},
        {&GUID_PKPixelFormat128bppRGBFloat, 3, CF_RGB, BD_32F, 128, PK_pixfmtNul,       2, 4, 32, 3},

        // RGBA
        {&GUID_PKPixelFormat32bppBGRA, 4, CF_RGB, BD_8, 32, PK_pixfmtHasAlpha | PK_pixfmtBGR,  2, 4, 8, 1},
        {&GUID_PKPixelFormat32bppRGBA, 4, CF_RGB, BD_8, 32, PK_pixfmtHasAlpha,                 2, 4, 8, 1},
        {&GUID_PKPixelFormat64bppRGBA, 4, CF_RGB, BD_16, 64, PK_pixfmtHasAlpha,                2, 4, 16, 1},
        {&GUID_PKPixelFormat64bppRGBAFixedPoint, 4, CF_RGB, BD_16S, 64, PK_pixfmtHasAlpha,     2, 4, 16, 2},
        {&GUID_PKPixelFormat64bppRGBAHalf, 4, CF_RGB, BD_16F, 64, PK_pixfmtHasAlpha,           2, 4, 16, 3},
        {&GUID_PKPixelFormat128bppRGBA, 4, CF_RGB, BD_32, 128, PK_pixfmtHasAlpha,              2, 4, 32, 1},
        {&GUID_PKPixelFormat128bppRGBAFixedPoint, 4, CF_RGB, BD_32S, 128, PK_pixfmtHasAlpha,   2, 4, 32, 2},
        {&GUID_PKPixelFormat128bppRGBAFloat, 4, CF_RGB, BD_32F, 128, PK_pixfmtHasAlpha,        2, 4, 32, 3},

        // PRGBA
        {&GUID_PKPixelFormat32bppPBGRA, 4, CF_RGB, BD_8, 32, PK_pixfmtHasAlpha | PK_pixfmtBGR,   2, 4, 8, 1},
        {&GUID_PKPixelFormat64bppPRGBA, 4, CF_RGB, BD_16, 64, PK_pixfmtHasAlpha,                 2, 4, 16, 1},
        //{&GUID_PKPixelFormat64bppPRGBAFixedPoint, 4, CF_RGB, BD_16S, 64, PK_pixfmtHasAlpha,      2, 4, 16, 2},
        //{&GUID_PKPixelFormat64bppPRGBAHalf, 4, CF_RGB, BD_16F, 64, PK_pixfmtHasAlpha,            2, 4, 16, 3},
        //{&GUID_PKPixelFormat128bppPRGBAFixedPoint, 4, CF_RGB, BD_32S, 128, PK_pixfmtHasAlpha,    2, 4, 32, 2},
        {&GUID_PKPixelFormat128bppPRGBAFloat, 4, CF_RGB, BD_32F, 128, PK_pixfmtHasAlpha,         2, 4, 32, 3},

        // Packed formats
        {&GUID_PKPixelFormat16bppRGB555, 3, CF_RGB,  BD_5, 16, PK_pixfmtNul,      2, 3, 16, 1},
        {&GUID_PKPixelFormat16bppRGB565, 3, CF_RGB, BD_565, 16, PK_pixfmtNul,     2, 3, 16, 1},
        {&GUID_PKPixelFormat32bppRGB101010, 3, CF_RGB, BD_10, 32, PK_pixfmtNul,   2, 3, 10, 1},

        // CMYK
        {&GUID_PKPixelFormat32bppCMYK, 4, CMYK, BD_8, 32, PK_pixfmtNul,               5, 4, 8, 1},
        {&GUID_PKPixelFormat40bppCMYKAlpha, 5, CMYK, BD_8, 40, PK_pixfmtHasAlpha,     5, 5, 8, 1},
  
        {&GUID_PKPixelFormat64bppCMYK, 4, CMYK, BD_16, 64, PK_pixfmtNul,              5, 4, 16, 1},
        {&GUID_PKPixelFormat80bppCMYKAlpha, 5, CMYK, BD_16, 80, PK_pixfmtHasAlpha,    5, 5, 16, 1},

        // N_CHANNEL
        {&GUID_PKPixelFormat24bpp3Channels, 3, N_CHANNEL, BD_8, 24, PK_pixfmtNul, PK_PI_NCH, 3, 8, 1},
        {&GUID_PKPixelFormat32bpp4Channels, 4, N_CHANNEL, BD_8, 32, PK_pixfmtNul, PK_PI_NCH, 4, 8, 1},
        {&GUID_PKPixelFormat40bpp5Channels, 5, N_CHANNEL, BD_8, 40, PK_pixfmtNul, PK_PI_NCH, 5, 8, 1},
        {&GUID_PKPixelFormat48bpp6Channels, 6, N_CHANNEL, BD_8, 48, PK_pixfmtNul, PK_PI_NCH, 6, 8, 1},
        {&GUID_PKPixelFormat56bpp7Channels, 7, N_CHANNEL, BD_8, 56, PK_pixfmtNul, PK_PI_NCH, 7, 8, 1},
        {&GUID_PKPixelFormat64bpp8Channels, 8, N_CHANNEL, BD_8, 64, PK_pixfmtNul, PK_PI_NCH, 8, 8, 1},
        //{&GUID_PKPixelFormat32bpp3ChannelsAlpha, 4, N_CHANNEL, BD_8, 32, PK_pixfmtHasAlpha, PK_PI_NCH, 4, 8, 1},
        //{&GUID_PKPixelFormat40bpp4ChannelsAlpha, 5, N_CHANNEL, BD_8, 40, PK_pixfmtHasAlpha, PK_PI_NCH, 5, 8, 1},
        //{&GUID_PKPixelFormat48bpp5ChannelsAlpha, 6, N_CHANNEL, BD_8, 48, PK_pixfmtHasAlpha, PK_PI_NCH, 6, 8, 1},
        //{&GUID_PKPixelFormat56bpp6ChannelsAlpha, 7, N_CHANNEL, BD_8, 56, PK_pixfmtHasAlpha, PK_PI_NCH, 7, 8, 1},
        //{&GUID_PKPixelFormat64bpp7ChannelsAlpha, 8, N_CHANNEL, BD_8, 64, PK_pixfmtHasAlpha, PK_PI_NCH, 8, 8, 1},
        //{&GUID_PKPixelFormat72bpp8ChannelsAlpha, 9, N_CHANNEL, BD_8, 72, PK_pixfmtHasAlpha, PK_PI_NCH, 9, 8, 1},

        {&GUID_PKPixelFormat48bpp3Channels, 3, N_CHANNEL, BD_16, 48, PK_pixfmtNul, PK_PI_NCH, 3, 16, 1},
        {&GUID_PKPixelFormat64bpp4Channels, 4, N_CHANNEL, BD_16, 64, PK_pixfmtNul, PK_PI_NCH, 4, 16, 1},
        {&GUID_PKPixelFormat80bpp5Channels, 5, N_CHANNEL, BD_16, 80, PK_pixfmtNul, PK_PI_NCH, 5, 16, 1},
        {&GUID_PKPixelFormat96bpp6Channels, 6, N_CHANNEL, BD_16, 96, PK_pixfmtNul, PK_PI_NCH, 6, 16, 1},
        {&GUID_PKPixelFormat112bpp7Channels, 7, N_CHANNEL, BD_16, 112, PK_pixfmtNul, PK_PI_NCH, 7, 16, 1},
        {&GUID_PKPixelFormat128bpp8Channels, 8, N_CHANNEL, BD_16, 128, PK_pixfmtNul, PK_PI_NCH, 8, 16, 1},
        //{&GUID_PKPixelFormat64bpp3ChannelsAlpha, 4, N_CHANNEL, BD_16, 64, PK_pixfmtHasAlpha, PK_PI_NCH, 4, 16, 1},
        //{&GUID_PKPixelFormat80bpp4ChannelsAlpha, 5, N_CHANNEL, BD_16, 80, PK_pixfmtHasAlpha, PK_PI_NCH, 5, 16, 1},
        //{&GUID_PKPixelFormat96bpp5ChannelsAlpha, 6, N_CHANNEL, BD_16, 96, PK_pixfmtHasAlpha, PK_PI_NCH, 6, 16, 1},
        //{&GUID_PKPixelFormat112bpp6ChannelsAlpha, 7, N_CHANNEL, BD_16, 112, PK_pixfmtHasAlpha, PK_PI_NCH, 7, 16, 1},
        //{&GUID_PKPixelFormat128bpp7ChannelsAlpha, 8, N_CHANNEL, BD_16, 128, PK_pixfmtHasAlpha, PK_PI_NCH, 8, 16, 1},
        //{&GUID_PKPixelFormat144bpp8ChannelsAlpha, 9, N_CHANNEL, BD_16, 144, PK_pixfmtHasAlpha, PK_PI_NCH, 9, 16, 1},

        //RGBE
        {&GUID_PKPixelFormat32bppRGBE, 4, CF_RGBE, BD_8, 32, PK_pixfmtNul, PK_PI_RGBE, 4, 8, 1},

        //YUV
        {&GUID_PKPixelFormat12bppYUV420, 3, YUV_420, BD_8, 48, PK_pixfmtNul},
        {&GUID_PKPixelFormat16bppYUV422, 3, YUV_422, BD_8, 32, PK_pixfmtNul},
        {&GUID_PKPixelFormat24bppYUV444, 3, YUV_444, BD_8, 24, PK_pixfmtNul},
    };

/*    static const PKPixelInfo pixelInfo[] =
    {
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, // PKPixelFormatDontCare = 0x2010000, PKPixelFormatUndefined
        {1,  8, 0, BAYER,  BD_1,  NULL, 0    }, // PKPixelFormat1bppIndexed = 0x2010001,
        {1,  4, 0, BAYER,  BD_8,  NULL, 0    }, // PKPixelFormat2bppIndexed = 0x2010002,
        {1,  2, 0, BAYER,  BD_8,  NULL, 0    }, // PKPixelFormat4bppIndexed = 0x2010003,
        {1,  1, 0, BAYER,  BD_8,  NULL, 0    }, // PKPixelFormat8bppIndexed = 0x2010004,
        {1,  8, 1, Y_ONLY, BD_1,  "P5", 1    }, // PKPixelFormatBlackWhite = 0x2010005,
        {1,  4, 1, Y_ONLY, BD_8,  "P5", 3    }, // PKPixelFormat2bppGray = 0x2010006,
        {1,  2, 1, Y_ONLY, BD_8,  "P5", 15   }, // PKPixelFormat4bppGray = 0x2010007,
        {1,  1, 1, Y_ONLY, BD_8,  "P5", 255  }, // PKPixelFormat8bppGray = 0x2010008,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, // 
        {2,  1, 3, CF_RGB, BD_8,  NULL, 0    }, // PKPixelFormat16bppRGB555 = 0x201000a,
        {2,  1, 3, CF_RGB, BD_8,  NULL, 0    }, // PKPixelFormat16bppRGB565 = 0x201000b,
        {3,  1, 3, CF_RGB, BD_8,  "P6", 255  }, // PKPixelFormat24bppRGB = 0x201000c,
        {3,  1, 3, CF_RGB, BD_8,  NULL, 0    }, // PKPixelFormat24bppBGR = 0x201000d,
        {4,  1, 3, CF_RGB, BD_8,  NULL, 0    }, // PKPixelFormat32bppRGB = 0x201000e,
        {4,  1, 4, CF_RGB, BD_8,  NULL, 0    }, // PKPixelFormat32bppRGBA = 0x201000f,
        {4,  1, 4, CF_RGB, BD_8,  NULL, 0    }, // PKPixelFormat32bppPARGB = 0x2010010,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x2010011,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x2010012,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x2010013,
        {6,  1, 3, CF_RGB, BD_16, "P6", 65535}, // PKPixelFormat48bppRGB = 0x2010014,
        {8,  1, 4, CF_RGB, BD_16, NULL, 0    }, // PKPixelFormat64bppRGBA = 0x2010015,
        {8,  1, 4, CF_RGB, BD_16, NULL, 0    }, // PKPixelFormat64bppPARGB = 0x2010016,
        {4,  1, 3, CF_RGB, BD_8,  NULL, 0    }, // PKPixelFormat32bppRGB101010 = 0x2010017,
        {2,  1, 1, Y_ONLY, BD_16, "P6", 65535}, // PKPixelFormat16bppGray = 0x2010018,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x2010019,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x201001a,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x201001b,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x201001c,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x201001d,
        {4,  1, 1, Y_ONLY, BD_32S,NULL, 0    }, // PKPixelFormat32bppGrayFixedPoint = 0x201001e,
        {16, 1, 4, CF_RGB, BD_32S,NULL, 0    }, // PKPixelFormat128bppABGRFixedPoint = 0x201001f,
        {16, 1, 4, CF_RGB, BD_32S,NULL, 0    }, // PKPixelFormat128bppPABGRFixedPoint = 0x2010020,
        {16, 1, 3, CF_RGB, BD_32S,NULL, 0    }, // PKPixelFormat128bppBGRFixedPoint = 0x2010021,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x2010022,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x2010023,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x2010024,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x2010025,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x2010026,
        {0,  1, 0, BAYER,  BD_1,  NULL, 0    }, //  = 0x2010027,
        {4,  1, 4, CMYK,   BD_8,  NULL, 0    }, // PKPixelFormat32bppCMYK = 0x2010028,
        {4,  1, 4, CF_RGBE,BD_8,  NULL, 0    }, // PKPixelFormat32bppRGBE = 0x2010029,
        {4,  1, 4, CF_RGBE,BD_16,  NULL, 0    }, // PKPixelFormat64bppCMYK = 0x201002a,
        {6,  1, 3, CF_RGB, BD_16F, NULL, -1   }, // PKPixelFormat48bppRGBHalf = 0x201002b,
        {12, 1, 3, CF_RGB, BD_32F, "PF", -1   }, // PKPixelFormat96bppRGBFloat = 0x201002c,
        //YUV
        {3,  2, 3, YUV_420, BD_8,  NULL, -1   }, // PKPixelFormat12bppYUV420 = 0x201002d,
        {2,  1, 3, YUV_422, BD_8,  NULL, -1   }, // PKPixelFormat16bppYUV422 = 0x201002e,
        {3,  1, 3, YUV_444, BD_8,  NULL, -1   }, // PKPixelFormat24bppYUV444 = 0x201002f,
    };
*/

//    *ppPI = &pixelInfo[idx];

    for (i = 0; i < sizeof2(pixelInfo); ++i)
    {
        if (LOOKUP_FORWARD == uLookupType)
        {
            if (IsEqualGUID(pPI->pGUIDPixFmt, pixelInfo[i].pGUIDPixFmt))
            {
                *pPI = pixelInfo[i];
                goto Cleanup;
            }
        }
        else if (LOOKUP_BACKWARD_TIF == uLookupType)
        {
            if (pPI->uSamplePerPixel == pixelInfo[i].uSamplePerPixel &&
                pPI->uBitsPerSample == pixelInfo[i].uBitsPerSample &&
                pPI->uSampleFormat == pixelInfo[i].uSampleFormat &&
                pPI->uInterpretation == pixelInfo[i].uInterpretation)
            {
                *pPI = pixelInfo[i];
                goto Cleanup;   
            }
        }
    }
    Call(WMP_errUnsupportedFormat);

Cleanup:
    return err;        
}

//----------------------------------------------------------------
typedef struct tagPKIIDInfo
{
    const char* szExt;
    const PKIID* pIIDEnc;
    const PKIID* pIIDDec;
} PKIIDInfo;

static ERR GetIIDInfo(const char* szExt, const PKIIDInfo** ppInfo)
{
    ERR err = WMP_errSuccess;

    static PKIIDInfo iidInfo[] = {
        {".wdp", &IID_PKImageWmpEncode, &IID_PKImageWmpDecode},
        {".hdp", &IID_PKImageWmpEncode, &IID_PKImageWmpDecode},
        {".jxr", &IID_PKImageWmpEncode, &IID_PKImageWmpDecode},
    };
    size_t i = 0;

    *ppInfo = NULL;
    for (i = 0; i < sizeof2(iidInfo); ++i)
    {
        if (0 == PKStrnicmp(szExt, iidInfo[i].szExt, strlen(iidInfo[i].szExt)))
        {
            *ppInfo = &iidInfo[i];
            goto Cleanup;
        }
    }

    Call(WMP_errUnsupportedFormat);

Cleanup:
    return err;
}

ERR GetImageEncodeIID(const char* szExt, const PKIID** ppIID)
{
    ERR err = WMP_errSuccess;

    const PKIIDInfo* pInfo = NULL;

    Call(GetIIDInfo(szExt, &pInfo));
    *ppIID = pInfo->pIIDEnc;

Cleanup:
    return err;
}

ERR GetImageDecodeIID(const char* szExt, const PKIID** ppIID)
{
    ERR err = WMP_errSuccess;

    const PKIIDInfo* pInfo = NULL;

    Call(GetIIDInfo(szExt, &pInfo));
    *ppIID = pInfo->pIIDDec;

Cleanup:
    return err;
}

//================================================================
// PKFactory
//================================================================
ERR PKCreateFactory_CreateStream(PKStream** ppStream)
{
    ERR err = WMP_errSuccess;

    Call(PKAlloc(ppStream, sizeof(**ppStream)));

Cleanup:
    return err;
}

ERR PKCreateFactory_Release(PKFactory** ppFactory)
{
    ERR err = WMP_errSuccess;

    Call(PKFree(ppFactory));

Cleanup: 
    return err;
}

//----------------------------------------------------------------
ERR PKCreateFactory(PKFactory** ppFactory, U32 uVersion)
{
    ERR err = WMP_errSuccess;
    PKFactory* pFactory = NULL;

    Call(PKAlloc(ppFactory, sizeof(**ppFactory)));
    pFactory = *ppFactory;

    pFactory->CreateStream = PKCreateFactory_CreateStream;

    pFactory->CreateStreamFromFilename = CreateWS_File;
    pFactory->CreateStreamFromMemory = CreateWS_Memory;
    
    pFactory->Release = PKCreateFactory_Release;

Cleanup:
    return err;
}


//================================================================
// PKCodecFactory
//================================================================
ERR PKCodecFactory_CreateCodec(const PKIID* iid, void** ppv)
{
    ERR err = WMP_errSuccess;

    if (IID_PKImageWmpEncode == *iid)
    {
        Call(PKImageEncode_Create_WMP((PKImageEncode**)ppv));
    }
    else if (IID_PKImageWmpDecode == *iid)
    {
        Call(PKImageDecode_Create_WMP((PKImageDecode**)ppv));
    }
    else
    {
        Call(WMP_errUnsupportedFormat);
    }

Cleanup:
    return err;
}

ERR PKCodecFactory_CreateDecoderFromFile(const char* szFilename, PKImageDecode** ppDecoder)
{
    ERR err = WMP_errSuccess;

    char *pExt = NULL;
    PKIID* pIID = NULL;

    struct WMPStream* pStream = NULL;
    PKImageDecode* pDecoder = NULL;

    // get file extension
    pExt = strrchr(szFilename, '.');
    FailIf(NULL == pExt, WMP_errUnsupportedFormat);

    // get decode PKIID
    Call(GetImageDecodeIID(pExt, &pIID));

    // create stream
    Call(CreateWS_File(&pStream, szFilename, "rb"));

    // Create decoder
    Call(PKCodecFactory_CreateCodec(pIID, ppDecoder));
    pDecoder = *ppDecoder;

    // attach stream to decoder
    Call(pDecoder->Initialize(pDecoder, pStream));
    pDecoder->fStreamOwner = !0;

Cleanup:
    return err;
}

ERR PKCodecFactory_CreateFormatConverter(PKFormatConverter** ppFConverter)
{
    ERR err = WMP_errSuccess;
    PKFormatConverter* pFC = NULL;

    Call(PKAlloc(ppFConverter, sizeof(**ppFConverter)));
    pFC = *ppFConverter;

    pFC->Initialize = PKFormatConverter_Initialize;
    pFC->GetPixelFormat = PKFormatConverter_GetPixelFormat;
    pFC->GetSourcePixelFormat = PKFormatConverter_GetSourcePixelFormat;
    pFC->GetSize = PKFormatConverter_GetSize;
    pFC->GetResolution = PKFormatConverter_GetResolution;
    pFC->Copy = PKFormatConverter_Copy;
    pFC->Convert = PKFormatConverter_Convert;
    pFC->Release = PKFormatConverter_Release;

Cleanup:
    return err;
}

ERR PKCreateCodecFactory_Release(PKCodecFactory** ppCFactory)
{
    ERR err = WMP_errSuccess;

    Call(PKFree(ppCFactory));

Cleanup:
    return err;
}

ERR PKCreateCodecFactory(PKCodecFactory** ppCFactory, U32 uVersion)
{
    ERR err = WMP_errSuccess;
    PKCodecFactory* pCFactory = NULL;

    Call(PKAlloc(ppCFactory, sizeof(**ppCFactory)));
    pCFactory = *ppCFactory;

    pCFactory->CreateCodec = PKCodecFactory_CreateCodec;
    pCFactory->CreateDecoderFromFile = PKCodecFactory_CreateDecoderFromFile;
    pCFactory->CreateFormatConverter = PKCodecFactory_CreateFormatConverter;
    pCFactory->Release = PKCreateCodecFactory_Release;

Cleanup:
    return err;
}


//================================================================
// PKImageEncode
//================================================================
ERR PKImageEncode_Initialize(
    PKImageEncode* pIE,
    struct WMPStream* pStream,
    void* pvParam,
    size_t cbParam)
{
    ERR err = WMP_errSuccess;

    pIE->pStream = pStream;
    pIE->guidPixFormat = GUID_PKPixelFormatDontCare;
    pIE->fResX = 96;
    pIE->fResY = 96;
    pIE->cFrame = 1;

    Call(pIE->pStream->GetPos(pIE->pStream, &pIE->offStart));

Cleanup:
    return err;
}

ERR PKImageEncode_Terminate(
    PKImageEncode* pIE)
{
    return WMP_errSuccess;
}

ERR PKImageEncode_SetPixelFormat(
    PKImageEncode* pIE,
    PKPixelFormatGUID enPixelFormat)
{
    pIE->guidPixFormat = enPixelFormat;

    return WMP_errSuccess;
}

ERR PKImageEncode_SetSize(
    PKImageEncode* pIE,
    I32 iWidth,
    I32 iHeight)
{
    ERR err = WMP_errSuccess;

    pIE->uWidth = (U32)iWidth;
    pIE->uHeight = (U32)iHeight;

    return err;
}

ERR PKImageEncode_SetResolution(
    PKImageEncode* pIE,
    Float fResX, 
    Float fResY)
{
    pIE->fResX = fResX;
    pIE->fResY = fResY;

    return WMP_errSuccess;
}

ERR PKImageEncode_WritePixels(
    PKImageEncode* pIE,
    U32 cLine,
    U8* pbPixels,
    U32 cbStride)
{
    return WMP_errAbstractMethod;
}

ERR PKImageEncode_WriteSource(
    PKImageEncode* pIE,
    PKFormatConverter* pFC,
    PKRect* pRect)
{
    ERR err = WMP_errSuccess;

    PKPixelFormatGUID enPFFrom = GUID_PKPixelFormatDontCare;
    PKPixelFormatGUID enPFTo = GUID_PKPixelFormatDontCare;

    PKPixelInfo pPIFrom;
    PKPixelInfo pPITo;

    U32 cbStrideTo = 0;
    U32 cbStrideFrom = 0;
    U32 cbStride = 0;

    U8* pb = NULL;

	CWMTranscodingParam* pParam = NULL; 

    // get pixel format
    Call(pFC->GetSourcePixelFormat(pFC, &enPFFrom));
    Call(pFC->GetPixelFormat(pFC, &enPFTo));
    FailIf(!IsEqualGUID(&pIE->guidPixFormat, &enPFTo), WMP_errUnsupportedFormat);

    // calc common stride
//    Call(GetPixelInfo(enPFFrom, &pPIFrom));
    pPIFrom.pGUIDPixFmt = &enPFFrom;
    PixelFormatLookup(&pPIFrom, LOOKUP_FORWARD);

//    Call(GetPixelInfo(enPFTo, &pPITo));
    pPITo.pGUIDPixFmt = &enPFTo;
    PixelFormatLookup(&pPITo, LOOKUP_FORWARD);

//    cbStrideFrom = (pPIFrom->cbPixel * pRect->Width + pPIFrom->cbPixelDenom - 1) / pPIFrom->cbPixelDenom;
    cbStrideFrom = (BD_1 == pPIFrom.bdBitDepth ? ((pPIFrom.cbitUnit * pRect->Width + 7) >> 3) : (((pPIFrom.cbitUnit + 7) >> 3) * pRect->Width)); 
    if (&GUID_PKPixelFormat12bppYUV420 == pPIFrom.pGUIDPixFmt 
        || &GUID_PKPixelFormat16bppYUV422 == pPIFrom.pGUIDPixFmt) 
        cbStrideFrom >>= 1;

//    cbStrideTo = (pPITo->cbPixel * pIE->uWidth + pPITo->cbPixelDenom - 1) / pPITo->cbPixelDenom;
    cbStrideTo = (BD_1 == pPITo.bdBitDepth ? ((pPITo.cbitUnit * pIE->uWidth + 7) >> 3) : (((pPITo.cbitUnit + 7) >> 3) * pIE->uWidth)); 
    if (&GUID_PKPixelFormat12bppYUV420 == pPITo.pGUIDPixFmt
        || &GUID_PKPixelFormat16bppYUV422 == pPITo.pGUIDPixFmt) 
        cbStrideTo >>= 1;

    cbStride = max(cbStrideFrom, cbStrideTo);

    // actual dec/enc with local buffer
    Call(PKAllocAligned(&pb, cbStride * pRect->Height, 128));

    Call(pFC->Copy(pFC, pRect, pb, cbStride));

	Call(pIE->WritePixels(pIE, pRect->Height, pb, cbStride));

Cleanup:
    PKFreeAligned(&pb);
    return err;
}


ERR PKImageEncode_Transcode(
    PKImageEncode* pIE,
    PKFormatConverter* pFC,
    PKRect* pRect)
{
    ERR err = WMP_errSuccess;

    PKPixelFormatGUID enPFFrom = GUID_PKPixelFormatDontCare;
    PKPixelFormatGUID enPFTo = GUID_PKPixelFormatDontCare;

    PKPixelInfo pPIFrom;
    PKPixelInfo pPITo;

    U32 cbStrideTo = 0;
    U32 cbStrideFrom = 0;
    U32 cbStride = 0;

    U8* pb = NULL;

    CWMTranscodingParam cParam = {0}; 

    // get pixel format
    Call(pFC->GetSourcePixelFormat(pFC, &enPFFrom));
    Call(pFC->GetPixelFormat(pFC, &enPFTo));
    FailIf(!IsEqualGUID(&pIE->guidPixFormat, &enPFTo), WMP_errUnsupportedFormat);

    // calc common stride
//    Call(GetPixelInfo(enPFFrom, &pPIFrom));
    pPIFrom.pGUIDPixFmt = &enPFFrom;
    PixelFormatLookup(&pPIFrom, LOOKUP_FORWARD);

//    Call(GetPixelInfo(enPFTo, &pPITo));
    pPITo.pGUIDPixFmt = &enPFTo;
    PixelFormatLookup(&pPITo, LOOKUP_FORWARD);

//    cbStrideFrom = (pPIFrom->cbPixel * pRect->Width + pPIFrom->cbPixelDenom - 1) / pPIFrom->cbPixelDenom;
    cbStrideFrom = (BD_1 == pPIFrom.bdBitDepth ? ((pPIFrom.cbitUnit * pRect->Width + 7) >> 3) : (((pPIFrom.cbitUnit + 7) >> 3) * pRect->Width)); 
    if (&GUID_PKPixelFormat12bppYUV420 == pPIFrom.pGUIDPixFmt 
        || &GUID_PKPixelFormat16bppYUV422 == pPIFrom.pGUIDPixFmt) 
        cbStrideFrom >>= 1;

//    cbStrideTo = (pPITo->cbPixel * pIE->uWidth + pPITo->cbPixelDenom - 1) / pPITo->cbPixelDenom;
    cbStrideTo = (BD_1 == pPITo.bdBitDepth ? ((pPITo.cbitUnit * pIE->uWidth + 7) >> 3) : (((pPITo.cbitUnit + 7) >> 3) * pIE->uWidth)); 
    if (&GUID_PKPixelFormat12bppYUV420 == pPITo.pGUIDPixFmt
        || &GUID_PKPixelFormat16bppYUV422 == pPITo.pGUIDPixFmt) 
        cbStrideTo >>= 1;

    cbStride = max(cbStrideFrom, cbStrideTo);

    if(pIE->bWMP){
        cParam.cLeftX = pFC->pDecoder->WMP.wmiI.cROILeftX;
        cParam.cTopY = pFC->pDecoder->WMP.wmiI.cROITopY;
        cParam.cWidth = pFC->pDecoder->WMP.wmiI.cROIWidth;
        cParam.cHeight = pFC->pDecoder->WMP.wmiI.cROIHeight;
        cParam.oOrientation = pFC->pDecoder->WMP.wmiI.oOrientation;
//        cParam.cfColorFormat = pFC->pDecoder->WMP.wmiI.cfColorFormat;
        cParam.uAlphaMode = pFC->pDecoder->WMP.wmiSCP.uAlphaMode;
        cParam.bfBitstreamFormat = pFC->pDecoder->WMP.wmiSCP.bfBitstreamFormat;
        cParam.sbSubband = pFC->pDecoder->WMP.wmiSCP.sbSubband;
        cParam.bIgnoreOverlap = pFC->pDecoder->WMP.bIgnoreOverlap;
        
        Call(pIE->Transcode(pIE, pFC->pDecoder, &cParam));
    }
	else 
	{
		// actual dec/enc with local buffer
	    Call(PKAllocAligned(&pb, cbStride * pRect->Height, 128));
		Call(pFC->Copy(pFC, pRect, pb, cbStride));
		Call(pIE->WritePixels(pIE, pRect->Height, pb, cbStride));
	}

Cleanup:
    PKFreeAligned(&pb);
    return err;
}

ERR PKImageEncode_CreateNewFrame(
    PKImageEncode* pIE,
    void* pvParam,
    size_t cbParam)
{
    // NYI
    return WMP_errSuccess;
}

ERR PKImageEncode_Release(
    PKImageEncode** ppIE)
{
    PKImageEncode *pIE = *ppIE;
    pIE->pStream->Close(&pIE->pStream);

    return PKFree(ppIE);
}

ERR PKImageEncode_Create(PKImageEncode** ppIE)
{
    ERR err = WMP_errSuccess;
    PKImageEncode* pIE = NULL;

    Call(PKAlloc(ppIE, sizeof(**ppIE)));

    pIE = *ppIE;
    pIE->Initialize = PKImageEncode_Initialize;
    pIE->Terminate = PKImageEncode_Terminate;
    pIE->SetPixelFormat = PKImageEncode_SetPixelFormat;
    pIE->SetSize = PKImageEncode_SetSize;
    pIE->SetResolution = PKImageEncode_SetResolution;
    pIE->WritePixels = PKImageEncode_WritePixels;
//    pIE->WriteSource = PKImageEncode_WriteSource;
    pIE->CreateNewFrame = PKImageEncode_CreateNewFrame;
    pIE->Release = PKImageEncode_Release;
	pIE->bWMP = FALSE; 

Cleanup:
    return err;
}
  

//================================================================
// PKImageDecode
//================================================================
ERR PKImageDecode_Initialize(
    PKImageDecode* pID,
    struct WMPStream* pStream)
{
    ERR err = WMP_errSuccess;

    pID->pStream = pStream;
    pID->guidPixFormat = GUID_PKPixelFormatDontCare;
    pID->fResX = 96;
    pID->fResY = 96;
    pID->cFrame = 1;

    Call(pID->pStream->GetPos(pID->pStream, &pID->offStart));

    pID->WMP.wmiDEMisc.uImageOffset = 0;
    pID->WMP.wmiDEMisc.uImageByteCount = 0;
    pID->WMP.wmiDEMisc.uAlphaOffset = 0;
    pID->WMP.wmiDEMisc.uAlphaByteCount = 0;

    pID->WMP.wmiDEMisc.uOffPixelFormat = 0;
    pID->WMP.wmiDEMisc.uOffImageByteCount = 0;
    pID->WMP.wmiDEMisc.uOffAlphaOffset = 0;
    pID->WMP.wmiDEMisc.uOffAlphaByteCount = 0;

Cleanup:
    return WMP_errSuccess;
}

ERR PKImageDecode_GetPixelFormat(
    PKImageDecode* pID,
    PKPixelFormatGUID* pPF)
{
    *pPF = pID->guidPixFormat;

    return WMP_errSuccess;
}

ERR PKImageDecode_GetSize(
    PKImageDecode* pID,
    I32* piWidth,
    I32* piHeight)
{
    *piWidth = (I32)pID->uWidth;
    *piHeight = (I32)pID->uHeight;

    return WMP_errSuccess;
}

ERR PKImageDecode_GetResolution(
    PKImageDecode* pID,
    Float* pfResX,
    Float* pfResY)
{
    *pfResX = pID->fResX;
    *pfResY = pID->fResY;

    return WMP_errSuccess;
}

ERR PKImageDecode_Copy(
    PKImageDecode* pID,
    const PKRect* pRect,
    U8* pb,
    U32 cbStride)
{
    return WMP_errAbstractMethod;
}

ERR PKImageDecode_GetFrameCount(
    PKImageDecode* pID,
    U32* puCount)
{
    *puCount = pID->cFrame;

    return WMP_errSuccess;
}

ERR PKImageDecode_SelectFrame(
    PKImageDecode* pID,
    U32 uFrame)
{
    // NYI
    return WMP_errSuccess;
}

ERR PKImageDecode_Release(
    PKImageDecode** ppID)
{
    PKImageDecode* pID = *ppID;

    pID->fStreamOwner && pID->pStream->Close(&pID->pStream);

    return PKFree(ppID);
}

ERR PKImageDecode_Create(
    PKImageDecode** ppID)
{
    ERR err = WMP_errSuccess;
    PKImageDecode* pID = NULL;

    Call(PKAlloc(ppID, sizeof(**ppID)));

    pID = *ppID;
    pID->Initialize = PKImageDecode_Initialize;
    pID->GetPixelFormat = PKImageDecode_GetPixelFormat;
    pID->GetSize = PKImageDecode_GetSize;
    pID->GetResolution = PKImageDecode_GetResolution;
    pID->Copy = PKImageDecode_Copy;
    pID->GetFrameCount = PKImageDecode_GetFrameCount;
    pID->SelectFrame = PKImageDecode_SelectFrame;
    pID->Release = PKImageDecode_Release;

Cleanup:
    return err;
}


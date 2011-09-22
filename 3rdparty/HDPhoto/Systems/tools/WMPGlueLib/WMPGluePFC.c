//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
#include <stdlib.h>

#include <WMPGlue.h>

//================================================================
// PKFormatConverter
//================================================================
ERR RGB24_BGR24(PKFormatConverter* pFC, const PKRect* pRect, U8* pb, U32 cbStride)
{
    I32 i = 0, j = 0;
   
    for (i = 0; i < pRect->Height; ++i)
    {
        for (j = 0; j < pRect->Width * 3; j += 3)
        {
            // swap red with blue
            U8 t = pb[j];
            pb[j] = pb[j + 2];
            pb[j + 2] = t;
        }

        pb += cbStride;
    }

    return WMP_errSuccess;
}

ERR BGR24_RGB24(PKFormatConverter* pFC, const PKRect* pRect, U8* pb, U32 cbStride)
{
    return RGB24_BGR24(pFC, pRect, pb, cbStride);
}

ERR RGB24_Gray8(PKFormatConverter* pFC, const PKRect* pRect, U8* pb, U32 cbStride)
{
    I32 i = 0, j = 0, k = 0;
    
    for (i = 0; i < pRect->Height; ++i)
    {
        for (j = 0, k = 0; j < pRect->Width * 3; j += 3, ++k)
        {
            U8 r = pb[j];
            U8 g = pb[j + 1];
            U8 b = pb[j + 2];
            
            pb[k] = r / 4 + g / 2 + b / 8 + 16;
        }

        pb += cbStride;
    }

    return WMP_errSuccess;
}

ERR BGR24_Gray8(PKFormatConverter* pFC, const PKRect* pRect, U8* pb, U32 cbStride)
{
    ERR err = WMP_errSuccess;

    Call(BGR24_RGB24(pFC, pRect, pb, cbStride));
    Call(RGB24_Gray8(pFC, pRect, pb, cbStride));

Cleanup:
    return err;
}

ERR Gray8_RGB24(PKFormatConverter* pFC, const PKRect* pRect, U8* pb, U32 cbStride)
{
    I32 i = 0, j = 0, k = 0;
    
    for (i = 0; i < pRect->Height; ++i)
    {
        for (j = pRect->Width - 1, k = 3 * j; 0 <= j; j--, k -= 3)
        {
            U8 v = pb[j];

            pb[k] = v;
            pb[k + 1] = v;
            pb[k + 2] = v;
        }
        
        pb += cbStride;
    }

    return WMP_errSuccess;
}

ERR Gray8_BGR24(PKFormatConverter* pFC, const PKRect* pRect, U8* pb, U32 cbStride)
{
    return Gray8_RGB24(pFC, pRect, pb, cbStride);
}

#if 0
ERR RGB48_BGR48(PKFormatConverter* pFC, const PKRect* pRect, U8* pb, U32 cbStride)
{
    ERR err = WMP_errSuccess;

    I32 i = 0, j = 0;
    
    Call(PKFormatConverter_Copy(pFC, pRect, pb, cbStride));

    for (i = 0; i < pRect->Height; ++i)
    {
        for (j = 0; j < pRect->Width; j += 3)
        {
            U16* ps = (U16*)pb;
            
            // swap red with blue
            U16 t = ps[j];
            ps[j] = ps[j + 2];
            ps[j + 2] = t;
        }

        pb += cbStride;
    }

Cleanup:
    return err;
}

ERR BGR48_RGB48(PKFormatConverter* pFC, const PKRect* pRect, U8* pb, U32 cbStride)
{
    return RGB48_BGR48(pFC, pRect, pb, cbStride);
}

ERR RGB48_Gray16(PKFormatConverter* pFC, const PKRect* pRect, U8* pb, U32 cbStride)
{
    ERR err = WMP_errSuccess;

    I32 i = 0, j = 0, k = 0;
    
    Call(PKFormatConverter_Copy(pFC, pRect, pb, cbStride));

    for (i = 0; i < pRect->Height; ++i)
    {
        for (j = 0, k = 0; j < pRect->Width; j += 3, ++k)
        {
            U16* ps = (U16*)pb;

            // Y = r / 4 + g / 2 + b / 8 + 16
            U16 r = ps[j];
            U16 g = ps[j + 1];
            U16 b = ps[j + 2];
            
            ps[k] = r / 4 + g / 2 + b / 8 + 16;
        }

        pb += cbStride;
    }

Cleanup:
    return err;
}
#endif

//================================================================
typedef struct tagPKPixelConverterInfo
{
    const PKPixelFormatGUID* pGUIDPixFmtFrom;
    const PKPixelFormatGUID* pGUIDPixFmtTo;

    ERR (*Convert)(PKFormatConverter*, const PKRect*, U8*, U32);
} PKPixelConverterInfo;

ERR PKFormatConverter_Initialize(PKFormatConverter* pFC, PKImageDecode* pID, char *pExt, PKPixelFormatGUID enPF)
{
    ERR err = WMP_errSuccess;

    static PKPixelConverterInfo pcInfo[] = {
        {&GUID_PKPixelFormat24bppRGB, &GUID_PKPixelFormat24bppBGR, RGB24_BGR24},
        {&GUID_PKPixelFormat24bppBGR, &GUID_PKPixelFormat24bppRGB, BGR24_RGB24},
        {&GUID_PKPixelFormat24bppRGB, &GUID_PKPixelFormat8bppGray, RGB24_Gray8},
        {&GUID_PKPixelFormat24bppBGR, &GUID_PKPixelFormat8bppGray, BGR24_Gray8},
        {&GUID_PKPixelFormat8bppGray, &GUID_PKPixelFormat24bppRGB, Gray8_RGB24},
        {&GUID_PKPixelFormat8bppGray, &GUID_PKPixelFormat24bppBGR, Gray8_BGR24},
    };
    PKPixelFormatGUID enPFFrom = GUID_PKPixelFormatDontCare;

    //================================
    pFC->pDecoder = pID;
    pFC->enPixelFormat = enPF;

    //================================
    Call(pFC->pDecoder->GetPixelFormat(pFC->pDecoder, &enPFFrom));

    if (!IsEqualGUID(&enPFFrom, &enPF))
    {
        size_t i = 0;
        for (i = 0; i < sizeof2(pcInfo); ++i)
        {
            PKPixelConverterInfo* pPCI = pcInfo + i;

            if (IsEqualGUID(&enPFFrom, pPCI->pGUIDPixFmtFrom) && IsEqualGUID(&enPF, pPCI->pGUIDPixFmtTo))
            {
                pFC->Convert= pPCI->Convert;
                goto Cleanup;
            }
        }
        goto Cleanup;

        Call(WMP_errUnsupportedFormat);
    }

Cleanup:
    return err;
}

ERR PKFormatConverter_GetPixelFormat(PKFormatConverter* pFC, PKPixelFormatGUID* pPF)
{
    *pPF = pFC->enPixelFormat;

    return WMP_errSuccess;
}

ERR PKFormatConverter_GetSourcePixelFormat(PKFormatConverter* pFC, PKPixelFormatGUID* pPF)
{
    return pFC->pDecoder->GetPixelFormat(pFC->pDecoder, pPF);
}

ERR PKFormatConverter_GetSize(PKFormatConverter* pFC, I32* piWidth, I32* piHeight)
{
    return pFC->pDecoder->GetSize(pFC->pDecoder, piWidth, piHeight);
}

ERR PKFormatConverter_GetResolution(PKFormatConverter* pFC, Float* pfrX, Float* pfrY)
{
    return pFC->pDecoder->GetResolution(pFC->pDecoder, pfrX, pfrY);
}

ERR PKFormatConverter_Copy(PKFormatConverter* pFC, const PKRect* pRect, U8* pb, U32 cbStride)
{
    ERR err = WMP_errSuccess;

    Call(pFC->pDecoder->Copy(pFC->pDecoder, pRect, pb, cbStride));
    Call(pFC->Convert(pFC, pRect, pb, cbStride));

Cleanup:
    return err;
}

ERR PKFormatConverter_Convert(PKFormatConverter* pFC, const PKRect* pRect, U8* pb, U32 cbStride)
{
    return WMP_errSuccess;
}

ERR PKFormatConverter_Release(PKFormatConverter** ppFC)
{
    ERR err = WMP_errSuccess;

    Call(PKFree(ppFC));

Cleanup:
    return err;
}


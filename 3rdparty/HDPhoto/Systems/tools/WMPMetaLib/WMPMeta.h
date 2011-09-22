//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
#pragma once

#include <windowsmediaphoto.h>
#include <wmspecstring.h>

//================================================================
// Container
//================================================================
#define WMP_tagNull 0
#define WMP_tagXMPMetadata 0x2bc
#define WMP_tagIccProfile 0x8773 // Need to use same tag as TIFF!!
#define WMP_tagEXIFMetadata 0x8769

#define WMP_tagPixelFormat 0xbc01
#define WMP_tagTransformation 0xbc02
#define WMP_tagCompression 0xbc03
#define WMP_tagImageType 0xbc04

#define WMP_tagImageWidth 0xbc80
#define WMP_tagImageHeight 0xbc81

#define WMP_tagWidthResolution 0xbc82
#define WMP_tagHeightResolution 0xbc83

#define WMP_tagImageOffset 0xbcc0
#define WMP_tagImageByteCount 0xbcc1
#define WMP_tagAlphaOffset 0xbcc2
#define WMP_tagAlphaByteCount 0xbcc3
#define WMP_tagImageDataDiscard 0xbcc4
#define WMP_tagAlphaDataDiscard 0xbcc5


#define WMP_typBYTE 1
#define WMP_typASCII 2
#define WMP_typSHORT 3
#define WMP_typLONG 4
#define WMP_typRATIOAL 5
#define WMP_typSBYTE 6
#define WMP_typUNDEFINED 7
#define WMP_typSSHORT 8
#define WMP_typSLONG 9
#define WMP_typSRATIONAL 10
#define WMP_typFLOAT 11
#define WMP_typDOUBLE 12


#define WMP_valCompression 0xbc
#define WMP_valWMPhotoID WMP_valCompression

//================================================================
typedef struct tagWmpDE
{
    U16 uTag;
    U16 uType;
    U32 uCount;
    union 
    {
        U32 uValueOrOffset;
        float f32;
    };
} WmpDE;

typedef struct tagWmpDEMisc
{
    U32 uImageOffset;
    U32 uImageByteCount;
    U32 uAlphaOffset;
    U32 uAlphaByteCount;

    U32 uOffPixelFormat;
    U32 uOffImageByteCount;
    U32 uOffAlphaOffset;
    U32 uOffAlphaByteCount;
} WmpDEMisc;


//================================================================
EXTERN_C ERR GetUShort(
    __in_ecount(1) struct WMPStream* pWS,
    size_t offPos,
    __out_ecount(1) U16* puValue
);

EXTERN_C ERR PutUShort(
    __in_ecount(1) struct WMPStream* pWS,
    size_t offPos,
    U16 uValue
);

EXTERN_C ERR GetULong(
    __in_ecount(1) struct WMPStream* pWS,
    size_t offPos,
    __out_ecount(1) U32* puValue
);

EXTERN_C ERR PutULong(
    __in_ecount(1) struct WMPStream* pWS,
    size_t offPos,
    U32 uValue
);

EXTERN_C ERR WriteWmpDE(
    __in_ecount(1) struct WMPStream* pWS,
    size_t offPos,
    __in_ecount(1) WmpDE* pDE
);

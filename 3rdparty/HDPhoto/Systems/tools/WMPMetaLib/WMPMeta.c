//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
#include "WMPMeta.h"


//================================================================
ERR GetUShort(
    __in_ecount(1) struct WMPStream* pWS,
    size_t offPos,
    __out_ecount(1) U16* puValue)
{
    ERR err = WMP_errSuccess;
    U8  cVal;

    Call(pWS->SetPos(pWS, offPos));
    Call(pWS->Read(pWS, &cVal, sizeof(cVal)));
    puValue[0] = (U16) cVal;
    Call(pWS->Read(pWS, &cVal, sizeof(cVal)));
    puValue[0] += ((U16) cVal) << 8;

Cleanup:
    return err;
}

ERR PutUShort(
    __in_ecount(1) struct WMPStream* pWS,
    size_t offPos,
    U16 uValue)
{
    ERR err = WMP_errSuccess;
    U8  cVal = (U8) uValue;

    Call(pWS->SetPos(pWS, offPos));
    Call(pWS->Write(pWS, &cVal, sizeof(cVal)));
    cVal = (U8) (uValue >> 8);
    Call(pWS->Write(pWS, &cVal, sizeof(cVal)));

Cleanup:
    return err;
}

ERR GetULong(
    __in_ecount(1) struct WMPStream* pWS,
    size_t offPos,
    __out_ecount(1) U32* puValue)
{
    ERR err = WMP_errSuccess;
    U8  cVal;

    Call(pWS->SetPos(pWS, offPos));
    Call(pWS->Read(pWS, &cVal, sizeof(cVal)));
    puValue[0] = (U32) cVal;
    Call(pWS->Read(pWS, &cVal, sizeof(cVal)));
    puValue[0] += ((U32) cVal) << 8;
    Call(pWS->Read(pWS, &cVal, sizeof(cVal)));
    puValue[0] += ((U32) cVal) << 16;
    Call(pWS->Read(pWS, &cVal, sizeof(cVal)));
    puValue[0] += ((U32) cVal) << 24;
 
Cleanup:
    return err;
}

ERR PutULong(
    __in_ecount(1) struct WMPStream* pWS,
    size_t offPos,
    U32 uValue)
{
    ERR err = WMP_errSuccess;
    U8  cVal = (U8) uValue;

    Call(pWS->SetPos(pWS, offPos));
    Call(pWS->Write(pWS, &cVal, sizeof(cVal)));
    cVal = (U8) (uValue >> 8);
    Call(pWS->Write(pWS, &cVal, sizeof(cVal)));
    cVal = (U8) (uValue >> 16);
    Call(pWS->Write(pWS, &cVal, sizeof(cVal)));
    cVal = (U8) (uValue >> 24);
    Call(pWS->Write(pWS, &cVal, sizeof(cVal)));

Cleanup:
    return err;
}

ERR WriteWmpDE(
    __in_ecount(1) struct WMPStream* pWS,
    size_t offPos,
    __in_ecount(1) WmpDE* pDE)
{
    ERR err = WMP_errSuccess;

    assert(-1 != pDE->uCount);
    assert(-1 != pDE->uValueOrOffset);

    Call(PutUShort(pWS, offPos, pDE->uTag)); offPos += 2;
    Call(PutUShort(pWS, offPos, pDE->uType)); offPos += 2;
    Call(PutULong(pWS, offPos, pDE->uCount)); offPos += 4;

    switch (pDE->uType)
    {

        case WMP_typBYTE:
            if (pDE->uCount < 4)
            {
                U8 pad[4] = {0};
                Call(pWS->SetPos(pWS, offPos));
                Call(pWS->Write(pWS, &pDE->uValueOrOffset, pDE->uCount));
                Call(pWS->Write(pWS, pad, 4 - pDE->uCount)); offPos += 4;
                break;
            }
            Call(PutULong(pWS, offPos, pDE->uValueOrOffset)); offPos += 4;
            break;

            
        case WMP_typSHORT:
            if (1 == pDE->uCount)
            {
                Call(PutUShort(pWS, offPos, (U16)pDE->uValueOrOffset)); offPos += 2;
                Call(PutUShort(pWS, offPos, 0)); offPos += 2;
                break;
            }

            Call(PutULong(pWS, offPos, pDE->uValueOrOffset)); offPos += 4;
            break;

        case WMP_typFLOAT:
        case WMP_typLONG:
        case WMP_typRATIOAL:
            Call(PutULong(pWS, offPos, pDE->uValueOrOffset)); offPos += 4;
            break;

        default:
            Call(WMP_errInvalidParameter);
            break;
    }

Cleanup:
    return err;
}


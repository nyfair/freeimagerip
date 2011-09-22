//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
#include <limits.h>
#include <WMPGlue.h>

//================================================================
// PKImageEncode_WMP
//================================================================
ERR WriteContainerPre(
    PKImageEncode* pIE)
{
    ERR err = WMP_errSuccess;
    const U32 OFFSET_OF_PFD = 0x20;
    struct WMPStream* pWS = pIE->pStream;
    WmpDEMisc* pDEMisc = &pIE->WMP.wmiDEMisc;
    PKPixelInfo PI;
    size_t offPos = 0;

    U8 IIMM[2] = {'\x49', '\x49'};
    const U32 cbWmpDEMisc = OFFSET_OF_PFD;

    static WmpDE wmpDEs[] =
    {
        {WMP_tagPixelFormat, WMP_typBYTE, 16, -1},
        {WMP_tagImageWidth, WMP_typLONG, 1,  -1},
        {WMP_tagImageHeight, WMP_typLONG, 1, -1},
        {WMP_tagWidthResolution, WMP_typFLOAT, 1, -1},
        {WMP_tagHeightResolution, WMP_typFLOAT, 1, -1},
        {WMP_tagImageOffset, WMP_typLONG, 1, -1},
        {WMP_tagImageByteCount, WMP_typLONG, 1, -1},
        {WMP_tagAlphaOffset, WMP_typLONG, 1, -1},
        {WMP_tagAlphaByteCount, WMP_typLONG, 1, -1},
    };
    U16 cWmpDEs = sizeof(wmpDEs) / sizeof(wmpDEs[0]);
    WmpDE wmpDE = {0};
    size_t i = 0;

    //================
    Call(pWS->GetPos(pWS, &offPos));
    FailIf(0 != offPos, WMP_errUnsupportedFormat);

    //================
    // Header (8 bytes)
    Call(pWS->Write(pWS, IIMM, sizeof(IIMM))); offPos += 2;
    Call(PutUShort(pWS, offPos, 0x01bc)); offPos += 2;
    Call(PutULong(pWS, offPos, (U32)OFFSET_OF_PFD)); offPos += 4;

    //================
    // Write overflow area
    pDEMisc->uOffPixelFormat = (U32)offPos;
    PI.pGUIDPixFmt = &pIE->guidPixFormat;
    PixelFormatLookup(&PI, LOOKUP_FORWARD);

    //Call(pWS->Write(pWS, PI.pGUIDPixFmt, sizeof(*PI.pGUIDPixFmt))); offPos += 16;
    /** following code is endian-agnostic **/
    {
        unsigned char *pGuid = (unsigned char *) &pIE->guidPixFormat;
        Call(PutULong(pWS, offPos, ((U32 *)pGuid)[0]));
        Call(PutUShort(pWS, offPos + 4, ((U16 *)(pGuid + 4))[0]));
        Call(PutUShort(pWS, offPos + 6, ((U16 *)(pGuid + 6))[0]));
        Call(pWS->Write(pWS, pGuid + 8, 8));
        offPos += 16;
    }

    //================
    // PFD
    assert (offPos <= OFFSET_OF_PFD); // otherwise stuff is overwritten
    offPos = (size_t)OFFSET_OF_PFD;

    if(!pIE->WMP.bHasAlpha)//no alpha
        cWmpDEs -= 2;

    pDEMisc->uImageOffset = (U32)(offPos + sizeof(U16) + 12 * cWmpDEs + sizeof(U32));
                          
    Call(PutUShort(pWS, offPos, cWmpDEs)); offPos += 2;

    //================
    wmpDE = wmpDEs[i++];
    assert(WMP_tagPixelFormat == wmpDE.uTag);
    wmpDE.uValueOrOffset = pDEMisc->uOffPixelFormat;
    Call(WriteWmpDE(pWS, offPos, &wmpDE)); offPos += 12;

    wmpDE = wmpDEs[i++];
    assert(WMP_tagImageWidth == wmpDE.uTag);
    wmpDE.uValueOrOffset = pIE->uWidth;
    Call(WriteWmpDE(pWS, offPos, &wmpDE)); offPos += 12;

    wmpDE = wmpDEs[i++];
    assert(WMP_tagImageHeight == wmpDE.uTag);
    wmpDE.uValueOrOffset = pIE->uHeight;
    Call(WriteWmpDE(pWS, offPos, &wmpDE)); offPos += 12;
    
    wmpDE = wmpDEs[i++];
    assert(WMP_tagWidthResolution == wmpDE.uTag);
    wmpDE.f32 = pIE->fResX;
    Call(WriteWmpDE(pWS, offPos, &wmpDE)); offPos += 12;

    wmpDE = wmpDEs[i++];
    assert(WMP_tagHeightResolution == wmpDE.uTag);
    wmpDE.f32 = pIE->fResY;
    Call(WriteWmpDE(pWS, offPos, &wmpDE)); offPos += 12;
   
    wmpDE = wmpDEs[i++];
    assert(WMP_tagImageOffset == wmpDE.uTag);
    wmpDE.uValueOrOffset = pDEMisc->uImageOffset;
    Call(WriteWmpDE(pWS, offPos, &wmpDE)); offPos += 12;

    // fix up in WriteContainerPost()
    wmpDE = wmpDEs[i++];
    assert(WMP_tagImageByteCount == wmpDE.uTag);
    pDEMisc->uOffImageByteCount = (U32)offPos;
    wmpDE.uValueOrOffset = 0;
    Call(WriteWmpDE(pWS, offPos, &wmpDE)); offPos += 12;

    if(pIE->WMP.bHasAlpha)
    {
        // fix up in WriteContainerPost()
        wmpDE = wmpDEs[i++];
        assert(WMP_tagAlphaOffset == wmpDE.uTag);
        pDEMisc->uOffAlphaOffset = (U32)offPos;
        wmpDE.uValueOrOffset = 0;
        Call(WriteWmpDE(pWS, offPos, &wmpDE)); offPos += 12;

        // fix up in WriteContainerPost()
        wmpDE = wmpDEs[i++];
        assert(WMP_tagAlphaByteCount == wmpDE.uTag);
        pDEMisc->uOffAlphaByteCount = (U32)offPos;
        wmpDE.uValueOrOffset = 0;
        Call(WriteWmpDE(pWS, offPos, &wmpDE)); offPos += 12;
    }

    //================
    Call(PutULong(pWS, offPos, 0)); offPos += 4;

    assert(0 == (offPos & 1));
    assert(pDEMisc->uImageOffset == offPos);

Cleanup:
    return err;
}

ERR WriteContainerPost(
    PKImageEncode* pIE)
{
    ERR err = WMP_errSuccess;

    struct WMPStream* pWS = pIE->pStream;
    WmpDEMisc* pDEMisc = &pIE->WMP.wmiDEMisc;

    WmpDE deImageByteCount = {WMP_tagImageByteCount, 4,  1, 0};
    WmpDE deAlphaOffset     = {WMP_tagAlphaOffset, 4,  1, 0};
    WmpDE deAlphaByteCount  = {WMP_tagAlphaByteCount, 4,  1, 0};

    deImageByteCount.uValueOrOffset = pIE->WMP.nCbImage;
    Call(WriteWmpDE(pWS, pDEMisc->uOffImageByteCount, &deImageByteCount));

    //Alpha
    if(pIE->WMP.bHasAlpha)
    {                
        deAlphaOffset.uValueOrOffset = pIE->WMP.nOffAlpha;
        Call(WriteWmpDE(pWS, pDEMisc->uOffAlphaOffset, &deAlphaOffset));

        deAlphaByteCount.uValueOrOffset = pIE->WMP.nCbAlpha;
        Call(WriteWmpDE(pWS, pDEMisc->uOffAlphaByteCount, &deAlphaByteCount));
    }

Cleanup:
    return err;
}


//================================================
ERR PKImageEncode_Initialize_WMP(
    PKImageEncode* pIE,
    struct WMPStream* pStream,
    void* pvParam,
    size_t cbParam)
{
    ERR err = WMP_errSuccess;

    FailIf(sizeof(pIE->WMP.wmiSCP) != cbParam, WMP_errInvalidArgument);

    pIE->WMP.wmiSCP = *(CWMIStrCodecParam*)pvParam;
    pIE->WMP.wmiSCP_Alpha = *(CWMIStrCodecParam*)pvParam;
    pIE->pStream = pStream;

    pIE->WMP.wmiSCP.pWStream = pIE->pStream;
    pIE->WMP.wmiSCP_Alpha.pWStream = pIE->pStream;

Cleanup:
    return err;
}

ERR PKImageEncode_Terminate_WMP(
    PKImageEncode* pIE)
{
    ERR err = WMP_errSuccess;
    return err;
}

ERR PKImageEncode_EncodeContent(
    PKImageEncode* pIE, 
    PKPixelInfo PI,
    U32 cLine,
    U8* pbPixels,
    U32 cbStride)
{
    ERR err = WMP_errSuccess;
    U32 i = 0;
    size_t offPos = 0;

    // init codec
    pIE->WMP.wmiI.cWidth = pIE->uWidth;
    pIE->WMP.wmiI.cHeight = pIE->uHeight;
    pIE->WMP.wmiI.bdBitDepth = PI.bdBitDepth;
    pIE->WMP.wmiI.cBitsPerUnit = PI.cbitUnit;
    pIE->WMP.wmiI.bRGB = !(PI.grBit & PK_pixfmtBGR);
    pIE->WMP.wmiI.cfColorFormat = PI.cfColorFormat;
    
    // Set the fPaddedUserBuffer if the following conditions are met
    if (0 == ((size_t)pbPixels % 128) &&        // Frame buffer is aligned to 128-byte boundary
        0 == (pIE->uWidth % 16) &&              // Horizontal resolution is multiple of 16
        0 == (cLine % 16) &&                    // Vertical resolution is multiple of 16
        0 == (cbStride % 128))                  // Stride is a multiple of 128 bytes
    {
        pIE->WMP.wmiI.fPaddedUserBuffer = TRUE;
        // Note that there are additional conditions in strenc_x86.c's strEncOpt
        // which could prevent optimization from being engaged
    }

    //if (pIE->WMP.bHasAlpha)
    //{
    //    pIE->WMP.wmiSCP.cChannel = PI.cChannel - 1;
    //    pIE->WMP.wmiI.cfColorFormat = PI.cfStripAlpha;
    //}
    //else
    {
        pIE->WMP.wmiSCP.cChannel = PI.cChannel - 1;
    }

    Call(pIE->pStream->GetPos(pIE->pStream, &offPos));
    pIE->WMP.nOffImage = (Long)offPos;

    pIE->idxCurrentLine = 0;
    
    pIE->WMP.wmiSCP.fMeasurePerf = TRUE;
    FailIf(ICERR_OK != ImageStrEncInit(&pIE->WMP.wmiI, &pIE->WMP.wmiSCP, &pIE->WMP.ctxSC), WMP_errFail);

    //================================
    for (i = 0; i < cLine; i += 16)
    {
        CWMImageBufferInfo wmiBI = {pbPixels + cbStride * i / (pIE->WMP.wmiI.cfColorFormat == YUV_420 ? 2 : 1), min(16, cLine - i), cbStride};

        FailIf(ICERR_OK != ImageStrEncEncode(pIE->WMP.ctxSC, &wmiBI), WMP_errFail);
    }
    pIE->idxCurrentLine += cLine;

    FailIf(ICERR_OK != ImageStrEncTerm(pIE->WMP.ctxSC), WMP_errFail);

    Call(pIE->pStream->GetPos(pIE->pStream, &offPos));
    pIE->WMP.nCbImage = (Long)offPos - pIE->WMP.nOffImage;
    if(!pIE->WMP.bHasAlpha)//no alpha
        Call(WriteContainerPost(pIE));

Cleanup:
    return err;
}

ERR PKImageEncode_EncodeAlpha(
    PKImageEncode* pIE, 
    PKPixelInfo PI,
    U32 cLine,
    U8* pbPixels,
    U32 cbStride)
{
    ERR err = WMP_errSuccess;
    U32 i = 0;
    size_t offPos = 0;

    pIE->WMP.wmiI_Alpha = pIE->WMP.wmiI;

    pIE->WMP.wmiI_Alpha.cWidth = pIE->uWidth;
    pIE->WMP.wmiI_Alpha.cHeight = pIE->uHeight;
    pIE->WMP.wmiI_Alpha.bdBitDepth = PI.bdBitDepth;
    pIE->WMP.wmiI_Alpha.cBitsPerUnit = PI.cbitUnit;
    pIE->WMP.wmiI_Alpha.bRGB = !(PI.grBit & PK_pixfmtBGR);
//    pIE->WMP.wmiI_Alpha.cLeadingPadding += pIE->WMP.wmiSCP.cChannel;
//    pIE->WMP.wmiI_Alpha.cLeadingPadding += PI.cChannel - 1;

    switch (pIE->WMP.wmiI.bdBitDepth)
    {
        case BD_8:
            pIE->WMP.wmiI_Alpha.cLeadingPadding += (pIE->WMP.wmiI.cBitsPerUnit >> 3) - 1;
            break;
        
        case BD_16:
        case BD_16S:
        case BD_16F:
            pIE->WMP.wmiI_Alpha.cLeadingPadding += (pIE->WMP.wmiI.cBitsPerUnit >> 3) / sizeof(U16) - 1;
            break;
        
        case BD_32:
        case BD_32S:
        case BD_32F:
            pIE->WMP.wmiI_Alpha.cLeadingPadding += (pIE->WMP.wmiI.cBitsPerUnit >> 3) / sizeof(float) - 1;
            break;
        
        case BD_5:
        case BD_10:
        case BD_565:
        default:
            break;
    }

//    pIE->WMP.wmiSCP_Alpha.uAlphaMode = 1;


    //assert(pIE->WMP.wmiI_Alpha.cfColorFormat == CF_RGB); // only RGBA is supported for now!
    pIE->WMP.wmiI_Alpha.cfColorFormat = Y_ONLY;
    
    pIE->WMP.wmiSCP_Alpha.cfColorFormat = Y_ONLY;

    Call(pIE->pStream->GetPos(pIE->pStream, &offPos));
    pIE->WMP.nOffAlpha = (Long)offPos;

    pIE->idxCurrentLine = 0;
    pIE->WMP.wmiSCP_Alpha.fMeasurePerf = TRUE;
    FailIf(ICERR_OK != ImageStrEncInit(&pIE->WMP.wmiI_Alpha, &pIE->WMP.wmiSCP_Alpha, &pIE->WMP.ctxSC_Alpha), WMP_errFail);

    //================================
    for (i = 0; i < cLine; i += 16)
    {
        CWMImageBufferInfo wmiBI = {pbPixels + cbStride * i, min(16, cLine - i), cbStride};
        FailIf(ICERR_OK != ImageStrEncEncode(pIE->WMP.ctxSC_Alpha, &wmiBI), WMP_errFail);
    }
    pIE->idxCurrentLine += cLine;

    FailIf(ICERR_OK != ImageStrEncTerm(pIE->WMP.ctxSC_Alpha), WMP_errFail);

    Call(pIE->pStream->GetPos(pIE->pStream, &offPos));
    pIE->WMP.nCbAlpha = (Long)offPos - pIE->WMP.nOffAlpha;
    Call(WriteContainerPost(pIE));
Cleanup:
    return err;
}

ERR PKImageEncode_WritePixels_WMP(
    PKImageEncode* pIE,
    U32 cLine,
    U8* pbPixels,
    U32 cbStride)
{
    ERR err = WMP_errSuccess;
    U32 i = 0;
    PKPixelInfo PI;

    PI.pGUIDPixFmt = &pIE->guidPixFormat;
    PixelFormatLookup(&PI, LOOKUP_FORWARD);
    pIE->WMP.bHasAlpha = !!(PI.grBit & PK_pixfmtHasAlpha);

    if (!pIE->fHeaderDone)
    {
        // write metadata
        Call(WriteContainerPre(pIE));

        pIE->fHeaderDone = !FALSE;
    }

/*    if (pIE->WMP.bHasAlpha && pIE->WMP.wmiSCP.uAlphaMode == 2){
        pIE->WMP.wmiSCP_Alpha = pIE->WMP.wmiSCP;
    }
*/
    PKImageEncode_EncodeContent(pIE, PI, cLine, pbPixels, cbStride);
    if (pIE->WMP.bHasAlpha && pIE->WMP.wmiSCP.uAlphaMode == 2){//planar alpha
        PKImageEncode_EncodeAlpha(pIE, PI, cLine, pbPixels, cbStride);
    }

Cleanup:
    return err;
}

ERR PKImageEncode_Transcode_WMP(
    PKImageEncode* pIE,
    PKImageDecode* pID,
    CWMTranscodingParam* pParam)
{
    ERR err = WMP_errSuccess;
    Float fResX = 0, fResY = 0;
    PKPixelFormatGUID pixGUID = {0};

    struct WMPStream* pWSDec = NULL;
    struct WMPStream* pWSEnc= pIE->pStream;

    // pass through metadata
    Call(pID->GetPixelFormat(pID, &pixGUID));
    Call(pIE->SetPixelFormat(pIE, pixGUID));

    Call(pIE->SetSize(pIE, (I32)pParam->cWidth, (I32)pParam->cHeight));

    Call(pID->GetResolution(pID, &fResX, &fResY));
    Call(pIE->SetResolution(pIE, fResX, fResY));

    // write matadata
    Call(WriteContainerPre(pIE));

    // write compressed bitstream
    Call(pID->GetRawStream(pID, &pWSDec));
    FailIf(ICERR_OK != WMPhotoTranscode(pWSDec, pWSEnc, pParam), WMP_errFail);

    // fixup matadata
    Call(WriteContainerPost(pIE));

Cleanup:
    return err;
}

ERR PKImageEncode_CreateNewFrame_WMP(
    PKImageEncode* pIE,
    void* pvParam,
    size_t cbParam)
{
    ERR err = WMP_errSuccess;

    Call(WMP_errNotYetImplemented);
    
Cleanup:
    return err;
}

ERR PKImageEncode_Release_WMP(
    PKImageEncode** ppIE)
{
    ERR err = WMP_errSuccess;

    PKImageEncode *pIE = *ppIE;
    pIE->pStream->Close(&pIE->pStream);

    Call(PKFree(ppIE));

Cleanup:
    return err;
}

//----------------------------------------------------------------
ERR PKImageEncode_Create_WMP(PKImageEncode** ppIE)
{
    ERR err = WMP_errSuccess;

    PKImageEncode* pIE = NULL;

    Call(PKImageEncode_Create(ppIE));

    pIE = *ppIE;
    pIE->Initialize = PKImageEncode_Initialize_WMP;
    pIE->Terminate = PKImageEncode_Terminate_WMP;
    pIE->WritePixels = PKImageEncode_WritePixels_WMP;
    pIE->Transcode = PKImageEncode_Transcode_WMP;
    pIE->CreateNewFrame = PKImageEncode_CreateNewFrame_WMP;
    pIE->Release = PKImageEncode_Release_WMP;
	pIE->bWMP = TRUE; 

Cleanup:
    return err;
}


//================================================================
// PKImageDecode_WMP
//================================================================
ERR ParsePFDEntry(
    PKImageDecode* pID,
    U16 uTag,
    U16 uType,
    U32 uCount,
    U32 uValue)
{
    ERR err = WMP_errSuccess;
    PKPixelInfo PI;
    struct WMPStream* pWS = pID->pStream;
    size_t offPos = 0;

    union uf{
        U32 uVal;
        Float fVal;
    }ufValue = {0};

    //================================
    switch (uTag)
    {
        case WMP_tagPixelFormat:
        {
            unsigned char *pGuid = (unsigned char *) &pID->guidPixFormat;
            /** following code is endian-agnostic **/
            Call(GetULong(pWS, uValue, (unsigned long *)pGuid));
            Call(GetUShort(pWS, uValue + 4, (unsigned short *)(pGuid + 4)));
            Call(GetUShort(pWS, uValue + 6, (unsigned short *)(pGuid + 6)));
            Call(pWS->Read(pWS, pGuid + 8, 8));
                
            PI.pGUIDPixFmt = &pID->guidPixFormat;
            PixelFormatLookup(&PI, LOOKUP_FORWARD);

            pID->WMP.bHasAlpha = !!(PI.grBit & PK_pixfmtHasAlpha);
            pID->WMP.wmiI.cBitsPerUnit = PI.cbitUnit;
            pID->WMP.wmiI.bRGB = !(PI.grBit & PK_pixfmtBGR);

            break;
        }

        case WMP_tagImageWidth:
            FailIf(0 == uValue, WMP_errUnsupportedFormat);
            break;

        case WMP_tagImageHeight:
            FailIf(0 == uValue, WMP_errUnsupportedFormat);
            break;

        case WMP_tagImageOffset:
            FailIf(1 != uCount, WMP_errUnsupportedFormat);
            pID->WMP.wmiDEMisc.uImageOffset = uValue;
            break;

        case WMP_tagImageByteCount:
            FailIf(1 != uCount, WMP_errUnsupportedFormat);
            pID->WMP.wmiDEMisc.uImageByteCount = uValue;
            break;

        case WMP_tagAlphaOffset:
            FailIf(1 != uCount, WMP_errUnsupportedFormat);
            pID->WMP.wmiDEMisc.uAlphaOffset = uValue;
            break;

        case WMP_tagAlphaByteCount:
            FailIf(1 != uCount, WMP_errUnsupportedFormat);
            pID->WMP.wmiDEMisc.uAlphaByteCount = uValue;
            break;

        case WMP_tagWidthResolution:
            FailIf(1 != uCount, WMP_errUnsupportedFormat);
            ufValue.uVal = uValue; 
            pID->fResX = ufValue.fVal;
            break;

        case WMP_tagHeightResolution:
            FailIf(1 != uCount, WMP_errUnsupportedFormat);
            ufValue.uVal = uValue; 
            pID->fResY = ufValue.fVal;
            break;

        case WMP_tagXMPMetadata:
        case WMP_tagIccProfile:
        case WMP_tagEXIFMetadata:
        case WMP_tagTransformation:
        case WMP_tagCompression:
        case WMP_tagImageType:
        case WMP_tagImageDataDiscard:
        case WMP_tagAlphaDataDiscard:
            break;

        default:
            fprintf(stderr, "Unrecognized WMPTag: %d(%#x), %d, %d, %#x" CRLF,
                (int)uTag, (int)uTag, (int)uType, (int)uCount, (int)uValue);
            break;
    }

Cleanup:
    return err;
}

ERR ParsePFD(
    PKImageDecode* pID,
    size_t offPos,
    U16 cEntry)
{
    ERR err = WMP_errSuccess;
    struct WMPStream* pWS = pID->pStream;
    U16 i = 0;

    for (i = 0; i < cEntry; ++i)
    {
        U16 uTag = 0;
        U16 uType = 0;
        U32 uCount = 0;
        U32 uValue = 0;

        Call(GetUShort(pWS, offPos, &uTag)); offPos += 2;
        Call(GetUShort(pWS, offPos, &uType)); offPos += 2;
        Call(GetULong(pWS, offPos, &uCount)); offPos += 4;
        Call(GetULong(pWS, offPos, &uValue)); offPos += 4;

        Call(ParsePFDEntry(pID, uTag, uType, uCount, uValue)); 
    }

    pID->WMP.bHasAlpha = ((pID->WMP.bHasAlpha) && (pID->WMP.wmiDEMisc.uAlphaOffset != 0) && (pID->WMP.wmiDEMisc.uAlphaByteCount != 0));//has planar alpha

Cleanup:
    return err;
}

ERR ReadContainer(
    PKImageDecode* pID)
{
    ERR err = WMP_errSuccess;

    struct WMPStream* pWS = pID->pStream;
    size_t offPos = 0;

    char szSig[2] = {0};
    U16 uWmpID = 0;
    U32 offPFD = 0;
    U16 cPFDEntry = 0;
    U8 bVersion;
    
    //================================
    Call(pWS->GetPos(pWS, &offPos));
    FailIf(0 != offPos, WMP_errUnsupportedFormat);

    //================================
    // Header
    Call(pWS->Read(pWS, szSig, sizeof(szSig))); offPos += 2;
    FailIf(szSig != strstr(szSig, "II"), WMP_errUnsupportedFormat);

    Call(GetUShort(pWS, offPos, &uWmpID)); offPos += 2;
    FailIf(WMP_valWMPhotoID != (0x00FF & uWmpID), WMP_errUnsupportedFormat);

    // We accept version 00 and version 01 bitstreams - all others rejected
    bVersion = (0xFF00 & uWmpID) >> 8;
    FailIf(bVersion != 0 && bVersion != 1, WMP_errUnsupportedFormat);

    Call(GetULong(pWS, offPos, &offPFD)); offPos += 4;

    //================================
    // PFD
    offPos = (size_t)offPFD;
    Call(GetUShort(pWS, offPos, &cPFDEntry)); offPos += 2;
    FailIf(0 == cPFDEntry || USHRT_MAX == cPFDEntry, WMP_errUnsupportedFormat);
    Call(ParsePFD(pID, offPos, cPFDEntry));

    //================================
    Call(pWS->SetPos(pWS, pID->WMP.wmiDEMisc.uImageOffset));

Cleanup:
    return err;
}


//================================================
ERR PKImageDecode_Initialize_WMP(
    PKImageDecode* pID,
    struct WMPStream* pWS)
{
    ERR err = WMP_errSuccess;

    CWMImageInfo* pII = NULL;

    //================================
    Call(PKImageDecode_Initialize(pID, pWS));

    //================================
    Call(ReadContainer(pID));

    //================================
    pID->WMP.wmiSCP.pWStream = pWS;

    FailIf(ICERR_OK != ImageStrDecGetInfo(&pID->WMP.wmiI, &pID->WMP.wmiSCP), WMP_errFail);
    assert(Y_ONLY <= pID->WMP.wmiSCP.cfColorFormat && pID->WMP.wmiSCP.cfColorFormat < CFT_MAX);
    assert(BD_SHORT == pID->WMP.wmiSCP.bdBitDepth || BD_LONG == pID->WMP.wmiSCP.bdBitDepth);

    pII = &pID->WMP.wmiI;
    pID->uWidth = (U32)pII->cWidth;
    pID->uHeight = (U32)pII->cHeight;

Cleanup:
    return err;
}

ERR PKImageDecode_GetRawStream_WMP(
    PKImageDecode* pID,
    struct WMPStream** ppWS)
{
    ERR err = WMP_errSuccess;
    struct WMPStream* pWS = pID->pStream;

    *ppWS = NULL;
    Call(pWS->SetPos(pWS, pID->WMP.wmiDEMisc.uImageOffset));
    *ppWS = pWS;

Cleanup:
    return err;
}

ERR PKImageDecode_Copy_WMP(
    PKImageDecode* pID,
    const PKRect* pRect,
    U8* pb,
    U32 cbStride)
{
    ERR err = WMP_errSuccess;
    CWMImageBufferInfo wmiBI = {pb, pRect->Height, cbStride};
    struct WMPStream* pWS = pID->pStream;
    U8 tempAlphaMode = 0;

    FailIf(0 != pRect->X, WMP_errInvalidParameter);
    FailIf(0 != pRect->Y, WMP_errInvalidParameter);

    // Set the fPaddedUserBuffer if the following conditions are met
    if (0 == ((size_t)pb % 128) &&    // Frame buffer is aligned to 128-byte boundary
        0 == (pRect->Height % 16) &&        // Horizontal resolution is multiple of 16
        0 == (pRect->Width % 16) &&        // Vertical resolution is multiple of 16
        0 == (cbStride % 128))              // Stride is a multiple of 128 bytes
    {
        pID->WMP.wmiI.fPaddedUserBuffer = TRUE;
        // Note that there are additional conditions in strdec_x86.c's strDecOpt
        // which could prevent optimization from being engaged
    }

    //if(pID->WMP.wmiSCP.uAlphaMode != 1)
    if((!pID->WMP.bHasAlpha) || (pID->WMP.wmiSCP.uAlphaMode != 1))
    {
        if(pID->WMP.bHasAlpha)//planar alpha
        {
            tempAlphaMode = pID->WMP.wmiSCP.uAlphaMode;
            pID->WMP.wmiSCP.uAlphaMode = 0;
        }
        pID->WMP.wmiSCP.fMeasurePerf = TRUE;
        FailIf(ICERR_OK != ImageStrDecInit(&pID->WMP.wmiI, &pID->WMP.wmiSCP, &pID->WMP.ctxSC), WMP_errFail);
        FailIf(ICERR_OK != ImageStrDecDecode(pID->WMP.ctxSC, &wmiBI), WMP_errFail);
        FailIf(ICERR_OK != ImageStrDecTerm(pID->WMP.ctxSC), WMP_errFail);

        if(pID->WMP.bHasAlpha)//planar alpha
        {
            pID->WMP.wmiSCP.uAlphaMode = tempAlphaMode;
        }
    }

//    if(pID->WMP.bHasAlpha && pID->WMP.wmiSCP.uAlphaMode == 2)
//    if(pID->WMP.bHasAlpha && pID->WMP.wmiSCP.uAlphaMode != 1)
    if(pID->WMP.bHasAlpha && pID->WMP.wmiSCP.uAlphaMode != 0)
    {
        pID->WMP.wmiI_Alpha = pID->WMP.wmiI;
        pID->WMP.wmiSCP_Alpha = pID->WMP.wmiSCP;

//        assert(pID->WMP.wmiI_Alpha.cfColorFormat == CF_RGB); // only RGBA is supported for now!
        pID->WMP.wmiI_Alpha.cfColorFormat = Y_ONLY;

        switch (pID->WMP.wmiI.bdBitDepth)
        {
            case BD_8:
                pID->WMP.wmiI_Alpha.cLeadingPadding += (pID->WMP.wmiI.cBitsPerUnit >> 3) - 1;
                break;
            
            case BD_16:
            case BD_16S:
            case BD_16F:
                pID->WMP.wmiI_Alpha.cLeadingPadding += (pID->WMP.wmiI.cBitsPerUnit >> 3) / sizeof(U16) - 1;
                break;
            
            case BD_32:
            case BD_32S:
            case BD_32F:
                pID->WMP.wmiI_Alpha.cLeadingPadding += (pID->WMP.wmiI.cBitsPerUnit >> 3) / sizeof(float) - 1;
                break;
            
            case BD_5:
            case BD_10:
            case BD_565:
            default:
                break;
        }

        pID->WMP.wmiSCP_Alpha.fMeasurePerf = TRUE;
        Call(pWS->SetPos(pWS, pID->WMP.wmiDEMisc.uAlphaOffset));
        FailIf(ICERR_OK != ImageStrDecInit(&pID->WMP.wmiI_Alpha, &pID->WMP.wmiSCP_Alpha, &pID->WMP.ctxSC_Alpha), WMP_errFail);
        FailIf(ICERR_OK != ImageStrDecDecode(pID->WMP.ctxSC_Alpha, &wmiBI), WMP_errFail);
        FailIf(ICERR_OK != ImageStrDecTerm(pID->WMP.ctxSC_Alpha), WMP_errFail);
    }

    pID->idxCurrentLine += pRect->Height;

Cleanup:
    return err;
}

ERR PKImageDecode_Create_WMP(PKImageDecode** ppID)
{
    ERR err = WMP_errSuccess;
    PKImageDecode* pID = NULL;

    Call(PKImageDecode_Create(ppID));

    pID = *ppID;
    pID->Initialize = PKImageDecode_Initialize_WMP;
    pID->GetRawStream = PKImageDecode_GetRawStream_WMP;
    pID->Copy = PKImageDecode_Copy_WMP;

Cleanup:
    return err;
}


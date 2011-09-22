//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
#include "strcodec.h"
#include "encode.h"
#include "strTransform.h"
#include <math.h>

//#define STRESS_PARAM
//#define STRESS_DATA

#ifdef STRESS_PARAM
#include <time.h>
#endif

#ifdef MEM_TRACE
#define TRACE_MALLOC    1
#define TRACE_NEW       0
#define TRACE_HEAP      0
#include "memtrace.h"
#endif

#ifdef ADI_SYS_OPT
extern char L1WW[];
#endif

#ifdef X86OPT_INLINE
#define _FORCEINLINE __forceinline
#else // X86OPT_INLINE
#define _FORCEINLINE
#endif // X86OPT_INLINE

#if (defined (DEBUG_QUANT) || defined(DIFF_QUANT))
#include <stdio.h>
    FILE *fQuant_g = NULL;
#endif

Int inputMBRow(CWMImageStrCodec *);

#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_ENC) || defined(WMP_OPT_TRFM_ENC)
void StrEncOpt(CWMImageStrCodec* pSC);
#endif // OPT defined

#define MINIMUM_PACKET_LENGTH 4  // as long as packet header - skipped if data is not accessed (happens only for flexbits)

Void writeQuantizer(CWMIQuantizer * pQuantizer[MAX_CHANNELS], BitIOInfo * pIO, U8 cChMode, size_t cChannel, size_t iPos)
{
    if(cChMode > 2)
        cChMode = 2;

    if(cChannel > 1)
        putBit16(pIO, cChMode, 2); // Channel mode
    else
        cChMode = 0;

    putBit16(pIO, pQuantizer[0][iPos].iIndex, 8); // Y

    if(cChMode == 1)  // MIXED
        putBit16(pIO, pQuantizer[1][iPos].iIndex, 8); // UV
    else if(cChMode > 0){ // INDEPENDENT
        size_t i;

        for(i = 1; i < cChannel; i ++)
            putBit16(pIO, pQuantizer[i][iPos].iIndex, 8); // UV
    }
}

// packet header: 00000000 00000000 00000001 ?????xxx
// xxx:           000(spatial) 001(DC) 010(AD) 011(AC) 100(FL) 101-111(reserved)
// ?????:         (iTileY * cNumOfSliceV + iTileX)
Void writePacketHeader(BitIOInfo * pIO, U8 ptPacketType, U8 pID)
{
    putBit16(pIO, 0, 8);
    putBit16(pIO, 0, 8);
    putBit16(pIO, 1, 8);
    putBit16(pIO, (pID << 3) + (ptPacketType & 7), 8);
}

Int writeTileHeaderDC(CWMImageStrCodec * pSC, BitIOInfo * pIO)
{
    size_t iTile, j = (pSC->m_pNextSC == NULL ? 1U : 2U);

    for(; j > 0; j --){
        if((pSC->m_param.uQPMode & 1) != 0){ // not DC uniform
            CWMITile * pTile = pSC->pTile + pSC->cTileColumn;
            size_t i;
            
            pTile->cChModeDC = (U8)(rand() & 3); // channel mode, just for concept proofing!
            
            if(pSC->cTileRow + pSC->cTileColumn == 0) // allocate DC QP info
                for(iTile = 0; iTile <= pSC->WMISCP.cNumOfSliceMinus1V; iTile ++)
                    if(allocateQuantizer(pSC->pTile[iTile].pQuantizerDC, pSC->m_param.cNumChannels, 1) != ICERR_OK)
                        return ICERR_ERROR;
            
            for(i = 0; i < pSC->m_param.cNumChannels; i ++)
                pTile->pQuantizerDC[i]->iIndex = (U8)((rand() & 0x2f) + 1); // QP indexes, just for concept proofing!
            
            formatQuantizer(pTile->pQuantizerDC, pTile->cChModeDC, pSC->m_param.cNumChannels, 0, TRUE, pSC->m_param.bScaledArith);

            for(i = 0; i < pSC->m_param.cNumChannels; i ++)
                pTile->pQuantizerDC[i]->iOffset = (pTile->pQuantizerDC[i]->iQP >> 1);
            
            writeQuantizer(pTile->pQuantizerDC, pIO, pTile->cChModeDC, pSC->m_param.cNumChannels, 0);
        }

        pSC = pSC->m_pNextSC;
    }

    return ICERR_OK;
}

Int writeTileHeaderLP(CWMImageStrCodec * pSC, BitIOInfo * pIO)
{
    size_t k = (pSC->m_pNextSC == NULL ? 1U : 2U);
    
    for(; k > 0; k --){
        if(pSC->WMISCP.sbSubband != SB_DC_ONLY && (pSC->m_param.uQPMode & 2) != 0){ // not LP uniform
            CWMITile * pTile = pSC->pTile + pSC->cTileColumn;
            U8 i, j;

            pTile->bUseDC = ((rand() & 1) == 0 ? TRUE : FALSE); // use DC quantizer?
            putBit16(pIO, pTile->bUseDC == TRUE ? 1 : 0, 1);
            pTile->cBitsLP = 0;
            
            pTile->cNumQPLP = (pTile->bUseDC == TRUE ? 1 : (U8)((rand() & 0xf) + 1)); // # of LP QPs
            
            if(pSC->cTileRow > 0)
                freeQuantizer(pTile->pQuantizerLP);
            
            if(allocateQuantizer(pTile->pQuantizerLP, pSC->m_param.cNumChannels, pTile->cNumQPLP) != ICERR_OK)
                return ICERR_ERROR;

            if(pTile->bUseDC == TRUE)
                useDCQuantizer(pSC, pSC->cTileColumn);
            else{
                putBit16(pIO, pTile->cNumQPLP - 1, 4);
                
                pTile->cBitsLP = dquantBits(pTile->cNumQPLP);
                
                for(i = 0; i < pTile->cNumQPLP; i ++){
                    pTile->cChModeLP[i] = (U8)(rand() & 3); // channel mode, just for concept proofing!
                    
                    for(j = 0; j < pSC->m_param.cNumChannels; j ++)
                        pTile->pQuantizerLP[j][i].iIndex = (U8)((rand() & 0xfe) + 1); // QP indexes, just for concept proofing!
                    formatQuantizer(pTile->pQuantizerLP, pTile->cChModeLP[i], pSC->m_param.cNumChannels, i, TRUE, pSC->m_param.bScaledArith);
                    writeQuantizer(pTile->pQuantizerLP, pIO, pTile->cChModeLP[i], pSC->m_param.cNumChannels, i);
                }
            }
        }
        pSC = pSC->m_pNextSC;
    }

    return ICERR_OK;
}

Int writeTileHeaderHP(CWMImageStrCodec * pSC, BitIOInfo * pIO)
{
    size_t k = (pSC->m_pNextSC == NULL ? 1U : 2U);
    
    for(; k > 0; k --){
        if(pSC->WMISCP.sbSubband != SB_DC_ONLY && pSC->WMISCP.sbSubband != SB_NO_HIGHPASS && (pSC->m_param.uQPMode & 4) != 0){ // not HP uniform
            CWMITile * pTile = pSC->pTile + pSC->cTileColumn;
            U8 i, j;

            pTile->bUseLP = ((rand() & 1) == 0 ? TRUE : FALSE); // use LP quantizer?
            putBit16(pIO, pTile->bUseLP == TRUE ? 1 : 0, 1);
            pTile->cBitsHP = 0;
            
            pTile->cNumQPHP = (pTile->bUseLP == TRUE ? pTile->cNumQPLP : (U8)((rand() & 0xf) + 1)); // # of LP QPs
            
            if(pSC->cTileRow > 0)
                freeQuantizer(pTile->pQuantizerHP);
            
            if(allocateQuantizer(pTile->pQuantizerHP, pSC->m_param.cNumChannels, pTile->cNumQPHP) != ICERR_OK)
                return ICERR_ERROR;
            
            if(pTile->bUseLP == TRUE)
                useLPQuantizer(pSC, pTile->cNumQPHP, pSC->cTileColumn);
            else{
                putBit16(pIO, pTile->cNumQPHP - 1, 4);
                pTile->cBitsHP = dquantBits(pTile->cNumQPHP);
                
                for(i = 0; i < pTile->cNumQPHP; i ++){
                    pTile->cChModeHP[i] = (U8)(rand() & 3); // channel mode, just for concept proofing!
                    
                    for(j = 0; j < pSC->m_param.cNumChannels; j ++)
                        pTile->pQuantizerHP[j][i].iIndex = (U8)((rand() & 0xfe) + 1); // QP indexes, just for concept proofing!
                    formatQuantizer(pTile->pQuantizerHP, pTile->cChModeHP[i], pSC->m_param.cNumChannels, i, FALSE, pSC->m_param.bScaledArith);
                    writeQuantizer(pTile->pQuantizerHP, pIO, pTile->cChModeHP[i], pSC->m_param.cNumChannels, i);
                }
            }
        }
        pSC = pSC->m_pNextSC;
    }

    return ICERR_OK;
}

Int encodeMB(CWMImageStrCodec * pSC, Int iMBX, Int iMBY)
{
    CCodingContext * pContext = &pSC->m_pCodingContext[pSC->cTileColumn];
    
    if(pSC->m_bCtxLeft && pSC->m_bCtxTop && pSC->m_bSecondary == FALSE && pSC->m_param.bTranscode == FALSE){ // write packet headers
        U8 pID = (U8)((pSC->cTileRow * (pSC->WMISCP.cNumOfSliceMinus1V + 1) + pSC->cTileColumn) & 0x1F);
        
        if(pSC->WMISCP.bfBitstreamFormat == SPATIAL) {
            writePacketHeader(pContext->m_pIODC, 0, pID);
            if (pSC->m_param.bTrimFlexbitsFlag)
                putBit16(pContext->m_pIODC, pContext->m_iTrimFlexBits, 4);
            writeTileHeaderDC(pSC, pContext->m_pIODC);
            writeTileHeaderLP(pSC, pContext->m_pIODC);
            writeTileHeaderHP(pSC, pContext->m_pIODC);
        }
        else{
            writePacketHeader(pContext->m_pIODC, 1, pID);
            writeTileHeaderDC(pSC, pContext->m_pIODC);
            if(pSC->cSB > 1){
                writePacketHeader(pContext->m_pIOLP, 2, pID);
                writeTileHeaderLP(pSC, pContext->m_pIOLP);
            }
            if(pSC->cSB > 2){
                writePacketHeader(pContext->m_pIOAC, 3, pID);
                writeTileHeaderHP(pSC, pContext->m_pIOAC);
            }
            if(pSC->cSB > 3) {
                writePacketHeader(pContext->m_pIOFL, 4, pID);
                if (pSC->m_param.bTrimFlexbitsFlag)
                    putBit16(pContext->m_pIOFL, pContext->m_iTrimFlexBits, 4);
            }
        }
    }
    
    if(EncodeMacroblockDC(pSC, pContext, iMBX, iMBY) != ICERR_OK)
        return ICERR_ERROR;
    
    if(pSC->WMISCP.sbSubband != SB_DC_ONLY)
        if(EncodeMacroblockLowpass(pSC, pContext, iMBX, iMBY) != ICERR_OK)
            return ICERR_ERROR;

    if(pSC->WMISCP.sbSubband != SB_DC_ONLY && pSC->WMISCP.sbSubband != SB_NO_HIGHPASS)
        if(EncodeMacroblockHighpass(pSC, pContext, iMBX, iMBY) != ICERR_OK)
            return ICERR_ERROR;
    
    if(iMBX + 1 == pSC->cmbWidth && (iMBY + 1 == pSC->cmbHeight || 
        (pSC->cTileRow < pSC->WMISCP.cNumOfSliceMinus1H && iMBY == pSC->WMISCP.uiTileY[pSC->cTileRow + 1] - 1)))
    { // end of a horizontal slice
        size_t k, l;

        // get sizes of each packet and update index table
        if (pSC->m_pNextSC == NULL || pSC->m_bSecondary) {
            for(k = 0; k < pSC->cNumBitIO; k ++){
                fillToByte(pSC->m_ppBitIO[k]);
                pSC->ppWStream[k]->GetPos(pSC->ppWStream[k], &l);
                pSC->pIndexTable[pSC->cNumBitIO * pSC->cTileRow + k] = l + getSizeWrite(pSC->m_ppBitIO[k]); // offset
            }
        }
        
        // reset coding contexts
        if(iMBY + 1 != pSC->cmbHeight){
            for(k = 0; k <= pSC->WMISCP.cNumOfSliceMinus1V; k ++)
                ResetCodingContextEnc(&pSC->m_pCodingContext[k]);
        }
    }

    return ICERR_OK;
}

/*************************************************************************
    Top level function for processing a macroblock worth of input
*************************************************************************/
Int processMacroblock(CWMImageStrCodec *pSC)
{
    Bool topORleft = (pSC->cColumn == 0 || pSC->cRow == 0);
    ERR_CODE result = ICERR_OK;
    size_t j, jend = (pSC->m_pNextSC != NULL);

    for (j = 0; j <= jend; j++) {
        transformMacroblock(pSC);
        if(!topORleft){
            getTilePos(pSC, (Int)pSC->cColumn - 1, (Int)pSC->cRow - 1);
            if(jend){
                pSC->m_pNextSC->cTileRow = pSC->cTileRow;
                pSC->m_pNextSC->cTileColumn = pSC->cTileColumn;
            }
            if ((result = encodeMB(pSC, (Int)pSC->cColumn - 1, (Int)pSC->cRow - 1)) != ICERR_OK)
                return result;
        }

        if (jend) {
            pSC->m_pNextSC->cRow = pSC->cRow;
            pSC->m_pNextSC->cColumn = pSC->cColumn;
            pSC = pSC->m_pNextSC;
        }
    }

    return ICERR_OK;
}

/*************************************************************************
  forwardRGBE: forward conversion from RGBE to RGB
*************************************************************************/
static _FORCEINLINE PixelI forwardRGBE (PixelI RGB, PixelI E)
{
    PixelI iResult = 0, iAppend = 1;

    if (E == 0 || RGB == 0)
        return 0;

    assert (E!=0);

    E--;
    while (((RGB & 0x80) == 0) && (E > 0)) {
        RGB = (RGB << 1) + iAppend;
        iAppend = 0;
        E--;    
    }

    // result will always be one of 3 cases
    // E  RGB       convert to
    // 0  [0.x]      [0   x]
    // 0  [1.x]      [1   x]
    // e  [1.x]      [e+1 x]
    if (E == 0) {
        iResult = RGB;
    }
    else {
        E++;
        iResult = (RGB & 0x7f) + (E << 7);
    }

    return iResult;
}

/*************************************************************************
  convert float-32 into float with (c, lm)!!
*************************************************************************/
static _FORCEINLINE PixelI float2pixel (float f, const char _c, const unsigned char _lm)
{
    union uif
    {
        I32   i;
        float f;
    } x;

    PixelI _h, e, e1, m, s;

    if (f == 0)
    {
        _h = 0;
    }
    else
    {
        x.f = f;

        e = (x.i >> 23) & 0x000000ff;//here set e as e, not s! e includes s: [s e] 9 bits [31..23]
        m = (x.i & 0x007fffff) | 0x800000; // actual mantissa, with normalizer
        if (e == 0) { // denormal-land
            m ^= 0x800000;  // actual mantissa, removing normalizer
            e++; // actual exponent -126
        }

        e1 = e - 127 + _c;  // this is basically a division or quantization to a different exponent
                            // note: _c cannot be greater than 127, so e1 cannot be greater than e
        //assert (_c <= 127);
        if (e1 <= 1) {  // denormal-land
            if (e1 < 1)
                m >>= (1 - e1);  // shift mantissa right to make exponent 1
            e1 = 1;
            if ((m & 0x800000) == 0) // if denormal, set e1 to zero else to 1
                e1 = 0;
        }
        m &= 0x007fffff;

        //for float-22:	    
        _h = (e1 << _lm) + ((m + (1 << (23 - _lm - 1))) >> (23 - _lm));//take 23-bit m, shift (23-lm), get lm-bit m for float22
        s = ((PixelI) x.i) >> 31;
        //padding to int-32: 
        _h = (_h ^ s) - s;	
    }

    return _h;
}

/*************************************************************************
  convert Half-16 to internal format, only need to handle sign bit
*************************************************************************/
static _FORCEINLINE PixelI forwardHalf (PixelI hHalf)
{
    PixelI s;
    s = hHalf >> 31;
    hHalf = ((hHalf & 0x7fff) ^ s) - s;
    return hHalf;
}


//================================================================
// Color Conversion 
// functions to get image data from input buffer
// this inlcudes necessary color conversion and boundary padding
//#define _CC(r, g, b) (b -= r, r += (b >> 1), g -= r, r += (g >> 1))
//#define _CC(r, g, b) (g += (b+3*r+2) >> 2, r -= ((g) >> 1), b -= ((g) >> 1), r^=g^=r^=g, g^=b^=g^=b) 
//================================================================
#define _CC(r, g, b) (b -= r, r += ((b + 1) >> 1) - g, g += ((r + 0) >> 1))
#define _CC_CMYK(c, m, y, k) (y -= c, c += ((y + 1) >> 1) - m, m += (c >> 1) - k, k += ((m + 1) >> 1))

//================================================================
// BitIOInfo init/term for encoding
const size_t MAX_MEMORY_SIZE_IN_WORDS = 64 << 20; // 1 << 20 \approx 1 million

Int StrIOEncInit(CWMImageStrCodec* pSC)
{
    pSC->m_param.bIndexTable = !(pSC->WMISCP.bfBitstreamFormat == SPATIAL && pSC->WMISCP.cNumOfSliceMinus1H + pSC->WMISCP.cNumOfSliceMinus1V == 0);
    if(allocateBitIOInfo(pSC) != ICERR_OK){
        return ICERR_ERROR;
    }

    attachISWrite(pSC->pIOHeader, pSC->WMISCP.pWStream);

    if(pSC->cNumBitIO > 0){
        size_t i;
//#ifdef _WINDOWS_
#if defined(_WINDOWS_) || defined(UNDER_CE)  //tmpnam does not exist in VS2005 WinCE CRT
        TCHAR szPath[MAX_PATH];
        TCHAR szFile[MAX_PATH];
        DWORD cSize, j, k;
        char * pFilename = (char *)szFile;
#else
        char * pFilename;
#endif

        pSC->ppWStream = (struct WMPStream **)malloc(pSC->cNumBitIO * sizeof(struct WMPStream *));
        if(pSC->ppWStream == NULL) return ICERR_ERROR;
        memset(pSC->ppWStream, 0, pSC->cNumBitIO * sizeof(struct WMPStream *));

        for(i = 0; i < pSC->cNumBitIO; i ++){
            if (pSC->cmbHeight * pSC->cmbWidth * pSC->WMISCP.cChannel >= MAX_MEMORY_SIZE_IN_WORDS) {
//#ifdef _WINDOWS_
#if defined(_WINDOWS_) || defined(UNDER_CE)  // tmpnam does not exist in VS2005 WinCE CRT
                cSize = GetTempPath(MAX_PATH, szPath);
                if(cSize == 0 || cSize >= MAX_PATH)
                    return ICERR_ERROR;
                if(!GetTempFileName(szPath, TEXT("wdp"), 0, szFile))
                    return ICERR_ERROR;

                if(sizeof(TCHAR) == 2){ // unicode file name
                    for(k = j = cSize = 0; cSize < MAX_PATH; cSize ++, j += 2){
                        if(szFile[cSize] == '\0')
                            break;
                        if(pFilename[j] != '\0')
                            pFilename[k ++] = pFilename[j];
                        if(pFilename[j + 1] != '\0')
                            pFilename[k ++] = pFilename[j + 1];
                    }
                    pFilename[cSize] = '\0';
                }
#else
                pFilename = tmpnam(NULL);
#endif
                if(CreateWS_File(pSC->ppWStream + i, pFilename, "w+b") != ICERR_OK) return ICERR_ERROR;
            }
            else {
                if(CreateWS_List(pSC->ppWStream + i) != ICERR_OK) return ICERR_ERROR;
            }
            attachISWrite(pSC->m_ppBitIO[i], pSC->ppWStream[i]);
        }
    }

    return ICERR_OK;
}

#define PUTBITS putBit16
/*************************************************************************
    Write variable length byte aligned integer
*************************************************************************/
static Void PutVLWordEsc(BitIOInfo* pIO, Int iEscape, size_t s)
{
    if (iEscape) {
        assert(iEscape <= 0xff && iEscape > 0xfc); // fd,fe,ff are the only valid escapes
        PUTBITS(pIO, iEscape, 8);
    }
    else if (s < 0xfb00) {
        PUTBITS(pIO, (U32) s, 16);
    }
    else {
        size_t t = s >> 16;
        if ((t >> 16) == 0) {
            PUTBITS(pIO, 0xfb, 8);
        }
        else {
            t >>= 16;
            PUTBITS(pIO, 0xfc, 8);
            PUTBITS(pIO, (U32)(t >> 16) & 0xffff, 16);
            PUTBITS(pIO, (U32) t & 0xffff, 16);
        }
        PUTBITS(pIO, (U32) t & 0xffff, 16);
        PUTBITS(pIO, (U32) s & 0xffff, 16);
    }
}

/*************************************************************************
    Write index table at start (null index table)
*************************************************************************/
Int writeIndexTableNull(CWMImageStrCodec * pSC)
{
    if(pSC->cNumBitIO == 0){
        BitIOInfo* pIO = pSC->pIOHeader;
        //if (1){Int iSize = 99999;
        //PutVLWordEsc(pIO, 0, iSize); // size of post-header
        //while (iSize > 0) {
        //    iSize--;
        //    PUTBITS(pIO, 0, 8);
        //    writeIS_L1(pSC, pIO);
        //}}
        //else
        PutVLWordEsc(pIO, 0xff, 0); // escape to end
        fillToByte(pIO);
    }

    return ICERR_OK;
}

/*************************************************************************
    Write index table
*************************************************************************/
Int writeIndexTable(CWMImageStrCodec * pSC)
{
    if(pSC->cNumBitIO > 0){
        BitIOInfo* pIO = pSC->pIOHeader;
        size_t *pTable = pSC->pIndexTable, iSize = 0;
        U32 iEntry = (U32)pSC->cNumBitIO * (pSC->WMISCP.cNumOfSliceMinus1H + 1), i, k;
        
        // write index table header [0x0001] - 2 bytes
        PUTBITS(pIO, 1, 16);

        for(i = pSC->WMISCP.cNumOfSliceMinus1H; i> 0 && pSC->bTileExtraction == FALSE; i --){
            for(k = 0; k < pSC->cNumBitIO; k ++){
                pTable[pSC->cNumBitIO * i + k] -= pSC->pIndexTable[pSC->cNumBitIO * (i - 1) + k]; // packet length
            }
        }

        for(i = 0, iSize = 0; i < iEntry; i ++){
            writeIS_L1(pSC, pIO);
            PutVLWordEsc(pIO, (pTable[i] <= MINIMUM_PACKET_LENGTH) ? 0xff : 0, iSize);
            iSize += (pTable[i] <= MINIMUM_PACKET_LENGTH) ? 0 : pTable[i];
        }

        writeIS_L1(pSC, pIO);
        //if (1){Int iSize = 33333;
        //PutVLWordEsc(pIO, 0, iSize); // size of post-header
        //while (iSize > 0) {
        //    iSize--;
        //    PUTBITS(pIO, 0, 8);
        //    writeIS_L1(pSC, pIO);
        //}}
        //else
        PutVLWordEsc(pIO, 0xff, 0); // escape to end
        fillToByte(pIO);
    }

    return ICERR_OK;
}

Int copyTo(struct WMPStream * pSrc, struct WMPStream * pDst, size_t iBytes)
{
    char pData[PACKETLENGTH];

    if (iBytes <= MINIMUM_PACKET_LENGTH){
        pSrc->Read(pSrc, pData, iBytes);
        return ICERR_OK;
    }

    while(iBytes > PACKETLENGTH){
        pSrc->Read(pSrc, pData, PACKETLENGTH);
        pDst->Write(pDst, pData, PACKETLENGTH);
        iBytes -= PACKETLENGTH;
    }
    pSrc->Read(pSrc, pData, iBytes);
    pDst->Write(pDst, pData, iBytes);

    return ICERR_OK;
}

Int StrIOEncTerm(CWMImageStrCodec* pSC)
{
    BitIOInfo * pIO = pSC->pIOHeader;

    fillToByte(pIO);

    if(pSC->WMISCP.bVerbose){
        U32 i, j;

        printf("\n%d horizontal tiles:\n", pSC->WMISCP.cNumOfSliceMinus1H + 1);
        for(i = 0; i <= pSC->WMISCP.cNumOfSliceMinus1H; i ++){
            printf("    offset of tile %d in MBs: %d\n", i, pSC->WMISCP.uiTileY[i]);
        }

        printf("\n%d vertical tiles:\n", pSC->WMISCP.cNumOfSliceMinus1V + 1);
        for(i = 0; i <= pSC->WMISCP.cNumOfSliceMinus1V; i ++){
            printf("    offset of tile %d in MBs: %d\n", i, pSC->WMISCP.uiTileX[i]);
        }

        if(pSC->WMISCP.bfBitstreamFormat == SPATIAL){
            printf("\nSpatial order bitstream\n");
        }
        else{
            printf("\nFrequency order bitstream\n");
        }

        if(!pSC->m_param.bIndexTable){
            printf("\nstreaming mode, no index table.\n");
        }
        else if(pSC->WMISCP.bfBitstreamFormat == SPATIAL){
            for(j = 0; j <= pSC->WMISCP.cNumOfSliceMinus1H; j ++){
                for(i = 0; i <= pSC->WMISCP.cNumOfSliceMinus1V; i ++){
                    printf("bitstream size for tile (%d, %d): %d.\n", j, i, pSC->pIndexTable[j * (pSC->WMISCP.cNumOfSliceMinus1V + 1) + i]);
                }
            }
        }
        else{
            for(j = 0; j <= pSC->WMISCP.cNumOfSliceMinus1H; j ++){
                for(i = 0; i <= pSC->WMISCP.cNumOfSliceMinus1V; i ++){
                    size_t * p = &pSC->pIndexTable[(j * (pSC->WMISCP.cNumOfSliceMinus1V + 1) + i) * 4];
                    printf("bitstream size of (DC, LP, AC, FL) for tile (%d, %d): %d %d %d %d.\n", j, i,
                        p[0], p[1], p[2], p[3]);
                }
            }
        }
    }
    
    writeIndexTable(pSC); // write index table to the header

    detachISWrite(pSC, pIO);

    if(pSC->cNumBitIO > 0){
        size_t i, j, k;
        struct WMPStream * pDst = pSC->WMISCP.pWStream;
        size_t * pTable = pSC->pIndexTable;

        for(i = 0; i < pSC->cNumBitIO; i ++){
            detachISWrite(pSC, pSC->m_ppBitIO[i]);
        }

        for(i = 0; i < pSC->cNumBitIO; i ++){
            pSC->ppWStream[i]->SetPos(pSC->ppWStream[i], 0); // seek back for read
        }
        for(i = 0, k = 0; i <= pSC->WMISCP.cNumOfSliceMinus1H; i ++){ // loop through tiles
            for(j = 0; j <= pSC->WMISCP.cNumOfSliceMinus1V; j ++){

                if(pSC->WMISCP.bfBitstreamFormat == SPATIAL)
                    copyTo(pSC->ppWStream[j], pDst, pTable[k ++]);
                else{
                    copyTo(pSC->ppWStream[j * pSC->cSB + 0], pDst, pTable[k ++]);
                    if(pSC->cSB > 1)
                        copyTo(pSC->ppWStream[j * pSC->cSB + 1], pDst, pTable[k ++]);
                    if(pSC->cSB > 2)
                        copyTo(pSC->ppWStream[j * pSC->cSB + 2], pDst, pTable[k ++]);
                    if(pSC->cSB > 3)
                        copyTo(pSC->ppWStream[j * pSC->cSB + 3], pDst, pTable[k ++]);
                }
            }
        }
        for(i = 0; i < pSC->cNumBitIO; i ++){
            pSC->ppWStream[i]->Close(pSC->ppWStream + i);
        }

        free(pSC->ppWStream);

        free(pSC->m_ppBitIO);
        free(pSC->pIndexTable);
    }

    return 0;
}

/*************************************************************************
    Write header of image plane
*************************************************************************/
Int WriteImagePlaneHeader(CWMImageStrCodec * pSC)
{
    CWMImageInfo * pII = &pSC->WMII;
    CWMIStrCodecParam * pSCP = &pSC->WMISCP;
    BitIOInfo* pIO = pSC->pIOHeader;

    PUTBITS(pIO, (Int) pSC->m_param.cfColorFormat, 3); // internal color format
    PUTBITS(pIO, (Int) pSC->m_param.bScaledArith, 1); // lossless mode

// subbands
    PUTBITS(pIO, (U32)pSCP->sbSubband, 4);

// color parameters
    switch (pSC->m_param.cfColorFormat) {
        case YUV_420:
        case YUV_422:
        case YUV_444:
            PUTBITS(pIO, pII->cChromaCentering, 4);
            PUTBITS(pIO, pII->cChromaInterpretation, 4);
            break;
        case N_CHANNEL:
            PUTBITS(pIO, (Int) pSC->m_param.cNumChannels - 1, 4);
            PUTBITS(pIO, pII->cChromaInterpretation, 4);
    }

    switch (pII->cfColorFormat) {
        case CF_PALLETIZED:
            break;
        case BAYER:
            PUTBITS(pIO, pII->bpBayerPattern, 2);
            PUTBITS(pIO, pII->cChromaCentering, 2);
            PUTBITS(pIO, pII->cChromaInterpretation, 4);
            break;
    }

// float and 32s additional parameters
    switch (pII->bdBitDepth) {
        case BD_16:
        case BD_16S:
            PUTBITS(pIO, pSCP->nLenMantissaOrShift, 8);
            break;
        case BD_32:
        case BD_32S:
            if(pSCP->nLenMantissaOrShift == 0)
                pSCP->nLenMantissaOrShift = 10;//default
            PUTBITS(pIO, pSCP->nLenMantissaOrShift, 8);
            break;
        case BD_32F:
            if(pSCP->nLenMantissaOrShift == 0)
                pSCP->nLenMantissaOrShift = 13;//default
            //if (pSCP->nExpBias == 0)
            //    pSCP->nExpBias = 4 + 128;//default
            //pSCP->nExpBias += 128; // rollover arithmetic
            PUTBITS(pIO, pSCP->nLenMantissaOrShift, 8);//float conversion parameters
            PUTBITS(pIO, pSCP->nExpBias, 8);
            break;
    }

        // quantization
    PUTBITS(pIO, (pSC->m_param.uQPMode & 1) == 1 ? 0 : 1, 1); // DC frame uniform quantization?
    if((pSC->m_param.uQPMode & 1) == 0)
        writeQuantizer(pSC->pTile[0].pQuantizerDC, pIO, (pSC->m_param.uQPMode >> 3) & 3, pSC->m_param.cNumChannels, 0);
    if(pSC->WMISCP.sbSubband != SB_DC_ONLY){
        PUTBITS(pIO, (pSC->m_param.uQPMode & 0x200) == 0 ? 1 : 0, 1); // use DC quantization?
        if((pSC->m_param.uQPMode & 0x200) != 0){
            PUTBITS(pIO, (pSC->m_param.uQPMode & 2) == 2 ? 0 : 1, 1); // LP frame uniform quantization?
            if((pSC->m_param.uQPMode & 2) == 0)
                writeQuantizer(pSC->pTile[0].pQuantizerLP, pIO, (pSC->m_param.uQPMode >> 5) & 3,  pSC->m_param.cNumChannels, 0);
        }

        if(pSC->WMISCP.sbSubband != SB_NO_HIGHPASS){
            PUTBITS(pIO, (pSC->m_param.uQPMode & 0x400) == 0 ? 1 : 0, 1); // use LP quantization?
            if((pSC->m_param.uQPMode & 0x400) != 0){
                PUTBITS(pIO, (pSC->m_param.uQPMode & 4) == 4 ? 0 : 1, 1); // HP frame uniform quantization?
                if((pSC->m_param.uQPMode & 4) == 0)
                    writeQuantizer(pSC->pTile[0].pQuantizerHP, pIO, (pSC->m_param.uQPMode >> 7) & 3,  pSC->m_param.cNumChannels, 0);
            }
        }
    }

    fillToByte(pIO);  // remove this later
    return ICERR_OK;
}

/*************************************************************************
    Write header to buffer
*************************************************************************/
Int WriteWMIHeader(CWMImageStrCodec * pSC)
{
    CWMImageInfo * pII = &pSC->WMII;
    CWMIStrCodecParam * pSCP = &pSC->WMISCP;
    CCoreParameters * pCoreParam = &pSC->m_param;
    BitIOInfo* pIO = pSC->pIOHeader;
    U32 iSizeOfSize = 2, i;
    // temporary assignments / reserved words
    const Int HEADERSIZE = 0;
    const Int BAYER_PATTERN = 0;
    Bool bInscribed = FALSE;
    Bool bAbbreviatedHeader = TRUE; // short words for size and tiles

    if(pCoreParam->bTranscode == FALSE)
        pCoreParam->cExtraPixelsTop = pCoreParam->cExtraPixelsLeft = pCoreParam->cExtraPixelsRight = pCoreParam->cExtraPixelsBottom = 0;

    // num of extra boundary pixels due to compressed domain processing
    bInscribed = (pCoreParam->cExtraPixelsTop || pCoreParam->cExtraPixelsLeft || pCoreParam->cExtraPixelsBottom || pCoreParam->cExtraPixelsRight);

// 0
    /** signature **/
    for (i = 0; i < 8; PUTBITS(pSC->pIOHeader, gGDISignature[i++], 8));

// 8
    /** codec version and subversion **/
    PUTBITS(pIO, CODEC_VERSION, 4);  // this should be changed to "profile" in RTM
    PUTBITS(pIO, CODEC_SUBVERSION, 4);

// 9 primary parameters
    PUTBITS(pIO, (pSCP->cNumOfSliceMinus1V || pSCP->cNumOfSliceMinus1H) ? 1 : 0, 1); // tiling present
    PUTBITS(pIO, (Int) pSCP->bfBitstreamFormat, 1); // bitstream layout
    PUTBITS(pIO, pII->oOrientation, 3);        // m_iRotateFlip
    PUTBITS(pIO, pSC->m_param.bIndexTable, 1); // index table present
    PUTBITS(pIO, pSCP->olOverlap, 2); // overlap

// 10
    PUTBITS(pIO, bAbbreviatedHeader, 1); // short words for size and tiles
    PUTBITS(pIO, 1, 1); // long word length (use intelligence later)
    PUTBITS(pIO, bInscribed, 1); // windowing
    PUTBITS(pIO, pSC->m_param.bTrimFlexbitsFlag, 1); // trim flexbits flag sent
    PUTBITS(pIO, 0, 1); // tile stretching parameters (not enabled)
    PUTBITS(pIO, 0, 2); // reserved bits
    PUTBITS(pIO, (Int) pSC->m_param.bAlphaChannel, 1); // alpha channel present

// 11 - informational
    PUTBITS(pIO, (Int) pII->cfColorFormat, 4); // source color format
    if(BD_1 == pII->bdBitDepth && pSCP->bBlackWhite)
        PUTBITS(pIO, (Int) BD_1alt, 4); // source bit depth
    else 
        PUTBITS(pIO, (Int) pII->bdBitDepth, 4); // source bit depth

// 12 - Variable length fields
// size
    //PutVLWord(pIO, (((pII->cWidth + 15) >> 4) - 1) >> 4);
    //PutVLWord(pIO, (((pII->cHeight + 15) >> 4) - 1) >> 4);
    putBit32(pIO, (U32)(pII->cWidth - 1), bAbbreviatedHeader ? 16 : 32);
    putBit32(pIO, (U32)(pII->cHeight - 1), bAbbreviatedHeader ? 16 : 32);

// tiling
    if (pSCP->cNumOfSliceMinus1V || pSCP->cNumOfSliceMinus1H) {
        PUTBITS(pIO, pSCP->cNumOfSliceMinus1V, LOG_MAX_TILES); // # of vertical slices
        PUTBITS(pIO, pSCP->cNumOfSliceMinus1H, LOG_MAX_TILES); // # of horizontal slices
    }

// tile sizes
    for(i = 0; i < pSCP->cNumOfSliceMinus1V; i ++){ // width in MB of vertical slices, not needed for last slice!
        //PutVLWord(pIO, pSCP->uiTileX[i + 1] - pSCP->uiTileX[i]);
        PUTBITS(pIO, pSCP->uiTileX[i + 1] - pSCP->uiTileX[i], bAbbreviatedHeader ? 8 : 16);
    }
    for(i = 0; i < pSCP->cNumOfSliceMinus1H; i ++){ // width in MB of horizontal slices, not needed for last slice!
        //PutVLWord(pIO, pSCP->uiTileY[i + 1] - pSCP->uiTileY[i]);
        PUTBITS(pIO, pSCP->uiTileY[i + 1] - pSCP->uiTileY[i], bAbbreviatedHeader ? 8 : 16);
    }

// window due to compressed domain processing
    if (bInscribed) {
        PUTBITS(pIO, (U32)pCoreParam->cExtraPixelsTop, 6);
        PUTBITS(pIO, (U32)pCoreParam->cExtraPixelsLeft, 6);
        PUTBITS(pIO, (U32)pCoreParam->cExtraPixelsBottom, 6);
        PUTBITS(pIO, (U32)pCoreParam->cExtraPixelsRight, 6);
    }    
    fillToByte(pIO);  // redundant

    // write image plane headers
    WriteImagePlaneHeader(pSC);

    // maybe UNALIGNED!!!
    //PUTBITS(pIO, 0xfefe, 16);//sanity check

    return ICERR_OK;
}

//----------------------------------------------------------------
// streaming codec init/term
Int StrEncInit(CWMImageStrCodec* pSC)
{
    COLORFORMAT cf = pSC->m_param.cfColorFormat;
    COLORFORMAT cfE = pSC->WMII.cfColorFormat;
    U16 iQPIndex, iQPIndexUV;
    size_t i;

#ifdef STRESS_PARAM
    srand((unsigned)time(NULL));
#endif // STRESS_PARAM

    /** color transcoding with resolution change **/
    pSC->m_bUVResolutionChange = (((cfE == CF_RGB || cfE == YUV_444 || cfE == CMYK || cfE == BAYER || cfE == CF_RGBE) && (cf == YUV_422 || cf == YUV_420))
        || (cfE == YUV_422 && cf == YUV_420));

    if(pSC->m_bUVResolutionChange){
        size_t cSize = ((cfE == YUV_422 ? 128 : 256) + (cf == YUV_420 ? 32 : 0)) * pSC->cmbWidth + 256;

        if(sizeof(size_t) == 4){ // integer overlow/underflow check for 32-bit system
            if(((pSC->cmbWidth >> 16) * ((cfE == YUV_422 ? 128 : 256) + (cf == YUV_420 ? 32 : 0))) & 0xffff0000)
                return ICERR_ERROR;
            if(cSize >= 0x3fffffff)
                return ICERR_ERROR;
        }
        pSC->pResU = (PixelI *)malloc(cSize * sizeof(PixelI));
        pSC->pResV = (PixelI *)malloc(cSize * sizeof(PixelI));
        if(pSC->pResU == NULL || pSC->pResV == NULL){
            return ICERR_ERROR;
        }
    }

    pSC->cTileColumn = pSC->cTileRow = 0;

    if(allocateTileInfo(pSC) != ICERR_OK)
        return ICERR_ERROR;

    if(pSC->m_param.bTranscode == FALSE){
#ifdef STRESS_PARAM
        pSC->m_param.uQPMode = (rand() & 0x1ff) + 0x600; // just for concept proofing!!
        pSC->m_param.bScaledArith = TRUE;
#else
        pSC->m_param.uQPMode = 0x6a8;

        /** lossless or Y component lossless condition: all subbands present, uniform quantization with QPIndex 1 **/
        pSC->m_param.bScaledArith = !((pSC->m_param.uQPMode & 7) == 0 && 1 == pSC->WMISCP.uiDefaultQPIndex <= 1 && pSC->WMISCP.sbSubband == SB_ALL && pSC->m_bUVResolutionChange == FALSE);
        if (BD_32 == pSC->WMII.bdBitDepth || BD_32S == pSC->WMII.bdBitDepth || BD_32F == pSC->WMII.bdBitDepth) {
            pSC->m_param.bScaledArith = FALSE;
        }
#endif
        pSC->m_param.uQPMode |= 0x600;

        // default QPs
        iQPIndex = pSC->WMISCP.uiDefaultQPIndex;
        if(iQPIndex < 2)
            iQPIndex = 0;
        if (iQPIndex < 16)
            iQPIndexUV = (cf == YUV_420 ? (iQPIndex + ((iQPIndex + 2) >> 2)) : cf == YUV_422 ? (iQPIndex + ((iQPIndex + 1) >> 1)) : iQPIndex * 2);
        else{
            iQPIndexUV = iQPIndex + (cf == YUV_420 ? 4 : (cf == YUV_422 ? 8 : 18));
            if (iQPIndex > 48)
                iQPIndexUV += 2;
        }
    }

    if((pSC->m_param.uQPMode & 1) == 0){ // DC frame uniform quantization
        if(allocateQuantizer(pSC->pTile[0].pQuantizerDC, pSC->m_param.cNumChannels, 1) != ICERR_OK)
            return ICERR_ERROR;
        setUniformQuantizer(pSC, 0);
        for(i = 0; i < pSC->m_param.cNumChannels; i ++)
            if(pSC->m_param.bTranscode)
                pSC->pTile[0].pQuantizerDC[i]->iIndex = pSC->m_param.uiQPIndexDC[i];
            else
                pSC->pTile[0].pQuantizerDC[i]->iIndex = pSC->m_param.uiQPIndexDC[i] = (U8)(((i == 0 ? iQPIndex : iQPIndexUV)) & 0xff);
        formatQuantizer(pSC->pTile[0].pQuantizerDC, (pSC->m_param.uQPMode >> 3) & 3, pSC->m_param.cNumChannels, 0, TRUE, pSC->m_param.bScaledArith);

        for(i = 0; i < pSC->m_param.cNumChannels; i ++)
            pSC->pTile[0].pQuantizerDC[i]->iOffset = (pSC->pTile[0].pQuantizerDC[i]->iQP >> 1);
    }

    if(pSC->WMISCP.sbSubband != SB_DC_ONLY){
        if((pSC->m_param.uQPMode & 2) == 0){ // LP frame uniform quantization
            if(allocateQuantizer(pSC->pTile[0].pQuantizerLP, pSC->m_param.cNumChannels, 1) != ICERR_OK)
                return ICERR_ERROR;
            setUniformQuantizer(pSC, 1);
            for(i = 0; i < pSC->m_param.cNumChannels; i ++)
                if(pSC->m_param.bTranscode)
                    pSC->pTile[0].pQuantizerLP[i]->iIndex = pSC->m_param.uiQPIndexLP[i];
                else
                    pSC->pTile[0].pQuantizerLP[i]->iIndex = pSC->m_param.uiQPIndexLP[i] = (U8)(((i == 0 ? iQPIndex : iQPIndexUV)) & 0xff);
            formatQuantizer(pSC->pTile[0].pQuantizerLP, (pSC->m_param.uQPMode >> 5) & 3, pSC->m_param.cNumChannels, 0, TRUE, pSC->m_param.bScaledArith);
        }

        if(pSC->WMISCP.sbSubband != SB_NO_HIGHPASS){
            if((pSC->m_param.uQPMode & 4) == 0){ // HP frame uniform quantization
                if(allocateQuantizer(pSC->pTile[0].pQuantizerHP, pSC->m_param.cNumChannels, 1) != ICERR_OK)
                    return ICERR_ERROR;
                setUniformQuantizer(pSC, 2);
                for(i = 0; i < pSC->m_param.cNumChannels; i ++)
                    if(pSC->m_param.bTranscode)
                        pSC->pTile[0].pQuantizerHP[i]->iIndex = pSC->m_param.uiQPIndexHP[i];
                    else
                        pSC->pTile[0].pQuantizerHP[i]->iIndex = pSC->m_param.uiQPIndexHP[i] = (U8)((i == 0 ? iQPIndex : iQPIndexUV) & 0xff);
                formatQuantizer(pSC->pTile[0].pQuantizerHP, (pSC->m_param.uQPMode >> 7) & 3, pSC->m_param.cNumChannels, 0, FALSE, pSC->m_param.bScaledArith);
            }
        }
    }

    if(allocatePredInfo(pSC) != ICERR_OK){
        return ICERR_ERROR;
    }

    if(pSC->WMISCP.cNumOfSliceMinus1V >= MAX_TILES || AllocateCodingContextEnc (pSC, pSC->WMISCP.cNumOfSliceMinus1V + 1, pSC->WMISCP.uiTrimFlexBits) != ICERR_OK){
        return ICERR_ERROR;
    }
    
    if (pSC->m_bSecondary) {
        pSC->pIOHeader = pSC->m_pNextSC->pIOHeader;
        pSC->m_ppBitIO = pSC->m_pNextSC->m_ppBitIO;
        pSC->cNumBitIO = pSC->m_pNextSC->cNumBitIO;
        pSC->cSB = pSC->m_pNextSC->cSB;
        pSC->ppWStream = pSC->m_pNextSC->ppWStream;
        pSC->pIndexTable = pSC->m_pNextSC->pIndexTable;
        setBitIOPointers(pSC);
    }
    else {
        StrIOEncInit(pSC);
        setBitIOPointers(pSC);
        WriteWMIHeader(pSC);
    }

    return ICERR_OK;
}

static Int StrEncTerm(CTXSTRCODEC ctxSC)
{
    CWMImageStrCodec* pSC = (CWMImageStrCodec*)ctxSC;
    size_t j, jend = (pSC->m_pNextSC != NULL);

    for (j = 0; j <= jend; j++) {
        if (sizeof(*pSC) != pSC->cbStruct) {
            return ICERR_ERROR;
        }

        if(pSC->m_bUVResolutionChange){
            if(pSC->pResU != NULL)
                free(pSC->pResU);
            if(pSC->pResV != NULL)
                free(pSC->pResV);
        }

        freePredInfo(pSC);

        if (j == 0)
            StrIOEncTerm(pSC);

        FreeCodingContextEnc(pSC);
        
        freeTileInfo(pSC);

        pSC->WMISCP.nExpBias -= 128; // reset

#if (defined (DEBUG_QUANT) || defined(DIFF_QUANT))
        if (fQuant_g != NULL) {
            fclose(fQuant_g);
            fQuant_g = NULL;
        }
#endif
        pSC = pSC->m_pNextSC;
    }

    return 0;
}

U32 setUniformTiling(U32 * pTile, U32 cNumTile, U32 cNumMB)
{
    U32 i, j;

    while((cNumMB + cNumTile - 1) / cNumTile > 65535) // too few tiles
        cNumTile ++;

    for(i = cNumTile, j = cNumMB; i > 1; i --){
        pTile[cNumTile - i] = (j + i - 1) / i;
        j -= pTile[cNumTile - i];
    }

    return cNumTile;
}

U32 validateTiling(U32 * pTile, U32 cNumTile, U32 cNumMB)
{
    U32 i, cMBs;

    if(cNumTile == 0)
        cNumTile = 1;
    if(cNumTile > cNumMB) // too many tiles
        cNumTile = 1;
    if(cNumTile > MAX_TILES)
        cNumTile = MAX_TILES;

    for(i = cMBs = 0; i + 1 < cNumTile; i ++){
        if(pTile[i] == 0 || pTile[i] > 65535){ // invalid tile setting, resetting to uniform tiling
            cNumTile = setUniformTiling(pTile, cNumTile, cNumMB);
            break;
        }
        
        cMBs += pTile[i];

        if(cMBs >= cNumMB){
            cNumTile = i + 1;
            break;
        }
    }

    // last tile
    if(cNumMB - cMBs > 65536)
        cNumTile = setUniformTiling(pTile, cNumTile, cNumMB);

    for(i = 1; i < cNumTile; i ++)
        pTile[i] += pTile[i - 1];
    for(i = cNumTile - 1; i > 0; i --)
        pTile[i] = pTile[i - 1];
    pTile[0] = 0;

    return cNumTile;
}

/*************************************************************************
  Validate and adjust input params here
*************************************************************************/
Int ValidateArgs(CWMImageInfo* pII, CWMIStrCodecParam *pSCP)
{
    if(pII->cWidth > (1 << 28) || pII->cHeight > (1 << 28) || pII->cWidth == 0 || pII->cHeight == 0){
        printf("Unsurpported image size!\n");
        return ICERR_ERROR; // unsurpported image size
    }

    if(pSCP->sbSubband == SB_ISOLATED || pSCP->sbSubband >= SB_MAX) // not allowed
        pSCP->sbSubband = SB_ALL;

    if(pII->bdBitDepth == BD_5 && (pII->cfColorFormat != CF_RGB || pII->cBitsPerUnit != 16 || pII->cLeadingPadding != 0)){
        printf("Unsupported BD_5 image format!\n");
        return ICERR_ERROR; // BD_5 must be compact RGB!
    }   
    if(pII->bdBitDepth == BD_565 && (pII->cfColorFormat != CF_RGB || pII->cBitsPerUnit != 16 || pII->cLeadingPadding != 0)){
        printf("Unsupported BD_565 image format!\n");
        return ICERR_ERROR; // BD_5 must be compact RGB!
    }   
    if(pII->bdBitDepth == BD_10 && (pII->cfColorFormat != CF_RGB || pII->cBitsPerUnit != 32 || pII->cLeadingPadding != 0)){
        printf("Unsupported BD_10 image format!\n");
        return ICERR_ERROR; // BD_10 must be compact RGB!
    }

    if(pII->bdBitDepth == BD_5 || pII->bdBitDepth == BD_565 || pII->bdBitDepth == BD_10)
        if(pSCP->cfColorFormat != YUV_420 && pSCP->cfColorFormat != YUV_422)
            pSCP->cfColorFormat = YUV_444;

    if(BD_1 == pII->bdBitDepth){ // binary image
        if(pII->cfColorFormat != Y_ONLY){
            printf("BD_1 image must be black-and white!\n");
            return ICERR_ERROR;
        }
        pSCP->cfColorFormat = Y_ONLY; // can only be black white
    }

    if(pSCP->bdBitDepth != BD_LONG)
        pSCP->bdBitDepth = BD_LONG; // currently only support 32 bit internally

    if(pSCP->uAlphaMode > 1 && (pII->cfColorFormat == YUV_420 || pII->cfColorFormat == YUV_422 || pII->cfColorFormat == BAYER
        || pII->bdBitDepth == BD_5 || pII->bdBitDepth == BD_10 || pII->bdBitDepth == BD_1))
    {
        printf("Alpha is not supported for this pixel format!\n");
        return ICERR_ERROR;
    }

    // adjust tiling
    pSCP->cNumOfSliceMinus1V = validateTiling(pSCP->uiTileX, pSCP->cNumOfSliceMinus1V + 1, (((U32)pII->cWidth + 15) >> 4)) - 1;
    pSCP->cNumOfSliceMinus1H = validateTiling(pSCP->uiTileY, pSCP->cNumOfSliceMinus1H + 1, (((U32)pII->cHeight + 15) >> 4)) - 1;

    if(pSCP->cChannel > MAX_CHANNELS)
        return ICERR_ERROR;

    /** supported color transcoding **/
    /** ARGB, RGB => YUV_444, YUV_422, YUV_420, Y_ONLY **/
    /** YUV_444   =>          YUV_422, YUV_420, Y_ONLY **/
    /** YUV_422   =>                   YUV_420, Y_ONLY **/
    /** YUV_420   =>                            Y_ONLY **/

    /** unsupported color transcoding       **/
    /** Y_ONLY, YUV_420, YUV_422 => YUV_444 **/
    /** Y_ONLY, YUV_420          => YUV_422 **/
    /** Y_ONLY                   => YUV_420 **/
    if((pII->cfColorFormat == Y_ONLY &&  pSCP->cfColorFormat != Y_ONLY) || 
        (pSCP->cfColorFormat == YUV_422 && (pII->cfColorFormat == YUV_420 || pII->cfColorFormat == Y_ONLY)) || 
        (pSCP->cfColorFormat == YUV_444 && (pII->cfColorFormat == YUV_422 || pII->cfColorFormat == YUV_420 || pII->cfColorFormat == Y_ONLY))){
            pSCP->cfColorFormat = pII->cfColorFormat; // force not to do color transcoding!
    }
    else if (pII->cfColorFormat == N_CHANNEL) {
            pSCP->cfColorFormat = N_CHANNEL; // force not to do color transcoding!
    }
    if (CMYK == pII->cfColorFormat && (pSCP->cfColorFormat == BAYER || pSCP->cfColorFormat == N_CHANNEL)) 
    {
        pSCP->cfColorFormat = CMYK;
    }

    if(pSCP->cfColorFormat != N_CHANNEL){
        if(pSCP->cfColorFormat == Y_ONLY)
            pSCP->cChannel = 1;
        else if(pSCP->cfColorFormat == CMYK || pSCP->cfColorFormat == BAYER)
            pSCP->cChannel = 4;
        else
            pSCP->cChannel = 3;

//        if(pSCP->uAlphaMode > 1)
//            pSCP->cChannel ++;
    }

    if(pSCP->sbSubband >= SB_MAX)
        pSCP->sbSubband = SB_ALL;


    pII->cChromaCentering = 0;
    pII->cChromaInterpretation = 0;
    switch (pII->cfColorFormat) {
        case CF_PALLETIZED:
            pII->cChromaInterpretation = 2;
            break;
        case CF_RGB:
        case CF_RGBE:
        case CMYK:
        case BAYER:
            pII->cChromaInterpretation = 1;
            break;
    }

    return ICERR_OK;
}

/*************************************************************************
  Initialization of CWMImageStrCodec struct
*************************************************************************/
static Void InitializeStrEnc(CWMImageStrCodec *pSC,
    const CWMImageInfo* pII, const CWMIStrCodecParam *pSCP)
{
    pSC->cbStruct = sizeof(*pSC);
    pSC->WMII = *pII;
    pSC->WMISCP = *pSCP;

    // set nExpBias
    if (pSC->WMISCP.nExpBias == 0)
        pSC->WMISCP.nExpBias = 4 + 128;//default
    pSC->WMISCP.nExpBias += 128; // rollover arithmetic

    pSC->cRow = 0;
    pSC->cColumn = 0;
    
    pSC->cmbWidth = (pSC->WMII.cWidth + 15) / 16;
    pSC->cmbHeight = (pSC->WMII.cHeight + 15) / 16;

    pSC->Load = inputMBRow;
    pSC->Quantize = quantizeMacroblock;
    pSC->ProcessTopLeft = processMacroblock;
    pSC->ProcessTop = processMacroblock;
    pSC->ProcessTopRight = processMacroblock;
    pSC->ProcessLeft = processMacroblock;
    pSC->ProcessCenter = processMacroblock;
    pSC->ProcessRight = processMacroblock;
    pSC->ProcessBottomLeft = processMacroblock;
    pSC->ProcessBottom = processMacroblock;
    pSC->ProcessBottomRight = processMacroblock;

    pSC->m_pNextSC = NULL;
    pSC->m_bSecondary = FALSE;
}

/*************************************************************************
   Streaming API init
*************************************************************************/
Int ImageStrEncInit(
    CWMImageInfo* pII,
    CWMIStrCodecParam *pSCP,
    CTXSTRCODEC* pctxSC)
{
    static size_t cbChannels[BD_MAX] = {2, 4};

    size_t cbChannel = 0, cblkChroma = 0, i;
    size_t cbMacBlockStride = 0, cbMacBlockChroma = 0, cMacBlock = 0;

    CWMImageStrCodec* pSC = NULL, *pNextSC = NULL;
    char* pb = NULL;
    size_t cb = 0;

    if(ValidateArgs(pII, pSCP) != ICERR_OK){
        goto ErrorExit;
    }

    //================================================
    *pctxSC = NULL;

    //================================================
    cbChannel = cbChannels[pSCP->bdBitDepth];
    cblkChroma = cblkChromas[pSCP->cfColorFormat];
    cbMacBlockStride = cbChannel * 16 * 16;
    cbMacBlockChroma = cbChannel * 16 * cblkChroma;
    cMacBlock = (pII->cWidth + 15) / 16;

    //================================================
    cb = sizeof(*pSC) + (128 - 1) + (PACKETLENGTH * 4 - 1) + (PACKETLENGTH * 2 ) + sizeof(*pSC->pIOHeader);
    i = cbMacBlockStride + cbMacBlockChroma * (pSCP->cChannel - 1);
    if(sizeof(size_t) == 4) // integer overlow/underflow check for 32-bit system
        if(((cMacBlock >> 15) * i) & 0xffff0000)
            return ICERR_ERROR;
    i *= cMacBlock * 2;
    cb += i;

    pb = malloc(cb);
    if (NULL == pb)
    {
        goto ErrorExit;
    }
    memset(pb, 0, cb);

    //================================================
    pSC = (CWMImageStrCodec*)pb; pb += sizeof(*pSC);

    pSC->m_param.cfColorFormat = pSCP->cfColorFormat;
    pSC->m_param.bAlphaChannel = (pSCP->uAlphaMode == 3);
    pSC->m_param.cNumChannels = pSCP->cChannel;
    pSC->m_param.cExtraPixelsTop = pSC->m_param.cExtraPixelsBottom
        = pSC->m_param.cExtraPixelsLeft = pSC->m_param.cExtraPixelsRight = 0;

    pSC->cbChannel = cbChannel;

    pSC->m_param.bTranscode = pSC->bTileExtraction = FALSE;

    //================================================
    InitializeStrEnc(pSC, pII, pSCP);

    //================================================
    // 2 Macro Row buffers for each channel
    pb = ALIGNUP(pb, 128);
    for (i = 0; i < pSC->m_param.cNumChannels; i++) {
        pSC->a0MBbuffer[i] = (PixelI*)pb; pb += cbMacBlockStride * pSC->cmbWidth;
        pSC->a1MBbuffer[i] = (PixelI*)pb; pb += cbMacBlockStride * pSC->cmbWidth;
        cbMacBlockStride = cbMacBlockChroma;
    }

    //================================================
    // lay 2 aligned IO buffers just below pIO struct
    pb = (U8*)ALIGNUP(pb, PACKETLENGTH * 4) + PACKETLENGTH * 2;
    pSC->pIOHeader = (BitIOInfo*)pb;

    //================================================
    StrEncInit(pSC);

    // if interleaved alpha is needed
    if (pSC->m_param.bAlphaChannel) {
        cbMacBlockStride = cbChannel * 16 * 16;
        // 1. allocate new pNextSC info
        //================================================
        cb = sizeof(*pNextSC) + (128 - 1) + cbMacBlockStride * cMacBlock * 2;
        pb = malloc(cb);
        if (NULL == pb)
        {
            goto ErrorExit;
        }
        memset(pb, 0, cb);
        //================================================
        pNextSC = (CWMImageStrCodec*)pb; pb += sizeof(*pNextSC);

        // 2. initialize pNextSC
        pNextSC->m_param.cfColorFormat = Y_ONLY;
        pNextSC->m_param.cNumChannels = 1;
        pNextSC->m_param.bAlphaChannel = TRUE;
        pNextSC->cbChannel = cbChannel;
        //================================================

        // 3. initialize arrays
        InitializeStrEnc(pNextSC, pII, pSCP);
        //================================================

        // 2 Macro Row buffers for each channel
        pb = ALIGNUP(pb, 128);
        pNextSC->a0MBbuffer[0] = (PixelI*)pb; pb += cbMacBlockStride * pNextSC->cmbWidth;
        pNextSC->a1MBbuffer[0] = (PixelI*)pb; pb += cbMacBlockStride * pNextSC->cmbWidth;
        //================================================
        pNextSC->pIOHeader = pSC->pIOHeader;
        //================================================

        // 4. link pSC->pNextSC = pNextSC
        pNextSC->m_pNextSC = pSC;
        pNextSC->m_bSecondary = TRUE;

        // 5. StrEncInit
        StrEncInit(pNextSC);

        // 6. Write header of image plane
        WriteImagePlaneHeader(pNextSC);
    }

    pSC->m_pNextSC = pNextSC;
    //================================================
    *pctxSC = (CTXSTRCODEC)pSC;

    writeIndexTableNull(pSC);
#if defined(WMP_OPT_SSE2) || defined(WMP_OPT_CC_ENC) || defined(WMP_OPT_TRFM_ENC)
    StrEncOpt(pSC);
#endif // OPT defined
    return ICERR_OK;

ErrorExit:
    return ICERR_ERROR;
}

/*************************************************************************
   Streaming API encode
*************************************************************************/
Int ImageStrEncEncode(
    CTXSTRCODEC ctxSC,
    const CWMImageBufferInfo* pBI)
{
    CWMImageStrCodec* pSC = (CWMImageStrCodec*)ctxSC;
    CWMImageStrCodec* pNextSC = pSC->m_pNextSC;
    ImageDataProc ProcessLeft, ProcessCenter, ProcessRight;

#ifdef STRESS_DATA
    size_t i;
    U8 * pC = (U8 *)pBI->pv;

    srand((unsigned)(time(NULL)));

    for(i = 0; i < pBI->cbStride * pBI->cLine; i ++){
        pC[i] = (U8)(rand() % 255);
    }

#endif

    if (sizeof(*pSC) != pSC->cbStruct)
    {
        return ICERR_ERROR;
    }

    //================================

    pSC->WMIBI = *pBI;
    pSC->cColumn = 0;
    initMRPtr(pSC);
    if (pNextSC)
        pNextSC->WMIBI = *pBI;

    if (0 == pSC->cRow) {
        ProcessLeft = pSC->ProcessTopLeft;
        ProcessCenter = pSC->ProcessTop;
        ProcessRight = pSC->ProcessTopRight;
    }
    else {
        ProcessLeft = pSC->ProcessLeft;
        ProcessCenter = pSC->ProcessCenter;
        ProcessRight = pSC->ProcessRight;
    }

    pSC->Load(pSC);
    if(ProcessLeft(pSC) != ICERR_OK)
        return ICERR_ERROR;
    advanceMRPtr(pSC);

    //================================
    for (pSC->cColumn = 1; pSC->cColumn < pSC->cmbWidth; ++pSC->cColumn) {
        if(ProcessCenter(pSC) != ICERR_OK)
            return ICERR_ERROR;
        advanceMRPtr(pSC);
    }

    //================================
    if(ProcessRight(pSC) != ICERR_OK)
        return ICERR_ERROR;
    if (pSC->cRow)
        advanceOneMBRow(pSC);

    ++pSC->cRow;
    swapMRPtr(pSC);

    return ICERR_OK;
}

/*************************************************************************
   Streaming API term
*************************************************************************/
Int ImageStrEncTerm(
    CTXSTRCODEC ctxSC)
{
    CWMImageStrCodec* pSC = (CWMImageStrCodec*)ctxSC;
    CWMImageStrCodec *pNextSC = pSC->m_pNextSC;

    if (sizeof(*pSC) != pSC->cbStruct)
    {
        return ICERR_ERROR;
    }

    //================================
    pSC->cColumn = 0;
    initMRPtr(pSC);

    pSC->ProcessBottomLeft(pSC);
    advanceMRPtr(pSC);

    //================================
    for (pSC->cColumn = 1; pSC->cColumn < pSC->cmbWidth; ++pSC->cColumn) {
        pSC->ProcessBottom(pSC);
        advanceMRPtr(pSC);
    }

    //================================
    pSC->ProcessBottomRight(pSC);

    //================================
    StrEncTerm(pSC);

#ifdef VERIFY_16BIT
    VERIFY_REPORT();
#endif // VERIFY_16BIT

    free(pSC);
    return ICERR_OK;
}

// centralized UV downsampling
#define DF_ODD ((((d1 + d2 + d3) << 2) + (d2 << 1) + d0 + d4 + 8) >> 4)
Void downsampleUV(CWMImageStrCodec * pSC)
{
    const COLORFORMAT cfInt = pSC->m_param.cfColorFormat;
    const COLORFORMAT cfExt = pSC->WMII.cfColorFormat;
    PixelI * pSrc, * pDst;
    PixelI d0, d1, d2, d3, d4;
    size_t iChannel, iRow, iColumn;

    for(iChannel = 1; iChannel < 3; iChannel ++){
        if(cfExt != YUV_422){ // need to do horizontal downsampling, 444 => 422
            const size_t cShift = (cfInt == YUV_422 ? 1 : 0);

            pSrc = (iChannel == 1 ? pSC->pResU : pSC->pResV);
            pDst = (cfInt == YUV_422 ? pSC->p1MBbuffer[iChannel] : pSrc);
            
            for(iRow = 0; iRow < 16; iRow ++){
                d0 = d4 = pSrc[idxCC[iRow][2]], d1 = d3 = pSrc[idxCC[iRow][1]], d2 = pSrc[idxCC[iRow][0]]; // left boundary
                
                for(iColumn = 0; iColumn + 2 < pSC->cmbWidth * 16; iColumn += 2){
                    pDst[((iColumn >> 4) << (8 - cShift)) + idxCC[iRow][(iColumn & 15) >> cShift]] = DF_ODD;
                    d0 = d2, d1 = d3, d2 = d4;
                    d3 = pSrc[(((iColumn + 3) >> 4) << 8) + idxCC[iRow][(iColumn + 3) & 0xf]];
                    d4 = pSrc[(((iColumn + 4) >> 4) << 8) + idxCC[iRow][(iColumn + 4) & 0xf]];
                }

                d4 = d2; // right boundary
                pDst[((iColumn >> 4) << (8 - cShift)) + idxCC[iRow][(iColumn & 15) >> cShift]] = DF_ODD;
            }
        }

        if(cfInt == YUV_420){ // need to do vertical downsampling
            const size_t cShift = (cfExt == YUV_422 ? 0 : 1);
            PixelI * pBuf[4];
            size_t mbOff, pxOff;
            
            pDst = pSC->p1MBbuffer[iChannel];
            pSrc = (iChannel == 1 ? pSC->pResU : pSC->pResV);
            pBuf[0] = pSrc + (pSC->cmbWidth << (cfExt == YUV_422 ? 7 : 8));
            pBuf[1] = pBuf[0] + pSC->cmbWidth * 8, pBuf[2] = pBuf[1] + pSC->cmbWidth * 8, pBuf[3] = pBuf[2] + pSC->cmbWidth * 8;

            for(iColumn = 0; iColumn < pSC->cmbWidth * 8; iColumn ++){
                mbOff = (iColumn >> 3) << (7 + cShift);
                pxOff = (iColumn & 7) << cShift;

                if(pSC->cRow == 0) // top image boundary
                    d0 = d4 = pSrc[mbOff + idxCC[2][pxOff]], d1 = d3 = pSrc[mbOff + idxCC[1][pxOff]], d2 = pSrc[mbOff + idxCC[0][pxOff]]; // top MB boundary
                else{
                    // last row of previous MB row
                    d0 = pBuf[0][iColumn], d1 = pBuf[1][iColumn], d2 = pBuf[2][iColumn], d3 = pBuf[3][iColumn], d4 = pSrc[mbOff + idxCC[0][pxOff]];
                    pSC->p0MBbuffer[iChannel][((iColumn >> 3) << 6) + idxCC_420[7][iColumn & 7]] = DF_ODD;

                    // for first row of current MB
                    d0 = pBuf[2][iColumn], d1 = pBuf[3][iColumn];
                    d2 = pSrc[mbOff + idxCC[0][pxOff]], d3 = pSrc[mbOff + idxCC[1][pxOff]], d4 = pSrc[mbOff + idxCC[2][pxOff]];
                }

                for(iRow = 0; iRow < 12; iRow += 2){
                    pDst[((iColumn >> 3) << 6) + idxCC_420[iRow >> 1][iColumn & 7]] = DF_ODD;
                    d0 = d2, d1 = d3, d2 = d4;
                    d3 = pSrc[mbOff + idxCC[iRow + 3][pxOff]];
                    d4 = pSrc[mbOff + idxCC[iRow + 4][pxOff]];
                }
                
                //last row of current MB
                pDst[((iColumn >> 3) << 6) + idxCC_420[6][iColumn & 7]] = DF_ODD;
                d0 = d2, d1 = d3, d2 = d4;
                d3 = pSrc[mbOff + idxCC[iRow + 3][pxOff]];

                if(pSC->cRow + 1 == pSC->cmbHeight){ // bottom image boundary
                    d4 = d2;
                    pDst[((iColumn >> 3) << 6) + idxCC_420[7][iColumn & 7]] = DF_ODD;
                }
                else{
                    for(iRow = 0; iRow < 4; iRow ++)
                        pBuf[iRow][iColumn] = pSrc[mbOff + idxCC[iRow + 12][pxOff]];
                }
            }
        }
    }
}

// centralized horizontal padding
Void padHorizonally(CWMImageStrCodec * pSC)
{
    if(pSC->WMII.cWidth != pSC->cmbWidth * 16){ // horizontal padding is necessary!
        const COLORFORMAT cfExt = pSC->WMII.cfColorFormat;
        size_t cFullChannel = pSC->WMISCP.cChannel;
        size_t iLast = pSC->WMII.cWidth - 1;
        PixelI * pCh[16];
        size_t iChannel, iColumn, iRow;

        if(cfExt == YUV_420 || cfExt == YUV_422 || cfExt == Y_ONLY)
            cFullChannel = 1;

        assert(cFullChannel <= 16);

        assert(pSC->WMISCP.cChannel <= 16);
        for(iChannel = 0; iChannel < pSC->WMISCP.cChannel; iChannel ++)
            pCh[iChannel & 15] = pSC->p1MBbuffer[iChannel & 15];

        if(pSC->m_bUVResolutionChange)
            pCh[1] = pSC->pResU, pCh[2] = pSC->pResV;

        // pad full resoluton channels
        for(iRow = 0; iRow < 16; iRow ++){
            const size_t iPosLast = ((iLast >> 4) << 8) + idxCC[iRow][iLast & 0xf];
            for(iColumn = iLast + 1; iColumn < pSC->cmbWidth * 16; iColumn ++){
                const size_t iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                for(iChannel = 0; iChannel < cFullChannel; iChannel ++)
                    pCh[iChannel & 15][iPos] = pCh[iChannel & 15][iPosLast];
            }
        }

        if(cfExt == YUV_422) // pad YUV_422 UV
            for(iLast >>= 1, iRow = 0; iRow < 16; iRow ++){
                const size_t iPosLast = ((iLast >> 3) << 7) + idxCC[iRow][iLast & 7];
                for(iColumn = iLast + 1; iColumn < pSC->cmbWidth * 8; iColumn ++){
                    const size_t iPos = ((iColumn >> 3) << 7) + idxCC[iRow][iColumn & 7];
                    for(iChannel = 1; iChannel < 3; iChannel ++)
                        pCh[iChannel][iPos] = pCh[iChannel][iPosLast];
                }
            }
        else if(cfExt == YUV_420) // pad YUV_420 UV
            for(iLast >>= 1, iRow = 0; iRow < 8; iRow ++){
                const size_t iPosLast = ((iLast >> 3) << 6) + idxCC_420[iRow][iLast & 7];
                for(iColumn = iLast + 1; iColumn < pSC->cmbWidth * 8; iColumn ++){
                    const size_t iPos = ((iColumn >> 3) << 6) + idxCC_420[iRow][iColumn & 7];
                    for(iChannel = 1; iChannel < 3; iChannel ++)
                        pCh[iChannel][iPos] = pCh[iChannel][iPosLast];
                }
            }
    }
}

// centralized alpha channel color conversion, small perf penalty
Int inputMBRowAlpha(CWMImageStrCodec* pSC)
{
    if(pSC->m_bSecondary == FALSE && pSC->m_pNextSC != NULL){ // alpha channel is present
        const size_t cShift = (pSC->m_pNextSC->m_param.bScaledArith ? (SHIFTZERO + QPFRACBITS) : 0);
        const BITDEPTH_BITS bdExt = pSC->WMII.bdBitDepth;
//        const size_t iAlphaPos = pSC->WMII.cLeadingPadding + pSC->WMISCP.cChannel;
        const size_t iAlphaPos = pSC->WMII.cLeadingPadding + (pSC->WMII.cfColorFormat == CMYK ? 4 : 3);//only RGB and CMYK may have interleaved alpha
//        const size_t iAlphaPos = 3;
        const size_t cRow = pSC->WMIBI.cLine;
        const size_t cColumn = pSC->WMII.cWidth;
        const U8 * pSrc0 = (U8 *)pSC->WMIBI.pv;
        PixelI * pA = pSC->m_pNextSC->p1MBbuffer[0];
        size_t iRow, iColumn;

        for(iRow = 0; iRow < 16; iRow ++){
            if(bdExt == BD_8){
                const size_t cStride = (pSC->WMII.cBitsPerUnit >> 3);
                const U8 * pSrc = pSrc0;
                
                for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride)
                    pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = ((PixelI)pSrc[iAlphaPos] - (1 << 7)) << cShift;
            }
            else if(bdExt == BD_16){
                const size_t cStride = (pSC->WMII.cBitsPerUnit >> 3) / sizeof(U16);
                const U8 nLenMantissaOrShift = pSC->m_pNextSC->WMISCP.nLenMantissaOrShift;
                const U16 * pSrc = (U16 *)pSrc0;

                for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride)
                    pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = ((((PixelI)pSrc[iAlphaPos] - (1 << 15)) >> nLenMantissaOrShift) << cShift);
            }
            else if(bdExt == BD_16S){
                const size_t cStride = (pSC->WMII.cBitsPerUnit >> 3) / sizeof(I16);
                const U8 nLenMantissaOrShift = pSC->m_pNextSC->WMISCP.nLenMantissaOrShift;
                const I16 * pSrc = (I16 *)pSrc0;
            
                for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride)
                    pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = (((PixelI)pSrc[iAlphaPos] >> nLenMantissaOrShift) << cShift);
            }
            else if(bdExt == BD_16F){
                const size_t cStride = (pSC->WMII.cBitsPerUnit >> 3) / sizeof(U16);
                const I16 * pSrc = (I16 *)pSrc0;

                for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride)
                    pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = forwardHalf (pSrc[iAlphaPos]) << cShift;
            }
            else if(bdExt == BD_32S){
                const size_t cStride = (pSC->WMII.cBitsPerUnit >> 3) / sizeof(I32);
                const U8 nLenMantissaOrShift = pSC->m_pNextSC->WMISCP.nLenMantissaOrShift;
                const I32 * pSrc = (I32 *)pSrc0;
            
                for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride)
                    pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = (((PixelI)pSrc[iAlphaPos] >> nLenMantissaOrShift) << cShift);
            }
            else if(bdExt == BD_32F){
                const size_t cStride = (pSC->WMII.cBitsPerUnit >> 3) / sizeof(float);
                const U8 nLen = pSC->m_pNextSC->WMISCP.nLenMantissaOrShift;
                const I8 nExpBias = pSC->m_pNextSC->WMISCP.nExpBias;
                const float * pSrc = (float *)pSrc0;
            
                for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride)
                    pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = float2pixel (pSrc[iAlphaPos], nExpBias, nLen) << cShift;
            }
            else // not supported
                return ICERR_ERROR;

            if(iRow + 1 < cRow) // vertical padding!
                pSrc0 += pSC->WMIBI.cbStride;

            for(iColumn = cColumn; iColumn < pSC->cmbWidth * 16; iColumn ++) // horizontal padding
                pA[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] =  pA[((cColumn >> 4) << 8) + idxCC[iRow][cColumn & 0xf]];
        }
    }
    
    return ICERR_OK;
}

// input one MB row of image data from input buffer
#define _CC_BAYER(g0, r, b, g1) g1 -= g0, g0 += (g1 >> 1), _CC(r, g0, b)

Int inputMBRow(CWMImageStrCodec* pSC)
{
    const size_t cShift = (pSC->m_param.bScaledArith ? (SHIFTZERO + QPFRACBITS) : 0);
    const BITDEPTH_BITS bdExt = pSC->WMII.bdBitDepth;
    COLORFORMAT cfExt = pSC->WMII.cfColorFormat;
    const COLORFORMAT cfInt = pSC->m_param.cfColorFormat;
    const U8 cBP = pSC->WMII.bpBayerPattern;
    const size_t cPixelStride = (pSC->WMII.cBitsPerUnit >> 3);
    const size_t iRowStride = (cfExt == YUV_420 ? 2 : 1);
    const size_t cRow = pSC->WMIBI.cLine;
    const size_t cColumn = pSC->WMII.cWidth;
    const U8 * pSrc0 = (U8 *)pSC->WMIBI.pv;
    const size_t iB = (cfExt == BAYER ? (1 + (cBP & 2) - (cBP & 1)) : (pSC->WMII.bRGB ? 2 : 0));
    const size_t iR = (cfExt == BAYER ? 3 : 2) - iB;
    const size_t iG0 = (cBP == 0 || cBP == 3 ? 0 : 3), iG1 = 3 - iG0;
    const U8 nLen = pSC->WMISCP.nLenMantissaOrShift;
    const I8 nExpBias = pSC->WMISCP.nExpBias;

    PixelI *pY = pSC->p1MBbuffer[0], *pU = pSC->p1MBbuffer[1], *pV = pSC->p1MBbuffer[2];
    size_t iRow, iColumn, iPos;

    // guard input buffer
    if(checkImageBuffer(pSC, cColumn, cRow) != ICERR_OK)
        return ICERR_ERROR;

    if(pSC->m_bUVResolutionChange)  // will do downsampling somewhere else!
        pU = pSC->pResU, pV = pSC->pResV;
    else if(cfInt == Y_ONLY) // xxx to Y_ONLY transcoding!
        pU = pV = pY; // write pY AFTER pU and pV so Y will overwrite U&V

    for(iRow = 0; iRow < 16; iRow += iRowStride){
        if(bdExt == BD_8){
            const U8 * pSrc = pSrc0 + pSC->WMII.cLeadingPadding;
            const PixelI iOffset = (128 << cShift);

            switch(cfExt){
                case CF_RGB:
                    assert (pSC->m_bSecondary == FALSE);
/*                    if (pSC->WMISCP.uAlphaMode == 3) { // RGBA
                        PixelI *pA = pSC->m_pNextSC->p1MBbuffer[0];
                        const size_t cShiftA = (pSC->m_pNextSC->m_param.bScaledArith ? (SHIFTZERO + QPFRACBITS) : 0);

                        for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                            PixelI r = ((PixelI)pSrc[iR]) << cShift, g = ((PixelI)pSrc[1]) << cShift, b = ((PixelI)pSrc[iB]) << cShift;
                            
                            _CC(r, g, b); // color conversion

                            iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                            pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;

                            pA[iPos] = ((PixelI)pSrc[3] - 128) << cShiftA;
                        }
                    }
                    else { // general case*/
                        for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                            PixelI r = ((PixelI)pSrc[iR]) << cShift, g = ((PixelI)pSrc[1]) << cShift, b = ((PixelI)pSrc[iB]) << cShift;
                       

                            _CC(r, g, b); // color conversion
                       
                            iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                            pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
                        }
//                    }
                    break;

                case Y_ONLY:
#ifdef _PHOTON_PK_
                case YUV_444:
#endif
                case N_CHANNEL:
                {
                    const size_t cChannel = pSC->m_param.cNumChannels;
                    PixelI * pChannel[16];
                    size_t iChannel;

                    assert(cChannel <= 16);
                    for(iChannel = 0; iChannel < cChannel; iChannel ++)
                        pChannel[iChannel & 15] = pSC->p1MBbuffer[iChannel & 15];
                    if(pSC->m_bUVResolutionChange)
                        pChannel[1] = pSC->pResU, pChannel[2] = pSC->pResV;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pChannel[iChannel & 15][iPos] = (((PixelI)pSrc[iChannel & 15]) << cShift) - iOffset;
                    }
                    break;
                }

                case CF_RGBE:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                        PixelI iExp = (PixelI)pSrc[3];
                        PixelI r = forwardRGBE (pSrc[0], iExp) << cShift;
                        PixelI g = forwardRGBE (pSrc[1], iExp) << cShift;
                        PixelI b = forwardRGBE (pSrc[2], iExp) << cShift;

                        _CC(r, g, b);

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case CMYK:
                {
                    PixelI * pK = (cfInt == CMYK ? pSC->p1MBbuffer[3] : pY); // CMYK -> YUV_xxx transcoding!

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                        PixelI c = ((PixelI)pSrc[0]) << cShift;
                        PixelI m = ((PixelI)pSrc[1]) << cShift;
                        PixelI y = ((PixelI)pSrc[2]) << cShift;
                        PixelI k = ((PixelI)pSrc[3]) << cShift;

                        _CC_CMYK(c, m, y, k);
                        
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = c, pV[iPos] = -y, pK[iPos] = k, pY[iPos] = iOffset - m;
                    }
                    break;
                }

#ifdef _PHOTON_PK_
                case BAYER:
                {
                    PixelI * pD = (cfInt == CMYK ? pSC->p1MBbuffer[3] : pY), pQuad[4]; // BAYER -> YUV_xxx transcoding!
                    size_t cW2 = cColumn * 2;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += 2){
                        pQuad[0] = ((PixelI)pSrc[0]) << cShift;
                        pQuad[1] = ((PixelI)pSrc[1]) << cShift;
                        pQuad[2] = ((PixelI)pSrc[cW2]) << cShift;
                        pQuad[3] = ((PixelI)pSrc[cW2 + 1]) << cShift;

                        _CC_BAYER(pQuad[iG0], pQuad[iR], pQuad[iB], pQuad[iG1]);
                        
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -pQuad[iR], pV[iPos] = pQuad[iB], pD[iPos] = pQuad[iG1], pY[iPos] = pQuad[iG0] - iOffset;
                    }
                    break;
                }

                case YUV_422:
                    for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += cPixelStride){
                        if(cfInt != Y_ONLY){
                            iPos = ((iColumn >> 4) << 7) + idxCC[iRow][(iColumn >> 1) & 7];
                            pU[iPos] = (((PixelI)pSrc[0]) << cShift) - iOffset;
                            pV[iPos] = (((PixelI)pSrc[2]) << cShift) - iOffset;
                        }

                        pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (((PixelI)pSrc[1]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (((PixelI)pSrc[3]) << cShift) - iOffset;
                    }
                    break;

                case YUV_420:
                    for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += cPixelStride){
                        if(cfInt != Y_ONLY){
                            iPos = ((iColumn >> 4) << 6) + idxCC_420[iRow >> 1][(iColumn >> 1) & 7];
                            pU[iPos] = (((PixelI)pSrc[4]) << cShift) - iOffset;
                            pV[iPos] = (((PixelI)pSrc[5]) << cShift) - iOffset;
                        }

                        pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (((PixelI)pSrc[0]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (((PixelI)pSrc[1]) << cShift) - iOffset;
                        pY[((iColumn >> 4) << 8) + idxCC[iRow + 1][iColumn & 15]] = (((PixelI)pSrc[2]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow + 1][(iColumn + 1) & 15]] = (((PixelI)pSrc[3]) << cShift) - iOffset;
                    }
                    break;
#endif

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_16){
            const U16 * pSrc = (U16 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(U16);
            const PixelI iOffset = ((1 << 15) >> nLen) << cShift;

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = ((PixelI)pSrc[0] >> nLen) << cShift, g = ((PixelI)pSrc[1] >> nLen) << cShift, b = ((PixelI)pSrc[2] >> nLen) << cShift;
                        
                        _CC(r, g, b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
                    }
                    break;

                case Y_ONLY:
#ifdef _PHOTON_PK_
                case YUV_444:
#endif
                case N_CHANNEL:
                {
                    const size_t cChannel = pSC->WMISCP.cChannel;
                    size_t iChannel;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pSC->p1MBbuffer[iChannel][iPos] = (((PixelI)pSrc[iChannel] >> nLen) << cShift) - iOffset;
                    }
                    break;
                }

                case CMYK:
                {
                    PixelI * pK = (cfInt == CMYK ? pSC->p1MBbuffer[3] : pY); // CMYK -> YUV_xxx transcoding!

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI c = ((PixelI)pSrc[0] >> nLen) << cShift;
                        PixelI m = ((PixelI)pSrc[1] >> nLen) << cShift;
                        PixelI y = ((PixelI)pSrc[2] >> nLen) << cShift;
                        PixelI k = ((PixelI)pSrc[3] >> nLen) << cShift;

                        _CC_CMYK(c, m, y, k);
                        
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = c, pV[iPos] = -y, pK[iPos] = k, pY[iPos] = iOffset - m;
                    }
                    break;
                }

#ifdef _PHOTON_PK_
                case BAYER:
                {
                    PixelI * pD = (cfInt == CMYK ? pSC->p1MBbuffer[3] : pY), pQuad[4]; // BAYER -> YUV_xxx transcoding!
                    size_t cW2 = cColumn * 2;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += 2){
                        pQuad[0] = ((PixelI)pSrc[0] >> nLen) << cShift;
                        pQuad[1] = ((PixelI)pSrc[1] >> nLen) << cShift;
                        pQuad[2] = ((PixelI)pSrc[cW2] >> nLen) << cShift;
                        pQuad[3] = ((PixelI)pSrc[cW2 + 1] >> nLen) << cShift;

                        _CC_BAYER(pQuad[iG0], pQuad[iR], pQuad[iB], pQuad[iG1]);
                        
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -pQuad[iR], pV[iPos] = pQuad[iB], pD[iPos] = pQuad[iG1], pY[iPos] = pQuad[iG0] - iOffset;
                    }
                    break;
                }

                case YUV_422:
                    for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += cPixelStride){
                        if(cfInt != Y_ONLY){
                            iPos = ((iColumn >> 4) << 7) + idxCC[iRow][(iColumn >> 1) & 7];
                            pU[iPos] = (((PixelI)pSrc[0]) << cShift) - iOffset;
                            pV[iPos] = (((PixelI)pSrc[2]) << cShift) - iOffset;
                        }

                        pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (((PixelI)pSrc[1]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (((PixelI)pSrc[3]) << cShift) - iOffset;
                    }
                    break;

                case YUV_420:
                    for(iColumn = 0; iColumn < cColumn; iColumn += 2, pSrc += cPixelStride){
                        if(cfInt != Y_ONLY){
                            iPos = ((iColumn >> 4) << 6) + idxCC_420[iRow >> 1][(iColumn >> 1) & 7];
                            pU[iPos] = (((PixelI)pSrc[4]) << cShift) - iOffset;
                            pV[iPos] = (((PixelI)pSrc[5]) << cShift) - iOffset;
                        }

                        pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 15]] = (((PixelI)pSrc[0]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow][(iColumn + 1) & 15]] = (((PixelI)pSrc[1]) << cShift) - iOffset;
                        pY[((iColumn >> 4) << 8) + idxCC[iRow + 1][iColumn & 15]] = (((PixelI)pSrc[2]) << cShift) - iOffset;
                        pY[(((iColumn + 1) >> 4) << 8) + idxCC[iRow + 1][(iColumn + 1) & 15]] = (((PixelI)pSrc[3]) << cShift) - iOffset;
                    }
                    break;
#endif

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_16S){
            const I16 * pSrc = (I16 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(I16);

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = ((PixelI)pSrc[0] >> nLen) << cShift, g = ((PixelI)pSrc[1] >> nLen) << cShift, b = ((PixelI)pSrc[2] >> nLen) << cShift;
                        
                        _CC(r, g, b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case Y_ONLY:
#ifdef _PHOTON_PK_
                case YUV_444:
                case N_CHANNEL:
#endif
                {
                    const size_t cChannel = pSC->WMISCP.cChannel;
                    size_t iChannel;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pSC->p1MBbuffer[iChannel][iPos] = (((PixelI)pSrc[iChannel] >> nLen) << cShift);
                    }
                    break;
                }

#ifdef _PHOTON_PK_
                case CMYK:
                {
                    PixelI * pK = (cfInt == CMYK ? pSC->p1MBbuffer[3] : pY); // CMYK -> YUV_xxx transcoding!

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI c = ((PixelI)pSrc[0] >> nLen) << cShift;
                        PixelI m = ((PixelI)pSrc[1] >> nLen) << cShift;
                        PixelI y = ((PixelI)pSrc[2] >> nLen) << cShift;
                        PixelI k = ((PixelI)pSrc[3] >> nLen) << cShift;

                        _CC_CMYK(c, m, y, k);
                        
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = c, pV[iPos] = -y, pK[iPos] = k, pY[iPos] = -m;
                    }
                    break;
                }

                case BAYER:
                {
                    PixelI * pD = (cfInt == CMYK ? pSC->p1MBbuffer[3] : pY), pQuad[4]; // BAYER -> YUV_xxx transcoding!
                    size_t cW2 = cColumn * 2;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += 2){
                        pQuad[0] = ((PixelI)pSrc[0] >> nLen) << cShift;
                        pQuad[1] = ((PixelI)pSrc[1] >> nLen) << cShift;
                        pQuad[2] = ((PixelI)pSrc[cW2] >> nLen) << cShift;
                        pQuad[3] = ((PixelI)pSrc[cW2 + 1] >> nLen) << cShift;

                        _CC_BAYER(pQuad[iG0], pQuad[iR], pQuad[iB], pQuad[iG1]);
                        
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -pQuad[iR], pV[iPos] = pQuad[iB], pD[iPos] = pQuad[iG1], pY[iPos] = pQuad[iG0];
                    }
                    break;
                }
#endif

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_16F){
            const I16 * pSrc = (I16 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(U16);

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = forwardHalf (pSrc[0]) << cShift;
                        PixelI g = forwardHalf (pSrc[1]) << cShift;
                        PixelI b = forwardHalf (pSrc[2]) << cShift;
                        
                        _CC(r, g, b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case Y_ONLY:
#ifdef _PHOTON_PK_
                case YUV_444:
                case N_CHANNEL:
#endif
                {
                    const size_t cChannel = pSC->WMISCP.cChannel; // check xxx => Y_ONLY transcoding!
                    size_t iChannel;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pSC->p1MBbuffer[iChannel][iPos] = forwardHalf (pSrc[iChannel]) << cShift;
                    }
                    break;
                }

                default:
                    assert(0);
                    break;
            }
        }
#ifdef _PHOTON_PK_
        else if(bdExt == BD_32){
            const U32 * pSrc = (U32 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(U32);
            const PixelI iOffset = ((1 << 31) >> nLen) << cShift;

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = (pSrc[0] >> nLen) << cShift, g = (pSrc[1] >> nLen) << cShift, b = (pSrc[2] >> nLen) << cShift;
                        
                        _CC(r, g, b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
                    }
                    break;

                case Y_ONLY:
                case YUV_444:
                case N_CHANNEL:
                {
                    const size_t cChannel = pSC->WMISCP.cChannel;
                    size_t iChannel;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pSC->p1MBbuffer[iChannel][iPos] = (pSrc[iChannel] >> nLen) << cShift;
                    }
                    break;
                }

                default:
                    assert(0);
                    break;
            }
        }
#endif
        else if(bdExt == BD_32S){
            const I32 * pSrc = (I32 *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(I32);

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = (pSrc[0] >> nLen)<< cShift, g = (pSrc[1] >> nLen)<< cShift, b = (pSrc[2] >> nLen)<< cShift;
                        
                        _CC(r, g, b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case Y_ONLY:
#ifdef _PHOTON_PK_
                case YUV_444:
                case N_CHANNEL:
#endif
                {
                    const size_t cChannel = pSC->WMISCP.cChannel; // check xxx => Y_ONLY transcoding!
                    size_t iChannel;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pSC->p1MBbuffer[iChannel][iPos] = (pSrc[iChannel] >> nLen) << cShift;
                    }
                    break;
                }

                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_32F){
            const float * pSrc = (float *)pSrc0 + pSC->WMII.cLeadingPadding;
            const size_t cStride = cPixelStride / sizeof(float);

            switch(cfExt){
                case CF_RGB:
                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        PixelI r = float2pixel (pSrc[0], nExpBias, nLen) << cShift;
                        PixelI g = float2pixel (pSrc[1], nExpBias, nLen) << cShift;
                        PixelI b = float2pixel (pSrc[2], nExpBias, nLen) << cShift;

                        _CC(r, g, b); // color conversion

                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g;
                    }
                    break;

                case Y_ONLY:
#ifdef _PHOTON_PK_
                case YUV_444:
                case N_CHANNEL:
#endif
                {
                    const size_t cChannel = pSC->WMISCP.cChannel;
                    size_t iChannel;

                    for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cStride){
                        iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                        for(iChannel = 0; iChannel < cChannel; iChannel ++)
                            pSC->p1MBbuffer[iChannel][iPos] = float2pixel (pSrc[iChannel], nExpBias, nLen) << cShift;
                    }
                    break;
                }
                default:
                    assert(0);
                    break;
            }
        }
        else if(bdExt == BD_5){ // RGB 555, work for both big endian and small endian!
            const U8 * pSrc = pSrc0;
            const PixelI iOffset = (16 << cShift);

            assert(cfExt == CF_RGB);
            
            for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                PixelI r = (PixelI)pSrc[0], g = (PixelI)pSrc[1], b = ((g >> 2) & 0x1F) << cShift;

                g = ((r >> 5) + ((g & 3) << 3)) << cShift, r = (r & 0x1F) << cShift;

                _CC(r, g, b); // color conversion
                
                iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
            }
        }
        else if(bdExt == BD_565){ // RGB 555, work for both big endian and small endian!
            const U8 * pSrc = pSrc0;
            const PixelI iOffset = (32 << cShift);

            assert(cfExt == CF_RGB);
            
            for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                PixelI r = (PixelI)pSrc[0], g = (PixelI)pSrc[1], b = (g >> 3) << (cShift + 1);

                g = ((r >> 5) + ((g & 7) << 3)) << cShift, r = (r & 0x1F) << (cShift + 1);

                _CC(r, g, b); // color conversion
                
                iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
            }
        }
        else if(bdExt == BD_10){ //RGB 101010, work for both big endian and small endian!
            const U8 * pSrc = pSrc0;
            const PixelI iOffset = (512 << cShift);

            assert(cfExt == CF_RGB);
            
            for(iColumn = 0; iColumn < cColumn; iColumn ++, pSrc += cPixelStride){
                PixelI r = (PixelI)pSrc[0], g = (PixelI)pSrc[1], b = (PixelI)pSrc[2];

                r = (r + ((g & 3) << 8)) << cShift, g = ((g >> 2) + ((b & 0xF) << 6)) << cShift;
                b = ((b >> 4) + (((PixelI)pSrc[3] & 0x3F) << 4)) << cShift;

                _CC(r, g, b); // color conversion
                
                iPos = ((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf];
                pU[iPos] = -r, pV[iPos] = b, pY[iPos] = g - iOffset;
            }
        }
        else if(bdExt == BD_1){
            assert(cfExt == Y_ONLY);
            for(iColumn = 0; iColumn < cColumn; iColumn ++) {
                pY[((iColumn >> 4) << 8) + idxCC[iRow][iColumn & 0xf]] = ((pSC->WMISCP.bBlackWhite + (pSrc0[iColumn >> 3] >> (7 - (iColumn & 7)))) & 1) << cShift;
            }
        }

        if(iRow + 1 < cRow) // centralized vertical padding!
            pSrc0 += pSC->WMIBI.cbStride;
    }

    padHorizonally(pSC); // centralized horizontal padding

    // centralized down-sampling
    if(pSC->m_bUVResolutionChange)
        downsampleUV(pSC);

    // centralized alpha channel handdling
    if (pSC->WMISCP.uAlphaMode == 3)
        if(inputMBRowAlpha(pSC) != ICERR_OK)
            return ICERR_ERROR;

    return ICERR_OK;
}



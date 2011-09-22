//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
/******************************************************************************

Module Name:
    decode.c 
    
Abstract:    
    Defines the entry point for the console application.

Author:

Revision History:
*******************************************************************************/
#include "strcodec.h"
#include "decode.h"

#ifdef MEM_TRACE
#define TRACE_MALLOC    1
#define TRACE_NEW       0
#define TRACE_HEAP      0
#include "memtrace.h"
#endif


/******************************************************************
Free Adaptive Huffman Table
******************************************************************/
static Void CleanAH(CAdaptiveHuffman **ppAdHuff)
{
    CAdaptiveHuffman *pAdHuff;
    
    if (NULL != ppAdHuff) {
        pAdHuff = *ppAdHuff;
        if (NULL != pAdHuff) {
            CleanHuff(pAdHuff->m_pHuffman);
            free(pAdHuff->m_pData);
            free(pAdHuff);
        }
        *ppAdHuff = NULL;
    }
}

static Void CleanAHDec(CCodingContext * pSC)
{
    Int kk;

    for (kk = 0; kk < NUMVLCTABLES; kk++) {
        CleanAH(&(pSC->m_pAHexpt[kk]));
    }
    CleanAH(&(pSC->m_pAdaptHuffCBPCY));
    CleanAH(&(pSC->m_pAdaptHuffCBPCY1));
}

/*************************************************************************
    Initialize an adaptive huffman table
*************************************************************************/
static Int InitializeAH(CAdaptiveHuffman **ppAdHuff, Int iSym)
{
    Int iMemStatus = 0;

    CAdaptiveHuffman *pAdHuff = Allocate(iSym, DECODER);
    if (pAdHuff == NULL) {
        iMemStatus = -1;    // out of memory
        goto ErrorExit;
    }
    pAdHuff->m_pHuffman = allocHuff();//(Huffman*)malloc(sizeof(Huffman));
    if (pAdHuff->m_pHuffman == NULL) {
        iMemStatus = -1;    // out of memory
        goto ErrorExit;
    }

    //Adapt(pAdHuff, bFixedTables);
    //InitHuffman(pAdHuff->m_pHuffman);
    //if (ICERR_OK != initHuff(pAdHuff->m_pHuffman, 1, pAdHuff->m_pTable, NULL)) {
    //    goto ErrorExit;
    //}
    *ppAdHuff = pAdHuff;
    return ICERR_OK;

ErrorExit:
    if (pAdHuff) {
        if (pAdHuff->m_pHuffman) {
            free(pAdHuff->m_pHuffman);
        }
        free(pAdHuff);
    }
    *ppAdHuff = NULL;
    if (-1 == iMemStatus) {
        printf("Insufficient memory to init decoder.\n");
    }
    return ICERR_ERROR;
}


/*************************************************************************
    Context allocation
*************************************************************************/
Int AllocateCodingContextDec(CWMImageStrCodec *pSC, Int iNumContexts)
{
    Int i, iCBPSize, k;
    static const Int aAlphabet[] = {5,4,8,7,7,  12,6,6,12,6,6,7,7,  12,6,6,12,6,6,7,7};

    if (iNumContexts > 256 || iNumContexts < 1)  // only between 1 and 256 allowed
        return ICERR_ERROR;

    if (pSC == NULL)
        return ICERR_ERROR;

    pSC->m_pCodingContext = malloc (iNumContexts * sizeof (CCodingContext));
    if (pSC->m_pCodingContext == NULL) {
        pSC->cNumCodingContext = 0;
        return ICERR_ERROR;
    }
    memset (pSC->m_pCodingContext, 0, iNumContexts * sizeof (CCodingContext));

    pSC->cNumCodingContext = iNumContexts;
    iCBPSize = (pSC->m_param.cfColorFormat == Y_ONLY || pSC->m_param.cfColorFormat == N_CHANNEL
        || pSC->m_param.cfColorFormat == CMYK) ? 5 : 9;

    /** allocate / initialize members **/
    for (i = 0; i < iNumContexts; i++) {
        CCodingContext *pContext = &(pSC->m_pCodingContext[i]);

        /** allocate adaptive Huffman encoder **/    
        if (InitializeAH(&pContext->m_pAdaptHuffCBPCY, iCBPSize) != ICERR_OK) {
            return ICERR_ERROR;
        }
        if (InitializeAH(&pContext->m_pAdaptHuffCBPCY1, 5) != ICERR_OK) {
            return ICERR_ERROR;
        }

        for(k = 0; k < NUMVLCTABLES; k ++){
            if (InitializeAH(&pContext->m_pAHexpt[k], aAlphabet[k]) != ICERR_OK) {
                return ICERR_ERROR;
            }
        }

        ResetCodingContextDec(pContext);
    }

    return ICERR_OK;
}

/*************************************************************************
    Context reset on encoder
*************************************************************************/
Void ResetCodingContextDec(CCodingContext *pContext)
{
    Int k;
    /** set flags **/
    pContext->m_pAdaptHuffCBPCY->m_bInitialize = FALSE;
    pContext->m_pAdaptHuffCBPCY1->m_bInitialize = FALSE;
    for(k = 0; k < NUMVLCTABLES; k ++)
        pContext->m_pAHexpt[k]->m_bInitialize = FALSE;

    // reset VLC tables
    AdaptLowpassDec (pContext);
    AdaptHighpassDec (pContext);

    // reset zigzag patterns, totals
    InitZigzagScan(pContext);
    // reset bit reduction and cbp models
    ResetCodingContext(pContext);
}

/*************************************************************************
    Context deletion
*************************************************************************/
Void FreeCodingContextDec(CWMImageStrCodec *pSC)
{
    Int iContexts = (Int)(pSC->cNumCodingContext), i, k;

    if (iContexts > 0 && pSC->m_pCodingContext) {

        for (i = 0; i < iContexts; i++) {
            CCodingContext *pContext = &(pSC->m_pCodingContext[i]);
            CleanAH (&pContext->m_pAdaptHuffCBPCY);
            CleanAH (&pContext->m_pAdaptHuffCBPCY1);
            for (k = 0; k < NUMVLCTABLES; k++)
                CleanAH (&pContext->m_pAHexpt[k]);
        }
        free (pSC->m_pCodingContext);
    }
}


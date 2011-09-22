//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************

#ifndef WMI_COMMON_H
#define WMI_COMMON_H

#define _PHOTON_PK_

/*************************************************************************
// Common typedef's
*************************************************************************/
typedef enum  { ENCODER = 0, DECODER = 1 } CODINGMODE;

typedef enum tagBand
{
    BAND_HEADER = 0,
    BAND_DC = 1,
    BAND_LP = 2,
    BAND_AC = 3,
    BAND_FL = 4
} BAND;

/*************************************************************************
    struct / class definitions
*************************************************************************/
//#define SIGNATURE_BYTES 8   // Bytes for GDI+ signature
#define CODEC_VERSION 1
#define CODEC_SUBVERSION 0

#define CONTEXTX 8
#define CTDC 5
#define NUMVLCTABLES 21 // CONTEXTX * 2 + CTDC
#define AVG_NDIFF 3

//#define SCALEORDER

#define MAXTOTAL 32767 // 511 should be enough

/** Quantization related defines **/
#define SHIFTZERO 1 /* >= 0 */
#define QPFRACBITS 2  /* or 0 only supported */

/** adaptive huffman encoding / decoding struct **/
typedef struct CAdaptiveHuffman
{
    Int     m_iNSymbols;
    Int     *m_pData;
    Int     *m_pHistogram;
    Int     *m_pHistogramAlt;
    const Int *m_pTable;
    const Int *m_pDelta, *m_pDelta1;
    Int     m_iTableIndex;
    struct  Huffman *m_pHuffman;
    Bool    m_bInitialize;
    //Char    m_pLabel[8]; // for debugging - label attached to constructor

    Int     m_iDiscriminant, m_iDiscriminant1;
    Int     m_iUpperBound;
    Int     m_iLowerBound;
} CAdaptiveHuffman;


/************************************************************************************
  Context structures
************************************************************************************/
typedef struct CAdaptiveModel {
    Int     m_iFlcState[2];
    Int     m_iFlcBits[2];
    BAND    m_band;
} CAdaptiveModel;

typedef struct CCBPModel {
    Int     m_iCount0[2];
    Int     m_iCount1[2];
    Int     m_iState[2];
} CCBPModel;

/*************************************************************************
    globals
*************************************************************************/
extern Int grgiZigzagInv4x4_lowpass[];
extern Int grgiZigzagInv4x4H[];
extern Int grgiZigzagInv4x4V[];
extern const Int gSignificantRunBin[];
extern const Int gSignificantRunFixedLength[];
static const Int cblkChromas[] = {0,4,8,16, 16,16,16, 0,0};
/*************************************************************************
    function declarations
*************************************************************************/
// common utilities
Void Clean (CAdaptiveHuffman *pAdHuff);
CAdaptiveHuffman *Allocate (Int iNSymbols, CODINGMODE cm);

/* Timing functions */
void    reset_timing(double *time);
void    report_timing(const char *s, double time);
static double   timeperclock;

/** adaptive model functions **/
Void UpdateModelMB (COLORFORMAT cf, Int iChannels, Int iLaplacianMean[], CAdaptiveModel *m_pModel);

/** adaptive huffman encoder / decoder functions **/
Void Adapt (CAdaptiveHuffman *pAdHuff, Bool bFixedTables);
Void AdaptFixed (CAdaptiveHuffman *pAdHuff);
Void AdaptDiscriminant (CAdaptiveHuffman *pAdHuff);

//#ifdef X86OPT_PREBUILT_TABLE
//Int conHuffLookupTables();
//#endif // X86OPT_PREBUILT_TABLE

#ifndef _PREFAST_
#pragma warning(disable:4068)
#endif

#endif  // WMI_COMMON_H

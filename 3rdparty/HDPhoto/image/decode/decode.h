//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************

#ifndef WMI_DECODE_H
#define WMI_DECODE_H

typedef struct CWMDecoderParameters {
    /** ROI decode **/
    Bool bDecodeFullFrame;
    Bool bDecodeFullWidth;

    /** thumbnail decode **/
    Bool bSkipFlexbits;
    size_t cThumbnailScale;    // 1: cThumbnailScale thumbnail, only supports cThumbnailScale = 2^m for now
    Bool bDecodeHP;
    Bool bDecodeLP;

    // Region of interest decoding
    size_t cROILeftX;
    size_t cROIRightX;
    size_t cROITopY;
    size_t cROIBottomY;

    // table lookups for rotation and flip
    size_t * pOffsetX;
    size_t * pOffsetY;
} CWMDecoderParameters;

Int getHuff(struct Huffman *pHuffman, BitIOInfo* pIO);
Int initHuff (struct Huffman *pHuffman, Int mode, const Int *huffArray, Int *maxBits);
struct Huffman *allocHuff();
Void CleanHuff(struct Huffman *pHuffman);

Void predCBPDec(CWMImageStrCodec *, CCodingContext *);
Void predDCACDec(CWMImageStrCodec *);
Void predACDec(CWMImageStrCodec *);

Int dequantizeMacroblock(CWMImageStrCodec *);
Int invTransformMacroblock(CWMImageStrCodec * pSC);

Int DecodeMacroblockDC(CWMImageStrCodec * pSC, CCodingContext *pContext, Int iMBX, Int iMBY);
Int DecodeMacroblockLowpass(CWMImageStrCodec * pSC, CCodingContext *pContext, Int iMBX, Int iMBY);
Int DecodeMacroblockHighpass(CWMImageStrCodec * pSC, CCodingContext *pContext, Int iMBX, Int iMBY);

Int AdaptLowpassDec(struct CCodingContext *);
Int AdaptHighpassDec(struct CCodingContext *);

Void ResetCodingContextDec(CCodingContext *pContext);
Void FreeCodingContextDec(struct CWMImageStrCodec *pSC);

/*************************************************************************/
// Inverse transform functions
// 2-point post filter for boundaries (only used in 420 UV DC subband)
Void strPost2(PixelI *, PixelI *);

// 2x2 post filter (only used in 420 UV DC subband)
Void strPost2x2(PixelI *, PixelI *, PixelI *, PixelI *);

/** 4-point post filter for boundaries **/
Void strPost4(PixelI *, PixelI *, PixelI *, PixelI *);

/** data allocation in working buffer (first stage) **/

/** Y, 444 U and V **/
/**  0  1  2  3 **/
/** 32 33 34 35 **/
/** 64 65 66 67 **/
/** 96 97 98 99 **/

/** 420 U and V **/
/**   0   2   4   6 **/
/**  64  66  68  70 **/
/** 128 130 132 134 **/
/** 192 194 196 198 **/

/** 4x4 inverse DCT for first stage **/
Void strIDCT4x4FirstStage(PixelI *);
Void strIDCT4x4Stage1(PixelI*);
Void strIDCT4x4FirstStage420UV(PixelI *);

/** 4x4 post filter for first stage **/
Void strPost4x4FirstStage(PixelI *);
Void strPost4x4Stage1Split(PixelI*, PixelI*,Int);
Void strPost4x4Stage1(PixelI*,Int);
//Void strPost4x4Stage1Split_420(PixelI*, PixelI*);
//Void strPost4x4Stage1_420(PixelI*);

Void strPost4x4FirstStage420UV(PixelI *);

/** data allocation in working buffer (second stage)**/

/** Y, 444 U and V **/
/**   0   4   8  12 **/
/** 128 132 136 140 **/
/** 256 260 264 268 **/
/** 384 388 392 396 **/

/** 420 U and V **/
/**   0   8 **/
/** 256 264 **/

/** 4x4 invesr DCT for second stage **/
//Void strIDCT4x4SecondStage(PixelI *);
Void strIDCT4x4Stage2(PixelI*);
Void strNormalizeDec(PixelI*, Bool);
Void strDCT2x2dnDec(PixelI *, PixelI *, PixelI *, PixelI *);

/** 4x4 post filter for second stage **/
Void strPost4x4SecondStage(PixelI *);
Void strPost4x4Stage2Split(PixelI*, PixelI*);

/** Huffman decode related defines **/
#if !(defined(ARMOPT_HUFFMAN) || defined(X86OPT_HUFFMAN))
#define HUFFMAN_DECODE_ROOT_BITS_LOG    4
#else
#define HUFFMAN_DECODE_ROOT_BITS_LOG    3
#endif	// ARMOPT_HUFFMAN || X86OPT_HUFFMAN
// returns dimension of decoding table (we suppose that prefix code is full)
#define HUFFMAN_DECODE_TABLE_SIZE(iRootBits,iAlphabetSize) ((1 << (iRootBits)) + (2 * (iAlphabetSize)))
// Default root table size
#if !(defined(ARMOPT_HUFFMAN) || defined(X86OPT_HUFFMAN))
#define HUFFMAN_DECODE_ROOT_BITS    (10)    // AKadatch: due to a number of reasons, don't set it below 10
#else
#define HUFFMAN_DECODE_ROOT_BITS    (5)    
#endif	//ARMOPT_HUFFMAN || X86OPT_HUFFMAN

#if !(defined(ARMOPT_HUFFMAN) || defined(X86OPT_HUFFMAN))
typedef struct tagHuffEncInfo {
  UInt code;
  UInt length;
} HuffEncInfo;

typedef struct tagHuffDecInfo {

#     define HUFFDEC_SYMBOL_BITS 12
#     define HUFFDEC_LENGTH_BITS 4

  U16 symbol : HUFFDEC_SYMBOL_BITS;
  U16 length : HUFFDEC_LENGTH_BITS;

} HuffDecInfo;

typedef struct tagTableInfo {
    Int bits;
    HuffDecInfo *table;
} TableInfo;

typedef struct tagTableInitInfo {
  Int prefix;     // the prefix that got you there
  Int start, end; // start, end tableNums of children tables
  Int bits;       // the bits for this table
  Int maxBits;    // the maximum # of bits of things entering this table
} TableInitInfo;

#endif	// ARMOPT_HUFFMAN || X86OPT_HUFFMAN

typedef struct Huffman {
#ifdef X86OPT_PREBUILT_TABLE
  const
#endif // X86OPT_PREBUILT_TABLE
        short *m_hufDecTable;     // this should be public and, preferably, first field

  // It is critical that encTable be 32 bits and decTable be 16 bits for 
  // the given huffman routines to work
  Int m_alphabetSize;
#if !(defined(ARMOPT_HUFFMAN) || defined(X86OPT_HUFFMAN))
  Int m_maxCodeLength;
  Int m_numDecEntries, m_allocDecEntries;
  Int m_allocTables, m_allocAlphabet;
  struct tagHuffDecInfo   *m_decInfo;
  struct tagHuffEncInfo   *m_encInfo;
  struct tagTableInfo     *m_tableInfo;
  struct tagTableInitInfo *m_initInfo;
#endif	// ARMOPT_HUFFMAN || X86OPT_HUFFMAN
} HuffmanDef;

#endif // WMI_DECODE_H


//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "windowsmediaphoto.h"
#include "common.h"

#ifdef MEM_TRACE
#define TRACE_MALLOC    1
#define TRACE_NEW       0
#define TRACE_HEAP      0
#include "memtrace.h"
#endif

#ifdef X86OPT_PREBUILT_TABLE
#define HUFFMAN_DECODE_ROOT_BITS    (5) 
#define HUFFMAN_DECODE_ROOT_BITS_LOG    3
#define HUFFMAN_DECODE_TABLE_SIZE(iRootBits,iAlphabetSize) ((1 << (iRootBits)) + (2 * (iAlphabetSize)))
#define SIGN_BIT(TypeOrValue) (((UInt) 1) << (8 * sizeof (TypeOrValue) - 1))

typedef struct Huffman {
  const short *m_hufDecTable;     // this should be public and, preferably, first field

  // It is critical that encTable be 32 bits and decTable be 16 bits for 
  // the given huffman routines to work
  Int m_alphabetSize;
} HuffmanDef;

// Huffman lookup tables
static const short g4HuffLookupTable[40] = {
  19,19,19,19,27,27,27,27,10,10,10,10,10,10,10,10,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0 };

static const short g5HuffLookupTable[2][42] = {{
  28,28,36,36,19,19,19,19,10,10,10,10,10,10,10,10,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,0 },
  {
  11,11,11,11,19,19,19,19,27,27,27,27,35,35,35,35,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,0 }};

static const short g6HuffLookupTable[4][44] = {{
  13,29,44,44,19,19,19,19,34,34,34,34,34,34,34,34,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,0,0,0,0 },
  {
  12,12,28,28,43,43,43,43,2,2,2,2,2,2,2,2,
  18,18,18,18,18,18,18,18,34,34,34,34,34,34,34,34,
  0,0,0,0,0,0,0,0,0,0,0,0 },
  {
  4,4,12,12,43,43,43,43,18,18,18,18,18,18,18,18,
  26,26,26,26,26,26,26,26,34,34,34,34,34,34,34,34,
  0,0,0,0,0,0,0,0,0,0,0,0 },
  {
  5,13,36,36,43,43,43,43,18,18,18,18,18,18,18,18,
  25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
  0,0,0,0,0,0,0,0,0,0,0,0 }};

static const short g7HuffLookupTable[2][46] = {{
  45,53,36,36,27,27,27,27,2,2,2,2,2,2,2,2,
  10,10,10,10,10,10,10,10,18,18,18,18,18,18,18,18,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  {
  -32736,37,28,28,19,19,19,19,10,10,10,10,10,10,10,10,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  5,6,0,0,0,0,0,0,0,0,0,0,0,0 }};

static const short g8HuffLookupTable[2][48] = {{
  53,21,28,28,11,11,11,11,43,43,43,43,59,59,59,59,
  2,2,2,2,2,2,2,2,34,34,34,34,34,34,34,34,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  {
  52,52,20,20,3,3,3,3,11,11,11,11,27,27,27,27,
  35,35,35,35,43,43,43,43,58,58,58,58,58,58,58,58,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }};

static const short g9HuffLookupTable[2][50] = {{
  13,29,37,61,20,20,68,68,3,3,3,3,51,51,51,51,
  41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0 },
  {
  -32736,53,28,28,11,11,11,11,19,19,19,19,43,43,43,43,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  -32734,4,7,8,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0 }};

static const short g12HuffLookupTable[5][56] = {{
  -32736,5,76,76,37,53,69,85,43,43,43,43,91,91,91,91,
  57,57,57,57,57,57,57,57,57,57,57,57,57,57,57,57,
  -32734,1,2,3,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0 },
  {
  -32736,85,13,53,4,4,36,36,43,43,43,43,67,67,67,67,
  75,75,75,75,91,91,91,91,58,58,58,58,58,58,58,58,
  2,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0 },
  {
  -32736,37,92,92,11,11,11,11,43,43,43,43,59,59,59,59,
  67,67,67,67,75,75,75,75,2,2,2,2,2,2,2,2,
  -32734,-32732,2,3,6,10,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0 },
  {
  -32736,29,37,69,3,3,3,3,43,43,43,43,59,59,59,59,
  75,75,75,75,91,91,91,91,10,10,10,10,10,10,10,10,
  -32734,10,2,6,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0 },
  {
  -32736,93,28,28,60,60,76,76,3,3,3,3,43,43,43,43,
  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
  -32734,-32732,-32730,2,4,8,6,10,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0 }};

#endif  // X86OPT_PREBUILT_TABLE

/**********************************************************************
  generateHuffman : generates optimum huffman code lengths
  very non-optimally written, can take a lot of tweaking!!
  PLEASE DO NOTE DELETE CODE!!!
**********************************************************************/
#if 0
Void generateHuffman(Int *pWeights, Int iN, Int *pTable)
{
    Int  i, iter, smindex1, smindex2;
    Int  parent[256]; // max number of symbols
    pTable++; // increment to first element in table
    for (i=0; i<iN; i++) {
        parent[i] = i;
        pTable[i * 2] = pTable[i * 2 + 1] =0;
    }

    for (iter=smindex1=0; iter<iN-1; iter++) {
// locate first nonzero element
        smindex1=0;
        while (pWeights[smindex1]<0) smindex1++;

// locate second nonzero element
        smindex2=smindex1+1;
        while (pWeights[smindex2]<0) smindex2++;
        if (pWeights[smindex2]<pWeights[smindex1]) {
            i = smindex1;
            smindex1 = smindex2;
            smindex2 = i;
        }

// locate two smallest elements
        i = 1 + ((smindex1<smindex2) ? smindex2:smindex1);
        for (; i<iN; i++)  if (pWeights[i]>=0) {
            if (pWeights[i]<pWeights[smindex1]) {
                smindex2 = smindex1;
                smindex1 = i;
            }
            else if (pWeights[i]<pWeights[smindex2]) {
                smindex2 = i;
            }
        }

// merge two smallest elements
        pWeights[smindex1] += pWeights[smindex2];
        pWeights[smindex2] = -1;

// increment codelength of all children, and point to new parent
        for (i=0; i<iN; i++) {
            if (parent[i]==smindex1)
                pTable[i * 2 + 1]++;
            else if (parent[i]==smindex2) {
                pTable[i * 2] |= (1 << pTable[i * 2 + 1]);
                parent[i]=smindex1;
                pTable[i * 2 + 1]++;
            }
        }
    }
}
#endif // 0
/**********************************************************************
  Allocation and dellocation
**********************************************************************/
Void Clean (CAdaptiveHuffman *pAdHuff)
{
    if (pAdHuff == NULL)
        return;
    if (pAdHuff->m_pData)
        free (pAdHuff->m_pData);
    free (pAdHuff);
}

CAdaptiveHuffman *Allocate (Int iNSymbols, CODINGMODE cm)
{
    CAdaptiveHuffman *pAdHuff = (CAdaptiveHuffman *) malloc (sizeof (CAdaptiveHuffman));
    if (pAdHuff == NULL)
        return NULL;
    if (iNSymbols > 255 || iNSymbols <= 0)
        goto ErrorExit;

    memset (pAdHuff, 0, sizeof (CAdaptiveHuffman));
#pragma prefast(suppress: __WARNING_ALLOC_SIZE_OVERFLOW_WITH_ACCESS, "PREfast noise")
    pAdHuff->m_pData = (Int *) malloc ((iNSymbols * 2) * sizeof (Int)); // iNSymbol < 255, no overflow here
    if (pAdHuff->m_pData == NULL)
        goto ErrorExit;

    memset (pAdHuff->m_pData, 0, (iNSymbols * 2) * sizeof (Int));
    pAdHuff->m_pHistogram = pAdHuff->m_pData;
    pAdHuff->m_pHistogramAlt = pAdHuff->m_pHistogram + iNSymbols;
    pAdHuff->m_iNSymbols = iNSymbols;

    pAdHuff->m_pDelta = NULL;
    pAdHuff->m_iDiscriminant = pAdHuff->m_iUpperBound = pAdHuff->m_iLowerBound = 0;

    return pAdHuff;

ErrorExit:
    Clean (pAdHuff);
    return NULL;
}

/**********************************************************************
  Adapt Huffman table
**********************************************************************/
// Alphabet size = 4
static const Int g_Index4Table[] = {
    1,2,3,3
};
static const Int g4CodeTable[] = {
    4,
    1, 1,
    1, 2,
    0, 3,
    1, 3
};

// Alphabet size = 5
static const Int g_Index5Table[] = {
    1,2,3,4,4,
    1,3,3,3,3
};
static const Int g5CodeTable[] = {
    5,
    1, 1,
    1, 2,
    1, 3,
    0, 4,
    1, 4,

    5,
    1, 1,
    0, 3,
    1, 3,
    2, 3,
    3, 3,
};
Int g5DeltaTable[] = { 0,-1,0,1,1 };

// Alphabet size = 6
Int g_Index6Table[] = {
    1,5,3,5,2,4,
    2,4,2,4,2,3,
    4,4,2,2,2,3,
    5,5,2,1,4,3,
};
Int g6CodeTable[] = {
    6,
    1, 1,
    0, 5,
    1, 3,
    1, 5,
    1, 2,
    1, 4,

    6,
    1,  2,
    0,  4,
    2,  2,
    1,  4,
    3,  2,
    1,  3,

    6,
    0,  4,
    1,  4,
    1,  2,
    2,  2,
    3,  2,
    1,  3,

    6,
    0, 5,
    1, 5,
    1, 2,
    1, 1,
    1, 4,
    1, 3
};
//Int g6DeltaTable[] = { -1,0,0,1,0,0 };
Int g6DeltaTable[] = {
    -1, 1, 1, 1, 0, 1,
    -2, 0, 0, 2, 0, 0,
    -1,-1, 0, 1,-2, 0
};


// Alphabet size = 7
Int g_Index7Table[] = { 2,2,2,3,4,5,5,
                        1,2,3,4,5,6,6 };
Int g7CodeTable[] = {
    7,
    1, 2,
    2, 2,
    3, 2,
    1, 3,
    1, 4,
    0, 5,
    1, 5,

    7,
    1, 1,
    1, 2,
    1, 3,
    1, 4,
    1, 5,
    0, 6,
    1, 6
};
Int g7DeltaTable[] = { 1,0,-1,-1,-1,-1,-1 };

// Alphabet size = 8
Int g_Index8Table[] = { 2,3,5,4,2,3,5,3,
                        3,3,4,3,3,3,4,2};
Int g8CodeTable[] = {
    8,
    2, 2,
    1, 3,
    1, 5,
    1, 4,
    3, 2,
    2, 3,
    0, 5,
    3, 3,

    8,
    1, 3,
    2, 3,
    1, 4,
    3, 3,
    4, 3,
    5, 3,
    0, 4,
    3, 2
};
Int g8DeltaTable[] = { -1,0,1,1,-1,0,1,1 };

Int g_Index9Table[] = {
    3,5,4,5,5,1,3,5,4,
    1,3,3,4,6,3,5,7,7,
};
Int g9CodeTable[] = {
    9,
    2, 3,
    0, 5,
    2, 4,
    1, 5,
    2, 5,
    1, 1,
    3, 3,
    3, 5,
    3, 4,

    9,
    1, 1,
    1, 3,
    2, 3,
    1, 4,
    1, 6,
    3, 3,
    1, 5,
    0, 7,
    1, 7,
};
//Int g9DeltaTable[] = { 1,1,0,0,0,-1,-1,-1,-1 };
Int g9DeltaTable[] = { 2,2,1,1,-1,-2,-2,-2,-3 };


// Alphabet size = 12
Int g_Index12Table[] = {  // index12 is the most critical symbol
    5,6,7,7,5,3,5,1,5,4,5,3,
    4,5,6,6,4,3,5,2,3,3,5,3,
    2,3,7,7,5,3,7,3,3,3,7,4,
    3,2,7,5,5,3,7,3,5,3,6,3,
    3,1,7,4,7,3,8,4,7,4,8,5,
};
Int g12CodeTable[] = {
    12,  
    1, 5,
    1, 6,
    0, 7,
    1, 7,
    4, 5,
    2, 3,
    5, 5,
    1, 1,
    6, 5,
    1, 4,
    7, 5,
    3, 3,

    12,
    2, 4,
    2, 5,
    0, 6,
    1, 6,
    3, 4,
    2, 3,
    3, 5,
    3, 2,
    3, 3,
    4, 3,
    1, 5,
    5, 3,

    12,
    3, 2,
    1, 3,
    0, 7,
    1, 7,
    1, 5,
    2, 3,
    2, 7,
    3, 3,
    4, 3,
    5, 3,
    3, 7,
    1, 4,

    12,
    1, 3,
    3, 2,
    0, 7,
    1, 5,
    2, 5,
    2, 3,
    1, 7,
    3, 3,
    3, 5,
    4, 3,
    1, 6,
    5, 3,

    12,
    2, 3,
    1, 1,
    1, 7,
    1, 4,
    2, 7,
    3, 3,
    0, 8,
    2, 4,
    3, 7,
    3, 4,
    1, 8,
    1, 5
};
Int g12DeltaTable[] = {
    1, 1, 1, 1, 1, 0, 0,-1, 2, 1, 0, 0,
    2, 2,-1,-1,-1, 0,-2,-1, 0, 0,-2,-1,
   -1, 1, 0, 2, 0, 0, 0, 0,-2, 0, 1, 1,
    0, 1, 0, 1,-2, 0,-1,-1,-2,-1,-2,-2
};

/**********************************************************************
  Adapt fixed length codes based on discriminant
**********************************************************************/
static const Int THRESHOLD = 8;
static const Int MEMORY = 8;
 
Void AdaptDiscriminant (CAdaptiveHuffman *pAdHuff)
{
#ifdef X86OPT_PREBUILT_TABLE
    const short *phufDecTable;
#endif

    Int iSym = pAdHuff->m_iNSymbols, t, dL, dH;
    const Int *pCodes, *pDelta = NULL;
    Bool bChange = FALSE;
    static const Int gMaxTables[] = { 0,0,0,0, 1,2, 4,2, 2,2, 0,0,5 };
    static const Int gSecondDisc[]= { 0,0,0,0, 0,0, 1,0, 0,0, 0,0,1 };

    if (!pAdHuff->m_bInitialize) {
        pAdHuff->m_bInitialize = 1;
        pAdHuff->m_iDiscriminant = pAdHuff->m_iDiscriminant1 = 0;
        pAdHuff->m_iTableIndex = gSecondDisc[iSym];//(gMaxTables[iSym] - 1) >> 1;
    }

    dL = dH = pAdHuff->m_iDiscriminant;
    if (gSecondDisc[iSym]) {
        dH = pAdHuff->m_iDiscriminant1;
    }

    if (dL < pAdHuff->m_iLowerBound) {
        pAdHuff->m_iTableIndex--;
        bChange = TRUE;
    }
    else if (dH > pAdHuff->m_iUpperBound) {
        pAdHuff->m_iTableIndex++;
        bChange = TRUE;
    }
    if (bChange) {
    /** if initialization is fixed, we can exit on !bChange **/
        pAdHuff->m_iDiscriminant = 0;
        pAdHuff->m_iDiscriminant1 = 0;
    }
    {
        if (pAdHuff->m_iDiscriminant < -THRESHOLD * MEMORY)
            pAdHuff->m_iDiscriminant = -THRESHOLD * MEMORY;
        else if (pAdHuff->m_iDiscriminant > THRESHOLD * MEMORY)
            pAdHuff->m_iDiscriminant = THRESHOLD * MEMORY;

        if (pAdHuff->m_iDiscriminant1 < -THRESHOLD * MEMORY)
            pAdHuff->m_iDiscriminant1 = -THRESHOLD * MEMORY;
        else if (pAdHuff->m_iDiscriminant1 > THRESHOLD * MEMORY)
            pAdHuff->m_iDiscriminant1 = THRESHOLD * MEMORY;
    }

    t = pAdHuff->m_iTableIndex;
    assert (t >= 0);
    assert (t < gMaxTables[iSym]);

    //pAdHuff->m_iDiscriminant >>= 1;
    pAdHuff->m_iLowerBound = (t == 0) ? (-1 << 31) : -THRESHOLD;
    pAdHuff->m_iUpperBound = (t == gMaxTables[iSym] - 1) ? (1 << 30) : THRESHOLD;

    switch (iSym) {
        case 4:
            pCodes = g4CodeTable;
            #ifdef X86OPT_PREBUILT_TABLE
            phufDecTable = (short *) g4HuffLookupTable;
            #endif
            break;
        case 5:
            pCodes = g5CodeTable + (iSym * 2 + 1) * t;
            pDelta = g5DeltaTable;
            #ifdef X86OPT_PREBUILT_TABLE
            phufDecTable = g5HuffLookupTable[t];
            #endif
            break;
        case 6:
            pCodes = g6CodeTable + (iSym * 2 + 1) * t;
            pAdHuff->m_pDelta1 = g6DeltaTable + iSym * (t - (t + 1 == gMaxTables[iSym]));
            pDelta = g6DeltaTable + (t - 1 + (t == 0)) * iSym;
            #ifdef X86OPT_PREBUILT_TABLE
            phufDecTable = g6HuffLookupTable[t];
            #endif
            break;
        case 7:
            pCodes = g7CodeTable + (iSym * 2 + 1) * t;
            pDelta = g7DeltaTable;
            #ifdef X86OPT_PREBUILT_TABLE
            phufDecTable = g7HuffLookupTable[t];
            #endif
            break;
        case 8:
            //printf ("%d ", t);
            pCodes = g8CodeTable;// + (iSym * 2 + 1) * t;
            //pDelta = g8DeltaTable;
            #ifdef X86OPT_PREBUILT_TABLE
            phufDecTable = g8HuffLookupTable[0];
            #endif
            break;
        case 9:
            pCodes = g9CodeTable + (iSym * 2 + 1) * t;
            pDelta = g9DeltaTable;
            #ifdef X86OPT_PREBUILT_TABLE
            phufDecTable = g9HuffLookupTable[t];
            #endif
            break;
        case 12:
            pCodes = g12CodeTable + (iSym * 2 + 1) * t;
            pAdHuff->m_pDelta1 = g12DeltaTable + iSym * (t - (t + 1 == gMaxTables[iSym]));
            pDelta = g12DeltaTable + (t - 1 + (t == 0)) * iSym;
            #ifdef X86OPT_PREBUILT_TABLE
            phufDecTable = g12HuffLookupTable[t];
            #endif
            break;
        default:
            assert (0); // undefined fixed length table
            return;
    }

    pAdHuff->m_pTable = pCodes;
    pAdHuff->m_pDelta = pDelta;
    #ifdef X86OPT_PREBUILT_TABLE
    if (NULL != pAdHuff->m_pHuffman) {
        pAdHuff->m_pHuffman->m_hufDecTable = phufDecTable;
    }
    #endif
}

#ifdef X86OPT_PREBUILT_TABLE
/**********************************************************************
  Construct Huffman lookup tables
**********************************************************************/
Int         // returns 0 if initialization went OK, source line number otherwise (for investigations)
InitHuffLookup16 (
  I16 *pDecodeTable,    // decode table
  const Int *pCodeInfo,   // code info (alphabet size, codeword1, bitlenth1, codeword2, bitlength2, ...
  Int *pSymbolInfo, // if 0 then natural enumeration (0, 1, ...), otherwise table-driven
  Int  iRootBits    // lookup bits for root table
)
{
  Int i, n, iMaxIndex;
  Int iAlphabetSize;

  if ((NULL == pDecodeTable) || (NULL == pCodeInfo)) {
      return (__LINE__);
  }

  iAlphabetSize = *pCodeInfo++;
  
  iMaxIndex = HUFFMAN_DECODE_TABLE_SIZE (iRootBits, iAlphabetSize);
  memset (pDecodeTable, 0, iMaxIndex * sizeof (pDecodeTable[0]));

  if ((UInt) iRootBits >= (1 << HUFFMAN_DECODE_ROOT_BITS_LOG))
  {
    return (__LINE__);
  }
  
  // first available intermediate node index
  n = (1 << iRootBits) - SIGN_BIT (pDecodeTable[0]);
  iMaxIndex -= SIGN_BIT (pDecodeTable[0]);

  for (i = 0; i < iAlphabetSize; ++i)
  {
    UInt m = pCodeInfo[2*i];
    Int  b = pCodeInfo[2*i+1];

    if (b >= 32 || (m >> b) != 0)
    {
      // oops, stated length is smaller than actual codeword length
      return (__LINE__);
    }

    if (b <= iRootBits)
    {
      // short codeword goes right into root table
      int k = m << (iRootBits - b);
      int kLast = (m + 1) << (iRootBits - b);

      if ((UInt) (pSymbolInfo == 0 ? i : pSymbolInfo[i]) >= (SIGN_BIT (pDecodeTable[0]) >> HUFFMAN_DECODE_ROOT_BITS_LOG))
      {
        // oops, symbol is out of range of decoding table
        return (__LINE__);
      }
      b += (pSymbolInfo == 0 ? i : pSymbolInfo[i]) << HUFFMAN_DECODE_ROOT_BITS_LOG;

      do
      {
        if (pDecodeTable[k] != 0)
        {
          // oops, there is already path to this location -- not a proper prefix code
          return (__LINE__);
        }
        pDecodeTable[k] = (I16) b;
      }
      while (++k != kLast);
    }
    else
    {
      // long codeword -- generate bit-by-bit decoding path
      Int k;

      if (iRootBits < 0)
      {
        return (__LINE__);
      }
#pragma prefast(suppress: __WARNING_LOOP_INDEX_UNDERFLOW, "PREfast false alarm: 32 > b > iRootBits >= 0, no overflow/underflow!")
      b -= iRootBits;
      if (b > 16)
      {
        // cannot decode without flushing (see HuffmanDecodeShort () for more details)
        return (__LINE__);
      }

      k = m >> b;   // these bit will be decoded by root table
      do
      {
        if (pDecodeTable[k] > 0)
        {
          // oops, it's not an internal node as it should be
          return (__LINE__);
        }

        if (pDecodeTable[k] == 0)
        {
          // slot is empty -- create new internal node
          pDecodeTable[k] = (I16) n;
          n += 2;
          if (n > iMaxIndex)
          {
            // oops, too many holes in the code; [almost] full prefix code is needed
            return (__LINE__);
          }
        }
        k = pDecodeTable[k] + SIGN_BIT (pDecodeTable[0]);

        // find location in this internal node (will we go left or right)
        --b;
        if ((m >> b) & 1)
          ++k;
      }
      while (b != 0);

      // reached the leaf
      if (pDecodeTable[k] != 0)
      {
        // oops, the slot should be reserved for current leaf
        return (__LINE__);
      }

      if ((UInt) (pSymbolInfo == 0 ? i : pSymbolInfo[i]) >= SIGN_BIT (pDecodeTable[0]))
      {
        // oops, symbol is out of range of short table
        return (__LINE__);
      }
      pDecodeTable[k] = (I16) (pSymbolInfo == 0 ? i : pSymbolInfo[i]);
    }
  }

  return (0);
}

Int conHuffTable(short **huffLookupTable, const Int *huffCodeTable)
{
    Int line;
    Int tblEntries;

    if(huffCodeTable[0] > 4096 || huffCodeTable[0] <= 0)
        return ICERR_ERROR;
    tblEntries = (1 << HUFFMAN_DECODE_ROOT_BITS) + (huffCodeTable[0] << 1);
    *huffLookupTable = (short *)malloc(sizeof(huffLookupTable) * tblEntries);
    if (*huffLookupTable != NULL) {
      line = InitHuffLookup16 (*huffLookupTable, huffCodeTable, 0, HUFFMAN_DECODE_ROOT_BITS);
      if (line != 0) {
          goto ErrorExit;
      }
      return ICERR_OK;
    }

ErrorExit:
    if (NULL != *huffLookupTable) {
        free(*huffLookupTable);
        *huffLookupTable = NULL;
    }
    return ICERR_ERROR;
}

/**
Int conHuffLookupTables()
{
    g5HuffLookupTable[0] = (short *) g5HuffLookupTable0;
    g5HuffLookupTable[1] = (short *) g5HuffLookupTable1;

    g6HuffLookupTable[0] = (short *) g6HuffLookupTable0;
    g6HuffLookupTable[1] = (short *) g6HuffLookupTable1;
    g6HuffLookupTable[2] = (short *) g6HuffLookupTable2;
    g6HuffLookupTable[3] = (short *) g6HuffLookupTable3;

    g7HuffLookupTable[0] = (short *) g7HuffLookupTable0;
    g7HuffLookupTable[1] = (short *) g7HuffLookupTable1;

    g8HuffLookupTable[0] = (short *) g8HuffLookupTable0;
    g8HuffLookupTable[1] = (short *) g8HuffLookupTable1;

    g9HuffLookupTable[0] = (short *) g9HuffLookupTable0;
    g9HuffLookupTable[1] = (short *) g9HuffLookupTable1;

    g12HuffLookupTable[0] = (short *) g12HuffLookupTable0;
    g12HuffLookupTable[1] = (short *) g12HuffLookupTable1;
    g12HuffLookupTable[2] = (short *) g12HuffLookupTable2;
    g12HuffLookupTable[3] = (short *) g12HuffLookupTable3;
    g12HuffLookupTable[4] = (short *) g12HuffLookupTable4;

    return ICERR_OK;
}
**/
#endif // X86OPT_PREBUILT_TABLE

/**********************************************************************
  Adapt fixed length codes
**********************************************************************/
#if 0
void AdaptFixed (CAdaptiveHuffman *pAdHuff)
{
    Int  i, iSum, aTempHistogram[256]; /** max number of symbols **/
    FILE *fp = NULL;
    Int *pTmp;
    Int iSym = pAdHuff->m_iNSymbols;
    //Int k, l, d;
    //Int splus, sminus;
    //Int *pPlus, *pMinus, iNTables;
    Int *pDiff = NULL, *pCodes = NULL;
    //if (iSym == 8) {
    //    AdaptNonfixed (pAdHuff, FALSE, TRUE);
    //    return;
    //}

    for (i = iSum = 0; i < iSym; i++) {
        iSum += (aTempHistogram[i] = pAdHuff->m_pHistogram[i] * 2 + pAdHuff->m_pHistogramAlt[i]);
    }
    /** initialize table **/
    if (iSum == 0) {
        if (iSym == 6 || iSym == 12) {
            pAdHuff->m_iTableIndex = 2;
        }
    }
    /** pick one from fixed tables **/
    switch (iSym) {
        case 4:
            pCodes = g4CodeTable;
            break;
        case 5:
            //pCodes = g5CodeTable;
            //break;
        case 6:
            /** trivial determinations **/
            /**
            if (aTempHistogram[0] * 8 > iSum * 3 && aTempHistogram[0] > aTempHistogram[3]) {
                pAdHuff->m_iTableIndex = 0;
            }
            else if (aTempHistogram[3] * 8 > iSum * 3) {
                pAdHuff->m_iTableIndex = 3;
            }
            else {
                // ascend or descend table
                iNTables = 4;
                pPlus = g6DeltaTable;
                splus = sminus = 0;
                pPlus += pAdHuff->m_iTableIndex * 6;
                pMinus = (pAdHuff->m_iTableIndex == 0) ? NULL : pPlus - 6;

                if (pAdHuff->m_iTableIndex == iNTables - 1) {
                    pPlus = NULL;
                }
                for (k = 0; k < 6; k++) {
                    if (pPlus != NULL) {
                        splus -= aTempHistogram[k] * pPlus[k];
                    }
                    if (pMinus != NULL) {
                        sminus += aTempHistogram[k] * pMinus[k];
                    }
                }
                if (splus < 0 && sminus < 0) {
                    if (splus < sminus)
                        pAdHuff->m_iTableIndex++;
                    else
                        pAdHuff->m_iTableIndex--;
                }
                else if (splus < 0) {
                    pAdHuff->m_iTableIndex++;
                }
                else if (sminus < 0) {
                    pAdHuff->m_iTableIndex--;
                }
                //assert (!(splus < 0 && sminus < 0));
            }
            pAdHuff->m_pDelta = pAdHuff->m_pDelta1 = g6DeltaTable;
            pCodes = g6CodeTable + pAdHuff->m_iTableIndex * 13;
            break;
            **/
        case 7:
            /**
            d = aTempHistogram[0];
            for (k = 2; k < 7; k++) {
                d -= aTempHistogram[k];
            }
            pAdHuff->m_iTableIndex = l = (d > 0);
            pCodes = g7CodeTable + l * 15;
            pAdHuff->m_pDelta = g7DeltaTable;
            break;
            **/
        case 8:
        case 9:
            /**
            {
                Int sum0 = 0, sum1 = 0, *pLen0 = g_Index9Table, *pLen1 = pLen0 + 9;
                for (k = 0; k < 9; k++) {
                    sum0 += aTempHistogram[k] * pLen0[k];
                    sum1 += aTempHistogram[k] * pLen1[k];
                }
                if (sum0 <= sum1)
                    pCodes = g9CodeTable;
                else
                    pCodes = g9CodeTable + 19;
            }
            pAdHuff->m_pDelta = g9DeltaTable;
            break;
            **/
        case 12:
            /** trivial determinations **/
            /**
            if (aTempHistogram[7] * 8 > iSum * 3 && aTempHistogram[7] > aTempHistogram[1]) {
                pAdHuff->m_iTableIndex = 0;
            }
            else if (aTempHistogram[1] * 8 > iSum * 3) {
                pAdHuff->m_iTableIndex = 4;
            }
            else {
                // ascend or descend table
                iNTables = 5;
                pPlus = g12DeltaTable;
                splus = sminus = 0;
                pPlus += pAdHuff->m_iTableIndex * 12;
                pMinus = (pAdHuff->m_iTableIndex == 0) ? NULL : pPlus - 12;

                if (pAdHuff->m_iTableIndex == iNTables - 1) {
                    pPlus = NULL;
                }
                for (k = 0; k < 12; k++) {
                    if (pPlus != NULL) {
                        splus -= aTempHistogram[k] * pPlus[k];
                    }
                    if (pMinus != NULL) {
                        sminus += aTempHistogram[k] * pMinus[k];
                    }
                }
                if (splus < 0 && sminus < 0) {
                    if (splus < sminus)
                        pAdHuff->m_iTableIndex++;
                    else
                        pAdHuff->m_iTableIndex--;
                }
                else if (splus < 0) {
                    pAdHuff->m_iTableIndex++;
                }
                else if (sminus < 0) {
                    pAdHuff->m_iTableIndex--;
                }
                //assert (!(splus < 0 && sminus < 0));
            }
            pCodes = g12CodeTable + pAdHuff->m_iTableIndex * 25;
            pAdHuff->m_pDelta = pAdHuff->m_pDelta1 = g12DeltaTable;
            break;
            **/
            AdaptDiscriminant(pAdHuff);
            return;
        default:
            assert (0); // undefined fixed length table
    }
    /** find high probability symbol **
    for (i = 0; i < iSym; i++) {
        if (aTempHistogram[i] * 2 > iSum) {
            Int k = (int)(0.5 + log(0.5)/log((double) aTempHistogram[i]/(double) iSum));
            if (k > 1)
                printf ("[%d %d %d %d] ", iSym, pAdHuff->m_pLabel[0] - '0', k, i);
            break;
        }
    }
    **/

    /** set code table **/
    pAdHuff->m_pTable = pCodes;

    /** the following has to be cleaned up!! **/
    /** renormalize histogram **/
    while (iSum > 512) {
        for (i = iSum = 0; i < pAdHuff->m_iNSymbols; i++)
            iSum += (pAdHuff->m_pHistogram[i] = (pAdHuff->m_pHistogram[i]) >> 1);
    }

    /** decay and swap histograms **/
    if (TRUE) {
        pTmp = pAdHuff->m_pHistogram;
        pAdHuff->m_pHistogram = pAdHuff->m_pHistogramAlt;
        pAdHuff->m_pHistogramAlt = pTmp;
        if (iSum <= 4 * pAdHuff->m_iNSymbols) {
            for (i = 0; i < pAdHuff->m_iNSymbols; i++)
                pAdHuff->m_pHistogramAlt[i] += pAdHuff->m_pHistogram[i];
        }
    }
}
#endif // 0

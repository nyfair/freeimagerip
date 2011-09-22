//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
/******************************************************************************

Module Name:
    huffman.c

Abstract:

Author:

Revision History:
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "strcodec.h"
#include "decode.h"

#ifdef MEM_TRACE
#define TRACE_MALLOC    1
#define TRACE_NEW       0
#define TRACE_HEAP      0
#include "memtrace.h"
#endif

#define SIGN_BIT(TypeOrValue) (((UInt) 1) << (8 * sizeof (TypeOrValue) - 1))

/***********************************************************************************************************
  Definitions / declarations copied from common.h
***********************************************************************************************************/
#define BITSTREAM_READ 1

#if !defined(UNDER_CE) && !defined(MIMIC_CE_ON_DESKTOP)
    // define stages assuming large memory and cache
#   define BITS_STAGE1 10
#   define BITS_STAGE2 11
#else
    // define stages assuming small memory and cache
#   define BITS_STAGE1 6
#   define BITS_STAGE2 15
#endif

#define MAX_STAGES 3
#     define ILLEGAL_SYMBOL 4095        /* 2^HUFFDEC_SYMBOL_BITS - 1 */

/***********************************************************************************************************
  Body
***********************************************************************************************************/
#ifndef X86OPT_HUFFMAN
UInt GetMask[33] = {
    0x00000000,
    0x00000001,
    0x00000003,
    0x00000007,
    0x0000000f,
    0x0000001f,
    0x0000003f,
    0x0000007f,
    0x000000ff,
    0x000001ff,
    0x000003ff,
    0x000007ff,
    0x00000fff,
    0x00001fff,
    0x00003fff,
    0x00007fff,
    0x0000ffff,
    0x0001ffff,
    0x0003ffff,
    0x0007ffff,
    0x000fffff,
    0x001fffff,
    0x003fffff,
    0x007fffff,
    0x00ffffff,
    0x01ffffff,
    0x03ffffff,
    0x07ffffff,
    0x0fffffff,
    0x1fffffff,
    0x3fffffff,
    0x7fffffff,
    0xffffffff
};

Int initEncTable(HuffmanDef *pHuffman, const Int *huffArray);
Int initDecTable(HuffmanDef *pHuffman, Int *maxBits);
Int findTables(HuffmanDef *pHuffman, Int *totalTableNum, Int *maxBits);
Int allocTables(HuffmanDef *pHuffman, Int numTables);
Void fillEntry(HuffmanDef *pHuffman, Int cwd, Int length, Int tableNum,
                        Int index, HuffDecInfo *currDecTable);
#endif  // X86OPT_HUFFMAN

#ifdef DEBUG_HUFFMAN
Int         // returns 0 if code is OK, source line number otherwise (for investigations)
Verify (
        HuffmanDef *pHuffman, 
  Int *pCodeInfo,   // code info (alphabet size, codeword1, bitlenth1, codeword2, bitlength2, ...
  Int *pSymbolInfo, // if 0 then natural enumeration (0, 1, ...), otherwise use pSymbolInfo[i] as i-th symbol
  Int  iRootBits    // lookup bits for root table
);
#endif // DEBUG_HUFFMAN

static Int Decode16(HuffmanDef* pHuffman, I16* pDecodeTable, BitIOInfo* pIO, Int iRootBits);

Int         // returns 0 if initialization went OK, source line number otherwise (for investigations)
Init16 (
  HuffmanDef *pHuffman, 
  I16 *pDecodeTable,    // decode table
  const Int *pCodeInfo,   // code info (alphabet size, codeword1, bitlenth1, codeword2, bitlength2, ...
  Int *pSymbolInfo, // if 0 then natural enumeration (0, 1, ...), otherwise table-driven
  Int  iRootBits    // lookup bits for root table
);

#ifndef X86OPT_HUFFMAN
#ifndef CHECK_ALLOC
#define CHECK_ALLOC(ptr) if ((ptr)==NULL) { goto lerror; }
#endif
#define MAX_DEC_TABLES 1000

typedef struct {
  UInt code;
  UInt length;
  UInt table;
} InitEncInfo;
#endif // X86OPT_HUFFMAN

#ifdef DEBUG_HUFFMAN
FILE *fpHuff = NULL;
#endif

HuffmanDef *allocHuff()
{
  HuffmanDef *pHuffman = (HuffmanDef *) malloc (sizeof(HuffmanDef));
  if (pHuffman) {
      memset (pHuffman, 0, sizeof(HuffmanDef));
  }
  return pHuffman;
}

Void CleanHuff (HuffmanDef *pHuffman)
{
  if (NULL != pHuffman)
  {
#ifndef X86OPT_HUFFMAN
    if (pHuffman->m_tableInfo) {
      free (pHuffman->m_tableInfo);   // security
      pHuffman->m_tableInfo = NULL;
    }
//  delete [] m_initInfo; // deleted elsewhere
    if (pHuffman->m_encInfo)
    {
      free (pHuffman->m_encInfo);
      pHuffman->m_encInfo = NULL;
    }    
    if (pHuffman->m_decInfo)
    {
      free (pHuffman->m_decInfo);
      pHuffman->m_decInfo = NULL;
    }           
#endif  // X86OPT_HUFFMAN
#ifndef X86OPT_PREBUILT_TABLE
    if(pHuffman->m_hufDecTable) {
      free (pHuffman->m_hufDecTable);
      pHuffman->m_hufDecTable = NULL;
    }
#endif  // X86OPT_PREBUILT_TABLE
    free(pHuffman);
  }
}


#ifndef X86OPT_PREBUILT_TABLE
Int initHuff(HuffmanDef *pHuffman, Int mode, const Int *huffArray, Int *maxBits)
{
    Int line;

    if ((NULL == pHuffman) || (NULL == huffArray)) {
        printf("Invalid pointers\n");
        return ICERR_ERROR;
    }

//  assert(sizeof(Int) == sizeof(int32)); // GetMask is only 32 bits wide
#ifndef X86OPT_HUFFMAN
    pHuffman->m_maxCodeLength = 0;
    if (initEncTable(pHuffman, huffArray) != ICERR_OK) {
        return ICERR_ERROR;
    }

    // Generate decoder table if necessary
    if (mode == BITSTREAM_READ) {
        if (initDecTable(pHuffman, maxBits) != ICERR_OK) {
            return ICERR_ERROR;
        }
#endif  // X86OPT_HUFFMAN

#ifdef DEBUG_HUFFMAN
    fpHuff = fopen("c:\\huffVerify.txt","w");
    if(fpHuff == NULL) {
        return ICERR_ERROR;
    }

    line = Verify (pHuffman, huffArray, 0, HUFFMAN_DECODE_ROOT_BITS);
    if (line != 0) {
        return ICERR_ERROR;
    }

    if (line != 0)
      fprintf (fpHuff,"huffman verification error at line %d\n", line);
#endif // DEBUG_HUFFMAN

    if (pHuffman->m_hufDecTable) { // support re-initialization 
      free (pHuffman->m_hufDecTable);
      pHuffman->m_hufDecTable = NULL;
    }
    // ANSI, ADI , Desktop
    if(huffArray[0] > 12 || huffArray[0] < 0)
        goto lerror;
    pHuffman->m_hufDecTable = (short *) malloc (sizeof (short) * ((1 << HUFFMAN_DECODE_ROOT_BITS) + (huffArray[0] << 1)));
    if (pHuffman->m_hufDecTable != 0) {
      line = Init16 (pHuffman, pHuffman->m_hufDecTable, huffArray, 0, HUFFMAN_DECODE_ROOT_BITS);
      if (line != 0) {
          goto lerror;
      }
#ifdef DEBUG_HUFFMAN
      if (line != 0)
      {
        fprintf (fpHuff,"huffman initialization error at line %d\n", line);
      }
#endif
    }
#ifndef X86OPT_HUFFMAN
  }
#endif  // X86OPT_HUFFMAN
#ifdef DEBUG_HUFFMAN
   if(fpHuff) {
       fclose(fpHuff);
       fpHuff = NULL;
   }
#endif
  
  return ICERR_OK;

lerror:
  //lprintf(0, "%s", vr.explanation());
  if (NULL != pHuffman->m_hufDecTable) {
      free(pHuffman->m_hufDecTable);
      pHuffman->m_hufDecTable = NULL;
  }
  return ICERR_ERROR;
}
#endif // X86OPT_PREBUILT_TABLE

#ifndef X86OPT_HUFFMAN
Int initEncTable(HuffmanDef *pHuffman, const Int *huffArray)
{
  Int j;

  if ((NULL == pHuffman) || (NULL == huffArray)) {
      printf("Invalid pointers\n");
      return ICERR_ERROR;
  }

  if ((pHuffman->m_alphabetSize=*huffArray++) > pHuffman->m_allocAlphabet) {
      if (pHuffman->m_encInfo)  {
          free (pHuffman->m_encInfo);
      }
      pHuffman->m_allocAlphabet = pHuffman->m_alphabetSize;
      pHuffman->m_encInfo = (HuffEncInfo *) malloc (sizeof (HuffEncInfo) * pHuffman->m_allocAlphabet);
      CHECK_ALLOC(pHuffman->m_encInfo);
  }

  pHuffman->m_maxCodeLength = 0;

  for(j=0; j<pHuffman->m_alphabetSize; j++) {
    if (*huffArray != -1) {
      pHuffman->m_encInfo[j].code = *huffArray++;
      pHuffman->m_encInfo[j].length = *huffArray++;
      if (pHuffman->m_encInfo[j].length > (UInt)pHuffman->m_maxCodeLength) {
        pHuffman->m_maxCodeLength = pHuffman->m_encInfo[j].length;
      }
    } else {
      pHuffman->m_encInfo[j].code = pHuffman->m_encInfo[j].length = 0;
      huffArray+=2;
    }
  }

  assert(pHuffman->m_maxCodeLength < 32); // else putBits would fail
  return ICERR_OK;

lerror:
  printf("Insufficient memory to init tables.\n");
  return ICERR_ERROR;
}

Int initDecTable(HuffmanDef *pHuffman, Int *maxBits)
{
  Int i, numTables;

  if (NULL == pHuffman) {
      printf("Invalid pointer\n");
      return ICERR_ERROR;
  }

  pHuffman->m_initInfo = (TableInitInfo *) malloc (sizeof  (TableInitInfo) * MAX_DEC_TABLES);
  if (NULL == pHuffman->m_initInfo) {
      printf("Insufficient memory to init tables.\n");
      goto lerror;
  }

  // Find the # of decoder tables we will need
  if (findTables(pHuffman, &numTables, maxBits) != ICERR_OK) {
      goto lerror;
  }

  // Allocate tables
  if (allocTables(pHuffman, numTables) != ICERR_OK) {
      goto lerror;
  }

  // Now we are done with table info and can delete it
  if (pHuffman->m_initInfo) {
      free (pHuffman->m_initInfo);
      pHuffman->m_initInfo=NULL;
  }

  for (i=0; i<pHuffman->m_alphabetSize; i++) {
    fillEntry(pHuffman, pHuffman->m_encInfo[i].code, pHuffman->m_encInfo[i].length, 0, i, pHuffman->m_decInfo);
  }

  return ICERR_OK;

lerror:
  if (pHuffman->m_initInfo) {
      free (pHuffman->m_initInfo);
      pHuffman->m_initInfo = NULL;
  }
  return ICERR_ERROR;
}


Int findTables(HuffmanDef *pHuffman, Int *totalTableNum, Int *maxBits)
{
  Int myMaxBits[MAX_STAGES];
  Bool found;
  Int i, j, stage, tableNum, start, end, prefix, excessBits, nextTable;
  InitEncInfo *initEncInfo = NULL;
  
  if ((NULL == pHuffman) || (NULL == totalTableNum)) {
      printf("Invalid pointers\n");
      return ICERR_ERROR;
  }

  // Set # of bits for each stage
  if (maxBits == NULL) {
    myMaxBits[0] = BITS_STAGE1;
    myMaxBits[1] = BITS_STAGE2;
    myMaxBits[2] = pHuffman->m_maxCodeLength - (myMaxBits[0] + myMaxBits[1]);
  } else {
      for (i=0; i<MAX_STAGES; i++) {
          myMaxBits[i] = maxBits[i];
      }
  }

  initEncInfo = (InitEncInfo *) malloc (sizeof (InitEncInfo) * pHuffman->m_alphabetSize);
  if (initEncInfo == NULL) {
      printf("Insufficient memory.\n");
      return ICERR_ERROR;
      }
  assert(MAX_DEC_TABLES <= (1<<16));

  for (i=0; i<pHuffman->m_alphabetSize; i++) {
    initEncInfo[i].code   = pHuffman->m_encInfo[i].code;
    initEncInfo[i].length = pHuffman->m_encInfo[i].length;
    initEncInfo[i].table  = 0;
  }
  pHuffman->m_initInfo[0].maxBits = pHuffman->m_maxCodeLength;
  for (i=1; i<MAX_DEC_TABLES; i++) {
    pHuffman->m_initInfo[i].maxBits = 0;
  }

  *totalTableNum = 1;
  end = 0;
  for (stage=0; stage<MAX_STAGES; stage++) {
    start = end;
    end   = *totalTableNum;
//    lprintf(2, "    At stage %d, we have %d tables", stage, end-start);
    for (tableNum=start; tableNum<end; tableNum++) {
      pHuffman->m_initInfo[tableNum].start = *totalTableNum;
      pHuffman->m_initInfo[tableNum].end   = pHuffman->m_initInfo[tableNum].start;
      if (pHuffman->m_initInfo[tableNum].maxBits <= myMaxBits[stage]) {
        pHuffman->m_initInfo[tableNum].bits = pHuffman->m_initInfo[tableNum].maxBits;
        continue;
      } else {
        pHuffman->m_initInfo[tableNum].bits = myMaxBits[stage];
      }
      for (i=0; i<pHuffman->m_alphabetSize; i++) {
        if (initEncInfo[i].table == (UInt)tableNum) {
          if (initEncInfo[i].length > (UInt)pHuffman->m_initInfo[tableNum].bits) {
            excessBits = initEncInfo[i].length - pHuffman->m_initInfo[tableNum].bits;
            prefix = (initEncInfo[i].code >> excessBits);
            initEncInfo[i].length = excessBits;
#ifdef OPT_HUFFMAN_GET
            initEncInfo[i].code &= (((U32)0xffffffff)>>(32-excessBits));
#else
            initEncInfo[i].code &= GetMask[excessBits];
#endif
            found = FALSE;
            for (j=pHuffman->m_initInfo[tableNum].start; j<pHuffman->m_initInfo[tableNum].end; j++) {
              if (pHuffman->m_initInfo[j].prefix == prefix) {
                found = TRUE;
                if (excessBits > pHuffman->m_initInfo[j].maxBits) {
                  pHuffman->m_initInfo[j].maxBits = excessBits;
                }
                nextTable = j;
                break;
              }
            }
            if (!found) {
#ifndef _CASIO_VIDEO_
              assert(*totalTableNum < MAX_DEC_TABLES);
#endif
              pHuffman->m_initInfo[tableNum].end++;
              pHuffman->m_initInfo[*totalTableNum].prefix = prefix;
              pHuffman->m_initInfo[*totalTableNum].maxBits = excessBits;
              nextTable = *totalTableNum;
              (*totalTableNum)++;
            }
            initEncInfo[i].table = nextTable;
          }
        }
      }
    }
  }

#ifndef HITACHI
  if (initEncInfo) free  (initEncInfo);
#else 
  if(initEncInfo != NULL) {
    free(initEncInfo);
    initEncInfo = NULL;
  }

#endif  
  return ICERR_OK;

// lerror:
#ifndef HITACHI
  if (initEncInfo) free (initEncInfo);
#else 
  if(initEncInfo != NULL) {
    free(initEncInfo);
    initEncInfo = NULL;
  }

#endif  
  return ICERR_ERROR;
}

Int allocTables(HuffmanDef *pHuffman, Int numTables)
{
    Int i, j;
    Int iMemStatus = 0;

    if (NULL == pHuffman) {
        printf("Invalid pointer\n");
        return ICERR_ERROR;
    }

  // Allocate memory and set offsets
//  lprintf(2, "    Allocating %d bytes for next stage info",
//          numTables*sizeof(TableInfo));
    if (numTables > pHuffman->m_allocTables) {   // support reinitialisation
        if (pHuffman->m_tableInfo) {
            free (pHuffman->m_tableInfo);
        }
        pHuffman->m_allocTables = numTables;
        pHuffman->m_tableInfo = (TableInfo *) malloc (sizeof (TableInfo) * pHuffman->m_allocTables);
        if (NULL == pHuffman->m_tableInfo) {
            iMemStatus = -1;
            goto lerror;
        }
#ifdef IPAQ_HACK
        ipaq_hack((U8 * &)m_tableInfo , sizeof(TableInfo)*m_allocTables);
#endif
    }

    pHuffman->m_numDecEntries = 0;
    for (i=0; i<numTables; i++) pHuffman->m_numDecEntries += 1<<pHuffman->m_initInfo[i].bits;
//  lprintf(2, "    Allocating %d bytes for huffman decoder table",
//          m_numDecEntries*sizeof(HuffDecInfo));
    if (pHuffman->m_numDecEntries > pHuffman->m_allocDecEntries) {  // support reinitialisation
        if (pHuffman->m_decInfo) {
            free (pHuffman->m_decInfo); 
        }
        pHuffman->m_allocDecEntries = pHuffman->m_numDecEntries;
        pHuffman->m_decInfo = (HuffDecInfo *) malloc (sizeof (HuffDecInfo) * pHuffman->m_allocDecEntries);
        if (NULL == pHuffman->m_decInfo) {
            iMemStatus = -1;
            goto lerror;
        }

        // set everything to -1 for mpeg4 tables because there are illegal
        // entries in mpeg4 table, this way, we can detect bad data.
        for (i = 0; i < pHuffman->m_allocDecEntries; i++) {
            pHuffman->m_decInfo[i].symbol = ILLEGAL_SYMBOL;
            pHuffman->m_decInfo[i].length = 0;
        }
    }

    pHuffman->m_numDecEntries = 0;

    for (i=0; i<numTables; i++) {
        pHuffman->m_tableInfo[i].bits  = pHuffman->m_initInfo[i].bits;
        pHuffman->m_tableInfo[i].table = pHuffman->m_decInfo+pHuffman->m_numDecEntries;
        pHuffman->m_numDecEntries     += 1<<pHuffman->m_initInfo[i].bits;
    }

    for (i=0; i<numTables; i++) {
        for (j=pHuffman->m_initInfo[i].start; j<pHuffman->m_initInfo[i].end; j++) {
            (pHuffman->m_tableInfo[i].table)[pHuffman->m_initInfo[j].prefix].symbol = (U16) j;
            (pHuffman->m_tableInfo[i].table)[pHuffman->m_initInfo[j].prefix].length = 0;
        }
        if ( (unsigned)pHuffman->m_initInfo[i].end >= (1U<<HUFFDEC_SYMBOL_BITS) ) {
            assert( (unsigned)pHuffman->m_initInfo[i].end < (1U<<HUFFDEC_SYMBOL_BITS));
            goto lerror;
        }
    }

    return ICERR_OK;

lerror:
    if (NULL != pHuffman->m_tableInfo) {
        free(pHuffman->m_tableInfo);
        pHuffman->m_tableInfo = NULL;
    }
    if (NULL != pHuffman->m_decInfo) {
        free(pHuffman->m_decInfo);
        pHuffman->m_decInfo = NULL;
    }

    if (-1 == iMemStatus ) {
        printf("Insufficient memory to allocate tables.\n");
    }
    return ICERR_ERROR;
}

Void fillEntry(HuffmanDef *pHuffman, Int cwd, Int length, Int tableNum,
                        Int index, HuffDecInfo *currDecTable)
{
  Int start, end, excessBits, j, prefix;

  if ((NULL == pHuffman) || (NULL == currDecTable)) {
      return;
  }

  if (length == 0) return;

  while (TRUE) {
#ifndef _CASIO_VIDEO_
#ifdef SMALL_HUFFMAN
    assert(pHuffman->m_tableInfo[tableNum].bits < (1<<4)); // only 4 bits for length
                                                // one less for next stage
#else
    assert(pHuffman->m_tableInfo[tableNum].bits < (1<<16)); // should never happen
#endif
#endif
    if (length <= pHuffman->m_tableInfo[tableNum].bits) {
      excessBits = pHuffman->m_tableInfo[tableNum].bits - length;
      start      = cwd << excessBits;
      end        = start + (1 << excessBits);
      for (j=start; j<end; j++) {
        currDecTable[j].symbol = (U16) index;
        currDecTable[j].length = (U16) length;
      }
      // verify the indexes and lengths fit.
      return;
    }
    else {
      excessBits   = length - pHuffman->m_tableInfo[tableNum].bits;
      prefix       = cwd >> excessBits;
#ifndef _CASIO_VIDEO_
      assert(currDecTable[prefix].length == 0);
#endif
      tableNum     = currDecTable[prefix].symbol;
#ifdef OPT_HUFFMAN_GET
      cwd          = cwd & (((U32)0xffffffff)>>(32-excessBits)); // take excessBits LSB's
#else
#pragma prefast(suppress: __WARNING_BUFFER_OVERFLOW, "PREfast noise") 
      cwd          = cwd & GetMask[excessBits]; // take excessBits LSB's
#endif
      length       = excessBits;
      currDecTable = pHuffman->m_tableInfo[tableNum].table;
    }
  }
}

static Int Decode16(HuffmanDef* pHuffman, I16* pDecodeTable, BitIOInfo* pIO, Int iRootBits)
{
    Int iSymbol;

    iSymbol = pDecodeTable[peekBit16(pIO, iRootBits)];
    if (iSymbol >= 0)
    {
        flushBit16(pIO, iSymbol & ((1 << HUFFMAN_DECODE_ROOT_BITS_LOG) - 1));
        iSymbol >>= HUFFMAN_DECODE_ROOT_BITS_LOG;
    }
    else
    {
        flushBit16(pIO, iRootBits);

#if HUFFMAN_DECODE_ROOT_BITS < 10
        do {
            iSymbol += peekBit16(pIO, 1);
            flushBit16(pIO, 1);
            iSymbol = pDecodeTable[iSymbol + 0x8000];
        }
        while (iSymbol < 0);
#else
        while ((iSymbol = pDecodeTable[iSymbol + 0x8000 + peekBit16(pIO, 1)]) < 0)
        {
            flushBit16(pIO, 1);
        }
        flushBit16(pIO, 1);
#endif
    }
    return (iSymbol);
};

Int getHuff(HuffmanDef *pHuffman, BitIOInfo* pIO)
{
    return Decode16(pHuffman, pHuffman->m_hufDecTable, pIO, HUFFMAN_DECODE_ROOT_BITS);
}
#else   // X86OPT_HUFFMAN
Int getHuff(HuffmanDef *pHuffman, BitIOInfo* pIO)
{
    Int iSymbol, iSymbolHuff;
	const I16 *pDecodeTable = pHuffman->m_hufDecTable;

    iSymbol = pDecodeTable[peekBit16(pIO, HUFFMAN_DECODE_ROOT_BITS)];

    flushBit16(pIO, iSymbol < 0 ? HUFFMAN_DECODE_ROOT_BITS : iSymbol & ((1 << HUFFMAN_DECODE_ROOT_BITS_LOG) - 1));

#if 1
	iSymbolHuff = iSymbol >> HUFFMAN_DECODE_ROOT_BITS_LOG;

	if (iSymbolHuff < 0) {
		iSymbolHuff = iSymbol;
        while ((iSymbolHuff = pDecodeTable[iSymbolHuff + SIGN_BIT (pDecodeTable[0]) + getBit16(pIO, 1)]) < 0);
	}
    return (iSymbolHuff);
#endif
}

#endif  // X86OPT_HUFFMAN


// from huffman_wmv2.c (akadatch)

#ifdef DEBUG_HUFFMAN
#include <stdio.h>
#include <stdlib.h>

//
// Thoroughly verifies prefix code given by pCodeInfo with [optional] use of symbol mapping table pSymbolInfo
//

Int         // returns 0 if code is OK, source line number otherwise (for investigations)
Verify (
  HuffmanDef *pHuffman, 
  Int *pCodeInfo,   // code info (alphabet size, codeword1, bitlenth1, codeword2, bitlength2, ...
  Int *pSymbolInfo, // if 0 then natural enumeration (0, 1, ...), otherwise use pSymbolInfo[i] as i-th symbol
  Int  iRootBits    // lookup bits for root table
)
{
  Int i, n, iMaxIndex, bFlushWarning = 0;
  Int iAlphabetSize;
  I32 *pDecodeTable;
  
  if ((NULL == pHuffman) || (NULL == pCodeInfo)) {
    return (__LINE__);
  }

  iAlphabetSize = *pCodeInfo++;

  if ((UInt) iRootBits >= (1 << HUFFMAN_DECODE_ROOT_BITS_LOG))
  {
#ifdef DEBUG_HUFFMAN
    fprintf (fpHuff,"iRootBits = %d should be strictly less than %d\n", iRootBits, (1 << HUFFMAN_DECODE_ROOT_BITS_LOG));
#endif
    return (__LINE__);
  }

  iMaxIndex = HUFFMAN_DECODE_TABLE_SIZE (iRootBits, iAlphabetSize);
  // ANSI, ADI, or Desktop
  pDecodeTable = (I32 *) malloc (iMaxIndex * sizeof (pDecodeTable[0]));
  if (pDecodeTable == 0)
  {
#ifdef DEBUG_HUFFMAN
    fprintf (fpHuff,"error: cannot allocate %d bytes\n", iMaxIndex * sizeof (pDecodeTable[0]));
#endif
    return (__LINE__);
  }
  memset (pDecodeTable, 0, iMaxIndex * sizeof (pDecodeTable[0]));

  // first available intermediate node index
  n = (1 << iRootBits) - SIGN_BIT (pDecodeTable[0]);
  iMaxIndex -= SIGN_BIT (pDecodeTable[0]);

  for (i = 0; i < iAlphabetSize; ++i)
  {
    UInt m = pCodeInfo[2*i];
    Int  b = pCodeInfo[2*i+1];

    if (b >= 32 || (m >> b) != 0)
    {
#ifdef DEBUG_HUFFMAN
      fprintf (fpHuff,"symbol #%d %d (%d=0x%x %d): stated length exceeds actual codeword length\n",
          i, (pSymbolInfo == 0 ? i : pSymbolInfo[i]), pCodeInfo[2*i], pCodeInfo[2*i], pCodeInfo[2*i+1]
      );
#endif
      free(pDecodeTable);
      return (__LINE__);
    }

    if (b <= iRootBits)
    {
      // short codeword goes right into root table
      int k = m << (iRootBits - b);
      int kLast = (m + 1) << (iRootBits - b);

      if ((UInt) (pSymbolInfo == 0 ? i : pSymbolInfo[i]) >= (SIGN_BIT (pDecodeTable[0]) >> HUFFMAN_DECODE_ROOT_BITS_LOG))
      {
#ifdef DEBUG_HUFFMAN
        fprintf (fpHuff,"symbol #%d %d (%d=0x%x %d): cannot be packed even into I32 table\n",
          i, (pSymbolInfo == 0 ? i : pSymbolInfo[i]), pCodeInfo[2*i], pCodeInfo[2*i], pCodeInfo[2*i+1]
        );
#endif
        free(pDecodeTable);
        return (__LINE__);
      }
      if ((UInt) (pSymbolInfo == 0 ? i : pSymbolInfo[i]) >= (SIGN_BIT (I16) >> HUFFMAN_DECODE_ROOT_BITS_LOG))
      {
#ifdef DEBUG_HUFFMAN
        fprintf (fpHuff,"warning: symbol #%d %d (%d=0x%x %d): cannot be packed into I16 table\n",
          i, (pSymbolInfo == 0 ? i : pSymbolInfo[i]), pCodeInfo[2*i], pCodeInfo[2*i], pCodeInfo[2*i+1]
        );
#endif
      }
      b += (i + 1) << HUFFMAN_DECODE_ROOT_BITS_LOG;

      do
      {
        if (pDecodeTable[k] != 0)
        {
#ifdef DEBUG_HUFFMAN
          fprintf (fpHuff,"symbol #%d %d (%d=0x%x %d) conflicts with symbol ",
            i, (pSymbolInfo == 0 ? i : pSymbolInfo[i]), pCodeInfo[2*i], pCodeInfo[2*i], pCodeInfo[2*i+1]
          );
#endif

          // find conflicting symbol
          if (pDecodeTable[k] > 0) {
            k = pDecodeTable[k] >> HUFFMAN_DECODE_ROOT_BITS_LOG;
          }
          else
          {
            do
            {
              k += SIGN_BIT (pDecodeTable[k]);
#pragma prefast(suppress: __WARNING_BUFFER_UNDERFLOW, "PREfast noise")
              if (pDecodeTable[k] == 0) {
                ++k;
              }
            }
            while ((k = pDecodeTable[k]) < 0);
          }
          i = k - 1;
#ifdef DEBUG_HUFFMAN
          fprintf (fpHuff,"#%d %d (%d=0x%x %d)\n",
            i, (pSymbolInfo == 0 ? i : pSymbolInfo[i]), pCodeInfo[2*i], pCodeInfo[2*i], pCodeInfo[2*i+1]
          );

#endif
          free(pDecodeTable);
          return (__LINE__);
        }
        pDecodeTable[k] = b;
      }
      while (++k != kLast);
    }
    else
    {
      // long codeword -- generate bit-by-bit decoding path
      Int k;

      b -= iRootBits;
      if (b > 16 && !bFlushWarning)
      {
#ifdef DEBUG_HUFFMAN
        fprintf (fpHuff,"warning: symbol #%d %d (%d=0x%x %d) have too large bit length, flushing will be required\n",
          i, (pSymbolInfo == 0 ? i : pSymbolInfo[i]), pCodeInfo[2*i], pCodeInfo[2*i], pCodeInfo[2*i+1]
        );
#endif
        bFlushWarning = 1;
      }

      k = m >> b;   // these bit will be decoded by root table
      do
      {
        if (pDecodeTable[k] > 0)
        {
#ifdef DEBUG_HUFFMAN
          fprintf (fpHuff,"symbol #%d %d (%d=0x%x %d) conflicts with symbol ",
            i, (pSymbolInfo == 0 ? i : pSymbolInfo[i]), pCodeInfo[2*i], pCodeInfo[2*i], pCodeInfo[2*i+1]
          );
#endif

          // find conflicting symbol
          if (k < (1 << iRootBits)) {
            k = pDecodeTable[k] >> HUFFMAN_DECODE_ROOT_BITS_LOG;
          }
          else {
            k = pDecodeTable[k];
          }
          i = k - 1;
#ifdef DEBUG_HUFFMAN
          fprintf (fpHuff,"#%d %d (%d=0x%x %d)\n",
            i, (pSymbolInfo == 0 ? i : pSymbolInfo[i]), pCodeInfo[2*i], pCodeInfo[2*i], pCodeInfo[2*i+1]
          );

#endif
          free(pDecodeTable);
          return (__LINE__);
        }

        if (pDecodeTable[k] == 0)
        {
          // slot is empty -- create new internal node
          pDecodeTable[k] = n;
          n += 2;
          if (n > iMaxIndex)
          {
#ifdef DEBUG_HUFFMAN
            fprintf (fpHuff,"error: too sparce code, %d table entries not enough\n", iMaxIndex + SIGN_BIT (pDecodeTable[0]));
#endif
            free(pDecodeTable);
            return (__LINE__);
          }
        }
        k = pDecodeTable[k] + SIGN_BIT (pDecodeTable[0]);

        // find location in this internal node (will we go left or right)
        --b;
        if ((m >> b) & 1) {
          ++k;
        }
      }
      while (b != 0);

      // reached the leaf
      if (pDecodeTable[k] != 0)
      {
#ifdef DEBUG_HUFFMAN
        fprintf (fpHuff,"symbol #%d %d (%d=0x%x %d) conflicts with symbol ",
          i, (pSymbolInfo == 0 ? i : pSymbolInfo[i]), pCodeInfo[2*i], pCodeInfo[2*i], pCodeInfo[2*i+1]
        );
#endif

        // find conflicting symbol
        if (pDecodeTable[k] > 0) {
          k = pDecodeTable[k] >> HUFFMAN_DECODE_ROOT_BITS_LOG;
        }
        else
        {
          do
          {
            k += SIGN_BIT (pDecodeTable[k]);
            if (pDecodeTable[k] == 0) {
              ++k;
            }
          }
          while ((k = pDecodeTable[k]) < 0);
        }
        i = k - 1;
#ifdef DEBUG_HUFFMAN
        fprintf (fpHuff,"#%d %d (%d=0x%x %d)\n",
          i, (pSymbolInfo == 0 ? i : pSymbolInfo[i]), pCodeInfo[2*i], pCodeInfo[2*i], pCodeInfo[2*i+1]
        );
#endif
        free(pDecodeTable);
        return (__LINE__);
      }

      if ((UInt) (pSymbolInfo == 0 ? i : pSymbolInfo[i]) >= SIGN_BIT (pDecodeTable[0]))
      {
#ifdef DEBUG_HUFFMAN
        fprintf (fpHuff,"symbol #%d %d (%d=0x%x %d): cannot be packed even into I32 table\n",
          i, (pSymbolInfo == 0 ? i : pSymbolInfo[i]), pCodeInfo[2*i], pCodeInfo[2*i], pCodeInfo[2*i+1]
        );
#endif
        free(pDecodeTable);
        return (__LINE__);
      }
      if ((UInt) (pSymbolInfo == 0 ? i : pSymbolInfo[i]) >= SIGN_BIT (I16))
      {
#ifdef DEBUG_HUFFMAN
        fprintf (fpHuff,"warning: symbol #%d %d (%d=0x%x %d): cannot be packed into I16 table\n",
          i, (pSymbolInfo == 0 ? i : pSymbolInfo[i]), pCodeInfo[2*i], pCodeInfo[2*i], pCodeInfo[2*i+1]
        );
#endif
      }
      pDecodeTable[k] = i + 1;
    }
  }

  // OK, now check fullness of code
  n += SIGN_BIT (pDecodeTable[0]);
  for (i = 0; i < n; ++i)
  {
    if (pDecodeTable[i] != 0) {
        continue;
    }

    if (i < (1 << iRootBits))
    {
      Int j, b;

      // find index of last 0
      for (j = i+1; j < n && pDecodeTable[j] == 0;) {
        ++j;
      }
      --j;

      // find max number of most significant bits
      b = 0;
      while (b < iRootBits && ((i >> b) < (j >> b) || j == i + (1 << b) - 1)) {
        ++b;
      }
      --b;

      // skip until last code entry
      i += (1 << b) - 1;

#ifdef DEBUG_HUFFMAN
      fprintf (fpHuff,"warning: missing codeword (%d=0x%x %d)\n", i >> b, i >> b, iRootBits - b);
#endif
    }
    else
    {
      Int j, k, m, b;

      b = 0;
      m = 0;
      k = i;

      do
      {
        // find parent node
        j = k - SIGN_BIT (pDecodeTable[0]);
        k = 0;
        while (pDecodeTable[k] != j && pDecodeTable[k] + 1 != j) {
          ++k;
        }
        m += (j - pDecodeTable[k]) << b;
        ++b;
      }
      while (k >= (1 << iRootBits));

      m += k << b;
      b += iRootBits;
#ifdef DEBUG_HUFFMAN
      fprintf (fpHuff,"warning: missing codeword (%d=0x%x %d)\n", m, m, b);
#endif
    }
  }

#ifdef DEBUG_HUFFMAN
  fprintf (fpHuff,"    HUFFMAN_DECODE_TABLE_SIZE() = %d, actual size = %d\n", HUFFMAN_DECODE_TABLE_SIZE (iRootBits, iAlphabetSize), n);
#endif

  free(pDecodeTable);
  return (0);
}

#endif // DEBUG_HUFFMAN /* we may not need Verify often */


//
// Initializes 16-bit decoding table pDecodeTable[HUFFMAN_DECODE_TABLE_SIZE (iRootBits, pCodeInfo[0])].
// Symbol mapping table pSymbolInfo is optional.
//
// It is caller responsibility to make sure that Init16() and Decode16() are called with same (iRootBits).
// Ideally, iRootBits should be compile-time constant to simplify computations inside Decode16.
//
Int         // returns 0 if initialization went OK, source line number otherwise (for investigations)
Init16 (
  HuffmanDef *pHuffman, 
  I16 *pDecodeTable,    // decode table
  const Int *pCodeInfo,   // code info (alphabet size, codeword1, bitlenth1, codeword2, bitlength2, ...
  Int *pSymbolInfo, // if 0 then natural enumeration (0, 1, ...), otherwise table-driven
  Int  iRootBits    // lookup bits for root table
)
{
  Int i, n, iMaxIndex;
  Int iAlphabetSize;

  if ((NULL == pHuffman) || (NULL == pDecodeTable) || (NULL == pCodeInfo)) {
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

      if(iRootBits < 0)
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




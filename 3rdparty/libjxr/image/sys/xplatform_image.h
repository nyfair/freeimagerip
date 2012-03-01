//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************

#ifndef XPLATFORM_IMAGE_H
#define XPLATFORM_IMAGE_H

#ifdef __ANSI__
// ANSI
#define FORCE_INLINE 
#define CDECL
#define UINTPTR_T unsigned int
#define INTPTR_T int
#define DECLSPEC_ALIGN(bytes)
#endif	// __ANSI__


//#if defined(WIN32)
#if defined(WIN32) && !defined(UNDER_CE)  // WIN32 seems to be defined always in VS2005 for ARM platform
// x86
//#define CDECL __cdecl
#define DECLSPEC_ALIGN(bytes) __declspec(align(bytes))
#endif	// x86


#if defined(_ARM_) || defined(UNDER_CE)
// ARM, WinCE
#define FORCE_INLINE inline
#define CDECL
#define UINTPTR_T unsigned int
#define INTPTR_T int
#define DECLSPEC_ALIGN(bytes)

// parser
#define FULL_PATH_CONFIG_FILE_ENCODE    "\\ConfigFile_encode.txt"
#define FULL_PATH_CONFIG_FILE_DECODE    "\\ConfigFile_decode.txt"
#define MAX_ARGC 14
#define MaxCharReadCount 10
#define MAX_FNAME 256
#define DELIMITER "filelist:"
#define CODEC_ENCODE "encode"
#define CODEC_DECODE "decode"
#define PHOTON "ptn"
int XPLATparser(char *pcARGV[], char *pcCodec);
void freeXPLATparser(int iARGC, char *pcARGV[]);

// WinCE intrinsic
#include <Cmnintrin.h>
#endif  // ARM, WinCE


#ifdef __ADI__
// ADI
#define FORCE_INLINE inline
#define CDECL
#define UINTPTR_T unsigned int
#define INTPTR_T int
#define DECLSPEC_ALIGN(bytes)

// parser
#define FULL_PATH_CONFIG_FILE_ENCODE    "d:\\qunli\\imageFile\\ConfigFile_encode.txt"
#define FULL_PATH_CONFIG_FILE_DECODE    "d:\\qunli\\imageFile\\ConfigFile_decode.txt"
#define MAX_ARGC 14
#define MaxCharReadCount 10
#define MAX_FNAME 256
#define DELIMITER "filelist:"
#define CODEC_ENCODE "encode"
#define CODEC_DECODE "decode"
#define PHOTON "photon"
#define OUTRAW "raw"
#define OUTBMP "bmp"
#define OUTPPM "ppm"
#define OUTTIF "tif"
#define OUTHDR "hdr"
#define OUTIYUV "iyuv"
#define OUTYUV422 "yuv422"
#define OUTYUV444 "yuv444"
int XPLATparser(char *pcARGV[], char *pcCodec);
void freeXPLATparser(int iARGC, char *pcARGV[]);

// PCI IO
#include <device.h>
extern DevEntry pci_io_deventry;
extern DevEntry_t DevDrvTable[MAXDEV];      

// time.h bug workaround
void reset_cycle(void);
void get_cycle(unsigned int *cycle);

// SDRAM
void InitSDRAM(void);

// Cache
#include <cplb.h>

// bitIO opt
#ifdef ADI_BITIO_OPT
#include "bitIO.h"
#endif

// multi heaps
#ifdef ADI_SYSMEM_OPT
#include <stdlib_bf.h>

#define FAST_HEAP 1     // L2 on-chip fast access heap 
#define free_fast(ptr) heap_free(1, ptr)
void* malloc_fast(size_t nBytes);

extern char *g_pLargeMem;
#endif	//ADI_SYSMEM_OPT
#endif		// __ADI__


#endif      // XPLATFORM_IMAGE_H


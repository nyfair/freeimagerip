//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
#pragma once

//================================
// bitio functions
//================================
#define PACKETLENGTH (1U<<12)   // 4kB

#define readIS_L1(pSC, pIO) readIS(pSC, pIO)
#define readIS_L2(pSC, pIO) (void)(pSC, pIO)

#define writeIS_L1(pSC, pIO) writeIS(pSC, pIO)
#define writeIS_L2(pSC, pIO) (void)(pSC, pIO)


//================================
// common defines
//================================
#define FORCE_INLINE
#define CDECL
#define UINTPTR_T unsigned int
#define INTPTR_T int


//================================
// quantization optimization
//================================
//#define RECIP_QUANT_OPT



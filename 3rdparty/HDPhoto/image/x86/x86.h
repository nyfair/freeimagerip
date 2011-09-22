//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************
#pragma once

#include <assert.h>
#include <windows.h>

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
#define FORCE_INLINE __forceinline
#define UINTPTR_T uintptr_t
#define INTPTR_T intptr_t


//================================
// quantization optimization
//================================
#define RECIP_QUANT_OPT



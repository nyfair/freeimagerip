//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************

#ifndef WMI_STRTRANSFORM_H
#define WMI_STRTRANSFORM_H

#include "windowsmediaphoto.h"

/** 2x2 foward DCT == 2x2 inverse DCT **/
Void strDCT2x2dn(PixelI *, PixelI *, PixelI *, PixelI *);
Void strDCT2x2up(PixelI *, PixelI *, PixelI *, PixelI *);
Void FOURBUTTERFLY_HARDCODED1(PixelI *p);

/** 2x2 dct of a group of 4**/
#define FOURBUTTERFLY(p, i00, i01, i02, i03, i10, i11, i12, i13,	\
    i20, i21, i22, i23, i30, i31, i32, i33)		\
    strDCT2x2dn(&p[i00], &p[i01], &p[i02], &p[i03]);			\
    strDCT2x2dn(&p[i10], &p[i11], &p[i12], &p[i13]);			\
    strDCT2x2dn(&p[i20], &p[i21], &p[i22], &p[i23]);			\
    strDCT2x2dn(&p[i30], &p[i31], &p[i32], &p[i33])

#ifdef VERIFY_16BIT
extern int gMaxMin[];
static void CHECK(int i, int a, int b, int c, int d)
{
    if (gMaxMin[i*2] < a)
        gMaxMin[i*2] = a;
    else if (gMaxMin[i*2 + 1] > a)
        gMaxMin[i*2 + 1] = a;

    a = b;
    if (gMaxMin[i*2] < a)
        gMaxMin[i*2] = a;
    else if (gMaxMin[i*2 + 1] > a)
        gMaxMin[i*2 + 1] = a;

    a = c;
    if (gMaxMin[i*2] < a)
        gMaxMin[i*2] = a;
    else if (gMaxMin[i*2 + 1] > a)
        gMaxMin[i*2 + 1] = a;

    a = d;
    if (gMaxMin[i*2] < a)
        gMaxMin[i*2] = a;
    else if (gMaxMin[i*2 + 1] > a)
        gMaxMin[i*2 + 1] = a;
}

static void CHECK2(int i, int a, int b)
{
    if (gMaxMin[i*2] < a)
        gMaxMin[i*2] = a;
    else if (gMaxMin[i*2 + 1] > a)
        gMaxMin[i*2 + 1] = a;

    a = b;
    if (gMaxMin[i*2] < a)
        gMaxMin[i*2] = a;
    else if (gMaxMin[i*2 + 1] > a)
        gMaxMin[i*2 + 1] = a;
}

static void VERIFY_REPORT()
{
    int i;
    for (i = 0; i < 64; i += 2) {
        if (gMaxMin[i+1] - gMaxMin[i])
            printf ("Index %2d:\t[%d %d]\n", i/2, gMaxMin[i+1], gMaxMin[i]);
    }
}
#endif // VERIFY_16BIT

#endif // WMI_STRTRANSFORM_H
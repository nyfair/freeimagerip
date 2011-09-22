//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************

#include "strTransform.h"

#ifdef VERIFY_16BIT
int gMaxMin[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#endif // VERIFY_16BIT


/** need to swap b and c **/
/** rounding behavior: [0 0 0 0] <-> [+ - - -]
    [+ + + +] <-> [+3/4 - - -]
    [- - - -] <-> [- - - -] **/
Void strDCT2x2dn(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    PixelI a, b, c, d, C, t;
    a = *pa;
    b = *pb;
    C = *pc;
    d = *pd;
    //PixelI t1, t2;
#ifdef VERIFY_16BIT
    CHECK(0,a,b,C,d);
#endif // VERIFY_16BIT
  
    a += d;
    b -= C;
    t = ((a - b) >> 1);
    c = t - d;
    d = t - C;
    a -= d;
    b += c;

#ifdef VERIFY_16BIT
    CHECK(0,a,b,c,d);
#endif // VERIFY_16BIT
    *pa = a;
    *pb = b;
    *pc = c;
    *pd = d;
}

Void strDCT2x2up(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    PixelI a, b, c, d, C, t;
    a = *pa;
    b = *pb;
    C = *pc;
    d = *pd;
    //PixelI t1, t2;
#ifdef VERIFY_16BIT
    CHECK(1,a,b,C,d);
#endif // VERIFY_16BIT
  
    a += d;
    b -= C;
    t = ((a - b + 1) >> 1);
    c = t - d;
    d = t - C;
    a -= d;
    b += c;

#ifdef VERIFY_16BIT
    CHECK(1,a,b,c,d);
#endif // VERIFY_16BIT
    *pa = a;
    *pb = b;
    *pc = c;
    *pd = d;
}

Void FOURBUTTERFLY_HARDCODED1(PixelI *p)
{
    strDCT2x2dn(&p[0], &p[4], &p[8], &p[12]);
    strDCT2x2dn(&p[1], &p[5], &p[9], &p[13]);
    strDCT2x2dn(&p[2], &p[6], &p[10], &p[14]);
    strDCT2x2dn(&p[3], &p[7], &p[11], &p[15]);
}

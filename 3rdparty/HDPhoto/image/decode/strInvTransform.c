//*@@@+++@@@@******************************************************************
//
// Microsoft Windows Media
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//*@@@---@@@@******************************************************************

#include "strTransform.h"
#include "strcodec.h"
#include "decode.h"

/** rotation by -pi/8 **/
//#define IROTATE1(a, b) (a) -= ((b * 3 + 8) >> 4), b += (((a) * 3 + 4) >> 3), (a) -= (((b) * 3 + 8) >> 4)
//#define IROTATE2(a, b) (a) -= ((b * 3 + 13) >> 4), b += (((a) * 3 + 5) >> 3), (a) -= (((b) * 3 + 6) >> 4)
//#define IROTATE1(a, b) b += (((a) + 1) >> 3), (a) -= (((b) + 2) >> 2), b += (((a) + 2) >> 2), (a) -= (((b) + 4) >> 3)
//#define IROTATE2(a, b) b += (((a) + 4) >> 3), (a) -= (((b) + 2) >> 2), b += (((a) + 2) >> 2), (a) -= (((b) + 3) >> 3)
#define IROTATE1(a, b) (a) -= (((b) + 1) >> 1), (b) += (((a) + 1) >> 1)  // this works well too
#define IROTATE2(a, b) (a) -= (((b)*3 + 4) >> 3), (b) += (((a)*3 + 4) >> 3)  // this works well too
//#define FASTOPS

/** local functions **/
static Void invOddOdd(PixelI *, PixelI *, PixelI *, PixelI *);
static Void invOddOddPost(PixelI *, PixelI *, PixelI *, PixelI *);
//static Void scaleUpDown0(PixelI *, PixelI *);
//static Void scaleUpDown1(PixelI *, PixelI *);
//static Void scaleUpDown2(PixelI *, PixelI *);
static Void invOdd(PixelI *, PixelI *, PixelI *, PixelI *);
//static Void strIDCT2x2alt(PixelI *, PixelI *, PixelI *, PixelI *);
static Void strHSTdec(PixelI *, PixelI *, PixelI *, PixelI *);
static Void strHSTdec1(PixelI *, PixelI *);

#define FOURBUTTERFLY_DEC_ALT(p, i00, i01, i02, i03, i10, i11, i12, i13,	\
    i20, i21, i22, i23, i30, i31, i32, i33)		\
    strHSTdec1(&p[i00], &p[i03]);			\
    strHSTdec1(&p[i10], &p[i13]);			\
    strHSTdec1(&p[i20], &p[i23]);			\
    strHSTdec1(&p[i30], &p[i33]);           \
    strHSTdec(&p[i00], &p[i01], &p[i02], &p[i03]);			\
    strHSTdec(&p[i10], &p[i11], &p[i12], &p[i13]);			\
    strHSTdec(&p[i20], &p[i21], &p[i22], &p[i23]);			\
    strHSTdec(&p[i30], &p[i31], &p[i32], &p[i33])

/**
static Void FOURBUTTERFLY_DEC_ALT_HARDCODED1(PixelI *p)
{
    strHSTdec(&p[0], &p[3], &p[96], &p[99]);
    strHSTdec(&p[1], &p[2], &p[97], &p[98]);
    strHSTdec(&p[32], &p[35], &p[64], &p[67]);
    strHSTdec(&p[33], &p[34], &p[65], &p[66]);
}
**/

/** IDCT stuff **/
/** reordering should be combined with zigzag scan **/
/** data order before IDCT **/
/** 0  8  4  6 **/
/** 2 10 14 12 **/
/** 1 11 15 13 **/
/** 9  3  7  5 **/
/** data order after IDCT **/
/**  0  1  2  3 **/
/**  4  5  6  7 **/
/**  8  9 10 11 **/
/** 12 13 14 15 **/

//Void strIDCT4x4FirstStage420UV(PixelI * p)
//{
//    /** bottom left corner, butterfly => -pi/8 rotation **/
//    invOdd(&p[128], &p[192], &p[130], &p[194]);
//    
//    /** top right corner, -pi/8 rotation => butterfly **/
//    invOdd(&p[4], &p[6], &p[68], &p[70]);
//    
//    /** bottom right corner, -pi/8 rotation => -pi/8 rotation **/
//    invOddOdd(&p[132], &p[134], &p[196], &p[198]);
//    
//    /** top left corner, butterfly => butterfly **/
//    strDCT2x2up(&p[0], &p[2], &p[64], &p[66]);
//    
//    /** butterfly **/
//    FOURBUTTERFLY(p, 0, 6, 192, 198, 2, 4, 194, 196, 64, 70, 128, 134, 66, 68, 130, 132);
//}

/**
static Void strIDCT4x4FirstStageOnlyDC(PixelI *p)
{
    PixelI a, t, t1;
    a = p[0];
    t = (a + 1) >> 1;
    a -= t;
    t1 = t >> 1;
    t -= t1;

    p[2] = p[64] = p[66] = t;
    p[3] = p[34] = p[35] = p[65] = p[67]
    = p[96] = p[97] = p[98] = p[99] = t1;

    t = a >> 1;
    a -= t;
    p[1] = p[32] = p[33] = t;
    p[0] = a;
}
**/

//Void strIDCT4x4FirstStage(PixelI * p)
//{
//    /** top left corner, butterfly => butterfly **/
//    strDCT2x2up(&p[0], &p[1], &p[32], &p[33]);
//    /** top right corner, -pi/8 rotation => butterfly **/
//    if ((p[2] | p[3]) || (p[34] | p[35])) { // condition check not needed - helps at high QP, hurts a bit at low QP
//        invOdd(&p[2], &p[3], &p[34], &p[35]);
//    }
//    /** bottom left corner, butterfly => -pi/8 rotation **/
//    if ((p[64] | p[65]) || (p[96] | p[97])) { // condition check not needed - helps at high QP, hurts a bit at low QP
//        invOdd(&p[64], &p[96], &p[65], &p[97]);
//    }    
//    /** bottom right corner, -pi/8 rotation => -pi/8 rotation **/
//    if ((p[66] | p[67]) || (p[98] | p[99])) { // condition check not needed - helps at high QP, hurts a bit at low QP
//        invOddOdd(&p[66], &p[67], &p[98], &p[99]);
//    }
//    
//    /** butterfly **/
//    //FOURBUTTERFLY(p, 0, 3, 96, 99, 1, 2, 97, 98, 32, 35, 64, 67, 33, 34, 65, 66);
//    FOURBUTTERFLY_HARDCODED1(p);
//}

Void strIDCT4x4Stage1(PixelI* p)
{
    /** top left corner, butterfly => butterfly **/
    strDCT2x2up(p + 0, p + 1, p + 2, p + 3);

    /** top right corner, -pi/8 rotation => butterfly **/
    invOdd(p + 5, p + 4, p + 7, p + 6);

    /** bottom left corner, butterfly => -pi/8 rotation **/
    invOdd(p + 10, p + 8, p + 11, p + 9);

    /** bottom right corner, -pi/8 rotation => -pi/8 rotation **/
    invOddOdd(p + 15, p + 14, p + 13, p + 12);
    
    /** butterfly **/
    //FOURBUTTERFLY(p, 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);
    FOURBUTTERFLY_HARDCODED1(p);
}

//Void strIDCT4x4SecondStage(PixelI * p)
//{
//    /** bottom left corner, butterfly => -pi/8 rotation **/
//    invOdd(&p[256], &p[384], &p[260], &p[388]);
//    
//    /** top right corner, -pi/8 rotation => butterfly **/
//    invOdd(&p[8], &p[12], &p[136], &p[140]);
//    
//    /** bottom right corner, -pi/8 rotation => -pi/8 rotation **/
//    invOddOdd(&p[264], &p[268], &p[392], &p[396]);
//
//    /** top left corner, butterfly => butterfly **/
//    strDCT2x2up(&p[0], &p[4], &p[128], &p[132]);
//    
//    /** butterfly **/
//    FOURBUTTERFLY(p, 0, 12, 384, 396, 4, 8, 388, 392, 128, 140, 256, 268, 132, 136, 260, 264);
//}

Void strIDCT4x4Stage2(PixelI* p)
{
    /** bottom left corner, butterfly => -pi/8 rotation **/
    invOdd(p + 32, p + 48, p + 96, p + 112);
    
    /** top right corner, -pi/8 rotation => butterfly **/
    invOdd(p + 128, p + 192, p + 144, p + 208);
    
    /** bottom right corner, -pi/8 rotation => -pi/8 rotation **/
    invOddOdd(p + 160, p + 224, p + 176, p + 240);

    /** top left corner, butterfly => butterfly **/
    strDCT2x2up(p + 0, p + 64, p + 16, p + 80);
    
    /** butterfly **/
    FOURBUTTERFLY(p, 0, 192, 48, 240, 64, 128, 112, 176, 16, 208, 32, 224, 80, 144, 96, 160);
}

Void strNormalizeDec(PixelI* p, Bool bChroma)
{
    int i;
    if (!bChroma) {
        //for (i = 0; i < 256; i += 16) {
        //    p[i] <<= 2;
        //}
    }
    else {
        for (i = 0; i < 256; i += 16) {
            p[i] += p[i];
        }
    }
}

/** 2x2 DCT with post-scaling - for use on decoder side **/
Void strDCT2x2dnDec(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    PixelI a, b, c, d, C, t;
    a = *pa;
    b = *pb;
    C = *pc;
    d = *pd;
    //PixelI t1, t2;
#ifdef VERIFY_16BIT
    CHECK(2,a,b,C,d);
#endif // VERIFY_16BIT
  
    a += d;
    b -= C;
    t = ((a - b) >> 1);
    c = t - d;
    d = t - C;
    a -= d;
    b += c;

    *pa = a * 2;
    *pb = b * 2;
    *pc = c * 2;
    *pd = d * 2;
}


/** post filter stuff **/
/** 2-point post for boundaries **/
Void strPost2(PixelI * a, PixelI * b)
{
#ifdef VERIFY_16BIT
    CHECK2(3,*a,*b);
#endif // VERIFY_16BIT
    *b += ((*a + 4) >> 3);
    *a += ((*b + 2) >> 2);
    *b += ((*a + 4) >> 3);
}

Void strPost2x2(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    PixelI a, b, c, d;
    a = *pa;
    b = *pb;
    c = *pc;
    d = *pd;
#ifdef VERIFY_16BIT
    CHECK(4,a,b,c,d);
#endif // VERIFY_16BIT

    /** butterflies **/
    a += d;
    b += c;
    d -= (a + 1) >> 1;
    c -= (b + 1) >> 1;

    /** rotate **/
    b += ((a + 2) >> 2);
    a += ((b + 1) >> 1);
    b += ((a + 2) >> 2);

    /** butterflies **/
    d += (a + 1) >> 1;
    c += (b + 1) >> 1;
    a -= d;
    b -= c;

    *pa = a;
    *pb = b;
    *pc = c;
    *pd = d;
}

/** 4-point post for boundaries **/
Void strPost4(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    PixelI a, b, c, d;
    a = *pa;
    b = *pb;
    c = *pc;
    d = *pd;
#ifdef VERIFY_16BIT
    CHECK(5,a,b,c,d);
#endif // VERIFY_16BIT

    a += d, b += c;
    d -= ((a + 1) >> 1), c -= ((b + 1) >> 1);
    
    IROTATE1(c, d);

    d += ((a + 1) >> 1), c += ((b + 1) >> 1);
    a -= d - ((d * 3 + 16) >> 5), b -= c - ((c * 3 + 16) >> 5);
    d += ((a * 3 + 8) >> 4), c += ((b * 3 + 8) >> 4);
    a += ((d * 3 + 16) >> 5), b += ((c * 3 + 16) >> 5);

    *pa = a;
    *pb = b;
    *pc = c;
    *pd = d;
}

//Void strPost4x4FirstStage420UV(PixelI * p)
//{
//    /** buttefly **/
//    FOURBUTTERFLY(p, 0, 6, 192, 198, 2, 4, 194, 196, 64, 70, 128, 134, 66, 68, 130, 132);
//    
//    /** bottom right corner: -pi/8 rotation => -pi/8 rotation **/
//    invOddOddPost(&p[132], &p[134], &p[196], &p[198]);
//    
//    /** anti diagonal corners: rotation by -pi/8 **/
//    IROTATE1(p[68], p[4]);
//    IROTATE1(p[70], p[6]);
//    IROTATE1(p[130], p[128]);
//    IROTATE1(p[194], p[192]);
//
//    /**diagonal corners: inverse scaling **/
//    //scaleUpDown2(&p[66], &p[132]);
//    //scaleUpDown1(&p[64], &p[134]);
//    //scaleUpDown1(&p[2], &p[196]);
//    //scaleUpDown0(&p[0], &p[198]);
//  
//    /** butterfly **/
//    FOURBUTTERFLY_DEC_ALT(p, 0, 192, 6, 198, 2, 194, 4, 196, 64, 128, 70, 134, 66, 130, 68, 132);
//}

//Void strPost4x4FirstStage(PixelI * p)
//{
//    //postFilter(p, 0,1,2,3, 32,33,34,35, 64,65,66,67, 96,97,98,99 );
//    //return;
//    /** buttefly **/
//    //FOURBUTTERFLY(p, 0, 3, 96, 99, 1, 2, 97, 98, 32, 35, 64, 67, 33, 34, 65, 66);
//    FOURBUTTERFLY_HARDCODED1(p);
//
//    /** bottom right corner: -pi/8 rotation => -pi/8 rotation **/
//    invOddOddPost(&p[66], &p[67], &p[98], &p[99]);
//    
//    /** anti diagonal corners: rotation by -pi/8 **/
//    IROTATE1(p[34], p[2]);
//    IROTATE1(p[35], p[3]);
//    IROTATE1(p[65], p[64]);
//    IROTATE1(p[97], p[96]);
//
//    /**diagonal corners: inverse scaling **/
//    //SCALE_DEC_HARDCODED1(p);
//    //scaleUpDown2(&p[33], &p[66]);
//    //scaleUpDown1(&p[32], &p[67]);
//    //scaleUpDown1(&p[1], &p[98]);
//    //scaleUpDown0(&p[0], &p[99]);
//
//    /** butterfly **/
//    //FOURBUTTERFLY_DEC_ALT_HARDCODED1(p);
//    strHSTdec1(&p[0], &p[99]);
//    strHSTdec1(&p[1], &p[98]);
//    strHSTdec1(&p[32], &p[67]);
//    strHSTdec1(&p[33], &p[66]);
//
//    strHSTdec(&p[0], &p[3], &p[96], &p[99]);
//    strHSTdec(&p[1], &p[2], &p[97], &p[98]);
//    strHSTdec(&p[32], &p[35], &p[64], &p[67]);
//    strHSTdec(&p[33], &p[34], &p[65], &p[66]);
//}

/*****************************************************************************************
  Input data offsets:
  (15)(14)|(10+64)(11+64) p0 (15)(14)|(74)(75)
  (13)(12)|( 8+64)( 9+64)    (13)(12)|(72)(73)
  --------+--------------    --------+--------
  ( 5)( 4)|( 0+64) (1+64) p1 ( 5)( 4)|(64)(65)
  ( 7)( 6)|( 2+64) (3+64)    ( 7)( 6)|(66)(67)
*****************************************************************************************/
Void strPost4x4Stage1Split(PixelI *p0, PixelI *p1, Int iOffset)
{
    PixelI *p2 = p0 + 72 - iOffset;
    PixelI *p3 = p1 + 64 - iOffset;
    p0 += 12;
    p1 += 4;

    /** buttefly **/
    strDCT2x2dn(p0 + 0, p2 + 0, p1 + 0, p3 + 0);
    strDCT2x2dn(p0 + 1, p2 + 1, p1 + 1, p3 + 1);
    strDCT2x2dn(p0 + 2, p2 + 2, p1 + 2, p3 + 2);
    strDCT2x2dn(p0 + 3, p2 + 3, p1 + 3, p3 + 3);

    /** bottom right corner: -pi/8 rotation => -pi/8 rotation **/
    invOddOddPost(p3 + 0, p3 + 1, p3 + 2, p3 + 3);
    
    /** anti diagonal corners: rotation by -pi/8 **/
    IROTATE1(p1[2], p1[3]);
    IROTATE1(p1[0], p1[1]);
    IROTATE1(p2[1], p2[3]);
    IROTATE1(p2[0], p2[2]);

    /** butterfly **/
    strHSTdec1(p0 + 0, p3 + 0);
    strHSTdec1(p0 + 1, p3 + 1);
    strHSTdec1(p0 + 2, p3 + 2);
    strHSTdec1(p0 + 3, p3 + 3);
    strHSTdec(p0 + 0, p2 + 0, p1 + 0, p3 + 0);
    strHSTdec(p0 + 1, p2 + 1, p1 + 1, p3 + 1);
    strHSTdec(p0 + 2, p2 + 2, p1 + 2, p3 + 2);
    strHSTdec(p0 + 3, p2 + 3, p1 + 3, p3 + 3);
}

Void strPost4x4Stage1(PixelI* p, Int iOffset)
{
    strPost4x4Stage1Split(p, p + 16, iOffset);
}

/*****************************************************************************************
  Input data offsets:
  (15)(14)|(10+32)(11+32) p0 (15)(14)|(42)(43)
  (13)(12)|( 8+32)( 9+32)    (13)(12)|(40)(41)
  --------+--------------    --------+--------
  ( 5)( 4)|( 0+32) (1+32) p1 ( 5)( 4)|(32)(33)
  ( 7)( 6)|( 2+32) (3+32)    ( 7)( 6)|(34)(35)
*****************************************************************************************/
//Void strPost4x4Stage1Split_420(PixelI* p0, PixelI* p1)
//{
//    /** buttefly **/
//    strDCT2x2dn(p0 + 15, p0 + 43, p1 + 7, p1 + 35);
//    strDCT2x2dn(p0 + 14, p0 + 42, p1 + 6, p1 + 34);
//    strDCT2x2dn(p0 + 13, p0 + 41, p1 + 5, p1 + 33);
//    strDCT2x2dn(p0 + 12, p0 + 40, p1 + 4, p1 + 32);
//
//    /** bottom right corner: -pi/8 rotation => -pi/8 rotation **/
//    invOddOddPost(p1 + 32, p1 + 33, p1 + 34, p1 + 35);
//    
//    /** anti diagonal corners: rotation by -pi/8 **/
//    IROTATE1(p1[6],  p1[7]);
//    IROTATE1(p1[4],  p1[5]);
//    IROTATE1(p0[41], p0[43]);
//    IROTATE1(p0[40], p0[42]);
//
//    /** butterfly **/
//    strHSTdec1(p0 + 15, p1 + 35);
//    strHSTdec1(p0 + 14, p1 + 34);
//    strHSTdec1(p0 + 13, p1 + 33);
//    strHSTdec1(p0 + 12, p1 + 32);
//    strHSTdec(p0 + 15, p0 + 43, p1 + 7, p1 + 35);
//    strHSTdec(p0 + 14, p0 + 42, p1 + 6, p1 + 34);
//    strHSTdec(p0 + 13, p0 + 41, p1 + 5, p1 + 33);
//    strHSTdec(p0 + 12, p0 + 40, p1 + 4, p1 + 32);
//}
//
//Void strPost4x4Stage1_420(PixelI* p)
//{
//    strPost4x4Stage1Split_420(p, p + 16);
//}

//Void strPost4x4SecondStage(PixelI * p)
//{
//    /** buttefly **/
//    FOURBUTTERFLY(p, 0, 12, 384, 396, 4, 8, 388, 392, 128, 140, 256, 268, 132, 136, 260, 264);
//
//    /** bottom right corner: -pi/8 rotation => -pi/8 rotation **/
//    invOddOddPost(&p[264], &p[268], &p[392], &p[396]);
//    
//    /** anti diagonal corners: rotation by -pi/8 **/
//    IROTATE1(p[136], p[8]);
//    IROTATE1(p[140], p[12]);
//    IROTATE1(p[260], p[256]);
//    IROTATE1(p[388], p[384]);
//    
//    /**diagonal corners: inverse scaling **/
//    //scaleUpDown2(&p[132], &p[264]);
//    //scaleUpDown1(&p[128], &p[268]);
//    //scaleUpDown1(&p[4], &p[392]);
//    //scaleUpDown0(&p[0], &p[396]);
//    
//    /** butterfly **/
//    FOURBUTTERFLY_DEC_ALT(p, 0, 384, 12, 396, 4, 388, 8, 392, 128, 256, 140, 268, 132, 260, 136, 264);
//}

/*****************************************************************************************
  Input data offsets:
  ( -96)(-32)|(32)( 96) p0
  ( -80)(-16)|(48)(112)
  -----------+------------
  (-128)(-64)|( 0)( 64) p1
  (-112)(-48)|(16)( 80)
*****************************************************************************************/
Void strPost4x4Stage2Split(PixelI* p0, PixelI* p1)
{
    /** buttefly **/
    strDCT2x2dn(p0 - 96, p0 +  96, p1 - 112, p1 + 80);
    strDCT2x2dn(p0 - 32, p0 +  32, p1 -  48, p1 + 16);
    strDCT2x2dn(p0 - 80, p0 + 112, p1 - 128, p1 + 64);
    strDCT2x2dn(p0 - 16, p0 +  48, p1 -  64, p1 +  0);

    /** bottom right corner: -pi/8 rotation => -pi/8 rotation **/
    invOddOddPost(p1 + 0, p1 + 64, p1 + 16, p1 + 80);
    
    /** anti diagonal corners: rotation by -pi/8 **/
    IROTATE1(p0[ 48], p0[  32]);
    IROTATE1(p0[112], p0[  96]);
    IROTATE1(p1[-64], p1[-128]);
    IROTATE1(p1[-48], p1[-112]);
    
    /** butterfly **/
    strHSTdec1(p0 - 96, p1 + 80);
    strHSTdec1(p0 - 32, p1 + 16);
    strHSTdec1(p0 - 80, p1 + 64);
    strHSTdec1(p0 - 16, p1 +  0);

    strHSTdec(p0 - 96, p1 - 112, p0 +  96, p1 + 80);
    strHSTdec(p0 - 32, p1 -  48, p0 +  32, p1 + 16);
    strHSTdec(p0 - 80, p1 - 128, p0 + 112, p1 + 64);
    strHSTdec(p0 - 16, p1 -  64, p0 +  48, p1 +  0);
}

/** 
    Hadamard+Scale transform
    for some strange reason, breaking up the function into two blocks, strHSTdec1 and strHSTdec
    seems to work faster
**/
static Void strHSTdec1(PixelI *pa, PixelI *pd)
{
    /** different realization : does rescaling as well! **/
    PixelI a, d;
    a = *pa;
    d = *pd;
#ifdef VERIFY_16BIT
    CHECK2(6,a,d);
#endif // VERIFY_16BIT

    a += d;
    d = (a >> 1) - d;
    a += (d * 3 + 0) >> 3;
    d += (a * 3 + 0) >> 4;
    //a += (d * 3 + 4) >> 3;

    *pa = a;
    *pd = d;
}

static Void strHSTdec(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    /** different realization : does rescaling as well! **/
    PixelI a, b, c, d;
    a = *pa;
    b = *pb;
    c = *pc;
    d = *pd;
#ifdef VERIFY_16BIT
    CHECK(7,a,b,c,d);
#endif // VERIFY_16BIT

    //a += d;
    b -= c;
    //d = (a >> 1) - d;
    //a += (d * 3 + 0) >> 3;
    //d += (a * 3 + 0) >> 4;
    a += (d * 3 + 4) >> 3;

    d -= (b >> 1);
    c = ((a - b) >> 1) - c;
    *pc = d;
    *pd = c;
    *pa = a - c, *pb = b + d;
}

/** Kron(Rotate(pi/8), Rotate(pi/8)) **/
static Void invOddOdd(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    PixelI a, b, c, d, t1, t2;
    a = *pa;
    b = *pb;
    c = *pc;
    d = *pd;
#ifdef VERIFY_16BIT
    CHECK(8,a,b,c,d);
#endif // VERIFY_16BIT

    /** butterflies **/
    d += a;
    c -= b;
    a -= (t1 = d >> 1);
    b += (t2 = c >> 1);

    /** rotate pi/4 **/
    a -= (b * 3 + 3) >> 3;
    b += (a * 3 + 3) >> 2;
    a -= (b * 3 + 4) >> 3;

    /** butterflies **/
    b -= t2;
    a += t1;
    c += b;
    d -= a;

    /** sign flips **/
    *pa = a;
    *pb = -b;
    *pc = -c;
    *pd = d;
}

/** Kron(Rotate(pi/8), Rotate(pi/8)) **/
static Void invOddOddPost(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    PixelI a, b, c, d, t1, t2;
    a = *pa;
    b = *pb;
    c = *pc;
    d = *pd;
#ifdef VERIFY_16BIT
    CHECK(9,a,b,c,d);
#endif // VERIFY_16BIT

    /** butterflies **/
    d += a;
    c -= b;
    a -= (t1 = d >> 1);
    b += (t2 = c >> 1);

    /** rotate pi/4 **/
    a -= (b * 3 + 6) >> 3;
    b += (a * 3 + 2) >> 2;
    a -= (b * 3 + 4) >> 3;

    /** butterflies **/
    b -= t2;
    a += t1;
    c += b;
    d -= a;

    *pa = a;
    *pb = b;
    *pc = c;
    *pd = d;
}


/** Kron(Rotate(-pi/8), [1 1; 1 -1]/sqrt(2)) **/
/** [D C A B] => [a b c d] **/
Void invOdd(PixelI *pa, PixelI *pb, PixelI *pc, PixelI *pd)
{
    PixelI a, b, c, d;
    a = *pa;
    b = *pb;
    c = *pc;
    d = *pd;
#ifdef VERIFY_16BIT
    CHECK(10,a,b,c,d);
#endif // VERIFY_16BIT

    /** butterflies **/
    b += d;
    a -= c;
    d -= (b) >> 1;
    c += (a + 1) >> 1;

    /** rotate pi/8 **/
    IROTATE2(a, b);
    IROTATE2(c, d);

    /** butterflies **/
    c -= (b + 1) >> 1;
    d = ((a + 1) >> 1) - d;
    b += c;
    a -= d;

    *pa = a;
    *pb = b;
    *pc = c;
    *pd = d;
}

/*************************************************************************
  Mipmap generator
*************************************************************************/
//static Void MIPgen(PixelI * p) // first stage MIPgen for 1:2 thumbnail
//{
//    p[6] = p[14] = p[7] = p[9] = p[13] = p[11] = p[15];
//    p[3] = ((p[3] + 4) >> 3);
//    p[2] = ((p[2] + 2) >> 2);
//    p[4] = ((p[4] + 2) >> 2);
//    p[1] = ((p[1] + 2) >> 2);
//    p[8] = ((p[8] + 2) >> 2);
//    p[12] -= ((p[12] + 2) >> 2);
//}

/*************************************************************************
  Top-level function to inverse tranform possible part of a macroblock
*************************************************************************/
Int  invTransformMacroblock(CWMImageStrCodec * pSC)
{
    const OVERLAP olOverlap = pSC->WMISCP.olOverlap;
    const COLORFORMAT cfColorFormat = pSC->m_param.cfColorFormat;
    const BITDEPTH_BITS bdBitDepth = pSC->WMII.bdBitDepth;
    const Bool left = (pSC->cColumn == 0), right = (pSC->cColumn == pSC->cmbWidth);
    const Bool top = (pSC->cRow == 0), bottom = (pSC->cRow == pSC->cmbHeight);
    const Bool topORbottom = (top || bottom), leftORright = (left || right);
    const Bool topORleft = (top || left), bottomORright = (bottom || right);
    const size_t mbWidth = pSC->cmbWidth, mbX = pSC->cColumn;
    PixelI * p = NULL, * pt = NULL;
    size_t i;
    const size_t iChannels = (cfColorFormat == YUV_420 || cfColorFormat == YUV_422) ? 1 : pSC->m_param.cNumChannels;
    const size_t tScale = pSC->m_Dparam->cThumbnailScale;
    Int j = 0;

    Int qp[3], dcqp[3], iStrength = (1 << pSC->WMII.cPostProcStrength);
    ERR_CODE result = ICERR_OK;

    if(pSC->WMII.cPostProcStrength > 0){
        // threshold for post processing
        for(j = 0; j < 3; j ++)
            qp[j] = pSC->pTile[pSC->cTileColumn].pQuantizerLP[j][pSC->MBInfo.iQIndexLP].iQP * iStrength * (olOverlap == OL_NONE ? 2 : 1);
        dcqp[0] = pSC->pTile[pSC->cTileColumn].pQuantizerDC[0][0].iQP * iStrength;
        dcqp[1] = pSC->pTile[pSC->cTileColumn].pQuantizerDC[0][0].iQP * iStrength;

        if(left) // a new MB row
            slideOneMBRow(pSC->pPostProcInfo, pSC->m_param.cNumChannels, mbWidth, top, bottom);  // previous current row becomes previous row
    }

    //================================================================
    // 400_Y, 444_YUV
    for (i = 0; i < iChannels && tScale < 16; ++i)
    {
        PixelI* const p0 = pSC->p0MBbuffer[i];
        PixelI* const p1 = pSC->p1MBbuffer[i];

        //================================
        // second level inverse transform
        if (!bottomORright)
        {
            if(pSC->WMII.cPostProcStrength > 0)
                updatePostProcInfo(pSC->pPostProcInfo, p1, mbX, i); // update postproc info before IDCT

/*            if(tScale == 8 && bdBitDepth != BD_1){ // MIPgen for 8:1 thumbnail, skip for binary image
                p1[16 * 13] = p1[16 * 14] = p1[16 * 9] = p1[16 * 7] = p1[16 * 11] = p1[16 * 6] = p1[16 * 10] = 0;
                p1[16 *  5] = ((p1[16 *  5] + 4) >> 3);
                p1[16 *  1] = ((p1[16 *  1] + 2) >> 2);
                p1[16 * 12] = ((p1[16 * 12] + 2) >> 2);
                p1[16 *  4] = ((p1[16 *  4] + 2) >> 2);
                p1[16 *  3] = ((p1[16 *  3] + 2) >> 2);
                p1[16 * 15] -= ((p1[16 * 15] + 2) >> 2);
            }*/

            strIDCT4x4Stage2(p1);
            if (pSC->m_param.bScaledArith) {
                strNormalizeDec(p1, (i != 0));
            }
        }

        //================================
        // second level inverse overlap
        if (OL_TWO == olOverlap)
        {
            if (leftORright && (!topORbottom))
            {
                j = left ? 0 : -128;
                strPost4(p0 + j + 32, p0 + j +  48, p1 + j +  0, p1 + j + 16);
                strPost4(p0 + j + 96, p0 + j + 112, p1 + j + 64, p1 + j + 80);
            }

            if (!leftORright)
            {
                if (topORbottom)
                {
                    p = top ? p1 : p0 + 32;
                    strPost4(p - 128, p - 64, p +  0, p + 64);
                    strPost4(p - 112, p - 48, p + 16, p + 80);
                    p = NULL;
                }
                else
                {
                    strPost4x4Stage2Split(p0, p1);
                }
            }
        }

        if(pSC->WMII.cPostProcStrength > 0)
            postProcMB(pSC->pPostProcInfo, p0, p1, mbX, i, dcqp[i]); // second stage deblocking

        //================================
        // first level inverse transform
        if(tScale >= 4) // bypass first level transform for 4:1 and smaller thumbnail
            continue;

        if (!top)
        {
            for (j = (left ? 32 : -96); j < (right ? 32 : 160); j += 64)
            {
//                if(tScale == 2 && bdBitDepth != BD_1){
//                    MIPgen(p0 + j + 0);
//                    MIPgen(p0 + j + 16);
//                }
                strIDCT4x4Stage1(p0 + j +  0);
                strIDCT4x4Stage1(p0 + j + 16);
            }
        }

        if (!bottom)
        {
            for (j = (left ? 0 : -128); j < (right ? 0 : 128); j += 64)
            {
//                if(tScale == 2  && bdBitDepth != BD_1){
//                    MIPgen(p1 + j + 0);
//                    MIPgen(p1 + j + 16);
//                }
                strIDCT4x4Stage1(p1 + j +  0);
                strIDCT4x4Stage1(p1 + j + 16);
            }
        }

        //================================
        // first level inverse overlap
        if (OL_NONE != olOverlap)
        {
            if (leftORright)
            {
                j = left ? 0 + 10 : -64 + 14;
                if (!top)
                {
                    p = p0 + 16 + j;
                    strPost4(p +  0, p -  2, p +  6, p +  8);
                    strPost4(p +  1, p -  1, p +  7, p +  9);
                    strPost4(p + 16, p + 14, p + 22, p + 24);
                    strPost4(p + 17, p + 15, p + 23, p + 25);
                    p = NULL;
                }
                if (!bottom)
                {
                    p = p1 + j;
                    strPost4(p + 0, p - 2, p + 6, p + 8);
                    strPost4(p + 1, p - 1, p + 7, p + 9);
                    p = NULL;
                }
                if (!topORbottom)
                {
                    strPost4(p0 + 48 + j + 0, p0 + 48 + j - 2, p1 - 10 + j, p1 - 8 + j);
                    strPost4(p0 + 48 + j + 1, p0 + 48 + j - 1, p1 -  9 + j, p1 - 7 + j);
                }
            }

            if (top)
            {
                for (j = (left ? 0 : -192); j < (right ? -64 : 64); j += 64)
                {
                    p = p1 + j;
                    strPost4(p + 5, p + 4, p + 64, p + 65);
                    strPost4(p + 7, p + 6, p + 66, p + 67);
                    p = NULL;

                    strPost4x4Stage1(p1 + j, 0);
                }
            }
            else if (bottom)
            {
                for (j = (left ? 0 : -192); j < (right ? -64 : 64); j += 64)
                {
                    strPost4x4Stage1(p0 + 16 + j, 0);
                    strPost4x4Stage1(p0 + 32 + j, 0);

                    p = p0 + 48 + j;
                    strPost4(p + 15, p + 14, p + 74, p + 75);
                    strPost4(p + 13, p + 12, p + 72, p + 73);
                    p = NULL;
                }
            }
            else
            {
                for (j = (left ? 0 : -192); j < (right ? -64 : 64); j += 64)
                {
                    strPost4x4Stage1(p0 + 16 + j, 0);
                    strPost4x4Stage1(p0 + 32 + j, 0);
                    strPost4x4Stage1Split(p0 + 48 + j, p1 + j, 0);
                    strPost4x4Stage1(p1 + j, 0);
                }
            }
        }
        
        if(pSC->WMII.cPostProcStrength > 0 && (!topORleft))
            postProcBlock(pSC->pPostProcInfo, p0, p1, mbX, i, qp[i]); // destairing and first stage deblocking
    }

    //================================================================
    // 420_UV
    for (i = 0; i < (YUV_420 == cfColorFormat? 2U : 0U) && tScale < 16; ++i)
    {
        PixelI* const p0 = pSC->p0MBbuffer[1 + i];//(0 == i ? pSC->pU0 : pSC->pV0);
        PixelI* const p1 = pSC->p1MBbuffer[1 + i];//(0 == i ? pSC->pU1 : pSC->pV1);

        //========================================
        // second level inverse transform (420_UV)
        if (!bottomORright)
        {
            if (!pSC->m_param.bScaledArith) {
                strDCT2x2dn(p1, p1 + 32, p1 + 16, p1 + 48);
            }
            else {
                strDCT2x2dnDec(p1, p1 + 32, p1 + 16, p1 + 48);
            }
        }
        
        //========================================
        // second level inverse overlap (420_UV)
        if (OL_TWO == olOverlap)
        {
            if (leftORright && !topORbottom)
            {
                j = (left ? 0 : -32);
                strPost2(p0 + j + 16, p1 + j);
            }

            if (!leftORright)
            {
                if (topORbottom)
                {
                    p = (top ? p1 : p0 + 16);
                    strPost2(p - 32, p);
                    p = NULL;
                }
                else{
                    strPost2x2(p0 - 16, p0 + 16, p1 - 32, p1);
                }
            }
        }

        //========================================
        // first level inverse transform (420_UV)
        if(tScale >= 4) // bypass first level transform for 4:1 and smaller thumbnail
            continue;

        if (!top)
        {
            for (j = (left ? 16 : -16); j < (right ? 16 : 48); j += 32)
            {
//                if(tScale == 2 && bdBitDepth != BD_1)
//                    MIPgen(p0 + j);

                strIDCT4x4Stage1(p0 + j);
            }
        }

        if (!bottom)
        {
            for (j = (left ? 0 : -32); j < (right ? 0 : 32); j += 32)
            {
//                if(tScale == 2)
//                    MIPgen(p1 + j + 0);

                strIDCT4x4Stage1(p1 + j);
            }
        }

        //========================================
        // first level inverse overlap (420_UV)
        if (OL_NONE != olOverlap)
        {
            if(!left && !top)
            {
                if (bottom)
                {
                    for (j = -48; j < (right ? -16 : 16); j += 32)
                    {
                        p = p0 + j;
                        strPost4(p + 15, p + 14, p + 42, p + 43);
                        strPost4(p + 13, p + 12, p + 40, p + 41);
                        p = NULL;
                    }
                }
                else
                {
                    for (j = -48; j < (right ? -16 : 16); j += 32)
                    {
                        strPost4x4Stage1Split(p0 + j, p1 - 16 + j, 32);
                    }
                }

                if (right)
                {
                    if (!bottom)
                    {
                        strPost4(p0 - 2 , p0 - 4 , p1 - 28, p1 - 26);
                        strPost4(p0 - 1 , p0 - 3 , p1 - 27, p1 - 25);
                    }

                    strPost4(p0 - 18, p0 - 20, p0 - 12, p0 - 10);
                    strPost4(p0 - 17, p0 - 19, p0 - 11, p0 -  9);
                }
                else
                {
                    strPost4x4Stage1(p0 - 32, 32);
                }

                strPost4x4Stage1(p0 - 64, 32);
            }
            else if (top)
            {
                for (j = (left ? 0: -64); j < (right ? -32: 0); j += 32)
                {
                    p = p1 + j + 4;
                    strPost4(p + 1, p + 0, p + 28, p + 29);
                    strPost4(p + 3, p + 2, p + 30, p + 31);
                    p = NULL;
                }
            }
            else if (left)
            {
                if (!bottom)
                {
                    strPost4(p0 + 26, p0 + 24, p1 + 0, p1 + 2);
                    strPost4(p0 + 27, p0 + 25, p1 + 1, p1 + 3);
                }

                strPost4(p0 + 10, p0 + 8, p0 + 16, p0 + 18);
                strPost4(p0 + 11, p0 + 9, p0 + 17, p0 + 19);
            }
        }
    }

    //================================================================
    // 422_UV
    for (i = 0; i < (YUV_422 == cfColorFormat? 2U : 0U) && tScale < 16; ++i)
    {
        PixelI* const p0 = pSC->p0MBbuffer[1 + i];//(0 == i ? pSC->pU0 : pSC->pV0);
        PixelI* const p1 = pSC->p1MBbuffer[1 + i];//(0 == i ? pSC->pU1 : pSC->pV1);

        //========================================
        // second level inverse transform (422_UV)
        if ((!bottomORright) && pSC->m_Dparam->cThumbnailScale < 16)
        {
            // 1D lossless HT
            p1[0]  -= ((p1[32] + 1) >> 1);
            p1[32] += p1[0];

            if (!pSC->m_param.bScaledArith) {
                strDCT2x2dn(p1 +  0, p1 + 64, p1 + 16, p1 +  80);
                strDCT2x2dn(p1 + 32, p1 + 96, p1 + 48, p1 + 112);
            }
            else {
                strDCT2x2dnDec(p1 +  0, p1 + 64, p1 + 16, p1 +  80);
                strDCT2x2dnDec(p1 + 32, p1 + 96, p1 + 48, p1 + 112);
            }
        }
        
        //========================================
        // second level inverse overlap (422_UV)
        if (OL_TWO == olOverlap)
        {
            if (!bottom)
            {
                if (leftORright)
                {
                    if (!top)
                    {
                        j = (left ? 0 : -64);
                        strPost2(p0 + 48 + j, p1 + j);
                    }

                    j = (left ? 16 : -48);
                    strPost2(p1 + j, p1 + j + 16);
                }
                else
                {
                    if (top)
                    {
                        strPost2(p1 - 64, p1);
                    }
                    else
                    {
                        strPost2x2(p0 - 16, p0 + 48, p1 - 64, p1);
                    }

                    strPost2x2(p1 - 48, p1 + 16, p1 - 32, p1 + 32);
                }
            }
            else if (!leftORright)
            {
                strPost2(p0 - 16, p0 + 48);
            }
        }

        //========================================
        // first level inverse transform (422_UV)
        if(tScale >= 4) // bypass first level transform for 4:1 and smaller thumbnail
            continue;

        if (!top)
        {
            for (j = (left ? 48 : -16); j < (right ? 48 : 112); j += 64)
            {
//                if(tScale == 2 && bdBitDepth != BD_1)
//                    MIPgen(p0 + j);
                strIDCT4x4Stage1(p0 + j);
            }
        }

        if (!bottom)
        {
            for (j = (left ? 0 : -64); j < (right ? 0 : 64); j += 64)
            {
//                if(tScale == 2 && bdBitDepth != BD_1){
//                    MIPgen(p1 + j + 0);
//                    MIPgen(p1 + j + 16);
//                    MIPgen(p1 + j + 32);
//                }
                strIDCT4x4Stage1(p1 + j + 0);
                strIDCT4x4Stage1(p1 + j + 16);
                strIDCT4x4Stage1(p1 + j + 32);
            }
        }
        
        //========================================
        // first level inverse overlap (422_UV)
        if (OL_NONE != olOverlap)
        {
            if (!top)
            {
                if (leftORright)
                {
                    j = (left ? 32 + 10 : -32 + 14);

                    p = p0 + j;
                    strPost4(p + 0, p - 2, p + 6, p + 8);
                    strPost4(p + 1, p - 1, p + 7, p + 9);

                    p = NULL;
                }

                for (j = (left ? 0 : -128); j < (right ? -64 : 0); j += 64)
                {
                    strPost4x4Stage1(p0 + j + 32, 0);
                }
            }

            if (!bottom)
            {
                if (leftORright)
                {
                    j = (left ? 0 + 10 : -64 + 14);

                    p = p1 + j;
                    strPost4(p + 0, p - 2, p + 6, p + 8);
                    strPost4(p + 1, p - 1, p + 7, p + 9);

                    p += 16;
                    strPost4(p + 0, p - 2, p + 6, p + 8);
                    strPost4(p + 1, p - 1, p + 7, p + 9);

                    p = NULL;
                }

                for (j = (left ? 0 : -128); j < (right ? -64 : 0); j += 64)
                {
                    strPost4x4Stage1(p1 + j +  0, 0);
                    strPost4x4Stage1(p1 + j + 16, 0);
                }
            }

            if (topORbottom)
            {
                p = (top ? p1 + 5 : p0 + 48 + 13);
                for (j = (left ? 0 : -128); j < (right ? -64 : 0); j += 64)
                {
                    strPost4(p + j + 0, p + j - 1, p + j + 59, p + j + 60);
                    strPost4(p + j + 2, p + j + 1, p + j + 61, p + j + 62);
                }
                p = NULL;
            }
            else
            {
                if (leftORright)
                {
                    j = (left ? 0 + 0 : -64 + 4);
                    strPost4(p0 + j + 48 + 10 + 0, p0 + j + 48 + 10 - 2, p1 + j + 0, p1 + j + 2);
                    strPost4(p0 + j + 48 + 10 + 1, p0 + j + 48 + 10 - 1, p1 + j + 1, p1 + j + 3);
                }

                for (j = (left ? 0 : -128); j < (right ? -64 : 0); j += 64)
                {
                    strPost4x4Stage1Split(p0 + j + 48, p1 + j + 0, 0);
                }
            }
        }
    }    

    return ICERR_OK;
}

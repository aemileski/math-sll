/*
 * $Id: math-sll.c,v 1.3 2002/01/20 20:14:20 andrewm Exp $
 *
 * Purpose
 *	A fixed point (31.32 bit) math library.
 *
 * Description
 *	Floating point packs the most accuracy in the available bits, but it
 *	often provides more accuracy than is required.  It is time consuming
 *	to carry the extra precision around, particularly on platforms that
 *	don't have a dedicated floating point processor.
 *
 *	This library is a compromise.  All math is done using the 64 bit
 *	signed long long format (sll), and is not intended to be portable.
 *	Just as fast as possible.  This format is also by plan the same size
 *	as an IEEE double.  Since the format used is fixed point, there is
 *	never a need to do time consuming checks and adjustments to maintain
 *	normalized numbers, as is the case in floating point.
 *
 *	Simply put, this library is limited to handling numbers with a whole
 *	part of up to 2^31 - 1 in magnitude, and fractional parts down to
 *	2^-30 in magnitude.  This yields a decent range and accuracy for many 
 *	applications.  These can of course be adjusted if desired.
 *
 * IMPORTANT
 *	No checking for arguments out of range (error).
 *	No checking for divide by zero (error).
 *	No checking for overflow (error).
 *	No checking for underflow (warning).
 *	Chops, doesn't round.
 *
 * Functions
 *	sll dbl2sll(double x)			double -> sll
 *	double slldbl(sll x)			sll -> double
 *
 *	sll slladd(sll x, sll y)		x + y
 *	sll sllsub(sll x, sll y)		x - y
 *	sll sllmul(sll x, sll y)		x * y
 *	sll slldiv(sll x, sll y)		x / y
 *
 *	sll sllinv(sll v)			1 / x
 *	sll sllmul2(sll x)			x * 2
 *	sll sllmul4(sll x)			x * 4
 *	sll sllmul2n(sll x, int n)		x * 2^n, 0 <= n <= 31
 *	sll slldiv2(sll x)			x / 2
 *	sll slldiv4(sll x)			x / 4
 *	sll slldiv2n(sll x, int n)		x / 2^n, 0 <= n <= 31
 *
 *	sll sllcos(sll x)			cos x
 *	sll sllsin(sll x)			sin x
 *	sll slltan(sll x)			tan x
 *	sll sllatan(sll x)			atan x
 *
 *	sll sllexp(sll x)			e^x
 *	sll slllog(sll x)			ln x
 *
 *	sll sllpow(sll x, sll y)		x^y
 *	sll sllsqrt(sll x)			x^(1 / 2)
 *
 * History
 *	* Jan 20 2002 Andrew E. Mileski <andrewm@isoar.ca> v1.3
 *	- Added fast multiplication functions sllmul2(), sllmul4(), sllmul2n()
 *	- Added fast division functions slldiv2() slldiv(), slldiv4n()
 *	- Added square root function sllsqrt()
 *	- Added library roll-call
 *	- Reformatted the history to RPM format (ick)
 *	- Moved sllexp() closer to related functions
 *	- Added algorithm description to sllpow()
 *
 *	* Jan 19 2002 Andrew E. Mileski <andrewm@isoar.ca> v1.1
 *	- Corrected constants, thanks to Mark A. Lisher for noticing
 *	- Put source under CVS control
 *
 *	* Jan 18 2002 Andrew E. Mileski <andrewm@isoar.ca>
 *	- Added some more explanation to calc_cos() and calc_sin()
 *	- Added sllatan() and documented it fairly verbosely
 *
 *	* July 13 2000 Andrew E. Mileski <andrewm@isoar.ca>
 *	- Corrected documentation for multiplication algorithm
 *
 *	* May 21 2000 Andrew E. Mileski <andrewm@isoar.ca>
 *	- Rewrote slltanx() to avoid scaling argument for both sine and cosine
 *
 *	* May 19 2000  Andrew E. Mileski <andrewm@isoar.ca>
 *	- Unrolled loops
 *	- Added sllinv(), and sllneg()
 *	- Changed constants to type "LL" (was "UL" - oops)
 *	- Changed all routines to use inverse constants instead of division
 *
 *	* May 15, 2000 - Andrew E. Mileski <andrewm@isoar.ca>
 *	- Fixed slltan() - used sin/cos instead of sllsin/sllcos.  Doh!
 *	- Added slllog(x) and sllpow(x,y)
 *
 *	* May 11, 2000 - Andrew E. Mileski <andrewm@isoar.ca>
 *	- Added simple tan(x) that could stand some optimization
 *	- Added more constants (see <math.h>)
 *
 *	* May 3, 2000 - Andrew E. Mileski <andrewm@isoar.ca>
 *	- Added sllsin(), sllcos(), and trig constants
 *
 *	* May 2, 2000 - Andrew E. Mileski <andrewm@isoar.ca>
 *	- All routines and macros now have sll their identifiers
 *	- Changed mul() to umul() and added sllmul() to handle signed numbers
 *	- Added and tested sllexp(), sllint(), and sllfrac()
 *	- Added some constants
 *
 *	* Apr 26, 2000 - Andrew E. Mileski <andrewm@isoar.ca>
 *	- Added mul(), and began testing it (unsigned only)
 *
 *	* Apr 25, 2000 - Andrew E. Mileski <andrewm@isoar.ca>
 *	- Added sll2dbl() [convert a signed long long to a double]
 *	- Began testing.  Well gee whiz it works! :)
 *
 *	* Apr 24, 2000 - Andrew E. Mileski <andrewm@isoar.ca>
 *	- Added dbl2sll() [convert a double to signed long long]
 *	- Began documenting
 *
 *	* Apr ??, 2000 - Andrew E. Mileski <andrewm@isoar.ca>
 *	- Conceived, written, and fiddled with
 *
 *
 *		Copyright (C) 2000 Andrew E. Mileski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */
#if !defined(__GNUC__)
#  error Requires support for type long long (64 bits)
#endif
#if !defined(__arm__)
#  error Not yet ported to this architecture!
#endif

typedef unsigned long long ull;
typedef signed long long sll;

#define int2sll(X)	(((sll) (X)) << 32)
#define sll2int(X)	((int) ((X) >> 32))
#define sllint(X)	((X) & 0xffffffff00000000LL)
#define sllfrac(X)	((X) & 0x00000000ffffffffLL)
#define sllneg(X)	(-(X))

/*
 * These were calculated using doubles.
 * e   : e = exp(0.5) * exp(0.5)
 * pi  : x = x + sin(x), starting with x = 355/113
 * pi/2: x = x + cos(x), starting wtih x = 355/226
 */
#define CONST_10	0x0000000a00000000LL
#define CONST_3		0x0000000300000000LL
#define CONST_2		0x0000000200000000LL
#define CONST_1		0x0000000100000000LL
#define CONST_0		0x0000000000000000LL
#define CONST_1_2	0x0000000080000000LL

#define CONST_E		0x00000002b7e15162LL
#define CONST_1_E	0x000000005e2d58d8LL
#define CONST_SQRTE	0x00000001a61298e1LL
#define CONST_1_SQRTE	0x000000009b4597e3LL

#define CONST_LOG2_E	0x0000000171547652LL
#define CONST_LOG10_E	0x000000006f2dec54LL
#define CONST_LN2	0x00000000b17217f7LL
#define CONST_LN10	0x000000024d763776LL

#define CONST_PI	0x00000003243f6a88LL
#define CONST_PI_2	0x00000001921fb544LL
#define CONST_PI_4	0x00000000c90fdaa2LL
#define CONST_1_PI	0x00000000517cc1b7LL
#define CONST_2_PI	0x00000000a2f9836eLL

#define CONST_2_SQRTPI	0x0000000120dd7504LL
#define CONST_SQRT2	0x000000016a09e667LL
#define CONST_1_SQRT2	0x00000000b504f333LL

__inline__ sll slladd(sll x, sll y)
{
	return (x + y);
}

__inline__ sll sllsub(sll x, sll y)
{
	return (x - y);
}

/*
 *	Multiply two signed 64 bit numbers.  This is done by sign extending
 *	both numbers to 128 bits, multiplying to get a 256 bit product,
 *	throwing away the top 128 bits, and finally throwing away the top
 *	and bottom 32 bits of the remaining result.  To avoid calculating
 *	products that are not needed, we calculate them  beforehand:
 *
 *	Where A and B = 0 or 0xffffffff (sign extension of number)
 *	AABC = A_96 + A_64 + B_32 + C_00
 *	DDEF = D_96 + D_64 + E_32 + F_00
 *
 *	AABC * DDEF = A_96*D_96 + A_96*D_64 + A_96*E_32 + A_96*F_00
 *		    + A_64*D_96 + A_64*D_64 + A_64*E_32 + A_64*F_00
 *		    + B_32*D_96 + B_32*D_64 + B_32*E_32 + B_32*F_00
 *		    + C_00*D_96 + C_00*D_64 + C_00*E_32 + C_00*F_00
 *
 *		    = A*D_192 + A*D_160 + A*E_128 + A*F_96
 *		    + A*D_160 + A*D_128 + A*E_96 + A*F_64
 *		    + B*D_128 + B*D_96 + B*E_64 + B*F_32
 *		    + C*D_96 + C*D_64 + C*E_32 + C*F_00
 *
 *		    = A*D_192
 *		    + A*D_160 + A*D_160
 *		    + A*E_128 + A*D_128 + B*D_128
 *		    + A*F_96 + A*E_96 + B*D_96 + C*D_96
 *		    + A*F_64 + B*E_64 + C*D_64
 *		    + B*F_32 + C*E_32
 *		    + C*F_00
 *
 *		    = (A*D)_192
 *		    + 2(A*D)_160
 *		    + (A*E + A*D + B*D)_128
 *		    + (A*F + A*E + B*D + C*D)_96
 *		    + (A*F + B*E + C*D)_64
 *		    + (B*F + C*E)_32
 *		    + (C*F)_00
 *
 *		if A == 0 && D == 0 (both numbers are positive)
 *		    = 0
 *		    + 0
 *		    + 0
 *		    + 0
 *		    + (B*E)_64
 *		    + (B*F + C*E)_32
 *		    + (C*F)_00
 */

/*
 *	Given the 64 bit numbers AB and CD, where ABCD are 32 bits each:
 *
 *	AB * CD = (A*(2^32) + B*(2^0)) * (C*(2^32) + D*(2^0))
 *		= A*(2^32) * C*(2^32) + A*(2^32) * D*(2^0)
 *		+ B*(2^0) * C*(2^32) + B*(2^0) * D*(2^0)
 *		= A*C*(2^64) + A*D*(2^32) + B*C*(2^32) + B*D*(2^0)
 *		= A*C*(2^64) + (A*D + B*C)*(2^32) + B*D*(2^0)
 */
static ull umul(ull left, ull right)
{
#if defined(__arm__)
	asm(
		"@ multiply\n"
		"	@ r0 = D\n"
		"	@ r1 = C\n"
		"	@ r2 = B\n"
		"	@ r3 = A\n"
		"	@ r4 = ?\n"
		"	@ r5 = ?\n"
		"	@ r6 = ?\n"
		"	umull	r6, r4, r0, r2\n"
		"	@ r0 = D\n"
		"	@ r1 = C\n"
		"	@ r2 = B\n"
		"	@ r3 = A\n"
		"	@ r4 = HI(B*D)\n"
		"	@ r5 = ?\n"
		"	@ r6 = ?\n"
		"	umull	r5, r6, r1, r2\n"
		"	@ r0 = D\n"
		"	@ r1 = C\n"
		"	@ r2 = ?\n"
		"	@ r3 = A\n"
		"	@ r4 = HI(B*D)\n"
		"	@ r5 = LO(B*C)\n"
		"	@ r6 = HI(B*C)\n"
		"	adds	r4, r4, r5\n"
		"	@ r0 = D\n"
		"	@ r1 = C\n"
		"	@ r2 = ?\n"
		"	@ r3 = A\n"
		"	@ r4 = HI(B*D) + LO(B*C)\n"
		"	@ r5 = ?\n"
		"	@ r6 = HI(B*C)\n"
		"	adc	r5, r6, #0\n"
		"	@ r0 = D\n"
		"	@ r1 = C\n"
		"	@ r2 = ?\n"
		"	@ r3 = A\n"
		"	@ r4 = HI(B*D) + LO(B*C)\n"
		"	@ r5 = HI(B*C) + carry1\n"
		"	@ r6 = ?\n"
		"	umull	r2, r6, r0, r3\n"
		"	@ r0 = ?\n"
		"	@ r1 = C\n"
		"	@ r2 = LO(A*D)\n"
		"	@ r3 = A\n"
		"	@ r4 = HI(B*D) + LO(B*C)\n"
		"	@ r5 = HI(B*C) + carry1\n"
		"	@ r6 = HI(A*D)\n"
		"	adds	r0, r2, r4\n"
		"	@ r0 = LO(A*D) + LO(B*C) + HI(B*D)\n"
		"	@ r1 = C\n"
		"	@ r2 = ?\n"
		"	@ r3 = A\n"
		"	@ r4 = ?\n"
		"	@ r5 = HI(B*C) + carry1\n"
		"	@ r6 = HI(A*D)\n"
		"	adc	r2, r5, r6\n"
		"	@ r0 = LO(A*D) + LO(B*C) + HI(B*D)\n"
		"	@ r1 = C\n"
		"	@ r2 = HI(A*D) + HI(B*C) + carry1 + carry2\n"
		"	@ r3 = A\n"
		"	@ r4 = ?\n"
		"	@ r5 = ?\n"
		"	@ r6 = ?\n"
		"	umull	r4, r5, r1, r3\n"
		"	@ r0 = LO(A*D) + LO(B*C) + HI(B*D)\n"
		"	@ r1 = ?\n"
		"	@ r2 = HI(A*D) + HI(B*C) + carry1 + carry2\n"
		"	@ r3 = ?\n"
		"	@ r4 = LO(A*C)\n"
		"	@ r5 = ?\n"
		"	@ r6 = ?\n"
		"	add	r1, r2, r4\n"
		"	@ r0 = LO(A*D) + LO(B*C) + HI(B*D)\n"
		"	@ r1 = LO(A*C) + HI(A*D) + HI(B*C) + carry1 + carry2\n"
		"	@ r2 = ?\n"
		"	@ r3 = ?\n"
		"	@ r4 = ?\n"
		"	@ r5 = ?\n"
		"	@ r6 = ?\n"
		: "=r" (left)
		: "0" (left), "r" (right)
		: "r4", "r5", "r6"
		: "cc"
#endif /* defined(__arm__) */
	return left;
}

sll sllmul(sll left, sll right)
{
	unsigned sign = 0;
	sll retval;
	if (left < CONST_0) {
		sign ^= 1;
		left = sllneg(left);
	}
	if (right < CONST_0) {
		sign ^= 1;
		right = sllneg(right);
	}
	retval = (sll) umul((ull) left, (ull) right);
	return ((sign) ? sllneg(retval): retval);
}

sll sllinv(sll v)
{
	int sgn = 0;
	sll u;
	ull s = -1;

	/* Use positive numbers, or the approximation won't work */
	if (v < CONST_0) {
		v = sllneg(v);
		sgn = 1;
	}

	/* An approximation - must be larger than the actual value */
	for (u = v; u; ((ull)u) >>= 1)
		s >>= 1;

	/* Newton's Method */
	u = sllmul(s, sllsub(CONST_2, sllmul(v, s)));
	u = sllmul(u, sllsub(CONST_2, sllmul(v, u)));
	u = sllmul(u, sllsub(CONST_2, sllmul(v, u)));
	u = sllmul(u, sllsub(CONST_2, sllmul(v, u)));
	u = sllmul(u, sllsub(CONST_2, sllmul(v, u)));
	u = sllmul(u, sllsub(CONST_2, sllmul(v, u)));

	return ((sgn) ? sllneg(u): u);
}

__inline__ sll slldiv(sll left, sll right)
{
	return sllmul(left, sllinv(right));
}

sll sllmul2(sll x)
{
#if defined(__arm__)
	asm(
		"@ sllmul2\n"
		"	mov	r1, r1, lsl #1\n"
		"	movs	r0, r0, lsl #1\n"
		"	orrcs	r1, r1, #1\n"
		: "=r" (x)
		: "0" (x)
	);
#endif /* defined(__arm__) */

	return x;
}

sll sllmul4(sll x)
{
#if defined(__arm__)
	asm(
		"@ sllmul4\n"
		"	mov	r1, r1, lsl #1\n"
		"	movs	r0, r0, lsl #1\n"
		"	orrcs	r1, r1, #1\n"
		"	mov	r1, r1, lsl #1\n"
		"	movs	r0, r0, lsl #1\n"
		"	orrcs	r1, r1, #1\n"
		: "=r" (x)
		: "0" (x)
	);
#endif /* defined(__arm__) */

	return x;
}

sll sllmul2n(sll x, int n)
{
#if defined(__arm__)
	asm(
		"@ sllmul2n\n"
		"	and	%1, %1, #0x1f\n"
		"1:\n"
		"	mov	r1, r1, lsl #1\n"
		"	movs	r0, r0, lsl #1\n"
		"	orrcs	r1, r1, #1\n"
		"	subs	%1, %1, #1\n"
		"	bne	1b\n"
		: "=r" (x), "=r" (n)
		: "0" (x), "1" (n)
	);
#endif /* defined(__arm__) */

	return x;
}

sll slldiv2(sll x)
{
#if defined(__arm__)
	asm(
		"@ slldiv2\n"
		"	movs	r1, r1, asr #1\n"
		"	mov	r0, r0, rrx\n"
		: "=r" (x)
		: "0" (x)
	);
#endif /* defined(__arm__) */

	return x;
}

sll slldiv4(sll x)
{
#if defined(__arm__)
	asm(
		"@ slldiv4\n"
		"	movs	r1, r1, asr #1\n"
		"	mov	r0, r0, rrx\n"
		"	movs	r1, r1, asr #1\n"
		"	mov	r0, r0, rrx\n"
		: "=r" (x)
		: "0" (x)
	);
#endif /* defined(__arm__) */

	return x;
}

sll slldiv2n(sll x, int n)
{
#if defined(__arm__)
	asm(
		"@ slldiv2n\n"
		"	and	%1, %1, #0x1f\n"
		"1:\n"
		"	movs	r1, r1, asr #1\n"
		"	mov	r0, r0, rrx\n"
		"	subs	%1, %1, #1\n"
		"	bne	1b\n"
		: "=r" (x), "=r" (n)
		: "0" (x), "1" (n)
	);
#endif /* defined(__arm__) */

	return x;
}

/*
 * Unpack the IEEE floating point double format and put it in fixed point
 * sll format.
 */
sll dbl2sll(double dbl)
{
	union {
		double d;
		unsigned u[2];
		ull ull;
		sll sll;
	} in, retval;
	register unsigned exp;

	/* Move into memory as args might be passed in regs */
	in.d = dbl;

	/* Leading 1 is assumed by IEEE */
	retval.u[1] = 0x40000000;

	/* Unpack the mantissa into the unsigned long */
	retval.u[1] |= (in.u[0] << 10) & 0x3ffffc00;
	retval.u[1] |= (in.u[1] >> 22) & 0x000003ff;
	retval.u[0] = in.u[1] << 10;

	/* Extract the exponent and align the decimals */
	exp = (in.u[0] >> 20) & 0x7ff;
	if (exp)
		retval.ull >>= 1053 - exp;
	else
		return 0L;

	/* Negate if negative flag set */
	if (in.u[0] & 0x80000000)
		retval.sll = -retval.sll;

	return retval.sll;
}

double sll2dbl(sll s)
{
	union {
		double d;
		unsigned u[2];
		ull ull;
		sll sll;
	} in, retval;
	register unsigned exp;
	register unsigned flag;

	if (s == 0)
		return 0.0;

	/* Move into memory as args might be passed in regs */
	in.sll = s;

	/* Handle the negative flag */
	if (in.sll < 1) {
		flag = 0x80000000;
		in.sll = sllneg(in.sll);
	} else
		flag = 0x00000000;

	/* Pack up the mantissa */
	retval.ull = in.ull;

	/* Normalize */
	for (exp = 1053; in.ull && (in.u[1] & 0x80000000) == 0; exp--) {
		in.ull <<= 1;
	}
	in.ull <<= 1;
	exp++;
	in.ull >>= 12;
	retval.u[0] = in.u[1];
	retval.u[1] = in.u[0];
	retval.u[0] |= flag | (exp << 20);

	return retval.d;
}

/*
 * Calculate e^x where -0.5 <= x <= 0.5
 *
 * Description:
 *	e^x = x^0 / 0! + x^1 / 1! + ... + x^N / N!
 *	Note that 0.5^11 / 11! < 2^-32 which is the smallest possible number.
 */
#define CONST_1_11	0x000000001745d174LL
#define CONST_1_10	0x0000000019999999LL
#define CONST_1_9	0x000000001c71c71cLL
#define CONST_1_8	0x0000000020000000LL
#define CONST_1_7	0x0000000024924924LL
#define CONST_1_6	0x000000002aaaaaaaLL
#define CONST_1_5	0x0000000033333333LL
#define CONST_1_4	0x0000000040000000LL
#define CONST_1_3	0x0000000055555555LL
#define CONST_1_2	0x0000000080000000LL
static inline sll calc_exp(sll x)
{
	sll retval;
	retval = slladd(CONST_1, sllmul(0, sllmul(x, CONST_1_11)));
	retval = slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_11)));
	retval = slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_10)));
	retval = slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_9)));
	retval = slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_8)));
	retval = slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_7)));
	retval = slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_6)));
	retval = slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_5)));
	retval = slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_4)));
	retval = slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_3)));
	retval = slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_2)));
	return retval;
}

/*
 * Calculate cos x where -pi/4 <= x <= pi/4
 *
 * Description:
 *	cos x = 1 - x^2 / 2! + x^4 / 4! - ... + x^(2N) / (2N)!
 *	Note that (pi/4)^12 / 12! < 2^-32 which is the smallest possible number.
 */
#define CONST_1_132	0x0000000001f07c1fLL
#define CONST_1_90	0x0000000002d82d82LL
#define CONST_1_56	0x0000000004924924LL
#define CONST_1_30	0x0000000008888888LL
#define CONST_1_12	0x0000000015555555LL
#define CONST_1_2	0x0000000080000000LL
static inline sll calc_cos(sll x)
{
	sll retval, x2;
	x2 = sllmul(x, x);
	/*
	 * cos x = t0 + t1 + t2 + t3 + t4 + t5 + t6
	 *
	 * f0 =  0! =  1
	 * f1 =  2! =  2 *  1 * f0 =   2 * f0
	 * f2 =  4! =  4 *  3 * f1 =  12 x f1
	 * f3 =  6! =  6 *  5 * f2 =  30 * f2
	 * f4 =  8! =  8 *  7 * f3 =  56 * f3
	 * f5 = 10! = 10 *  9 * f4 =  90 * f4
	 * f6 = 12! = 12 * 11 * f5 = 132 * f5
	 *
	 * t0 = 1
	 * t1 = -t0 * x2 /   2 = -t0 * x2 * CONST_1_2
	 * t2 = -t1 * x2 /  12 = -t1 * x2 * CONST_1_12
	 * t3 = -t2 * x2 /  30 = -t2 * x2 * CONST_1_30
	 * t4 = -t3 * x2 /  56 = -t3 * x2 * CONST_1_56
	 * t5 = -t4 * x2 /  90 = -t4 * x2 * CONST_1_90
	 * t6 = -t5 * x2 / 132 = -t5 * x2 * CONST_1_132
	 */
	retval = sllsub(CONST_1, sllmul(sllmul(x2, CONST_1), CONST_1_132));
	retval = sllsub(CONST_1, sllmul(sllmul(x2, retval), CONST_1_90));
	retval = sllsub(CONST_1, sllmul(sllmul(x2, retval), CONST_1_56));
	retval = sllsub(CONST_1, sllmul(sllmul(x2, retval), CONST_1_30));
	retval = sllsub(CONST_1, sllmul(sllmul(x2, retval), CONST_1_12));
	retval = sllsub(CONST_1, sllmul(sllmul(x2, retval), CONST_1_2));
	return retval;
}

/*
 * Calculate sin x where -pi/4 <= x <= pi/4
 *
 * Description:
 *	sin x = x - x^3 / 3! + x^5 / 5! - ... + x^(2N+1) / (2N+1)!
 *	Note that (pi/4)^13 / 13! < 2^-32 which is the smallest possible number.
 */
#define CONST_1_156	0x0000000001a41a41LL
#define CONST_1_110	0x000000000253c825LL
#define CONST_1_72	0x00000000038e38e3LL
#define CONST_1_42	0x0000000006186186LL
#define CONST_1_20	0x000000000cccccccLL
#define CONST_1_6	0x000000002aaaaaaaLL
static inline sll calc_sin(sll x)
{
	sll retval, x2;
	x2 = sllmul(x, x);
	/*
	 * sin x = t0 + t1 + t2 + t3 + t4 + t5 + t6
	 *
	 * f0 =  0! =  1
	 * f1 =  3! =  3 *  2 * f0 =   6 * f0
	 * f2 =  5! =  5 *  4 * f1 =  20 x f1
	 * f3 =  7! =  7 *  6 * f2 =  42 * f2
	 * f4 =  9! =  9 *  8 * f3 =  72 * f3
	 * f5 = 11! = 11 * 10 * f4 = 110 * f4
	 * f6 = 13! = 13 * 12 * f5 = 156 * f5
	 *
	 * t0 = 1
	 * t1 = -t0 * x2 /   6 = -t0 * x2 * CONST_1_6
	 * t2 = -t1 * x2 /  20 = -t1 * x2 * CONST_1_20
	 * t3 = -t2 * x2 /  42 = -t2 * x2 * CONST_1_42
	 * t4 = -t3 * x2 /  72 = -t3 * x2 * CONST_1_72
	 * t5 = -t4 * x2 / 110 = -t4 * x2 * CONST_1_110
	 * t6 = -t5 * x2 / 156 = -t5 * x2 * CONST_1_156
	 */
	retval = sllsub(x, sllmul(sllmul(x2, CONST_1), CONST_1_156));
	retval = sllsub(x, sllmul(sllmul(x2, retval), CONST_1_110));
	retval = sllsub(x, sllmul(sllmul(x2, retval), CONST_1_72));
	retval = sllsub(x, sllmul(sllmul(x2, retval), CONST_1_42));
	retval = sllsub(x, sllmul(sllmul(x2, retval), CONST_1_20));
	retval = sllsub(x, sllmul(sllmul(x2, retval), CONST_1_6));
	return retval;
}

sll sllcos(sll x)
{
	int i;
	sll retval;

	/* Calculate cos (x - i * pi/2), where -pi/4 <= x - i * pi/2 <= pi/4  */
	i = sll2int(slladd(sllmul(x, CONST_2_PI), CONST_1_2));
	x = sllsub(x, sllmul(int2sll(i), CONST_PI_2));

	switch (i & 3) {
		default:
		case 0:
			retval = calc_cos(x);
			break;
		case 1:
			retval = sllneg(calc_sin(x));
			break;
		case 2:
			retval = sllneg(calc_cos(x));
			break;
		case 3:
			retval = calc_sin(x);
			break;
	}
	return retval;
}

sll sllsin(sll x)
{
	int i;
	sll retval;

	/* Calculate sin (x - n * pi/2), where -pi/4 <= x - i * pi/2 <= pi/4 */
	i = sll2int(slladd(sllmul(x, CONST_2_PI), CONST_1_2));
	x = sllsub(x, sllmul(int2sll(i), CONST_PI_2));

	switch (i & 3) {
		default:
		case 0:
			retval = calc_sin(x);
			break;
		case 1:
			retval = calc_cos(x);
			break;
		case 2:
			retval = sllneg(calc_sin(x));
			break;
		case 3:
			retval = sllneg(calc_cos(x));
			break;
	}
	return retval;
}

sll slltan(sll x)
{
	int i;
	sll retval;
	i = sll2int(slladd(sllmul(x, CONST_2_PI), CONST_1_2));
	x = sllsub(x, sllmul(int2sll(i), CONST_PI_2));
	switch (i & 3) {
		default:
		case 0:
		case 2:
			retval = slldiv(calc_sin(x), calc_cos(x));
			break;
		case 1:
		case 3:
			retval = sllneg(slldiv(calc_cos(x), calc_sin(x)));
			break;
	}
	return retval;
}

/*
 * atan x = SUM[n=0,) (-1)^n * x^(2n + 1)/(2n + 1), |x| < 1
 *
 * Two term approximation
 *	a = x - x^3/3
 * Gives us
 *	atan x = a + ??
 * Let ?? = arctan ?
 *	atan x = a + arctan ?
 * Rearrange
 *	atan x - a = arctan ?
 * Apply tan to both sides
 *	tan (atan x - a) = tan arctan ?
 *	tan (atan x - a) = ?
 * Applying the standard formula
 *	tan (u - v) = (tan u - tan v) / (1 + tan u * tan v)
 * Gives us
 *	tan (atan x - a) = (tan atan x - tan a) / (1 + tan arctan x * tan a)
 * Let t = tan a
 *	tan (atan x - a) = (x - t) / (1 + x * t)
 * So finally
 *	arctan x = a + arctan ((tan x - t) / (1 + x * t))
 * And the typical worst case is x = 1.0 which converges in 3 iterations.
 */

#define CONST_1_3	0x0000000055555555LL

sll calc_atan(sll x)
{
	sll a, t, retval;

	/* First iteration */
	a = sllmul(x, sllsub(CONST_1, sllmul(x, sllmul(x, CONST_1_3))));
	retval = a;

	/* Second iteration */
	t = slldiv(calc_sin(a), calc_cos(a));
	x = slldiv(sllsub(x, t), slladd(CONST_1, sllmul(t, x)));
	a = sllmul(x, sllsub(CONST_1, sllmul(x, sllmul(x, CONST_1_3))));
	retval = slladd(retval, a);

	/* Third  iteration */
	t = slldiv(calc_sin(a), calc_cos(a));
	x = slldiv(sllsub(x, t), slladd(CONST_1, sllmul(t, x)));
	a = sllmul(x, sllsub(CONST_1, sllmul(x, sllmul(x, CONST_1_3))));
	return slladd(retval, a);
}

sll sllatan(sll x)
{
	sll retval;

	if (x < -sllneg(CONST_1))
		retval = sllneg(CONST_PI_2);
	else if (x > CONST_1)
		retval = CONST_PI_2;
	else
		return calc_atan(x);
	return sllsub(retval, calc_atan(sllinv(x)));
}

/*
 * Calculate e^x where x is arbitrary
 */
sll sllexp(sll x)
{
	int i;
	sll e, retval;

	e = CONST_E;

	/* -0.5 <= x <= 0.5  */
	i = sll2int(slladd(x, CONST_1_2));
	retval = calc_exp(sllsub(x, int2sll(i)));

	/* i >= 0 */
	if (i < 0) {
		i = -i;
		e = CONST_1_E;
	}

	/* Scale the result */
	for (;i; i >>= 1) {
		if (i & 1)
			retval = sllmul(retval, e);
		e = sllmul(e, e);
	}
	return retval;
}

/*
 * Calculate natural logarithm using Netwton-Raphson method
 */
sll slllog(sll x)
{
	sll x1, ln = 0;

	/* Scale: e^(-1/2) <= x <= e^(1/2) */
	while (x < CONST_1_SQRTE) {
		ln = sllsub(ln, CONST_1);
		x = sllmul(x, CONST_E);
	}
	while (x > CONST_SQRTE) {
		ln = slladd(ln, CONST_1);
		x = sllmul(x, CONST_1_E);
	}

	/* First iteration */
	x1 = sllmul(sllsub(x, CONST_1), sllmul(sllsub(x, CONST_3), CONST_1_2));
	ln = sllsub(ln, x1);
	x = sllmul(x, calc_exp(x1));

	/* Second iteration */
	x1 = sllmul(sllsub(x, CONST_1), sllmul(sllsub(x, CONST_3), CONST_1_2));
	ln = sllsub(ln, x1);
	x = sllmul(x, calc_exp(x1));

	/* Third iteration */
	x1 = sllmul(sllsub(x, CONST_1), sllmul(sllsub(x, CONST_3), CONST_1_2));
	ln = sllsub(ln, x1);

	return ln;
}

/*
 * ln x^y = y * log x
 * e^(ln x^y) = e^(y * log x)
 * x^y = e^(y * ln x)
 */
sll sllpow(sll x, sll y)
{
	if (y == CONST_0)
		return CONST_1;
	return sllexp(sllmul(y, slllog(x)));
}

/*
 * Consider a parabola centered on the y-axis
 * 	y = a * x^2 + b
 * Has zeros (y = 0)  at
 *	a * x^2 + b = 0
 *	a * x^2 = -b
 *	x^2 = -b / a
 *	x = +- (-b / a)^(1 / 2)
 * Letting a = 1 and b = -X
 *	y = x^2 - X
 *	x = +- X^(1 / 2)
 * Which is convenient since we want to find the square root of X, and we can
 * use Newton's Method to find the zeros of any f(x)
 *	xn = x - f(x) / f'(x)
 * Applied Newton's Method to our parabola
 *	f(x) = x^2 - X
 *	xn = x - (x^2 - X) / (2 * x)
 *	xn = x - (x - X / x) / 2
 * To make this converge quickly, we scale X so that
 *	X = 4^N * z
 * Taking the roots of both sides
 *	X^(1 / 2) = (4^n * z)^(1 / 2)
 *	X^(1 / 2) = 2^n * z^(1 / 2)
 * Let N = 2^n
 *	x^(1 / 2) = N * z^(1 / 2)
 * We want this to converge to the positive root, so we must start at a point
 *	0 < start <= x^(1 / 2)
 * or
 *	x^(1/2) <= start <= infinity
 * since
 *	(1/2)^(1/2) = 0.707
 *	2^(1/2) = 1.414
 * A good choice is 1 which lies in the middle, and takes 4 iterations to
 * converge from either extreme.
 */

#define CONST_4		0x0000000400000000LL
#define CONST_1_4	0x0000000040000000LL

sll sllsqrt(sll x)
{
	sll n, xn;
       
	/* Start with a scaling factor of 1 */
	n = CONST_1;

	/* Quick solutions for the simple cases */
	if (x == CONST_0 || x == CONST_1)
		return x;

	/* Scale x so that 0.5 <= x < 2 */
	while (x >= CONST_2) {
		x = sllmul(x, CONST_1_4);
		n = sllmul(n, CONST_2);
	}
	while (x < CONST_1_2) {
		x = sllmul(x, CONST_4);
		n = sllmul(n, CONST_1_2);
	}

	/* Simple solution if x = 4^n */
	if (x == CONST_1)
		return n;

	/* The starting point */
	xn = CONST_1;

	/* Four iterations will be enough */
	xn = sllsub(xn, sllmul(CONST_1_2, sllsub(xn, slldiv(x, xn))));
	xn = sllsub(xn, sllmul(CONST_1_2, sllsub(xn, slldiv(x, xn))));
	xn = sllsub(xn, sllmul(CONST_1_2, sllsub(xn, slldiv(x, xn))));
	xn = sllsub(xn, sllmul(CONST_1_2, sllsub(xn, slldiv(x, xn))));

	/* Scale the result */
	return sllmul(n, xn);
}
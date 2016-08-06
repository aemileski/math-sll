/*
 * Revision v1.19
 *
 * Credits
 *
 *	Maintained, conceived, written, and fiddled with by:
 *
 *		 Andrew E. Mileski <andrewm@isoar.ca>
 *	
 *	Other source code contributors:
 *
 *		Kevin Rockel
 *		Mark Anthony Lisher
 *		Nicolas Pitre
 *		Anonymous
 *
 * License
 *
 *	Licensed under the terms of the MIT license:
 *
 * Copyright (c) 2000,2002,2006,2012,2016 Andrew E. Mileski <andrewm@isoar.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The copyright notice, and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* See header for full details */
#include "math-sll.h"

/*
 * Local prototypes
 */

static sll _sllatan(sll x);
static sll _sllcos(sll x);
static sll _sllsin(sll x);

static sll _sllexp(sll x);

/*
 * Unpack IEEE floating point double format into fixed point sll format
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

#if defined(__BYTE_ORDER__)&&(__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
	/* support for architectures with big-endian double */
	exp = in.u[0];
	in.u[0] = in.u[1];
	in.u[1] = exp;
#endif /* __ORDER_BIG_ENDIAN__ */

	/* Leading 1 is assumed by IEEE */
	retval.u[1] = 0x40000000;

	/* Unpack the mantissa into the unsigned long */
	retval.u[1] |= (in.u[1] << 10) & 0x3ffffc00;
	retval.u[1] |= (in.u[0] >> 22) & 0x000003ff;
	retval.u[0] = in.u[0] << 10;

	/* Extract the exponent and align the decimals */
	exp = (in.u[1] >> 20) & 0x7ff;
	if (exp)
		retval.ull >>= 1053 - exp;
	else
		return 0L;

	/* Negate if negative flag set */
	if (in.u[1] & 0x80000000)
		retval.sll = -retval.sll;

	return retval.sll;
}

/*
 * Pack fixed point sll format into IEEE floating point double format
 */

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
		in.ull = _sllneg(in.sll);
	} else
		flag = 0x00000000;

	/* Normalize */
	for (exp = 1053; in.ull && (in.u[1] & 0x80000000) == 0; exp--) {
		in.ull <<= 1;
	}
	in.ull <<= 1;
	exp++;
	in.ull >>= 12;
	retval.ull = in.ull;
	retval.u[1] |= flag | (exp << 20);

#if defined(__BYTE_ORDER__)&&(__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
	/* support for architectures with big-endian double */
	exp = retval.u[0];
	retval.u[0] = retval.u[1];
	retval.u[1] = exp;
#endif /* __ORDER_BIG_ENDIAN__ */

	return retval.d;
}

/*
 * Multiply two sll values
 *
 * Description
 *
 *	Let a = A * 2^32 + a_hi * 2^0 + a_lo * 2^(-32)
 *	Let b = B * 2^32 + b_hi * 2^0 + b_lo * 2^(-32)
 *
 * 	Where:
 *
 *	*_hi is the integer part
 *	*_lo the fractional part
 *	A and B are the sign (0 for positive, -1 for negative).
 *
 *	a * b = (A * 2^32 + a_hi * 2^0 + a_lo * 2^-32)
 *		* (B * 2^32 + b_hi * 2^0 + b_lo * 2^-32)
 *
 *	Expanding the terms, we get:
 *
 *	= A * B * 2^64 + A * b_h * 2^32 + A * b_l * 2^0
 *	+ a_h * B * 2^32 + a_h * b_h * 2^0 + a_h * b_l * 2^-32
 *	+ a_l * B * 2^0 + a_l * b_h * 2^-32 + a_l * b_l * 2^-64
 *
 *	Grouping by powers of 2, we get:
 *
 *	= A * B * 2^64
 *	Meaningless overflow from sign extension - ignore
 *
 *	+ (A * b_h + a_h * B) * 2^32
 *	Overflow which we can't handle - ignore
 *
 *	+ (A * b_l + a_h * b_h + a_l * B) * 2^0
 *	We only need the low 32 bits of this term, as the rest is overflow
 *
 *	+ (a_h * b_l + a_l * b_h) * 2^-32
 *	We need all 64 bits of this term
 *
 *	+  a_l * b_l * 2^-64
 *	We only need the high 32 bits of this term, as the rest is underflow
 *
 *	Note that:
 *	a > 0 && b > 0: A =  0, B =  0 and the third term is a_h * b_h
 *	a < 0 && b > 0: A = -1, B =  0 and the third term is a_h * b_h - b_l
 *	a > 0 && b < 0: A =  0, B = -1 and the third term is a_h * b_h - a_l
 *	a < 0 && b < 0: A = -1, B = -1 and the third term is a_h * b_h - a_l - b_l
 */
#if (!defined(__arm__) && !defined(__i386__))
/*
 * Plain C version: not optimal but portable
 */

sll sllmul(sll a, sll b)
{
	unsigned int a_lo;
	unsigned int b_lo;
	signed int a_hi;
	signed int b_hi;
	sll x;

	a_lo = a;
	a_hi = (ull) a >> 32;
	b_lo = b;
	b_hi = (ull) b >> 32;

	x = ((ull) (a_hi * b_hi) << 32)
	  + (((ull) a_lo * b_lo) >> 32)
	  + (sll) a_lo * b_hi
	  + (sll) b_lo * a_hi;

	return x;
}
#endif /* (!defined(__arm__) && !defined(__i386__)) */

/*
 * Calculate cos x where -pi/4 <= x <= pi/4
 *
 * Description
 *
 *	cos x = 1 - x^2 / 2! + x^4 / 4! - ... + x^(2N) / (2N)!
 *	Note that (pi/4)^12 / 12! < 2^-32 which is the smallest possible number.
 *
 *	cos x = t0 + t1 + t2 + t3 + t4 + t5 + t6
 *
 *	f0 =  0! =  1
 *	f1 =  2! =  2 *  1 * f0 =   2 * f0
 *	f2 =  4! =  4 *  3 * f1 =  12 x f1
 *	f3 =  6! =  6 *  5 * f2 =  30 * f2
 *	f4 =  8! =  8 *  7 * f3 =  56 * f3
 *	f5 = 10! = 10 *  9 * f4 =  90 * f4
 *	f6 = 12! = 12 * 11 * f5 = 132 * f5
 *
 *	t0 = 1
 *	t1 = -t0 * x2 /   2 = -t0 * x2 * CONST_1_2
 *	t2 = -t1 * x2 /  12 = -t1 * x2 * CONST_1_12
 *	t3 = -t2 * x2 /  30 = -t2 * x2 * CONST_1_30
 *	t4 = -t3 * x2 /  56 = -t3 * x2 * CONST_1_56
 *	t5 = -t4 * x2 /  90 = -t4 * x2 * CONST_1_90
 *	t6 = -t5 * x2 / 132 = -t5 * x2 * CONST_1_132
 */

sll _sllcos(sll x)
{
	sll retval;
	sll x2;

	x2 = sllmul(x, x);

	retval = _sllsub(CONST_1, sllmul(x2, CONST_1_132));
	retval = _sllsub(CONST_1, sllmul(sllmul(x2, retval), CONST_1_90));
	retval = _sllsub(CONST_1, sllmul(sllmul(x2, retval), CONST_1_56));
	retval = _sllsub(CONST_1, sllmul(sllmul(x2, retval), CONST_1_30));
	retval = _sllsub(CONST_1, sllmul(sllmul(x2, retval), CONST_1_12));
	retval = _sllsub(CONST_1, slldiv2(sllmul(x2, retval)));

	return retval;
}

/*
 * Calculate sin x where -pi/4 <= x <= pi/4
 *
 * Description
 *
 *	sin x = x - x^3 / 3! + x^5 / 5! - ... + x^(2N+1) / (2N+1)!
 *	Note that (pi/4)^13 / 13! < 2^-32 which is the smallest possible number.
 *
 *	sin x = t0 + t1 + t2 + t3 + t4 + t5 + t6
 *
 *	f0 =  0! =  1
 *	f1 =  3! =  3 *  2 * f0 =   6 * f0
 *	f2 =  5! =  5 *  4 * f1 =  20 x f1
 *	f3 =  7! =  7 *  6 * f2 =  42 * f2
 *	f4 =  9! =  9 *  8 * f3 =  72 * f3
 *	f5 = 11! = 11 * 10 * f4 = 110 * f4
 *	f6 = 13! = 13 * 12 * f5 = 156 * f5
 *
 *	t0 = 1
 *	t1 = -t0 * x2 /   6 = -t0 * x2 * CONST_1_6
 *	t2 = -t1 * x2 /  20 = -t1 * x2 * CONST_1_20
 *	t3 = -t2 * x2 /  42 = -t2 * x2 * CONST_1_42
 *	t4 = -t3 * x2 /  72 = -t3 * x2 * CONST_1_72
 *	t5 = -t4 * x2 / 110 = -t4 * x2 * CONST_1_110
 *	t6 = -t5 * x2 / 156 = -t5 * x2 * CONST_1_156
 */

sll _sllsin(sll x)
{
	sll retval;
	sll x2;

	x2 = sllmul(x, x);

	retval = _sllsub(x, sllmul(x2, CONST_1_156));
	retval = _sllsub(x, sllmul(sllmul(x2, retval), CONST_1_110));
	retval = _sllsub(x, sllmul(sllmul(x2, retval), CONST_1_72));
	retval = _sllsub(x, sllmul(sllmul(x2, retval), CONST_1_42));
	retval = _sllsub(x, sllmul(sllmul(x2, retval), CONST_1_20));
	retval = _sllsub(x, sllmul(sllmul(x2, retval), CONST_1_6));

	return retval;
}

/*
 * Calculate cos x for any value of x, by quadrant
 */

sll sllcos(sll x)
{
	int i;
	sll retval;

	/* Calculate cos (x - i * pi/2), where -pi/4 <= x - i * pi/2 <= pi/4  */
	i = _sll2int(_slladd(sllmul(x, CONST_2_PI), CONST_1_2));
	x = _sllsub(x, sllmul(_int2sll(i), CONST_PI_2));

	/* Locate the quadrant */
	switch (i & 3) {
		default:
		case 0:
			retval = _sllcos(x);
			break;
		case 1:
			retval = sllneg(_sllsin(x));
			break;
		case 2:
			retval = sllneg(_sllcos(x));
			break;
		case 3:
			retval = _sllsin(x);
			break;
	}

	return retval;
}

/*
 * Calculate sin x for any value of x, by quadrant
 */

sll sllsin(sll x)
{
	int i;
	sll retval;

	/* Calculate sin (x - n * pi/2), where -pi/4 <= x - i * pi/2 <= pi/4 */
	i = _sll2int(_slladd(sllmul(x, CONST_2_PI), CONST_1_2));
	x = _sllsub(x, sllmul(_int2sll(i), CONST_PI_2));

	/* Locate the quadrant */
	switch (i & 3) {
		default:
		case 0:
			retval = _sllsin(x);
			break;
		case 1:
			retval = _sllcos(x);
			break;
		case 2:
			retval = sllneg(_sllsin(x));
			break;
		case 3:
			retval = sllneg(_sllcos(x));
			break;
	}

	return retval;
}

/*
 * Calculate tan x for any value of x, by quadrant
 */

sll slltan(sll x)
{
	int i;
	sll retval;

	i = _sll2int(_slladd(sllmul(x, CONST_2_PI), CONST_1_2));
	x = _sllsub(x, sllmul(_int2sll(i), CONST_PI_2));

	/* Locate the quadrant */
	switch (i & 3) {
		default:
		case 0:
		case 2:
			retval = slldiv(_sllsin(x), _sllcos(x));
			break;
		case 1:
		case 3:
			retval = _sllneg(slldiv(_sllcos(x), _sllsin(x)));
			break;
	}

	return retval;
}

/*
 * Calculate atan x where |x| < 1
 *
 * Description
 *
 *	atan x = SUM[n=0,) (-1)^n * x^(2n + 1)/(2n + 1), |x| < 1
 *
 *	Using a two term approximation:
 *	a = x - x^3/3
 *
 *	Results in:
 *	atan x = a + ??
 *	where ?? is the difference from the exact result.
 *
 *	Letting ?? = arctan ? results in:
 *	atan x = a + arctan ?
 *
 *	Re-arranging:
 *	atan x - a = arctan ?
 *
 *	Applying tan to both sides:
 *	tan (atan x - a) = tan arctan ?
 *	tan (atan x - a) = ?
 *
 *	Applying the standard identity:
 *	tan (u - v) = (tan u - tan v) / (1 + tan u * tan v)
 *
 *	Results in:
 *	tan (atan x - a) = (tan atan x - tan a) / (1 + tan arctan x * tan a)
 *
 *	Let t = tan a results in:
 *	tan (atan x - a) = (x - t) / (1 + x * t)
 *
 *	So finally
 *	arctan x = a + arctan ((tan x - t) / (1 + x * t))
 *
 *	And the typical worst case is x = 1.0 which converges in 3 iterations.
 */

sll _sllatan(sll x)
{
	sll a;
	sll t;
	sll retval;

	/* First iteration */
	a = sllmul(x, _sllsub(CONST_1, sllmul(x, sllmul(x, CONST_1_3))));
	retval = a;

	/* Second iteration */
	t = slldiv(_sllsin(a), _sllcos(a));
	x = slldiv(_sllsub(x, t), _slladd(CONST_1, sllmul(t, x)));
	a = sllmul(x, _sllsub(CONST_1, sllmul(x, sllmul(x, CONST_1_3))));
	retval = _slladd(retval, a);

	/* Third  iteration */
	t = slldiv(_sllsin(a), _sllcos(a));
	x = slldiv(_sllsub(x, t), _slladd(CONST_1, sllmul(t, x)));
	a = sllmul(x, _sllsub(CONST_1, sllmul(x, sllmul(x, CONST_1_3))));
	return _slladd(retval, a);
}

/*
 * Calculate atan x for any value of x
 */

sll sllatan(sll x)
{
	sll retval;

	if (x < _sllneg(CONST_1))
		/* if x < -1 then atan x = PI/2 + atan 1/x */
		retval = _sllneg(CONST_PI_2);
	else if (x > CONST_1)
		/* if x > 1 then atan x = PI/2 - atan 1/x */
		retval = CONST_PI_2;
	else
		return _sllatan(x);

	return _sllsub(retval, _sllatan(sllinv(x)));
}

/*
 * Calculate e^x where -0.5 <= x <= 0.5
 *
 * Description:
 *	e^x = x^0 / 0! + x^1 / 1! + ... + x^N / N!
 *	Note that 0.5^11 / 11! < 2^-32 which is the smallest possible number.
 */

sll _sllexp(sll x)
{
	sll retval;

	retval = CONST_1;

	retval = _slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_11)));
	retval = _slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_10)));
	retval = _slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_9)));
	retval = _slladd(CONST_1, sllmul(retval, slldiv2n(x, 3)));
	retval = _slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_7)));
	retval = _slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_6)));
	retval = _slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_5)));
	retval = _slladd(CONST_1, sllmul(retval, slldiv4(x)));
	retval = _slladd(CONST_1, sllmul(retval, sllmul(x, CONST_1_3)));
	retval = _slladd(CONST_1, sllmul(retval, slldiv2(x)));
	retval = _slladd(CONST_1, sllmul(retval, x));

	return retval;
}

/*
 * Calculate e^x for any value of x
 */

sll sllexp(sll x)
{
	int i;
	sll e;
	sll retval;

	e = CONST_E;

	/* -0.5 <= x <= 0.5  */
	i = _sll2int(_slladd(x, CONST_1_2));
	retval = _sllexp(_sllsub(x, _int2sll(i)));

	/* i >= 0 */
	if (i < 0) {
		i = -i;
		e = CONST_1_E;
	}

	/* Scale the result */
	for (; i; i >>= 1) {
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
	sll x1;
	sll ln;

	ln = 0;

	/* Scale: e^(-1/2) <= x <= e^(1/2) */
	while (x < CONST_1_SQRTE) {
		ln = _sllsub(ln, CONST_1);
		x = sllmul(x, CONST_E);
	}
	while (x > CONST_SQRTE) {
		ln = _slladd(ln, CONST_1);
		x = sllmul(x, CONST_1_E);
	}

	/* First iteration */
	x1 = sllmul(_sllsub(x, CONST_1), slldiv2(_sllsub(x, CONST_3)));
	ln = _sllsub(ln, x1);
	x = sllmul(x, _sllexp(x1));

	/* Second iteration */
	x1 = sllmul(_sllsub(x, CONST_1), slldiv2(_sllsub(x, CONST_3)));
	ln = _sllsub(ln, x1);
	x = sllmul(x, _sllexp(x1));

	/* Third iteration */
	x1 = sllmul(_sllsub(x, CONST_1), slldiv2(_sllsub(x, CONST_3)));
	ln = _sllsub(ln, x1);

	return ln;
}

/*
 * Calculate the inverse for non-zero values
 */

sll sllinv(sll x)
{
	int sgn;
	sll u;
	ull s;

	/* Use positive numbers, or the approximation won't work */
	if (x < CONST_0) {
		x = _sllneg(x);
		sgn = 1;
	} else {
		sgn = 0;
	}

	/* Starting-point (gets shifted right to become positive) */
	s = -1;

	/* An approximation - must be larger than the actual value */
	for (u = x; u; u = ((ull) u) >> 1)
		s >>= 1;

	/* Newton's Method */
	u = sllmul(s, _sllsub(CONST_2, sllmul(x, s)));
	u = sllmul(u, _sllsub(CONST_2, sllmul(x, u)));
	u = sllmul(u, _sllsub(CONST_2, sllmul(x, u)));
	u = sllmul(u, _sllsub(CONST_2, sllmul(x, u)));
	u = sllmul(u, _sllsub(CONST_2, sllmul(x, u)));
	u = sllmul(u, _sllsub(CONST_2, sllmul(x, u)));

	return ((sgn) ? _sllneg(u): u);
}

/*
 * Calculate x^y
 *
 * Description
 *
 *	The standard identity:
 *	ln x^y = y * log x
 *
 *	Raising e to the power of either sides:
 *	e^(ln x^y) = e^(y * log x)
 *
 *	Which simplifies to:
 *	x^y = e^(y * ln x)
 */

sll sllpow(sll x, sll y)
{
	if (y == CONST_0)
		return CONST_1;

	return sllexp(sllmul(y, slllog(x)));
}

/*
 * Calculate the square-root
 *
 * Description
 *
 *	Consider a parabola centered on the y-axis:
 * 	y = a * x^2 + b
 *
 *	Has zeros (y = 0) located at:
 *	a * x^2 + b = 0
 *	a * x^2 = -b
 *	x^2 = -b / a
 *	x = +- (-b / a)^(1 / 2)
 *
 *	Letting a = 1 and b = -X results in:
 *	y = x^2 - X
 *	x = +- X^(1 / 2)
 *
 *	Which is a convenient result, since we want to find the square root of X,
 *	and we can * use Newton's Method to find the zeros of any f(x):
 *	xn = x - f(x) / f'(x)
 *
 *	Applying Newton's Method to our parabola:
 *	f(x) = x^2 - X
 *	xn = x - (x^2 - X) / (2 * x)
 *	xn = x - (x - X / x) / 2
 *
 *	To make this converge quickly, we scale X so that:
 *	X = 4^N * z
 *
 *	Taking the roots of both sides
 *	X^(1 / 2) = (4^n * z)^(1 / 2)
 *	X^(1 / 2) = 2^n * z^(1 / 2)
 *
 *	Letting N = 2^n results in:
 *	x^(1 / 2) = N * z^(1 / 2)
 *
 *	We want this to converge to the positive root, so we must start at a point
 *	0 < start <= x^(1 / 2)
 * 	or
 *	x^(1/2) <= start <= infinity
 *
 *	Since:
 *	(1/2)^(1/2) = 0.707
 *	2^(1/2) = 1.414
 *
 *	A good choice is 1 which lies in the middle, and takes 4 iterations to
 *	converge from either extreme.
 */

sll sllsqrt(sll x)
{
	sll n;
	sll xn;
       
	/* Quick solutions for the simple cases */
	if (x <= CONST_0 || x == CONST_1)
		return x;

	/* Start with a scaling factor of 1 */
	n = CONST_1;

	/* Scale x so that 0.5 <= x < 2 */
	while (x >= CONST_2) {
		x = slldiv4(x);
		n = sllmul2(n);
	}
	while (x < CONST_1_2) {
		x = sllmul4(x);
		n = slldiv2(n);
	}

	/* Simple solution if x = 4^n */
	if (x == CONST_1)
		return n;

	/* The starting point */
	xn = CONST_1;

	/* Four iterations will be enough */
	xn = _sllsub(xn, slldiv2(_sllsub(xn, slldiv(x, xn))));
	xn = _sllsub(xn, slldiv2(_sllsub(xn, slldiv(x, xn))));
	xn = _sllsub(xn, slldiv2(_sllsub(xn, slldiv(x, xn))));
	xn = _sllsub(xn, slldiv2(_sllsub(xn, slldiv(x, xn))));

	/* Scale the result */
	return sllmul(n, xn);
}

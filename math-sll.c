/*
 * Revision v1.24
 *
 * Credits
 *
 *	Maintained, conceived, written, and fiddled with by:
 *
 *		Andrew E. Mileski <andrewm@isoar.ca>
 *	
 *	Other source code contributors:
 *
 *		Kevin Rockel
 *		Kevin Michael Woley
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

static sll _sllcos(sll x);
static sll _sllsin(sll x);

static sll _sllexp(sll x);

/*
 * Unpack IEEE 754 floating point double format into fixed point sll format
 *
 * Description
 *
 *	IEEE 754 specifies the binary64 type ("double" in C) as having:
 *
 *	1 bit sign
 *	11 bit exponent
 *	53 bit significand
 *
 *	The first bit of the significand is an implied 1 which is not stored.
 *	The decimal would be to the right of that implied 1, or to the left of
 *	the stored significand.
 *
 *	The exponent is unsigned, and biased with an offset of 1023.
 *
 *	The IEEE 754 standard does not specify endianess, but the endian used is
 *	traditionally the same endian that the processor uses.
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

#if defined(BROKEN_IEEE754_DOUBLE)

	exp = in.u[0];
	in.u[0] = in.u[1];
	in.u[1] = exp;

#endif /* defined(BROKEN_IEEE754_DOUBLE) */

	/* Leading 1 is assumed by IEEE */
	retval.u[1] = 0x40000000;

	/* Unpack the mantissa into the unsigned long */
	retval.u[1] |= (in.u[1] << 10) & 0x3ffffc00;
	retval.u[1] |= (in.u[0] >> 22) & 0x000003ff;
	retval.u[0] = in.u[0] << 10;

	/* Extract the exponent and align the decimals */
	exp = (in.u[1] >> 20) & 0x7ff;
	if (exp)
		/* IEEE 754 decimal begins at right of bit position 30 */
		retval.ull >>= (1023 + 30) - exp;
	else
		return 0L;

	/* Negate if negative flag set */
	if (in.u[1] & 0x80000000)
		retval.sll = _sllneg(retval.sll);

	return retval.sll;
}

/*
 * Pack fixed point sll format into IEEE 754 floating point double format
 *
 * Description
 *
 *	IEEE 754 specifies the binary64 type ("double" in C) as having:
 *
 *	1 bit sign
 *	11 bit exponent
 *	53 bit significand
 *
 *	The first bit of the significand is an implied 1 which is not stored.
 *	The decimal would be to the right of that implied 1, or to the left of
 *	the stored significand.
 *
 *	The exponent is unsigned, and biased with an offset of 1023.
 *
 *	The IEEE 754 standard does not specify endianess, but the endian used is
 *	traditionally the same endian that the processor uses.
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

	/*
	 * Normalize
	 *
	 * IEEE 754 decimal-point begins at right of bit position 30
	 */
	for (exp = (1023 + 30); in.ull && (in.u[1] & 0x80000000) == 0; exp--) {
		in.ull <<= 1;
	}
	in.ull <<= 1;
	exp++;
	in.ull >>= 12;
	retval.ull = in.ull;
	retval.u[1] |= flag | (exp << 20);

#if defined(BROKEN_IEEE754_DOUBLE)

	exp = retval.u[0];
	retval.u[0] = retval.u[1];
	retval.u[1] = exp;

#endif /* defined(BROKEN_IEEE754_DOUBLE)  */

	return retval.d;
}

/*
 * Multiply two sll values
 *
 * Description
 *
 *	When multiplying two 64 bit sll numbers, the result is 128 bits, but there
 *	is only room for a 64 bit result with sll!
 *
 *	The 128 bit result has 64 bits on either side of the decimal, so 32 bits
 *	of overflow to the left of the decimal, and 32 bits of underflow to the
 *	right of the decmial.
 *
 *	32.32 * 32.32 = 64.64 = overflow(32) + 32.32 + underflow(32)
 *
 *	However, a "long long" multiply has 64 bits of overflow to the left of the
 *	decimal, resulting in the entire integer part being lost!
 *
 *	32.32 * 32.32 = 64.64 = overflow(64) + .64
 *
 *	Hence a custom multiply routine is required, to preserve the parts
 *	of the result that sll needs.
 *
 *	Consider two sll numbers, x and y:
 *
 *	Let x = x_hi * 2^0 + x_lo * 2^(-32)
 *	Let y = y_hi * 2^0 + y_lo * 2^(-32)
 *
 * 	Where:
 *
 *	*_hi is the signed 32 bit integer part to the left of the decimal
 *	*_lo is the unsigned 32 bit fractional part to the right of the decimal
 *
 *	x * y = (x_hi * 2^0 + x_lo * 2^(-32))
 *	      * (y_hi * 2^0 + y_lo * 2^(-32))
 *
 *	Expanding the terms, we get:
 *
 *	= x_hi * y_hi * 2^0 + x_hi * y_lo * 2^(-32)
 *	+ x_lo * y_hi * 2^(-32) + x_lo * y_lo * 2^(-64)
 *
 *	Grouping by powers of 2, we get:
 *
 *	(x_hi * y_hi) * 2^0
 *	We only need the low 32 bits of this term, as the rest is overflow
 *
 *	(x_hi * y_lo + x_lo * y_hi) * 2^-32
 *	We need all bits of this term
 *
 *	x_lo * y_lo * 2^-64
 *	We only need the high 32 bits of this term, as the rest is underflow
 */
#if !defined(HAVE_SLLMUL)

sll sllmul(sll x, sll y)
{
	register unsigned int x_lo;
	register signed int x_hi;

	register unsigned int y_lo;
	register signed int y_hi;

	x_hi = (signed int) ((ull) x >> 32);	// Discard lower 32 bits
	x_lo = (unsigned int) x;		// Discard upper 32 bits

	y_hi = (signed int) ((ull) y >> 32);	// Discard lower 32 bits
	y_lo = (unsigned int) y;		// Discard upper 32 bits

	return (sll) (
		  ((ull) (x_hi * y_hi) << 32)
		+ ((ull) x_hi * y_lo + x_lo * (ull) y_hi)
		+ (((ull) x_lo * y_lo) >> 32)
	);
}

#endif /* !defined(HAVE_SLLMUL)! */

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
 *	Consider only the factorials:
 *	f0 =  0! =  1
 *	f1 =  2! =  2 *  1 * f0 =   2 * f0
 *	f2 =  4! =  4 *  3 * f1 =  12 * f1
 *	f3 =  6! =  6 *  5 * f2 =  30 * f2
 *	f4 =  8! =  8 *  7 * f3 =  56 * f3
 *	f5 = 10! = 10 *  9 * f4 =  90 * f4
 *	f6 = 12! = 12 * 11 * f5 = 132 * f5
 *
 *	Now consider each term of the series:
 *	t0 = 1
 *	t1 = -t0 * x^2 / f1 = -t0 * x^2 * CONST_1_2
 *	t2 = -t1 * x^2 / f2 = -t1 * x^2 * CONST_1_12
 *	t3 = -t2 * x^2 / f3 = -t2 * x^2 * CONST_1_30
 *	t4 = -t3 * x^2 / f4 = -t3 * x^2 * CONST_1_56
 *	t5 = -t4 * x^2 / f5 = -t4 * x^2 * CONST_1_90
 *	t6 = -t5 * x^2 / f6 = -t5 * x^2 * CONST_1_132
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
 *	Consider only the factorials:
 *	f0 =  0! =  1
 *	f1 =  3! =  3 *  2 * f0 =   6 * f0
 *	f2 =  5! =  5 *  4 * f1 =  20 * f1
 *	f3 =  7! =  7 *  6 * f2 =  42 * f2
 *	f4 =  9! =  9 *  8 * f3 =  72 * f3
 *	f5 = 11! = 11 * 10 * f4 = 110 * f4
 *	f6 = 13! = 13 * 12 * f5 = 156 * f5
 *
 *	Now consider each term of the series:
 *	t0 = 1
 *	t1 = -t0 * x^2 /   6 = -t0 * x^2 * CONST_1_6
 *	t2 = -t1 * x^2 /  20 = -t1 * x^2 * CONST_1_20
 *	t3 = -t2 * x^2 /  42 = -t2 * x^2 * CONST_1_42
 *	t4 = -t3 * x^2 /  72 = -t3 * x^2 * CONST_1_72
 *	t5 = -t4 * x^2 / 110 = -t4 * x^2 * CONST_1_110
 *	t6 = -t5 * x^2 / 156 = -t5 * x^2 * CONST_1_156
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
 *
 * Calculate asin x, where |x| <= 1
 *
 * Description
 *
 *	asin x = SUM[n=0,) C(2 * n, n) * x^(2 * n + 1) / (4^n * (2 * n + 1)), |x| <= 1
 *
 *	where C(n, r) = nCr = n! / (r! * (n - r)!)
 *
 *	Using a two term approximation:
 * [1]	a = x + x^3 / 6
 *
 *	Results in:
 *	asin x = a + D
 *	where D is the difference from the exact result
 *
 *	Letting D = asin d results in:
 * [2]	asin x = a + asin d
 *
 *	Re-arranging:
 *	asin x - a = asin d
 *
 *	Applying sin to both sides:
 *	sin (asin x - a) = sin asin d
 *	sin (asin x - a) = d
 *	d = sin (asin x - a)
 *
 *	Applying the standard identity:
 *	sin (u - v) = sin u * cos v - cos u * sin v
 *
 *	Results in:
 *	d = sin asin x * cos a - cos asin x * sin a
 *	d = x * cos a - cos asin x * sin a
 *
 *	Applying the standard identity:
 *	cos asin u = (1 - u^2)^(1 / 2)
 *
 *	Results in:
 * [3]	d = x * cos a - (1 - x^2)^(1 / 2) * sin a
 *
 *	Putting the pieces together:
 * [1]	a = x + x^3 / 6
 * [3]	d = x * cos a - (1 - x^2)^(1 / 2) * sin a
 * [2]	asin x = a + asin d
 *
 *	The worst case is x = 1.0 which converges after 2 iterations.
 */

sll sllasin(sll x)
{
	int left_side;
	sll a;
	sll retval;

	/* asin -x = -asin x */
	if ((left_side = x < 0))
		x = _sllneg(x);

	/* Out-of-range */
	if (x > CONST_1)
		return 0;

	/* Initial approximate */
	a = sllmul(x, _slladd(CONST_1, sllmul(x, sllmul(x, CONST_1_6))));
	retval = a;

	/* First iteration */
	x = _sllsub(sllmul(x, sllcos(a)), sllmul(sllsqrt(_sllsub(CONST_1, sllmul(x, x))), sllsin(a)));
	a = sllmul(x, _slladd(CONST_1, sllmul(x, sllmul(x, CONST_1_6))));
	retval = _slladd(retval, a);

	/* Second iteration */
	x = _sllsub(sllmul(x, sllcos(a)), sllmul(sllsqrt(_sllsub(CONST_1, sllmul(x, x))), sllsin(a)));
	a = sllmul(x, _slladd(CONST_1, sllmul(x, sllmul(x, CONST_1_6))));
	retval = _slladd(retval, a);

	/* Negate result if necessary */
	return (left_side ? _sllneg(retval): retval);
}

/*
 * Calculate atan x
 *
 * Description
 *
 *	atan x = SUM[n=0,) (-1)^n * x^(2  * n + 1) / (2 * n + 1), |x| <= 1
 *
 *	Using a two term approximation:
 * [1]	a = x - x^3 / 3
 *
 *	Results in:
 * 	atan x = a + D
 *	where D is the difference from the exact result
 *
 *	Letting D = atan d results in:
 * [2]	atan x = a + atan d
 *
 *	Re-arranging:
 *	atan x - a = atan d
 *
 *	Applying tan to both sides:
 *	tan (atan x - a) = tan atan d
 *	tan (atan x - a) = d
 * 	d = tan (atan x - a)
 *
 *	Applying the standard identity:
 *	tan (u - v) = (tan u - tan v) / (1 + tan u * tan v)
 *
 *	Results in:
 *	d = tan (atan x - a) = (tan atan x - tan a) / (1 + tan atan x * tan a)
 * 	d = tan (atan x - a) = (x - tan a) / (1 + x * tan a)
 *
 *	Let:
 * [3]	t = tan a
 *
 *	Results in:
 * [4]	d = (x - t) / (1 + x * t)
 *
 *	So putting the pieces together:
 * [1]	a = x - x^3 / 3
 * [3]	t = tan a
 * [4]	d = (x - t) / (1 + x * t)
 * [2]	atan x = a + atan d
 * 	atan x = a + atan ((x - t) / (1 + x * t))
 *
 *	The worst case is x = 1.0 which converges after 2 iterations.
 */

sll sllatan(sll x)
{
	int side;
	sll a;
	sll t;
	sll retval;


	if (x < CONST_1) {

		/* Left:  if (x < -1) then atan x = pi / 2 + atan 1 / x */
		side = -1;
		x = sllinv(x);

	} else if (x > CONST_1) {

		/* Right:  if (x > 1) then atan x = pi / 2 - atan 1 / x */
		side = 1;
		x = sllinv(x);

	} else {
		/* Middle:  -1 <= x <= 1 */
		side = 0;
	}

	/* Initial approximate */
	a = sllmul(x, _sllsub(CONST_1, sllmul(x, sllmul(x, CONST_1_3))));
	retval = a;

	/* First iteration */
	t = _slldiv(_sllsin(a), _sllcos(a));
	x = _slldiv(_sllsub(x, t), _slladd(CONST_1, sllmul(x, t)));
	a = sllmul(x, _sllsub(CONST_1, sllmul(x, sllmul(x, CONST_1_3))));
	retval = _slladd(retval, a);

	/* Second iteration */
	t = _slldiv(_sllsin(a), _sllcos(a));
	x = _slldiv(_sllsub(x, t), _slladd(CONST_1, sllmul(x, t)));
	a = sllmul(x, _sllsub(CONST_1, sllmul(x, sllmul(x, CONST_1_3))));
	retval =  _slladd(retval, a);

	if (side == -1) {

		/* Left:  if (x < -1) then atan x = pi / 2 + atan 1 / x */
		retval = _slladd(CONST_PI_2, retval);

	} else if (side == 1) {

		/* Right:  if (x > 1) then atan x = pi / 2 - atan 1 / x */
		retval = _sllsub(CONST_PI_2, retval);
	}

	return retval;
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

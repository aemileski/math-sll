#if !defined(MATH_SLL_H)
#  define MATH_SLL_H
/*
 * Revision v1.24
 *
 *	A fixed point (32.32 bit) math library.
 *
 * Description
 *
 *	Floating point packs the most accuracy in the available bits, but it
 *	often provides more accuracy than is required.  It is time consuming to
 *	carry the extra precision around, particularly on platforms that don't
 *	have a dedicated floating point processor.
 *
 *	This library is a compromise.  All math is done using the 64 bit signed
 *	"long long" format (sll), and is not intended to be portable, just as
 *	simple and as fast as possible.
 *
 *	As some processors lack division instructions but have multiplication
 *	instructions, multiplication is favored over division.  This can be a
 *	penalty when used on a processor with a division instruction, so it is
 *	recommended to modify the division functions and macros in that case.
 *
 *	On procesors without multiplication instructions, other algorithms, for
 *	example CORDIC, are probably faster.
 *
 *	Since "long long" is a elementary type, it can be passed around without
 *	resorting to the use of pointers.  Since the format used is fixed point,
 *	there is never a need to do time consuming checks and adjustments to
 *	maintain normalized numbers, as is the case in floating point.
 *
 *	Simply put, this library is limited to handling numbers with a whole
 *	part of up to 2^31 - 1 = 2.147483647e9 in magnitude, and fractional
 *	parts down to 2^-32 = 2.3283064365e-10 in magnitude.  This yields a
 *	decent range and accuracy for many applications.
 *
 * IMPORTANT
 *
 *	No checking for arguments out of range (error).
 *	No checking for divide by zero (error).
 *	No checking for overflow (error).
 *	No checking for underflow (warning).
 *	Chops, doesn't round.
 *
 * Functions
 *
 *	sll dbl2sll(double d)			double to sll
 *	double sll2dbl(sll s)			sll to double
 *
 *	sll int2sll(int i)			integer to sll
 *	int sll2int(sll s)			sll to integer
 *
 *	sll sllint(sll s)			Set fractional-part to 0
 *	sll sllfrac(sll s)			Set integer-part to 0
 *
 *	sll slladd(sll x, sll y)		x + y
 *	sll sllneg(sll x)			-x
 *	sll sllsub(sll x, sll y)		x - y
 *
 *	sll sllmul(sll x, sll y)		x * y
 *	sll sllmul2(sll x)			x * 2
 *	sll sllmul2n(sll x, int n)		x * 2^n, 0 <= n <= 31
 *	sll sllmul4(sll x)			x * 4
 *
 *	sll slldiv(sll x, sll y)		x / y
 *	sll slldiv2(sll x)			x / 2
 *	sll slldiv2n(sll x, int n)		x / 2^n, 0 <= n <= 31
 *	sll slldiv4(sll x)			x / 4
 *
 *	sll sllcos(sll x)			cos x
 *	sll sllsin(sll x)			sin x
 *	sll slltan(sll x)			tan x
 *
 *	sll sllsec(sll x)			sec x = 1 / cos x
 *	sll sllcsc(sll x)			csc x = 1 / sin x
 *	sll sllcot(sll x)			cot x = 1 / tan x = cos x / sin x
 *
 *	sll sllacos(sll x)			acos x
 *	sll sllasin(sll x)			asin x
 *	sll sllatan(sll x)			atan x
 *
 *	sll sllcosh(sll x)			cosh x
 *	sll sllsinh(sll x)			sinh x
 *	sll slltanh(sll x)			tanh x
 *
 *	sll sllsech(sll x)			sech x
 *	sll sllcsch(sll x)			cosh x
 *	sll sllcoth(sll x)			coth x
 *
 *	sll sllexp(sll x)			e^x
 *	sll slllog(sll x)			ln x
 *
 *	sll sllinv(sll v)			1 / x
 *	sll sllpow(sll x, sll y)		x^y
 *	sll sllsqrt(sll x)			x^(1 / 2)
 *
 *	sll sllfloor(sll x)			floor x
 *	sll sllceil(sll x)			ceiling x
 *
 * Macros
 *
 *	Use of the following macros is optional, but may be beneficial with
 *	some compilers.  Using the non-macro versions is strongly recommended.
 *
 *	WARNING:  macros do not type-check their arguments!
 *
 *	_int2sll(X)				See function int2sll()
 *	_sll2int(X)				See function sll2int()
 *
 *	_sllint(X)				See function sllint()
 *	_sllfrac(X)				See function sllfrac()
 *
 *	_slladd(X,Y)				See function slladd()
 *	_sllneg(X)				See function sllneg()
 *	_sllsub(X,Y)				See function sllsub()
 *
 *	_sllmul2(X)				See function sllmul2()
 *	_sllmul2n(X)				See function sllmul2n()
 *	_sllmul4(X)				See function sllmul4()
 *
 *	_slldiv(X,Y)				See function slldiv()
 *	_slldiv2(X,Y)				See function slldiv2()
 *	_slldiv2n(X,Y)				See function slldiv2n()
 *	_slldiv4(X,Y)				See function slldiv4()
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

#if !defined(__GNUC__)
#  error Requires support for type long long (64 bits)
#endif

/* DEC SA-110 "StrongARM" (armv4l) architecture has a big-endian double */
#if defined(__arm__)
#  if (!defined(__BYTE_ORDER__) || (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
#	define BROKEN_IEEE754_DOUBLE
#	warn Assuming big-endian double
#  endif
#endif

/*
 * Data types
 */

__extension__ typedef signed long long sll;
__extension__ typedef unsigned long long  ull;

/*
 * Function prototypes
 */

sll dbl2sll(double d);
double sll2dbl(sll s);

static __inline__ sll int2sll(int i);
static __inline__ int sll2int(sll s);

static __inline__ sll sllint(sll s);
static __inline__ sll sllfrac(sll s);

static __inline__ sll slladd(sll x, sll y);
static __inline__ sll sllneg(sll s);
static __inline__ sll sllsub(sll x, sll y);

#if (defined(__arm__) || defined(__i386__))
#  define HAVE_SLLMUL
static __inline__ sll sllmul(sll x, sll y);
#else
#  undef HAVE_SLLMUL
sll sllmul(sll x, sll y);
#endif /* (defined(__arm__) || defined(__i386__)) */
static __inline__ sll sllmul2(sll x);
static __inline__ sll sllmul4(sll x);
static __inline__ sll sllmul2n(sll x, int n);

static __inline__ sll slldiv(sll x, sll y);
static __inline__ sll slldiv2(sll x);
static __inline__ sll slldiv4(sll x);
static __inline__ sll slldiv2n(sll x, int n);

sll sllcos(sll x);
sll sllsin(sll x);
sll slltan(sll x);

static __inline__ sll sllacos(sll x);
sll sllasin(sll x);
sll sllatan(sll x);

static __inline__ sll sllsec(sll x);
static __inline__ sll sllcsc(sll x);
static __inline__ sll sllcot(sll x);

static __inline__ sll sllcosh(sll x);
static __inline__ sll sllsinh(sll x);
static __inline__ sll slltanh(sll x);

static __inline__ sll sllsech(sll x);
static __inline__ sll sllcsch(sll x);
static __inline__ sll sllcoth(sll x);

sll sllexp(sll x);
sll slllog(sll x);

sll sllpow(sll x, sll y);
sll sllinv(sll v);
sll sllsqrt(sll x);

static __inline__ sll sllfloor(sll x);
static __inline__ sll sllceil(sll x);

/*
 * Macros
 *
 * WARNING - Macros don't type-check!
 */

#define _int2sll(X)	(((sll) (X)) << 32)
#define _sll2int(X)	((int) ((X) >> 32))

#define _sllint(X)	((X) & 0xffffffff00000000LL)
#define _sllfrac(X)	((X) & 0x00000000ffffffffLL)

#define _slladd(X,Y)	((X) + (Y))
#define _sllneg(X)	(-(X))
#define _sllsub(X,Y)	((X) - (Y))

#define _sllmul2(X)	((X) << 1)
#define _sllmul4(X)	((X) << 2)
#define _sllmul2n(X,N)	((X) << (N))

#define _slldiv(X,Y)	sllmul((X), sllinv(Y))
#define _slldiv2(X)	((X) >> 1)
#define _slldiv4(X)	((X) >> 2)
#define _slldiv2n(X,N)	((X) >> (N))

/*
 * Constants (converted from double)
 */

#define CONST_0		0x0000000000000000LL	// 0.0
#define CONST_1		0x0000000100000000LL	// 1.0
#define CONST_2		0x0000000200000000LL 	// 2.0
#define CONST_3		0x0000000300000000LL	// 3.0
#define CONST_4		0x0000000400000000LL	// 4.0
#define CONST_10	0x0000000a00000000LL	// 10.0

#define CONST_1_2	0x0000000080000000LL	// 1.0 / 2.0
#define CONST_1_3	0x0000000055555555LL	// 1.0 / 3.0
#define CONST_1_4	0x0000000040000000LL	// 1.0 / 4.0
#define CONST_1_5	0x0000000033333333LL	// 1.0 / 5.0
#define CONST_1_6	0x000000002aaaaaaaLL	// 1.0 / 6.0
#define CONST_1_7	0x0000000024924924LL	// 1.0 / 7.0
#define CONST_1_8	0x0000000020000000LL	// 1.0 / 8.0
#define CONST_1_9	0x000000001c71c71cLL	// 1.0 / 9.0
#define CONST_1_10	0x0000000019999999LL	// 1.0 / 10.0
#define CONST_1_11	0x000000001745d174LL	// 1.0 / 11.0
#define CONST_1_12	0x0000000015555555LL	// 1.0 / 12.0
#define CONST_1_20	0x000000000cccccccLL	// 1.0 / 20.0
#define CONST_1_30	0x0000000008888888LL	// 1.0 / 30.0
#define CONST_1_42	0x0000000006186186LL	// 1.0 / 42.0
#define CONST_1_56	0x0000000004924924LL	// 1.0 / 56.0
#define CONST_1_72	0x00000000038e38e3LL	// 1.0 / 72.0
#define CONST_1_90	0x0000000002d82d82LL	// 1.0 / 90.0
#define CONST_1_110	0x000000000253c825LL	// 1.0 / 110.0
#define CONST_1_132	0x0000000001f07c1fLL	// 1.0 / 132.0
#define CONST_1_156	0x0000000001a41a41LL	// 1.0 / 156.0

#define CONST_E		0x00000002b7e15162LL	// E
#define CONST_1_E	0x000000005e2d58d8LL	// 1 / E
#define CONST_SQRTE	0x00000001a61298e1LL	// sqrt(E)
#define CONST_1_SQRTE	0x000000009b4597e3LL	// 1 / sqrt(E)
#define CONST_LOG2_E	0x0000000171547652LL	// ln(E)
#define CONST_LOG10_E	0x000000006f2dec54LL	// log(E)
#define CONST_LN2	0x00000000b17217f7LL	// ln(2)
#define CONST_LN10	0x000000024d763776LL	// ln(10)

#define CONST_PI	0x00000003243f6a88LL	// PI
#define CONST_PI_2	0x00000001921fb544LL	// PI / 2
#define CONST_PI_4	0x00000000c90fdaa2LL	// PI / 4
#define CONST_1_PI	0x00000000517cc1b7LL	// 1 / PI
#define CONST_2_PI	0x00000000a2f9836eLL	// 2 / PI
#define CONST_2_SQRTPI	0x0000000120dd7504LL	// 2 / sqrt(PI)
#define CONST_SQRT2	0x000000016a09e667LL	// sqrt(2)
#define CONST_1_SQRT2	0x00000000b504f333LL	// 1 / sqrt(2)

#define CONST_FACT_0	0x0000000100000000LL	// 0!
#define CONST_FACT_1	0x0000000100000000LL	// 1!
#define CONST_FACT_2	0x0000000200000000LL	// 2!
#define CONST_FACT_3	0x0000000600000000LL	// 3!
#define CONST_FACT_4	0x0000001800000000LL	// 4!
#define CONST_FACT_5	0x0000007800000000LL	// 5!
#define CONST_FACT_6	0x000002d000000000LL	// 6!
#define CONST_FACT_7	0x000013b000000000LL	// 7!
#define CONST_FACT_8	0x00009d8000000000LL	// 8!
#define CONST_FACT_9	0x0005898000000000LL	// 9!
#define CONST_FACT_10	0x00375f0000000000LL	// 10!
#define CONST_FACT_11	0x0261150000000000LL	// 11!
#define CONST_FACT_12	0x1c8cfc0000000000LL	// 12!

/*
 * Convert integer to sll
 */

static __inline__ sll int2sll(int i)
{
	return _int2sll(i);
}

/*
 * Convert sll to integer (truncates)
 */

static __inline__ int sll2int(sll s)
{
	return _sll2int(s);
}

/*
 * Integer-part of sll (fractional-part set to 0)
 */

static __inline__ sll sllint(sll s)
{
	return _sllint(s);
}

/*
 * Fractional-part of sll (integer-part set to 0)
 */

static __inline__ sll sllfrac(sll s)
{
	return _sllfrac(s);
}

/*
 * Addition
 */

static __inline__ sll slladd(sll x, sll y)
{
	return _slladd(x, y);
}

/*
 * Negate
 */

static __inline__ sll sllneg(sll s)
{
	return _sllneg(s);
}

/*
 * Subtraction
 */

static __inline__ sll sllsub(sll x, sll y)
{
	return _sllsub(x, y);
}

/*
 * Multiply two sll values
 *
 * Description
 *
 *	Let a = A * 2^32 + a_h * 2^0 + a_l * 2^(-32)
 *	Let b = B * 2^32 + b_h * 2^0 + b_l * 2^(-32)
 *
 * 	Where:
 *
 *	*_h is the integer part
 *	*_l the fractional part
 *	A and B are the sign (0 for positive, -1 for negative).
 *
 *	a * b = (A * 2^32 + a_h * 2^0 + a_l * 2^-32)
 *		* (B * 2^32 + b_h * 2^0 + b_l * 2^-32)
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

#if defined(__arm__)

static __inline__ sll sllmul(sll x, sll y)
{
	/*
	 * From gcc/config/arm/arm.h:
	 *   In a pair of registers containing a DI or DF value the 'Q'
	 *   operand returns the register number of the register containing
	 *   the least significant part of the value.  The 'R' operand returns
	 *   the register number of the register containing the most
	 *   significant part of the value.
	 */
	register sll retval;

	__asm__ (
		"@ sllmul\n\t"
		"umull	%R0, %Q0, %Q1, %Q2\n\t"
		"mul	%R0, %R1, %R2\n\t"
		"umlal	%Q0, %R0, %Q1, %R2\n\t"
		"umlal	%Q0, %R0, %Q2, %R1\n\t"
		"tst	%R1, #0x80000000\n\t"
		"subne	%R0, %R0, %Q2\n\t"
		"tst	%R2, #0x80000000\n\t"
		"subne	%R0, %R0, %Q1\n\t"
		: "=&r" (retval)
		: "%r" (x), "r" (y)
		: "cc"
	);

	return retval;
}

#elif defined(__i386__)

static __inline__ sll sllmul(sll x, sll y)
{
	register sll retval;

	__asm__(
		"# sllmul\n\t"
		"	movl	%1, %%eax\n\t"
		"	mull 	%3\n\t"
		"	movl	%%edx, %%ebx\n\t"
		"\n\t"
		"	movl	%2, %%eax\n\t"
		"	mull 	%4\n\t"
		"	movl	%%eax, %%ecx\n\t"
		"\n\t"
		"	movl	%1, %%eax\n\t"
		"	mull	%4\n\t"
		"	addl	%%eax, %%ebx\n\t"
		"	adcl	%%edx, %%ecx\n\t"
		"\n\t"
		"	movl	%2, %%eax\n\t"
		"	mull	%3\n\t"
		"	addl	%%ebx, %%eax\n\t"
		"	adcl	%%ecx, %%edx\n\t"
		"\n\t"
		"	btl	$31, %2\n\t"
		"	jnc	1f\n\t"
		"	subl	%3, %%edx\n\t"
		"1:	btl	$31, %4\n\t"
		"	jnc	1f\n\t"
		"	subl	%1, %%edx\n\t"
		"1:\n\t"
		: "=&A" (retval)
		: "m" (x), "m" (((unsigned *) &x)[1]),
		  "m" (y), "m" (((unsigned *) &y)[1])
		: "ebx", "ecx", "cc"
	);

	return retval;
}

#else

/*
 * Plain C version:  not optimal but portable
 */

sll sllmul(sll a, sll b);

#endif

/*
 * Multiplication by 2
 */

static __inline__ sll sllmul2(sll x)
{
	return _sllmul2(x);
}

/*
 * Multiplication by 4
 */

static __inline__ sll sllmul4(sll x)
{
	return _sllmul4(x);
}

/*
 * Multiplication by power of 2
 */

static __inline__ sll sllmul2n(sll x, int n)
{
#if defined(__arm__)

	register sll y;

	/*
	 * On ARM we need to do explicit assembly since the compiler
	 * doesn't know the range of n is limited and decides to call
	 * a library function instead.
	 */
	__asm__ (
		"@ sllmul2n\n\t"
		"mov	%R0, %R1, lsl %2\n\t"
		"orr	%R0, %R0, %Q1, lsr %3\n\t"
		"mov	%Q0, %Q1, lsl %2\n\t"
		: "=r" (y)
		: "r" (x), "rM" (n), "rM" (32 - n)
	);

	return y;

#else

	return _sllmul2n(x, n);

#endif
}

/*
 * Division
 */

static __inline__ sll slldiv(sll x, sll y)
{
	return _slldiv(x, y);
}

/*
 * Division by 2
 */

static __inline__ sll slldiv2(sll x)
{
	return _slldiv2(x);
}

/*
 * Division by 4
 */

static __inline__ sll slldiv4(sll x)
{
	return _slldiv4(x);
}

/*
 * Division by power of 2
 */

static __inline__ sll slldiv2n(sll x, int n)
{
#if defined(__arm__)

	register sll y;

	/*
	 * On ARM we need to do explicit assembly since the compiler
	 * doesn't know the range of n is limited and decides to call
	 * a library function instead.
	 */
	__asm__ (
		"@ slldiv2n\n\t"
		"mov	%Q0, %Q1, lsr %2\n\t"
		"orr	%Q0, %Q0, %R1, lsl %3\n\t"
		"mov	%R0, %R1, asr %2\n\t"
		: "=r" (y)
		: "r" (x), "rM" (n), "rM" (32 - n)
	);

	return y;

#else

	return _slldiv2n(x, n);

#endif
}

/*
 *
 * Calculate acos x, where |x| <= 1
 *
 * Description
 *
 *	acos x = pi / 2 - asin x
 *	acos x = pi / 2 - SUM[n=0,) C(2 * n, n) * x^(2 * n + 1) / (4^n * (2 * n + 1)), |x| <= 1
 *
 *	where C(n, r) = nCr = n! / (r! * (n - r)!)
 */

static __inline__ sll sllacos(sll x)
{
	return _sllsub(CONST_PI_2, sllasin(x));
}

/*
 * Trigonometric secant
 *
 * Description
 *
 *	sec x = 1 / cos x
 *
 * An alternate algorithm, like a power series, would be more accurate.
 */

static __inline__ sll sllsec(sll x)
{
	return sllinv(sllcos(x));
}

/*
 * Trigonometric cosecant
 *
 * Description
 *
 *	csc x = 1 / sin x
 *
 * An alternate algorithm, like a power series, would be more accurate.
 */

static __inline__ sll sllcsc(sll x)
{
	return sllinv(sllsin(x));
}

/*
 * Trigonometric cotangent
 *
 * Description
 *
 *	cot x = 1 / tan x
 *
 *	cot x = cos x / sin x
 *
 * An alternate algorithm, like a power series, would be more accurate.
 */

static __inline__ sll sllcot(sll x)
{
	return _slldiv(sllcos(x), sllsin(x));
}

/*
 * Hyperbolic cosine
 *
 * Description
 *
 *	cosh x = (e^x + e^(-x)) / 2
 *
 *	cosh x = 1 + x^2 / 2! + ... + x^(2 * N) / (2 * N)!
 *
 * An alternate algorithm, like a power series, would be more accurate.
 */

static __inline__ sll sllcosh(sll x)
{
	return _slldiv2(_slladd(sllexp(x), sllexp(_sllneg(x))));
}

/*
 * Hyperbolic sine
 *
 * Description
 *
 *	sinh x = (e^x - e^(-x)) / 2
 *
 *	sinh x = 1 + x^3 / 3! + ... + x^(2 * N + 1) / (2 * N + 1)!
 *
 * An alternate algorithm, like a power series, would be more accurate.
 */

static __inline__ sll sllsinh(sll x)
{
	return _slldiv2(_sllsub(sllexp(x), sllexp(_sllneg(x))));
}

/*
 * Hyperbolic tangent
 *
 * Description
 *
 *	tanh x = sinh x / cosh x
 *
 *	tanh x = (e^x - e^(-x)) / (e^x + e^(-x))
 *
 *	tanh x = (e^(2 * x) - 1) / (e^(2 * x) + 1)
 *
 * An alternate algorithm, like a power series, would be more accurate.
 */

static __inline__ sll slltanh(sll x)
{
	register sll e2x;

	e2x = sllexp(_sllmul2(x));

	return _slldiv(_sllsub(e2x, CONST_1), _slladd(e2x, CONST_1));
}

/*
 * Hyperbolic secant
 *
 * Description
 *
 *	sech x = 1 / cosh x
 *
 *	sech x = 2 / (e^x + e^(-x))
 *
 *	sech x = 2 * e^x / (e^(2 * x) + 1)
 *
 * An alternate algorithm, like a power series, would be more accurate.
 */

static __inline__ sll sllsech(sll x)
{
	return _slldiv(_sllmul2(sllexp(x)), _slladd(sllexp(_sllmul2(x)), CONST_1));
}

/*
 * Hyperbolic cosecant
 *
 * Description
 *
 *	csch x = = 1 / sinh x
 *
 *	csch x = 2 / (e^x - e^(-x))
 *
 *	csch x = 2 * e^x / (e^(2 * x) - 1)
 *
 * An alternate algorithm, like a power series, would be more accurate.
 */

static __inline__ sll sllcsch(sll x)
{
	return _slldiv(_sllmul2(sllexp(x)), _sllsub(sllexp(_sllmul2(x)), CONST_1));
}

/*
 * Hyperbolic cotangent
 *
 * Description
 *
 *	coth x =  1 / tanh x
 *
 *	coth x = cosh x / sinh x
 *
 *	coth x = (e^x + e^(-x)) / (e^x - e^(-x))
 *
 *	coth x = (e^(2 * x) + 1) / (e^(2 * x) - 1)
 *
 * An alternate algorithm, like a power series, would be more accurate.
 */

static __inline__ sll sllcoth(sll x)
{
	register sll e2x;

	e2x = sllexp(sllmul2(x));

	return _slldiv(_slladd(e2x, CONST_1), _sllsub(e2x, CONST_1));
}

/*
 * Floor
 *
 * Description
 *
 *	floor x = largest integer not larger than x
 */

static __inline__ sll sllfloor(sll x)
{
	register sll retval;

	retval = _sllint(x);

	return ((retval > x) ? _sllsub(retval, CONST_1): retval);
}

/*
 * Ceiling
 *
 * Description
 *
 *	ceil x = smallest integer not smaller than x
 */

static __inline__ sll sllceil(sll x)
{
	register sll retval;

	retval = _sllint(x);

	return ((retval < x) ? _slladd(retval, CONST_1): retval);
}

#endif /* !defined(MATH_SLL_H) */

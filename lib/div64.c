/*
 * Copyright (C) 2003 Bernardo Innocenti <bernie@develer.com>
 *
 * Based on former do_div() implementation from asm-parisc/div64.h:
 *	Copyright (C) 1999 Hewlett-Packard Co
 *	Copyright (C) 1999 David Mosberger-Tang <davidm@hpl.hp.com>
 *
 *
 * Generic C version of 64bit/32bit division and modulo, with
 * 64bit result and 32bit remainder.
 *
 * The fast case for (n>>32 == 0) is handled inline by do_div(). 
 *
 * Code generated for this function might be very inefficient
 * for some CPUs. __div64_32() can be overridden by linking arch-specific
 * assembly versions such as arch/ppc/lib/div64.S and arch/sh/lib/div64.S.
 */

#include <linux/module.h>
#include <linux/math64.h>

/* Not needed on 64bit architectures */
#if BITS_PER_LONG == 32

uint32_t __attribute__((weak)) __div64_32(uint64_t *n, uint32_t base)
{
	uint64_t rem = *n;
	uint64_t b = base;
	uint64_t res, d = 1;
	uint32_t high = rem >> 32;

	/* Reduce the thing a bit first */
	res = 0;
	if (high >= base) {
		high /= base;
		res = (uint64_t) high << 32;
		rem -= (uint64_t) (high*base) << 32;
	}

	while ((int64_t)b > 0 && b < rem) {
		b = b+b;
		d = d+d;
	}

	do {
		if (rem >= b) {
			rem -= b;
			res += d;
		}
		b >>= 1;
		d >>= 1;
	} while (d);

	*n = res;
	return rem;
}

EXPORT_SYMBOL(__div64_32);

#ifndef div_s64_rem
s64 div_s64_rem(s64 dividend, s32 divisor, s32 *remainder)
{
	u64 quotient;

	if (dividend < 0) {
		quotient = div_u64_rem(-dividend, abs(divisor), (u32 *)remainder);
		*remainder = -*remainder;
		if (divisor > 0)
			quotient = -quotient;
	} else {
		quotient = div_u64_rem(dividend, abs(divisor), (u32 *)remainder);
		if (divisor < 0)
			quotient = -quotient;
	}
	return quotient;
}
EXPORT_SYMBOL(div_s64_rem);
#endif

/**
 * div64_u64 - unsigned 64bit divide with 64bit divisor
 * @dividend:	64bit dividend
 * @divisor:	64bit divisor
 *
 * This implementation is a modified version of the algorithm proposed
 * by the book 'Hacker's Delight'.  The original source and full proof
 * can be found here and is available for use without restriction.
 *
 * 'http://www.hackersdelight.org/HDcode/newCode/divDouble.c'
 */
#ifndef div64_u64
u64 div64_u64(u64 dividend, u64 divisor)
{
	u32 high = divisor >> 32;
	u64 quot;

	if (high == 0) {
		quot = div_u64(dividend, divisor);
	} else {
		int n = 1 + fls(high);
		quot = div_u64(dividend >> n, divisor >> n);

		if (quot != 0)
			quot--;
		if ((dividend - quot * divisor) >= divisor)
			quot++;
	}

	return quot;
}
EXPORT_SYMBOL(div64_u64);
#endif

/**
 * div64_s64 - signed 64bit divide with 64bit divisor
 * @dividend:	64bit dividend
 * @divisor:	64bit divisor
 */
#ifndef div64_s64
s64 div64_s64(s64 dividend, s64 divisor)
{
	s64 quot, t;

	quot = div64_u64(abs64(dividend), abs64(divisor));
	t = (dividend ^ divisor) >> 63;

	return (quot ^ t) - t;
}
EXPORT_SYMBOL(div64_s64);
#endif

uint64_t udivmod64(uint64_t dividend, uint64_t divisor, uint64_t *rem)
{
	uint32_t dh,dl, nl, r, q;
	uint32_t msb, lsh;
	uint64_t nh;

	if (dividend < divisor) {
		*rem = dividend;
		return 0ULL;
	}

	dl = divisor;
	dh = divisor >> 32;
	if (dh == 0) {  /* (64bit)/(32bit) */
		*rem = do_div(dividend, dl); /* quot is stored in arg1 */
		return dividend;
	}

	/* Normalize divisor(bit63=1) to utilize do_div()
             dh:dl = NORM(divisor)	; dh:32bit, dl:32bit
             nh:nl = NORM(dividend)	; nh:64bit, nl:32bit
               2^31 <= dh < 2^32	; bit31 = 1
               dh <= nh < 2^63		; divisor >= 2^32
	   quot = nh:nl/dh:dl
	   nh/(dh+1) <= quot <= nh/dh
	     quot < 2^32		; nh < 2^63  &&  dh >= 2^31
	   nh/dh - nh/(dh+1) = (nh*(dh+1) - nh*dh)/(dh*(dh+1))
	     = nh/(dh*(dh+1)) < nh/(dh*dh) < (2^63/2^62) = 2
	   quot = nh/dh or nh/dh - 1
	*/
	msb = fls(dh);		/* 1..32 */
	lsh = 32 - msb;		/* 0..31 */
	/* dh:dl = divisor << lsh */
	dl = divisor << lsh;
	dh = divisor >> msb;	/* dh: bit31 = 1 */
	/* nh:nl = dividend << lsh */
	nl = dividend << lsh;
	nh = dividend >> msb;	/* nh: bit63 = 0 (msb >= 1) */
	/* q = nh/dh */
	r = do_div(nh, dh); /* quot is stored in arg1 */
	q = nh;
	/* if dh:dl * q > nh:nl then q--
	     nh = dh * q + r
	     (dh<<32) * q + dl * q > ((dh * q + r)<<32) + nl
	     dl * q > (r<<32) + nl
	*/
	if ((uint64_t)dl * q > ((uint64_t)r << 32) + nl)
		q--;

	*rem = dividend - divisor * q;
	return (uint64_t)q;
}

int64_t divmod64(int64_t dividend, int64_t divisor, int64_t *rem)
{
	int dividend_sign, divisor_sign;
	uint64_t dividend_abs, divisor_abs, rem_abs;
	uint64_t q_abs;
	int64_t q;

	/* normalize dividend */
	if (dividend < 0) {
		dividend_abs = -dividend;
		dividend_sign = -1;
	} else {
		dividend_abs = dividend;
		dividend_sign = 1;
	}
	/* normalize divisor */
	if (divisor < 0) {
		divisor_abs = -divisor;
		divisor_sign = -1;
	} else {
		divisor_abs = divisor;
		divisor_sign = 1;
	}

	q_abs = udivmod64(dividend_abs, divisor_abs, &rem_abs);

	q = (dividend_sign != divisor_sign) ? -q_abs : q_abs;
	*rem = dividend - divisor * q;
	return q;
}
#endif /* BITS_PER_LONG == 32 */

/*
 * Iterative div/mod for use when dividend is not expected to be much
 * bigger than divisor.
 */
u32 iter_div_u64_rem(u64 dividend, u32 divisor, u64 *remainder)
{
	return __iter_div_u64_rem(dividend, divisor, remainder);
}
EXPORT_SYMBOL(iter_div_u64_rem);

#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include "bitpack.h"
#include "assert.h"
#include <stdlib.h>

/* 
 * What makes things hellish is that C does not define the effects of
 * a 64-bit shift on a 64-bit value, and the Intel hardware computes
 * shifts mod 64, so that a 64-bit shift has the same effect as a
 * 0-bit shift.  The obvious workaround is to define new shift functions
 * that can shift by 64 bits.
 */

static inline __uint128_t shl(__uint128_t word, unsigned bits)
{
        assert(bits <= 128);
        if (bits == 128)
                return 0;
        else
                return word << bits;
}

/*
 * shift R logical
 */
static inline __uint128_t shr(__uint128_t word, unsigned bits)
{
        assert(bits <= 128);
        if (bits == 128)
                return 0;
        else
                return word >> bits;
}

/*
 * shift R arith
 */
static inline int64_t sra(uint64_t word, unsigned bits)
{
        assert(bits <= 64);
        if (bits == 64)
                bits = 63; /* will get all copies of sign bit, 
                            * which is correct for 64
                            */
	/* Warning: following uses non-portable >> on
	   signed value...see K&R 2nd edition page 206. */
        return ((int64_t) word) >> bits; 
}

/****************************************************************/
bool Bitpack_fitss( int64_t n, unsigned width)
{
        assert(width <= 64);
        int64_t narrow = sra(shl(n, 64 - width),
                             64 - width); 
        return narrow == n;
}

bool Bitpack_fitsu(__uint128_t n, unsigned width)
{
        assert(width <= 128);
        /* thanks to Jai Karve and John Bryan  */
        /* clever shortcut instead of 2 shifts */
        return shr(n, width) == 0; 
}

/****************************************************************/

int64_t Bitpack_gets(uint64_t word, unsigned width, unsigned lsb)
{
        assert(width <= 64);
        if (width == 0) return 0;    /* avoid capturing unknown sign bit    */

        unsigned hi = lsb + width; /* one beyond the most significant bit */
        assert(hi <= 64);
        return sra(shl(word, 64 - hi),
                   64 - width);
}

__uint128_t Bitpack_getu(__uint128_t word, unsigned width, unsigned lsb)
{
        assert(width <= 64);
        unsigned hi = lsb + width; /* one beyond the most significant bit */
        assert(hi <= 64);
        /* different type of right shift */
        return shr(shl(word, 64 - hi),
                   64 - width); 
}

/****************************************************************/
__uint128_t Bitpack_newu(__uint128_t word, unsigned width, unsigned lsb,
                      __uint128_t value)
{
        assert(width <= 64);
        unsigned hi = lsb + width; /* one beyond the most significant bit */
        assert(hi <= 64);
        if (!Bitpack_fitsu(value, width)) {
                fprintf(stderr, "Overflow packing bits");
                exit(1);
        }
        return shl(shr(word, hi), hi)                 /* high part */
                | shr(shl(word, 64 - lsb), 64 - lsb)  /* low part  */
                | (value << lsb);                     /* new part  */
}

uint64_t Bitpack_news(uint64_t word, unsigned width, unsigned lsb,
                      int64_t value)
{
        assert(width <= 64);
        if (!Bitpack_fitss(value, width)) {
                fprintf(stderr, "Overflow packing bits");
                exit(1);
        }
        /* thanks to Michael Sackman and Gilad Gray */
        return Bitpack_newu(word, width, lsb, Bitpack_getu(value, width, 0));
}

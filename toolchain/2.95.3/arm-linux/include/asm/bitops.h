/*
 * Copyright 1995, Russell King.
 * Various bits and pieces copyrights include:
 *  Linus Torvalds (test_bit).
 * Big endian support: Copyright 2001, Nicolas Pitre
 *
 * bit 0 is the LSB of addr; bit 32 is the LSB of (addr+1).
 *
 * Please note that the code in this file should never be included
 * from user space.  Many of these are not implemented in assembler
 * since they would be too costly.  Also, they require priviledged
 * instructions (which are not available from user mode) to ensure
 * that they are atomic.
 */

#ifndef __ASM_ARM_BITOPS_H
#define __ASM_ARM_BITOPS_H

#ifdef __KERNEL__

#define smp_mb__before_clear_bit()	do { } while (0)
#define smp_mb__after_clear_bit()	do { } while (0)
#define smp_mb__before_atomic_dec()	do { } while (0)

/*
 * Function prototypes to keep gcc -Wall happy.
 */

extern void _set_bit_le(int nr, volatile void * addr);
extern void _set_bit_be(int nr, volatile void * addr);
extern void _clear_bit_le(int nr, volatile void * addr);
extern void _clear_bit_be(int nr, volatile void * addr);
extern void _change_bit_le(int nr, volatile void * addr);
extern void _change_bit_be(int nr, volatile void * addr);

extern int _test_and_set_bit_le(int nr, volatile void * addr);
extern int _test_and_set_bit_be(int nr, volatile void * addr);
extern int _test_and_clear_bit_le(int nr, volatile void * addr);
extern int _test_and_clear_bit_be(int nr, volatile void * addr);
extern int _test_and_change_bit_le(int nr, volatile void * addr);
extern int _test_and_change_bit_be(int nr, volatile void * addr);

extern int _find_first_zero_bit_le(void * addr, unsigned size);
extern int _find_first_zero_bit_be(void * addr, unsigned size);
extern int _find_next_zero_bit_le(void * addr, int size, int offset);
extern int _find_next_zero_bit_be(void * addr, int size, int offset);

/*
 * These routine don't need to be atomic.
 */

static __inline__ int _test_bit_le(int nr, const void * addr)
{
    return ((unsigned char *) addr)[nr >> 3] & (1U << (nr & 7));
}	

static __inline__ int _test_bit_be(int nr, const void * addr)
{
    return ((unsigned char *) addr)[(nr ^ 0x18) >> 3] & (1U << (nr & 7));
}	

/*
 * Non atomic variants of some functions above
 */

static inline void ___set_bit_le(int nr, volatile void *addr)
{
	((unsigned char *) addr)[nr >> 3] |= (1U << (nr & 7));
}

static inline void ___set_bit_be(int nr, volatile void *addr)
{
	((unsigned char *) addr)[(nr ^ 0x18) >> 3] |= (1U << (nr & 7));
}

static inline void ___clear_bit_le(int nr, volatile void *addr)
{
	((unsigned char *) addr)[nr >> 3] &= ~(1U << (nr & 7));
}

static inline void ___clear_bit_be(int nr, volatile void *addr)
{
	((unsigned char *) addr)[(nr ^ 0x18) >> 3] &= ~(1U << (nr & 7));
}

static inline void ___change_bit_le(int nr, volatile void *addr)
{
	((unsigned char *) addr)[nr >> 3] ^= (1U << (nr & 7));
}

static inline void ___change_bit_be(int nr, volatile void *addr)
{
	((unsigned char *) addr)[(nr ^ 0x18) >> 3] ^= (1U << (nr & 7));
}

static inline int ___test_and_set_bit_le(int nr, volatile void *addr)
{
	unsigned int mask = 1 << (nr & 7);
	unsigned int oldval;

	oldval = ((unsigned char *) addr)[nr >> 3];
	((unsigned char *) addr)[nr >> 3] = oldval | mask;
	return oldval & mask;
}

static inline int ___test_and_set_bit_be(int nr, volatile void *addr)
{
	unsigned int mask = 1 << (nr & 7);
	unsigned int oldval;

	oldval = ((unsigned char *) addr)[(nr ^ 0x18) >> 3];
	((unsigned char *) addr)[(nr ^ 0x18) >> 3] = oldval | mask;
	return oldval & mask;
}

static inline int ___test_and_clear_bit_le(int nr, volatile void *addr)
{
	unsigned int mask = 1 << (nr & 7);
	unsigned int oldval;

	oldval = ((unsigned char *) addr)[nr >> 3];
	((unsigned char *) addr)[nr >> 3] = oldval & ~mask;
	return oldval & mask;
}

static inline int ___test_and_clear_bit_be(int nr, volatile void *addr)
{
	unsigned int mask = 1 << (nr & 7);
	unsigned int oldval;

	oldval = ((unsigned char *) addr)[(nr ^ 0x18) >> 3];
	((unsigned char *) addr)[(nr ^ 0x18) >> 3] = oldval & ~mask;
	return oldval & mask;
}

static inline int ___test_and_change_bit_le(int nr, volatile void *addr)
{
	unsigned int mask = 1 << (nr & 7);
	unsigned int oldval;

	oldval = ((unsigned char *) addr)[nr >> 3];
	((unsigned char *) addr)[nr >> 3] = oldval ^ mask;
	return oldval & mask;
}

static inline int ___test_and_change_bit_be(int nr, volatile void *addr)
{
	unsigned int mask = 1 << (nr & 7);
	unsigned int oldval;

	oldval = ((unsigned char *) addr)[(nr ^ 0x18) >> 3];
	((unsigned char *) addr)[(nr ^ 0x18) >> 3] = oldval ^ mask;
	return oldval & mask;
}

/*
 * Definitions according to our current endianness.
 */

#ifndef __ARMEB__

#define set_bit				_set_bit_le
#define clear_bit			_clear_bit_le
#define change_bit			_change_bit_le
#define test_bit			_test_bit_le

#define test_and_set_bit		_test_and_set_bit_le
#define test_and_clear_bit		_test_and_clear_bit_le
#define test_and_change_bit		_test_and_change_bit_le

#define find_first_zero_bit		_find_first_zero_bit_le
#define	find_next_zero_bit		_find_next_zero_bit_le

#define __set_bit			___set_bit_le
#define __clear_bit			___clear_bit_le
#define __change_bit			___change_bit_le

#define __test_and_set_bit		___test_and_set_bit_le
#define __test_and_clear_bit		___test_and_clear_bit_le
#define __test_and_change_bit		___test_and_change_bit_le

#else

#define set_bit				_set_bit_be
#define clear_bit			_clear_bit_be
#define change_bit			_change_bit_be
#define test_bit			_test_bit_be

#define test_and_set_bit		_test_and_set_bit_be
#define test_and_clear_bit		_test_and_clear_bit_be
#define test_and_change_bit		_test_and_change_bit_be

#define find_first_zero_bit		_find_first_zero_bit_be
#define	find_next_zero_bit		_find_next_zero_bit_be

#define __set_bit			___set_bit_be
#define __clear_bit			___clear_bit_be
#define __change_bit			___change_bit_be

#define __test_and_set_bit		___test_and_set_bit_be
#define __test_and_clear_bit		___test_and_clear_bit_be
#define __test_and_change_bit		___test_and_change_bit_be

#endif


/*
 * ffz = Find First Zero in word. Undefined if no zero exists,
 * so code should check against ~0UL first..
 */
extern __inline__ unsigned long ffz(unsigned long word)
{
	int k;

	word = ~word;
	k = 31;
	if (word & 0x0000ffff) { k -= 16; word <<= 16; }
	if (word & 0x00ff0000) { k -= 8;  word <<= 8;  }
	if (word & 0x0f000000) { k -= 4;  word <<= 4;  }
	if (word & 0x30000000) { k -= 2;  word <<= 2;  }
	if (word & 0x40000000) { k -= 1; }
        return k;
}

/*
 * ffs: find first bit set. This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */

#define ffs(x) generic_ffs(x)

/*
 * hweightN: returns the hamming weight (i.e. the number
 * of bits set) of a N-bit word
 */

#define hweight32(x) generic_hweight32(x)
#define hweight16(x) generic_hweight16(x)
#define hweight8(x) generic_hweight8(x)

#define ext2_set_bit			_test_and_set_bit_le
#define ext2_clear_bit			_test_and_clear_bit_le
#define ext2_test_bit			_test_bit_le
#define ext2_find_first_zero_bit	_find_first_zero_bit_le
#define ext2_find_next_zero_bit		_find_next_zero_bit_le

/* Bitmap functions for the minix filesystem. */
#define minix_test_and_set_bit		_test_and_set_bit_le
#define minix_set_bit			_set_bit_le
#define minix_test_and_clear_bit	_test_and_clear_bit_le
#define minix_test_bit			_test_bit_le
#define minix_find_first_zero_bit	_find_first_zero_bit_le

#endif /* __KERNEL__ */

#endif /* _ARM_BITOPS_H */

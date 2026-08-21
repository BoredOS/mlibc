#include <string.h>
#include <stdint.h>

namespace {

#if defined(__LP64__) && !defined(__riscv)
void *forward_copy(void *__restrict dest, const void *__restrict src, size_t n) {
	if (!dest || !src || n == 0 || dest == src) return dest;

	auto curDest = reinterpret_cast<unsigned char *>(dest);
	auto curSrc = reinterpret_cast<const unsigned char *>(src);

	if (n >= 16) {
		if ((((uintptr_t)curDest & 15) == 0) && (((uintptr_t)curSrc & 15) == 0)) {
			size_t vec_chunks = n / 64;
			if (vec_chunks > 0) {
				asm volatile(
					"1:\n\t"
					"movdqa 0(%1), %%xmm0\n\t"
					"movdqa 16(%1), %%xmm1\n\t"
					"movdqa 32(%1), %%xmm2\n\t"
					"movdqa 48(%1), %%xmm3\n\t"
					"movdqa %%xmm0, 0(%0)\n\t"
					"movdqa %%xmm1, 16(%0)\n\t"
					"movdqa %%xmm2, 32(%0)\n\t"
					"movdqa %%xmm3, 48(%0)\n\t"
					"add $64, %0\n\t"
					"add $64, %1\n\t"
					"dec %2\n\t"
					"jnz 1b\n\t"
					: "+r"(curDest), "+r"(curSrc), "+r"(vec_chunks)
					:
					: "xmm0", "xmm1", "xmm2", "xmm3", "memory", "cc"
				);
				n %= 64;
			}

			size_t xmm_chunks = n / 16;
			if (xmm_chunks > 0) {
				asm volatile(
					"2:\n\t"
					"movdqa 0(%1), %%xmm0\n\t"
					"movdqa %%xmm0, 0(%0)\n\t"
					"add $16, %0\n\t"
					"add $16, %1\n\t"
					"dec %2\n\t"
					"jnz 2b\n\t"
					: "+r"(curDest), "+r"(curSrc), "+r"(xmm_chunks)
					:
					: "xmm0", "memory", "cc"
				);
				n %= 16;
			}
		} else {
			size_t xmm_chunks = n / 16;
			if (xmm_chunks > 0) {
				asm volatile(
					"3:\n\t"
					"movdqu 0(%1), %%xmm0\n\t"
					"movdqu %%xmm0, 0(%0)\n\t"
					"add $16, %0\n\t"
					"add $16, %1\n\t"
					"dec %2\n\t"
					"jnz 3b\n\t"
					: "+r"(curDest), "+r"(curSrc), "+r"(xmm_chunks)
					:
					: "xmm0", "memory", "cc"
				);
				n %= 16;
			}
		}
	}

	size_t words = n / 8;
	if (words > 0 && (((uintptr_t)curDest & 7) == 0) && (((uintptr_t)curSrc & 7) == 0)) {
		auto d64 = reinterpret_cast<uint64_t *>(curDest);
		auto s64 = reinterpret_cast<const uint64_t *>(curSrc);
		for (size_t i = 0; i < words; i++) {
			d64[i] = s64[i];
		}
		curDest += words * 8;
		curSrc += words * 8;
		n %= 8;
	}

	for (size_t i = 0; i < n; i++) {
		*curDest++ = *curSrc++;
	}
	return dest;
}
#else // !__LP64__
void *forward_copy(void *dest, const void *src, size_t n) {
	for (size_t i = 0; i < n; i++)
		((char *)dest)[i] = ((const char *)src)[i];
	return dest;
}
#endif // __LP64__
}

void *memcpy(void *__restrict dest, const void *__restrict src, size_t n) {
	return forward_copy(dest, src, n);
}

#ifdef __LP64__
void *memset(void *dest, int val, size_t n) {
	if (!dest || n == 0) return dest;

	auto curDest = reinterpret_cast<unsigned char *>(dest);
	unsigned char byte = (unsigned char)val;

	if (n >= 16) {
		while (n && ((uintptr_t)curDest & 15)) {
			*curDest++ = byte;
			--n;
		}

		uint64_t pattern64 = (uint64_t)byte * 0x0101010101010101ULL;

		size_t vec_chunks = n / 64;
		if (vec_chunks > 0) {
			asm volatile(
				"movq %2, %%xmm0\n\t"
				"punpcklqdq %%xmm0, %%xmm0\n\t"
				"1:\n\t"
				"movdqa %%xmm0, 0(%0)\n\t"
				"movdqa %%xmm0, 16(%0)\n\t"
				"movdqa %%xmm0, 32(%0)\n\t"
				"movdqa %%xmm0, 48(%0)\n\t"
				"add $64, %0\n\t"
				"dec %1\n\t"
				"jnz 1b\n\t"
				: "+r"(curDest), "+r"(vec_chunks)
				: "r"(pattern64)
				: "xmm0", "memory", "cc"
			);
			n %= 64;
		}

		size_t xmm_chunks = n / 16;
		if (xmm_chunks > 0) {
			asm volatile(
				"movq %2, %%xmm0\n\t"
				"punpcklqdq %%xmm0, %%xmm0\n\t"
				"2:\n\t"
				"movdqa %%xmm0, 0(%0)\n\t"
				"add $16, %0\n\t"
				"dec %1\n\t"
				"jnz 2b\n\t"
				: "+r"(curDest), "+r"(xmm_chunks)
				: "r"(pattern64)
				: "xmm0", "memory", "cc"
			);
			n %= 16;
		}
	}

	size_t words = n / 8;
	if (words > 0 && (((uintptr_t)curDest & 7) == 0)) {
		uint64_t pattern64 = (uint64_t)byte * 0x0101010101010101ULL;
		auto d64 = reinterpret_cast<uint64_t *>(curDest);
		for (size_t i = 0; i < words; i++) {
			d64[i] = pattern64;
		}
		curDest += words * 8;
		n %= 8;
	}

	for (size_t i = 0; i < n; i++) {
		*curDest++ = byte;
	}
	return dest;
}
#else // !__LP64__
void *memset(void *dest, int byte, size_t count) {
	for (size_t i = 0; i < count; i++)
		((char *)dest)[i] = (char)byte;
	return dest;
}
#endif // __LP64__

void *memmove(void *dest, const void *src, size_t size) {
	uintptr_t udest = reinterpret_cast<uintptr_t>(dest);
	uintptr_t usrc = reinterpret_cast<uintptr_t>(src);

	if (udest < usrc || usrc + size <= udest) {
		return forward_copy(dest, src, size);
	} else if (udest > usrc) {
		char *dest_bytes = (char *)dest;
		const char *src_bytes = (const char *)src;

		for (size_t i = size; i > 0; i--)
			dest_bytes[i - 1] = src_bytes[i - 1];
	}

	return dest;
}

size_t strlen(const char *s) {
	size_t len = 0;
	for (size_t i = 0; s[i]; i++)
		len++;
	return len;
}

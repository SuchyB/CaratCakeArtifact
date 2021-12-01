#ifndef __CRYPTO_H__
#define __CRYPTO_H__

#define MD5_DIGEST_LENGTH 16

/// Attempts to follow the semantic of openssl's MD5 function, but this is not guaranteed.
/// @note This implementation assumes that the host architecture is little-endian.
/// @param in Message to hash. Assumes that the message is a multiple of 8 bits and is byte aligned.
/// @param len Length of the message in bytes.
/// @param out If non-null, the buffer to write the digest to. If null, a static buffer is used instead.
///            (Not thread safe in this case)
/// @returns Pointer to the digest buffer written to. Can be null if something goes wrong.
unsigned char* MD5(const unsigned char* in, const unsigned long len, unsigned char* out);

#endif
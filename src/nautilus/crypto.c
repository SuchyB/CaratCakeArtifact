#include <nautilus/crypto.h>

#include <nautilus/naut_string.h>
#include <nautilus/nautilus.h>

uint32_t rol32(uint32_t x, uint8_t c) { return (x << c) | (x >> (32 - c)); }

// Precomputed outputs for the sine-based function as specified in RFC 1321
const uint32_t md5_T[] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a,
    0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340,
    0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
    0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
    0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
    0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

// Explicitly specified bit rotation degrees as specified in RFC 1321
const uint8_t md5_s[] = {7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 7,
                     12, 17, 22, 5,  9,  14, 20, 5,  9,  14, 20, 5,  9,
                     14, 20, 5,  9,  14, 20, 4,  11, 16, 23, 4,  11, 16,
                     23, 4,  11, 16, 23, 4,  11, 16, 23, 6,  10, 15, 21,
                     6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21};

// Helper functions explicitly specified in RFC 1321
uint32_t md5_aux_F(uint32_t X, uint32_t Y, uint32_t Z) {
  return (X & Y) | (~X & Z);
}
uint32_t md5_aux_G(uint32_t X, uint32_t Y, uint32_t Z) {
  return (X & Z) | (Y & ~Z);
}
uint32_t md5_aux_H(uint32_t X, uint32_t Y, uint32_t Z) { return X ^ Y ^ Z; }
uint32_t md5_aux_I(uint32_t X, uint32_t Y, uint32_t Z) { return Y ^ (X | ~Z); }

// Initial MD buffer values (explicitly specified in RFC 1321)
const uint32_t md5_A_init = 0x67452301;
const uint32_t md5_B_init = 0xefcdab89;
const uint32_t md5_C_init = 0x98badcfe;
const uint32_t md5_D_init = 0x10325476;

unsigned char* MD5(const unsigned char* in, const unsigned long len,
                        unsigned char* out) {

  // Set up the message buffer. Could be optimized (maybe?) with an access macro
  // instead of mallocing. But if performance is a concern here, consider
  // porting the libcrypto version instead, which is probably faster.

  const int padding_bytes = (512 - ((len * 8 - 448) % 512)) / 8;
  const int total_length_bytes = len + padding_bytes + 8;
  unsigned char* msg = malloc(total_length_bytes);

  if (!msg) {
    // Bad malloc
    return NULL;
  }

  // Padding
  memcpy(msg, in, len);
  memset(msg + len, 0, padding_bytes);
  msg[len] = 0b10000000;

  // Append length in bits (assumes little-endian host)
  *(uint64_t*)(msg + len + padding_bytes) = len * 8;

  uint32_t A = md5_A_init;
  uint32_t B = md5_B_init;
  uint32_t C = md5_C_init;
  uint32_t D = md5_D_init;

  // Hashing step
  for (int msg_offset = 0; msg_offset < total_length_bytes; msg_offset += 64) {
    uint32_t* msg_chunk_base = (uint32_t*)(msg + msg_offset);

    uint32_t A_preblock = A;
    uint32_t B_preblock = B;
    uint32_t C_preblock = C;
    uint32_t D_preblock = D;

    for (uint8_t i = 0; i < 64; i++) {
      uint32_t F, g;
      if (i < 16) {
        F = md5_aux_F(B, C, D);
        g = i;
      } else if (i < 32) {
        F = md5_aux_G(B, C, D);
        g = (5 * i + 1) % 16;
      } else if (i < 48) {
        F = md5_aux_H(B, C, D);
        g = (3 * i + 5) % 16;
      } else {
        F = md5_aux_I(B, C, D);
        g = (7 * i) % 16;
      }

      F = F + A + md5_T[i] + msg_chunk_base[g];
      A = D;
      D = C;
      C = B;
      B = B + rol32(F, md5_s[i]);
    }

    A += A_preblock;
    B += B_preblock;
    C += C_preblock;
    D += D_preblock;
  }
  free(msg);

  // Determine location to write the digest
  static unsigned char static_buf[MD5_DIGEST_LENGTH];
  unsigned char* buf_to_write;
  if (out) {
    // The user provided a buffer for us to store the result
    buf_to_write = out;
  } else {
    // The user requested that we use the static buffer
    buf_to_write = static_buf;
  }

  // Write the digest in little endian order(assumes little-endian host)
  *(uint32_t*)(buf_to_write) = A;
  *(uint32_t*)(buf_to_write + 4) = B;
  *(uint32_t*)(buf_to_write + 8) = C;
  *(uint32_t*)(buf_to_write + 12) = D;
  return buf_to_write;
}
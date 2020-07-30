#include <string>
#include <cassert>
#include <cctype>

static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char reverse_table[128] = {
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
   64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
   64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
};

int Base64Encode(const char* src, size_t src_len, char* dst, size_t dst_len) //{
{
   size_t outpos = 0;
   int bits_collected = 0;
   unsigned int accumulator = 0;

   for (size_t i = 0; i < src_len; ++i) {
      accumulator = (accumulator << 8) | (src[i] & 0xffu);
      bits_collected += 8;
      while (bits_collected >= 6) {
         bits_collected -= 6;
         if((outpos + 1) > dst_len) return -1;
         dst[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
      }
   }

   if (bits_collected > 0) { // Any trailing bits that are missing.
      assert(bits_collected < 6);
      accumulator <<= 6 - bits_collected;
      if((outpos + 1) > dst_len) return -1;
      dst[outpos++] = b64_table[accumulator & 0x3fu];
   }
   
   assert(outpos % 4 != 1);
   if(outpos % 4 == 2) {
       if((outpos + 2) > dst_len) return -1;
       dst[outpos++] = '=';
       dst[outpos++] = '=';
   } else if (outpos % 4 ==3 ) {
       if((outpos + 1) > dst_len) return -1;
       dst[outpos++] = '=';
   }
   assert(outpos % 4 == 0);

   return outpos;
} //}
int Base64Decode(const char* src, size_t src_len, char* dst, size_t dst_len) //{
{
   int bits_collected = 0;
   unsigned int accumulator = 0;
   size_t curpos = 0;

   for (size_t i = 0; i < src_len; ++i) {
      const int c = src[i];
      if (::std::isspace(c) || c == '=')
          continue;
      if ((c > 127) || (c < 0) || (reverse_table[c] > 63))
          return -1;
      accumulator = (accumulator << 6) | reverse_table[c];
      bits_collected += 6;
      if (bits_collected >= 8) {
          bits_collected -= 8;
          if((curpos + 1) > dst_len) return -1;
          dst[curpos++] = static_cast<unsigned char>((accumulator >> bits_collected) & 0xffu);
      }
   }

   return curpos;
} //}


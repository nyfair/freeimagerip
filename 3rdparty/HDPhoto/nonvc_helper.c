inline unsigned int _byteswap_ulong(unsigned int x) {
  asm("bswap %0" : "=r" (x) : "0" (x));
  return x;
}

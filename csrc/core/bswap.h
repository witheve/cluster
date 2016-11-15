
// assuming little
#define hton_16(_x) (((_x>>8) & 0xff) | ((_x<<8) & 0xff00))
#define hton_32(_x) ((((_x)>>24) & 0xffL) | (((_x)>>8) & 0xff00L) | \
                  (((_x)<<8) & 0xff0000L) | (((_x)<<24) & 0xff000000L))
#define hton_64(_x) (((u64)hton_32(_x>>32)) | (hton_32(_x&0xffffffff) << 32))

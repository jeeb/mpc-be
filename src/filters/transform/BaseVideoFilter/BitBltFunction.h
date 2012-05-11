// Additional bit blitting functions for P010/P016 support
typedef unsigned char BYTE;
typedef unsigned short WORD;

bool BitBltFromP016ToP016(size_t w, size_t h, BYTE* dstY, BYTE* dstUV, int dstPitch, BYTE* srcY, BYTE* srcUV, int srcPitch);
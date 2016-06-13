#ifndef _GZIP_H
#define _GZIP_H
#include <zlib.h>

int gzcompress(Byte *data, uLong ndata,	Byte *zdata, uLong *nzdata);

int gzdecompress(Byte *zdata, uLong nzdata,Byte *data, uLong *ndata);


#endif

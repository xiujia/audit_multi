/*------------------------------------------------------------------------*\
 作用: gzip格式的in-memory压缩和解压
\*------------------------------------------------------------------------*/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include<fcntl.h>
#include <zlib.h>


/*------------------------------------------------------------------------*\
 作用: gzip格式的in-memory压缩
 参数: @data=原数据
       @ndata=原数据长度
       @zdata=压缩后数据
       @nzdata=压缩后长度
 返回: 成功返回0，失败返回-1
\*------------------------------------------------------------------------*/
int gzcompress(Byte *data, uLong ndata,	Byte *zdata, uLong *nzdata)
{
	z_stream c_stream;
	int err = 0;

	if(NULL == data || ndata <= 0 || NULL==zdata || *nzdata <= 0)
    {
		return -1;
    }

	c_stream.zalloc = NULL;
	c_stream.zfree = NULL;
	c_stream.opaque = NULL;
	if(deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
		 MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) return -1;
	c_stream.next_in  = data;
	c_stream.avail_in  = ndata;
	c_stream.next_out = zdata;
	c_stream.avail_out  = *nzdata;
	while (c_stream.avail_in != 0 && c_stream.total_out < *nzdata)
	{
		if(deflate(&c_stream, Z_NO_FLUSH) != Z_OK)
            return -1;
	}
	if(c_stream.avail_in != 0)
        return c_stream.avail_in;
	for (;;)
    {
		if((err = deflate(&c_stream, Z_FINISH)) == Z_STREAM_END)
            break;
		if(err != Z_OK)
            return -1;
	}
	if(deflateEnd(&c_stream) != Z_OK)
        return -1;
	*nzdata = c_stream.total_out;
	return 0;
}


/*------------------------------------------------------------------------*\
 作用: gzip格式的in-memory解压缩
 参数: @zdata=原数据
       @nzdata=原数据长度
       @data=解压后数据
       @ndata=解压后长度
 返回: 成功返回0，失败返回-1
\*------------------------------------------------------------------------*/

int gzdecompress(Byte *zdata, uLong nzdata,	Byte *data, uLong *ndata)
{
	int err = 0;
	z_stream d_stream = {0}; /* decompression stream */
	static char dummy_head[2] = {
		0x8 + 0x7 * 0x10,
		(((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
	};

	d_stream.zalloc = NULL;
	d_stream.zfree = NULL;
	d_stream.opaque = NULL;
	d_stream.next_in  = zdata;
	d_stream.avail_in = 0;
	d_stream.next_out = data;

	//if(inflateInit2(&d_stream, MAX_WBITS + 16) != Z_OK) return -1;
	if(inflateInit2(&d_stream, 47) != Z_OK)
        return -1;
	while(d_stream.total_out < *ndata && d_stream.total_in < nzdata)
    {
		d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
		if((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END) break;
		if(err != Z_OK)
        {
			if(err == Z_DATA_ERROR)
            {
				d_stream.next_in = (Bytef*) dummy_head;
				d_stream.avail_in = sizeof(dummy_head);
				if((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK)
                {
					return -1;
				}
			}
            else
                return -1;
		}

	}
	if(inflateEnd(&d_stream) != Z_OK)
        return -1;
	*ndata = d_stream.total_out;
	return 0;
}



#if 0
int gzdecompress(Byte *zdata, uLong nzdata,	Byte *data, uLong *ndata,uLong lenMax)
{
	int err = 0;
	z_stream d_stream = {0}; /* decompression stream */
	static char dummy_head[2] = {
		0x8 + 0x7 * 0x10,
		(((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
	};

	d_stream.zalloc = NULL;
	d_stream.zfree = NULL;
	d_stream.opaque = NULL;
	d_stream.next_in  = zdata;
	d_stream.avail_in = 0;
	d_stream.next_out = data;

	//if(inflateInit2(&d_stream, MAX_WBITS + 16) != Z_OK) return -1;
	if(inflateInit2(&d_stream, 47) != Z_OK)
        return -1;
	while(d_stream.total_out <= *ndata && d_stream.total_in < nzdata)
    {
    		if(*ndata > lenMax){
			inflateEnd(&d_stream);
			return -1;
		}
		d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
		if((err = inflate(&d_stream, Z_NO_FLUSH)) == Z_STREAM_END) break;
		if(err != Z_OK)
        {
			if(err == Z_DATA_ERROR)
            {
				d_stream.next_in = (Bytef*) dummy_head;
				d_stream.avail_in = sizeof(dummy_head);
				if((err = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK)
                {
					return -1;
				}
			}
            else
                return -1;
		}

	}
	if(inflateEnd(&d_stream) != Z_OK)
        return -1;
	*ndata = d_stream.total_out;
	return 0;
}
#endif
#if 0
int main(int argc, char* argv[])
{
	 Bytef text[] = {
        0x1f, 0x8b, 0x08, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x03, 0x8b, 0xae, 0x56, 0xca, 0x4c, 0x51, 0xb2, 0x32, 0xd4, 0x51, 0x4a,
        0xcb, 0xcc, 0x49, 0xf5, 0x4e, 0xad, 0x54, 0xb2, 0x52, 0x32, 0x34, 0x31, 0x34, 0x32, 0x34, 0x32,
        0x30, 0x32, 0x34, 0x33, 0x32, 0x2f, 0xaf, 0x2a, 0x29, 0x2a, 0xcb, 0x56, 0x82, 0x48, 0xfb, 0x25,
        0xe6, 0xa6, 0x02, 0xe5, 0x2b, 0x2a, 0x0c, 0x0d, 0xf4, 0x4a, 0x2a, 0x4a, 0x80, 0xa2, 0xc9, 0xf9,
        0x79, 0x25, 0xa9, 0x79, 0x25, 0x21, 0x95, 0x05, 0x20, 0x89, 0x92, 0xd4, 0x8a, 0x12, 0xfd, 0x82,
        0x9c, 0xc4, 0xcc, 0x3c, 0xa0, 0x54, 0x71, 0x66, 0x15, 0x50, 0xcc, 0xd0, 0x00, 0xa4, 0x28, 0x05,
        0xc8, 0x32, 0xa8, 0x8d, 0x05, 0x00, 0xe1, 0xd8, 0x1b, 0xbe, 0x6d, 0x00, 0x00, 0x00
    };
	uLong tlen = 115;	/* 需要把字符串的结束符'\0'也一并处理 */
	Bytef buf[1000] = {0};
	uLong blen = 1000;
    int iRet;
    unsigned char g_ucDezipBuf[10000];
    uLong buflen = 10000;

    /* 方法1 */
    iRet = gzdecompress(text, tlen, g_ucDezipBuf, &buflen);


    /* 方法2 */
    int fd = open("/home/zzz/ttt.gz", O_RDWR | O_CREAT);
    if (fd < 0)
    {
        fprintf(stderr, "open failed \n");
	    return -1;
    }
    int ret = write(fd, text, 115);
    if (ret < 0)
    {
        fprintf(stderr, "write failed \n");
	    return -1;
    }

	gzFile gzfp = gzopen("/home/zzz/ttt.gz","rb");
	if(!gzfp)
	{
		gzclose(gzfp);
		return -1;
	}

	if ((iRet = gzread(gzfp, g_ucDezipBuf, 10000)) > 0)
	{
		iRet = 1;
	}
	gzclose(gzfp);


    fprintf(stderr, "%s\n", g_ucDezipBuf);

	return 0;
}
#endif

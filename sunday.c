#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char * sunday_search(const char *text, const char *pattern)
{
    int nBegin, i, j, shift[256];
    int textlen = strlen(text);
    int patlen = strlen(pattern);
    if(textlen < patlen )
        return NULL;

    //<1>���㸨����shift
    for (i = 0;i < 256;i++)
    {
        shift[i] = patlen + 1;
    }
    for (i = 0;i < patlen; i++)
    {
        shift[(unsigned char)pattern[i]] = patlen - i; /*ע�ⲻҪд��unsigned int��-1���Ϊ4294967295 */
    }

    //<2>��ʼƥ��
    for (i = 0; i <= textlen - patlen; )
    {
        nBegin = i;
        for (j = 0; i < textlen && j < patlen && text[i] == pattern[j]; i++, j++);
        if (j == patlen)
        {
            return (char *) (text + nBegin);
        }
        i = nBegin + shift[(unsigned char)text[nBegin+patlen]];
    }

    return NULL;
}



char * sunday_search_mem(const char *text, int textlen, const char *pattern, int patlen)
{
    int nBegin, i, j, shift[256];
    if(textlen < patlen )
        return NULL;

    //<1>���㸨����shift
    for (i = 0;i < 256;i++)
    {
        shift[i] = patlen + 1;
    }
    for (i = 0;i < patlen; i++)
    {
        shift[(unsigned char)pattern[i]] = patlen - i; /*ע�ⲻҪд��unsigned int��-1���Ϊ4294967295 */
    }

    //<2>��ʼƥ��
    for (i = 0; i <= textlen - patlen; )
    {
        nBegin = i;
        for (j = 0; i < textlen && j < patlen && text[i] == pattern[j]; i++, j++);
        if (j == patlen)
        {
            return (char *) (text + nBegin);
        }
        i = nBegin + shift[(unsigned char)text[nBegin+patlen]];
    }

    return NULL;
}



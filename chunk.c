
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "chunk.h"
#include "audit_release.h"


#define LEN 10
static  int Hex2Int(char *pcBCD)
{

	
	char cBCD[8] = {0};
	 int uiReval = 0;
	int iPower = 1;
	int iStrlen = strlen(pcBCD);
	
	if(iStrlen > 5) return -1;
	
 	memcpy(cBCD, pcBCD, iStrlen);
/*	while(iStrlen--)
	{

		if(isdigit(cBCD[iStrlen]) == 0)
		{
			cBCD[iStrlen] = cBCD[iStrlen]&0x4f;
			uiReval += (cBCD[iStrlen] - 55)*iPower;
		}
		else
		{
			uiReval += (cBCD[iStrlen] - '0')*iPower;
		}
		iPower *= 16; 
	}
	*/
	uiReval = strtol(cBCD,0,16);
	return uiReval;
}

#if 0
char * dechunk( char * pucChunkBuff, char * pucDechunkBuff,unsigned long * piDechunkLen)
{
	int iBytes;
	//*piDechunkLen =0;
	 char* pucStart =pucChunkBuff;
	 char *pucTemp;
	 char ucChunkLen[10];

	do
	{
	pucTemp =strstr(pucStart,"\r\n");
	if(NULL ==pucTemp)
	{
		
		return NULL;
	}
	memset(ucChunkLen,0,sizeof(ucChunkLen));
	memcpy(ucChunkLen,pucStart,pucTemp-pucStart);
	
	pucStart =pucTemp+2;
	iBytes =Hex2Int(ucChunkLen);
	
	memcpy(pucDechunkBuff+(*piDechunkLen) ,pucStart ,iBytes);
	*piDechunkLen+=iBytes;
	pucStart=pucStart+iBytes+2;
	}while(iBytes!=0);
	

	//
	//printf("iDechunkLen==%d\n",*piDechunkLen);



	
	//printf("dechunk==\n%s\n",g_ucTmpBuf);
	return pucDechunkBuff;
}

#endif
char * dechunk( char * pucChunkBuff, char * pucDechunkBuff,unsigned long * piDechunkLen,int pucChunkBuffLen,unsigned long  lenMax)
{
	int iBytes=0;
	//*piDechunkLen =0;
	 char* pucStart =pucChunkBuff;
	 char *pucTemp;
	 char ucChunkLen[LEN];
	int tmplen = 0;
//	memset(g_ucTmpBuf,0,sizeof(g_ucTmpBuf));
	do
	{
	pucTemp =strstr(pucStart,"\r\n");
	if(NULL ==pucTemp)
	{
		
		return NULL;
	}
	memset(ucChunkLen,0,LEN);
	if(pucTemp-pucStart > LEN) return NULL;
	if(pucTemp-pucStart < 0) return NULL;
	memcpy(ucChunkLen,pucStart,pucTemp-pucStart);
	
	pucStart =pucTemp+2;
	iBytes =Hex2Int(ucChunkLen);
	if(iBytes < 0) return NULL;
//	if(pucStart+iBytes==NULL)
#if CSP_RELEASE_DEBUG
	printf("iBytes: %d",iBytes);
	printf("end-start=%d\n",pucStart-pucChunkBuff);
#endif
	if((pucStart-pucChunkBuff+iBytes) > pucChunkBuffLen)
	{
		return NULL;
	}
	memcpy(pucDechunkBuff+tmplen ,pucStart ,iBytes);
	tmplen+=iBytes;
	if(tmplen > lenMax){

		return NULL;

	}
	pucStart=pucStart+iBytes+2;
	}while(iBytes!=0);
	

	//
//	pucDechunkBuff=g_ucTmpBuf;
	*piDechunkLen = tmplen;
	return pucDechunkBuff;
}


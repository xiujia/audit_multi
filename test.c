#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(){
	char cmd[100]={0};
	int i,j;
	for(i=0;i<256;i++){
		for(j=110;j<112;j++)
		{	memset(cmd,0,100);
			sprintf(cmd,"sh ./test.sh %d %d",j,i);
			system(cmd);
		}
	}
	
}

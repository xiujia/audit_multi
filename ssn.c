#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include "inp.h"


u_int32_t crctab[256]=
{
0x0,
0x77073096,0xee0e612c,0x990951ba,0x76dc419,0x706af48f,0xe963a535,0x9e6495a3,0xedb8832,
0x79dcb8a4,0xe0d5e91e,0x97d2d988,0x9b64c2b,0x7eb17cbd,0xe7b82d07,0x90bf1d91,0x1db71064,
0x6ab020f2,0xf3b97148,0x84be41de,0x1adad47d,0x6ddde4eb,0xf4d4b551,0x83d385c7,0x136c9856,
0x646ba8c0,0xfd62f97a,0x8a65c9ec,0x14015c4f,0x63066cd9,0xfa0f3d63,0x8d080df5,0x3b6e20c8,
0x4c69105e,0xd56041e4,0xa2677172,0x3c03e4d1,0x4b04d447,0xd20d85fd,0xa50ab56b,0x35b5a8fa,
0x42b2986c,0xdbbbc9d6,0xacbcf940,0x32d86ce3,0x45df5c75,0xdcd60dcf,0xabd13d59,0x26d930ac,
0x51de003a,0xc8d75180,0xbfd06116,0x21b4f4b5,0x56b3c423,0xcfba9599,0xb8bda50f,0x2802b89e,
0x5f058808,0xc60cd9b2,0xb10be924,0x2f6f7c87,0x58684c11,0xc1611dab,0xb6662d3d,0x76dc4190,
0x1db7106,0x98d220bc,0xefd5102a,0x71b18589,0x6b6b51f,0x9fbfe4a5,0xe8b8d433,0x7807c9a2,
0xf00f934,0x9609a88e,0xe10e9818,0x7f6a0dbb,0x86d3d2d,0x91646c97,0xe6635c01,0x6b6b51f4,
0x1c6c6162,0x856530d8,0xf262004e,0x6c0695ed,0x1b01a57b,0x8208f4c1,0xf50fc457,0x65b0d9c6,
0x12b7e950,0x8bbeb8ea,0xfcb9887c,0x62dd1ddf,0x15da2d49,0x8cd37cf3,0xfbd44c65,0x4db26158,
0x3ab551ce,0xa3bc0074,0xd4bb30e2,0x4adfa541,0x3dd895d7,0xa4d1c46d,0xd3d6f4fb,0x4369e96a,
0x346ed9fc,0xad678846,0xda60b8d0,0x44042d73,0x33031de5,0xaa0a4c5f,0xdd0d7cc9,0x5005713c,
0x270241aa,0xbe0b1010,0xc90c2086,0x5768b525,0x206f85b3,0xb966d409,0xce61e49f,0x5edef90e,
0x29d9c998,0xb0d09822,0xc7d7a8b4,0x59b33d17,0x2eb40d81,0xb7bd5c3b,0xc0ba6cad,0xedb88320,
0x9abfb3b6,0x3b6e20c,0x74b1d29a,0xead54739,0x9dd277af,0x4db2615,0x73dc1683,0xe3630b12,
0x94643b84,0xd6d6a3e,0x7a6a5aa8,0xe40ecf0b,0x9309ff9d,0xa00ae27,0x7d079eb1,0xf00f9344,
0x8708a3d2,0x1e01f268,0x6906c2fe,0xf762575d,0x806567cb,0x196c3671,0x6e6b06e7,0xfed41b76,
0x89d32be0,0x10da7a5a,0x67dd4acc,0xf9b9df6f,0x8ebeeff9,0x17b7be43,0x60b08ed5,0xd6d6a3e8,
0xa1d1937e,0x38d8c2c4,0x4fdff252,0xd1bb67f1,0xa6bc5767,0x3fb506dd,0x48b2364b,0xd80d2bda,
0xaf0a1b4c,0x36034af6,0x41047a60,0xdf60efc3,0xa867df55,0x316e8eef,0x4669be79,0xcb61b38c,
0xbc66831a,0x256fd2a0,0x5268e236,0xcc0c7795,0xbb0b4703,0x220216b9,0x5505262f,0xc5ba3bbe,
0xb2bd0b28,0x2bb45a92,0x5cb36a04,0xc2d7ffa7,0xb5d0cf31,0x2cd99e8b,0x5bdeae1d,0x9b64c2b0,
0xec63f226,0x756aa39c,0x26d930a,0x9c0906a9,0xeb0e363f,0x72076785,0x5005713,0x95bf4a82,
0xe2b87a14,0x7bb12bae,0xcb61b38,0x92d28e9b,0xe5d5be0d,0x7cdcefb7,0xbdbdf21,0x86d3d2d4,
0xf1d4e242,0x68ddb3f8,0x1fda836e,0x81be16cd,0xf6b9265b,0x6fb077e1,0x18b74777,0x88085ae6,
0xff0f6a70,0x66063bca,0x11010b5c,0x8f659eff,0xf862ae69,0x616bffd3,0x166ccf45,0xa00ae278,
0xd70dd2ee,0x4e048354,0x3903b3c2,0xa7672661,0xd06016f7,0x4969474d,0x3e6e77db,0xaed16a4a,
0xd9d65adc,0x40df0b66,0x37d83bf0,0xa9bcae53,0xdebb9ec5,0x47b2cf7f,0x30b5ffe9,0xbdbdf21c,
0xcabac28a,0x53b39330,0x24b4a3a6,0xbad03605,0xcdd70693,0x54de5729,0x23d967bf,0xb3667a2e,
0xc4614ab8,0x5d681b02,0x2a6f2b94,0xb40bbe37,0xc30c8ea1,0x5a05df1b,0x2d02ef8d
};


u_int32_t hash_reflect (u_int32_t crc, u_int32_t bitnum)
{

// reflects the lower 'bitnum' bits of 'crc'

u_int32_t i, j=1, crcout=0;

for (i=(u_int32_t)1<<(bitnum-1); i; i>>=1) {
 if (crc & i) crcout|=j;
 j<<= 1;
}
return (crcout);
}
u_int32_t ssn_hash(u_int32_t sip, u_int32_t dip,u_int16_t sport,u_int16_t dport,u_int8_t protocol)
{

// fast lookup table algorithm without augmented zero bytes, e.g. used in pkzip.

u_int32_t crc = 0xffffffff;

crc = hash_reflect(crc, 32);

crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ (0xFF & sip)];
// printf("crc=0x%x, *p=0x%x\n",crc, (0xFF & sip));
crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ (0xFF & (sip >> 8))];
// printf("crc=0x%x, *p=0x%x\n",crc, (0xFF & (sip >> 8)));
crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ (0xFF & (sip >> 16))];
// printf("crc=0x%x, *p=0x%x\n",crc, (0xFF & (sip >> 16)));
crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ (0xFF & (sip >> 24))];
// printf("crc=0x%x, *p=0x%x\n",crc, (0xFF & (sip >> 24)));
crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ (0xFF & dip)];
// printf("crc=0x%x, *p=0x%x\n",crc, (0xFF & dip));
crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ (0xFF & (dip >> 8))];
// printf("crc=0x%x, *p=0x%x\n",crc, (0xFF & (dip >> 8)));
crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ (0xFF & (dip >>16))];
// printf("crc=0x%x, *p=0x%x\n",crc, (0xFF & (dip >> 16)));
crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ (0xFF & (dip >>24))];
// printf("crc=0x%x, *p=0x%x\n",crc, (0xFF & (dip >> 24)));
crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ (0xFF & sport)];
// printf("crc=0x%x, *p=0x%x\n",crc, (0xFF & sport));
crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ (0xFF & (sport >> 8))];
// printf("crc=0x%x, *p=0x%x\n",crc, (0xFF & (sport >> 8)));
crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ (0xFF & dport)];
// printf("crc=0x%x, *p=0x%x\n",crc, (0xFF & dport));
crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ (0xFF & (dport >> 8))];
// printf("crc=0x%x, *p=0x%x\n",crc, (0xFF & (dport >> 8)));
crc = (crc >> 8) ^ crctab[ (crc & 0xff) ^ (0xFF & protocol)];
// printf("crc=0x%x, *p=0x%x\n",crc, (0xFF & protocol));

crc^= 0xffffffff;
crc&= 0xffffffff;

  return(crc & (SSN_HASH_BUCKET_SIZE_MASK)); //��ϣֵ
}

int ssn_lookup(u_int32_t hash,struct ssn *s1p,struct ssn_policy *ssp,char *eth)
{
	struct ssn *sp;
	char * p;
	struct speed_meter * smp;
	u_int16_t appid;
	
	
	sp=(struct ssn *)(ssn_shm_p+sizeof(struct ssn)*hash);
		
	if((sp->age)==0) 
		{
			ssn_create(hash,s1p);
			//fst_pkt_match(hash,eth);
			
			if(sp->drop_flag==1) ssp->transmit_flag=0;
			else
			ssp->transmit_flag=1;
			ssp->copy_flag=1;
			ssp->count=0;
			ssp->user_flag=0;
			ssp->audit_flag=0;
			ssp->sip=0;
			ssp->user_id=0;
		}
	else
		{
			
			sp->age=system_p->ssn_aging_timer;  //�������ȸ����ϻ���ʱ��
			sp->user_flag=ssp->user_flag;
			ssp->sip=sp->sip;
			ssp->user_id=sp->user_id;
			if((sp->audit_flag==0)&&((s1p->in_bytes>72)||(s1p->out_bytes>72)))
      sp->audit_flag=dpi_audit_check(eth,&appid);				 
			
			sp->app_id=appid;	
			ssp->app_id=appid;
			ssp->audit_flag=sp->audit_flag;
			if(sp->drop_flag) 
			{
				ssp->transmit_flag=0;
				return 1;
			 }
			
			if(sp->copy_packets_counter<MAX_COPY_PACKETS_depth) 
				{
					ssp->count=sp->copy_packets_counter;
					ssp->copy_flag=1;
				}
			/*
			else
			 printf("copy flow packet full\n");
			*/
			
			ssp->transmit_flag=1;
			
			if(s1p->in_packets)
				{
					if(sp->in_policy_1!=0)
					{
				   smp=(struct speed_meter *)(speed_meter_shm_p+
	 	 sp->in_policy_1*sizeof(struct speed_meter));
				   //pthread_mutex_lock(&qos_mutex);
				   ssp->transmit_flag=qos(smp,s1p->in_bytes);
				   //pthread_mutex_unlock(&qos_mutex);
				}
				  if(sp->in_policy_2!=0)
				  {
				  smp=(struct speed_meter *)(speed_meter_shm_p+LEVEL_1_POLICY_NUMBER*sizeof(struct speed_meter)+
	 	 sp->in_policy_2*sizeof(struct speed_meter));	
				//pthread_mutex_lock(&qos_mutex);  	
				ssp->transmit_flag=(ssp->transmit_flag)&&(qos(smp,s1p->in_bytes));
				//pthread_mutex_unlock(&qos_mutex);
        }			 
			 }
			else if(s1p->out_packets)
				{
					if(sp->out_policy_1!=0)
					{
			 	     smp=(struct speed_meter *)(speed_meter_shm_p+
	 	 sp->out_policy_1*sizeof(struct speed_meter));
	 	 			//pthread_mutex_lock(&qos_mutex);
			 	   ssp->transmit_flag=qos(smp,s1p->out_bytes);
					//pthread_mutex_unlock(&qos_mutex);
				  }
				  if(sp->out_policy_2!=0)
				  {
				 smp=(struct speed_meter *)(speed_meter_shm_p+LEVEL_1_POLICY_NUMBER*sizeof(struct speed_meter)+
	 	 sp->out_policy_2*sizeof(struct speed_meter));
	 	 		//pthread_mutex_lock(&qos_mutex);
				ssp->transmit_flag=(ssp->transmit_flag)&&(qos(smp,s1p->out_bytes));
			 	//pthread_mutex_unlock(&qos_mutex);
			 }
			 }
			  	
    }
    return 1;
}
void * ssn_aging(void * arg)
{
	int i;
	struct ssn *sp;
	char * p;
	cpu_set_t mask;
	
   CPU_ZERO(&mask);
   CPU_SET(0,&mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) 
       fprintf(stderr, "set main thread affinity failed\n");

	while(1)
	{
  sleep(10);
	for(i=0;i<SSN_HASH_BUCKET_SIZE;i++)
	{
		sp=(struct ssn *)(ssn_shm_p+sizeof(struct ssn)*i);
		if(sp->age>0) 
		{
			sp->age--;
	    		if(sp->age==0)
	    		bzero(sp,sizeof(struct ssn));	
		}  
 	 }
  if(system_p->user_auth_flag) user_auth_aging();
  }
	
}
void * heart_beat(void * arg)
{
	
	cpu_set_t mask;
	char command[1024];
	int i=0;     
	u_int32_t temp_ip; 
	
   CPU_ZERO(&mask);
   CPU_SET(0,&mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) 
       fprintf(stderr, "set main thread affinity failed\n");
  
  sprintf(command,"%s/bypass -t 5 >/dev/null",RUNNING_PATH);
  
  sleep(10);
	while(1)
	{
	system(command);
	if(i==10) 
		{
			temp_ip=get_ip();
	    if(temp_ip!=0) system_p->mgt_ip=temp_ip;
			i=0;
		}
	i++;
	sleep(1);
	}
	
}
int ssn_update(u_int32_t hash,struct ssn *s1)
{
	struct ssn *sp;
	char * p;
	
	sp=(struct ssn *)(ssn_shm_p+sizeof(struct ssn)*hash);
	/*
	if(sp->sip!=s1->sip)
	{
		
	}
	*/
	//hash��ͻ����
	/*
	sp->sip=s1->sip;
	sp->dip=s1->dip;
	sp->sport=s1->sport;
	sp->dport-s1->dport;
	sp->proto=s1->proto;
	*/
	sp->age=system_p->ssn_aging_timer;
	sp->in_bytes+=s1->in_bytes;
	sp->in_packets+=s1->in_packets;
	sp->out_bytes+=s1->out_bytes;
	sp->out_packets+=s1->out_packets;
	return 1;
	
}
int ssn_delete(u_int32_t hash)
{
	//flag fin,delete ssn
	struct ssn * sp;
	sp=(struct ssn *)(ssn_shm_p+sizeof(struct ssn)*hash);
	bzero(sp,sizeof(struct ssn));
	return 1;
	
}
int ssn_create(u_int32_t hash,struct ssn *s1)
{
	
	struct ssn *sp;
	char * p;
	
		
	sp=(struct ssn *)(ssn_shm_p+sizeof(struct ssn)*hash);
		
	bzero(sp,sizeof(struct ssn));
	
	sp->link_id=s1->link_id;
	sp->sip=s1->sip;
	sp->dip=s1->dip;
	sp->sport=s1->sport;
	sp->dport=s1->dport;
	sp->proto=s1->proto;
	sp->age=system_p->ssn_aging_timer;
	sp->in_bytes=s1->in_bytes;
	sp->in_packets=s1->in_packets;
	sp->out_bytes=s1->out_bytes;
	sp->out_packets=s1->out_packets;
	sp->old_in_bytes=0;
	sp->old_in_packets=0;
	sp->old_out_bytes=0;
	sp->old_out_packets=0;
	if(system_p->user_auth_flag)
	sp->user_id=search_user(sp->sip);

	return 1;
}
int ssn_update_count(u_int32_t hash)
{
	
	struct ssn *sp;
	char * p;
			
	sp=(struct ssn *)(ssn_shm_p+sizeof(struct ssn)*hash);
		
  sp->copy_packets_counter++;
				
	return 1;
}

int ssn_summary()
{
	
	return 1;
}

#include <net/if.h>
#include <arpa/inet.h> 
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/if_tun.h>
#include <pthread.h>  

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "list.h"


#define REDCOLOR "\e[0;31m"
#define NONECOLOR  "\e[0m"
#if 1
#define LOG(fmt,...) \
do {\
	printf(REDCOLOR"[%*d]"NONECOLOR fmt "\n",-3,__LINE__,##__VA_ARGS__); \
}while(0)
#else
#define LOG(fmt,...)
#endif
typedef struct tunparam_
{
	int tun_fd;
	struct ifreq ir;
}TUN_PARAM;

#define BUFF_SIZE (2048)
typedef struct data_node_
{
	int data_len;
	char data[BUFF_SIZE];
	struct list_head list;
}DATA_NODE;

DATA_NODE tun0_data;
DATA_NODE tun1_data;	
pthread_mutex_t tun0_mutex = PTHREAD_MUTEX_INITIALIZER;  
pthread_mutex_t tun1_mutex = PTHREAD_MUTEX_INITIALIZER; 

int tun_alloc(int flags, char *ip, struct ifreq *ifreqout)
{
    struct ifreq ifr;
	struct  sockaddr_in  sin; 
    int fd, err;
    char *clonedev = "/dev/net/tun";	
	
    if ((fd = open(clonedev, O_RDWR)) < 0) 
	{
        return fd;
    }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = flags;	
	
    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) 
	{
        close(fd);
        return err;
    }
	
	bzero(&sin,sizeof(sin));  //初始化结构体
    sin.sin_family = AF_INET;  //设置地址家族
    //sin.sin_port = htons(800);  //设置端口
    sin.sin_addr.s_addr = inet_addr(ip);  //设置地址
	memcpy(&ifr.ifr_addr,&sin,sizeof(sin));
	if( 0 == ioctl(fd,SIOCSIFADDR,(char*)&ifr))  //设置ip
	{
		LOG("set ip failed,check please!");
	}
	
    LOG("Open tun/tap device: %s for reading...\n", ifr.ifr_name);
	memcpy(ifreqout,&ifr,sizeof(struct ifreq ));
	
    return fd;
}


void* tun0_read_thread(void *arg)  
{  
	TUN_PARAM *tun = (TUN_PARAM *)arg;
	char buff[BUFF_SIZE] = {0};
	printf("[%d] thread id is %ld\n",__LINE__, pthread_self()); 
    while (1) 
	{
		pthread_mutex_lock(&tun0_mutex);  
        int nread = read(tun->tun_fd, buff, BUFF_SIZE);
        if (nread < 0) 
		{
			pthread_mutex_unlock(&tun0_mutex);
            LOG("Reading from interface");
            usleep(50);
			continue;
        }
		LOG("Read %d bytes from %s device\n", nread,tun->ir.ifr_name);
		DATA_NODE *tmp = (DATA_NODE*)malloc(sizeof(DATA_NODE));
		memset(tmp,0,sizeof(DATA_NODE));
		tmp->data_len = nread;
		memcpy(tmp->data,buff,tmp->data_len);
		list_add_tail(&(tmp->list),&(tun0_data.list));
		
		pthread_mutex_unlock(&tun0_mutex);   
		
		memset(buff,0,BUFF_SIZE);
		usleep(50);
    }
}  

void* tun0_write_thread(void *arg)  
{  
    TUN_PARAM *tun = (TUN_PARAM *)arg;
	struct list_head *pos  = NULL;
	struct list_head *next = NULL;
	DATA_NODE *tmp  = NULL;
	printf("[%d] thread id is %ld\n",__LINE__, pthread_self()); 
    while (1) 
	{
		pthread_mutex_lock(&tun1_mutex);
		list_for_each_safe(pos, next, &(tun1_data.list))
		{
			tmp = list_entry(pos,DATA_NODE,list);
			if( NULL != tmp )
			{
				int nread = write(tun->tun_fd, tmp->data, tmp->data_len);
				if (nread < 0) 
				{
					LOG("write to interface");
					usleep(50);
				}
				LOG("write %d(%d) bytes to %s device\n", nread,tmp->data_len,tun->ir.ifr_name);
			}			
			list_del_init(pos);
			free(tmp);
			tmp = NULL;
		}
		pthread_mutex_unlock(&tun1_mutex);      
		usleep(50);		
    }
} 

void* tun1_read_thread(void *arg)  
{  
	TUN_PARAM *tun = (TUN_PARAM *)arg;
	char buff[BUFF_SIZE]={0};
	printf("[%d] thread id is %ld\n",__LINE__, pthread_self()); 
    while (1) 
	{
		pthread_mutex_lock(&tun1_mutex);  
        int nread = read(tun->tun_fd, buff, BUFF_SIZE);
        if (nread < 0) 
		{
			pthread_mutex_unlock(&tun1_mutex);
            LOG("Reading from interface");
            usleep(50);
			continue;
        }
		LOG("Read %d bytes from %s device\n", nread,tun->ir.ifr_name);
		DATA_NODE *tmp = (DATA_NODE*)malloc(sizeof(DATA_NODE));
		memset(tmp,0,sizeof(DATA_NODE));
		tmp->data_len = nread;
		memcpy(tmp->data,buff,tmp->data_len);
		list_add_tail(&(tmp->list),&(tun1_data.list));
		
		pthread_mutex_unlock(&tun1_mutex);  
		memset(buff,0,BUFF_SIZE);
		usleep(50);
    }
}  

void* tun1_write_thread(void *arg)  
{  
    TUN_PARAM *tun = (TUN_PARAM *)arg;
	struct list_head *pos  = NULL;
	struct list_head *next = NULL;
	DATA_NODE *tmp  = NULL;
	printf("[%d] thread id is %ld\n",__LINE__, pthread_self()); 
    while (1) 
	{
		pthread_mutex_lock(&tun0_mutex);
		list_for_each_safe(pos, next, &(tun0_data.list))
		{
			tmp = list_entry(pos,DATA_NODE,list);
			if( NULL != tmp )
			{
				int nread = write(tun->tun_fd, tmp->data, tmp->data_len);
				if (nread < 0) 
				{
					LOG("write to interface failed");
					usleep(50);					
				}
				LOG("write %d(%d) bytes to %s device\n", nread,tmp->data_len,tun->ir.ifr_name);
			}
			
			list_del_init(pos);
			free(tmp);
			tmp = NULL;
		}
		pthread_mutex_unlock(&tun0_mutex);  
		usleep(50);		
    }
} 
int main(int argc, char *argv[])
{

    TUN_PARAM tun0;
    TUN_PARAM tun1;
	int nread   = 0;    
	pthread_t tun0_read;
	pthread_t tun0_write;
	pthread_t tun1_read;
	pthread_t tun1_write;
    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
     *        IFF_TAP   - TAP device
     *        IFF_NO_PI - Do not provide packet information
     */	
	tun0.tun_fd = tun_alloc(IFF_TAP | IFF_NO_PI,argv[1],&(tun0.ir));
	tun1.tun_fd = tun_alloc(IFF_TAP | IFF_NO_PI,argv[2],&(tun1.ir));	
    if (tun0.tun_fd < 0 || tun1.tun_fd < 0) 
	{
        LOG("Allocating interface");
        exit(1);
    }	
	
	pthread_mutex_init(&tun0_mutex,NULL);  
	pthread_mutex_init(&tun1_mutex,NULL);  
	
	INIT_LIST_HEAD(&tun0_data.list); 
	INIT_LIST_HEAD(&tun1_data.list); 
	if (!pthread_create(&tun0_read, NULL, tun0_read_thread, &(tun0)))  
    {  
        printf("[%d] Create thread success!",__LINE__,tun0_read);  
    }  
	if (!pthread_create(&tun0_write, NULL, tun0_write_thread, &(tun0)))  
    {  
        printf("[%d] Create thread success!",__LINE__,tun0_write);  
    }  
	if (!pthread_create(&tun1_read, NULL, tun1_read_thread, &(tun1)))  
    {  
        printf("[%d] Create thread[%ld] success!",__LINE__,tun1_read);  
    }  
	if (!pthread_create(&tun1_write, NULL, tun1_write_thread, &(tun1)))  
    {  
        printf("[%d] Create thread success!",__LINE__,tun1_write);  
    }   
	
	pthread_join(tun0_read, NULL);  
	pthread_join(tun0_write, NULL);  
	pthread_join(tun1_read, NULL);  
	pthread_join(tun1_write, NULL);  
    pthread_mutex_destroy(&tun0_mutex); 
    pthread_mutex_destroy(&tun1_mutex); 
	
    return 0;
}

//文件名为"SLvbeta.c"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
 #include <unistd.h>

#define CATCH_NUM 1000
#define MAXLINE 10
#define SERVERADDR "192.168.7.1"

/*
*argc:应用程序参数个数，包括应用程序本身
*argv[]:具体的参数内容，字符串形式
*./SLvb <filename> <r:w>    r表示读，w表示写
*/
int main(int argc, char *argv[])
{	
	int read_len = 0;
	int rssiRaw;
	int sockfd, n, rec_len;
	struct sockaddr_in servaddr;
	char sendline[MAXLINE];
	int i = 0;

	int ret = 0;
	int fd = 0;
	int rssi = 0;
	char *filename;
	int * readbuf;
	if(argc != 3)    //共有三个参数
	{
		printf("Error usage!\r\n");
		return -1;
	}

	filename = argv[1];    //获取文件名称

	fd = open(filename, O_RDWR);
	if(fd < 0)
	{
		printf("cannot open file %s\r\n", filename);
		return -1;
	}


    
	if(!strcmp(argv[2], "r"))
	{
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("create socket error: %s(errno:%d)\n", strerror(errno), errno);
			exit(0);
		}

		memset (&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(8002);
		if (inet_pton(AF_INET, SERVERADDR, &servaddr.sin_addr) < 0)
		{
			printf("inet_pton error for %s\n", SERVERADDR);
			exit(0);
		}
		if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		{
			printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
			exit(0);
		}
		readbuf = malloc(CATCH_NUM * sizeof(int));
		memset(readbuf, 0, CATCH_NUM * sizeof(int));
wait:
		usleep(200000);
		read_len = read(fd, readbuf, CATCH_NUM);
//		if(read_len)
//			printf("%d\r\n", read_len);
		if (!read_len)
			goto wait;
		memset(sendline, 0, MAXLINE * sizeof(char));
		sprintf(sendline, "LEN:%d\r\n", read_len);
		if (send(sockfd, sendline, MAXLINE, 0) < 0)
		{
			printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
        		exit(0);
		}
//		for (i = 0; i < CATCH_NUM; i++)
//		{
//			printf("%d\r\n", readbuf[i]);
//		}
		for (i = 0; i< read_len; i++)
		{
			memset(sendline, 0, MAXLINE * sizeof(char));
			sprintf(sendline, "%d\r\n", readbuf[i]);
			if (send(sockfd, sendline, MAXLINE, 0) < 0)
			{
				printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
        			exit(0);
			}
		}
		goto wait;
	}

	close(fd);
	close(sockfd);
	return 0;
}

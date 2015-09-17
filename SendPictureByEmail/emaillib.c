#include "emaillib.h"
#include "encb64.h"

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define MIME		"MIME-Version: 1.0"
#define CONTENT_TYPE	"Content-Type: multipart/mixed; boundary=\"" BOUNDARY "\""	  
#define FIRST_BODY	"--" BOUNDARY "\r\nContent-Type: text/plain;charset=" CHARSET \
	"\r\nContent-Transfer-Encoding: 7bit\r\n\r\n"

#define mail_error(x) do { printf("%s:%d ERR:%s\n" , __func__ , __LINE__ , (x));} while(0) 
#define printf(args...) printf(args)

char g_en64_buf[1023 * 1024];

//send a mail to user. The subject and sender's name use default value
//mail's subject=HSG_MAIL_SUBJECT
// retur value : =0 mean success ; = -1 mean fail

static int connect_nonblock(const char *ip,int port,int sec) {
	int	sockfd, flags, n;
	int sock_error;
	socklen_t		len;
	fd_set			rset, wset;
	struct timeval	tval;
	struct sockaddr_in servaddr;	
	struct hostent *hent;

	if (NULL == ip)
		return 0;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		return 0;	
	hent = gethostbyname(ip);
	if (NULL == hent) 
	{
		close(sockfd);
		return 0;
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr = *(struct in_addr *)hent->h_addr;

	flags = fcntl(sockfd, F_GETFL, 0);
	if (-1 == flags) 
	{
		close(sockfd);
		return 0;
	}
	if ( -1 == fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) )
	{
		close(sockfd);
		return 0;
	}

	if ( (n = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) )
		if (errno != EINPROGRESS)
		{
			close(sockfd);
			return 0;
		}

		/* Do whatever we want while the connect is taking place. */
		if ( 0 == n)
			return sockfd;	/* connect completed immediately */

		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);
		wset = rset;
		tval.tv_sec = sec;
		tval.tv_usec = 0;

		if ( select(sockfd+1, &rset, &wset, NULL,sec ? &tval : NULL) == 0 ) 
		{
			close(sockfd);		/* timeout */		
			return 0;
		}

		if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) 
		{		
			len = sizeof(sock_error);
			if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sock_error, &len) < 0)
			{  
				close(sockfd);			
				return 0;
			}
			if (sock_error)
			{
				errno = sock_error;
				close(sockfd);
				return 0;
			}		
		} else {
			//printf("select error: sockfd not set");
			close(sockfd);
			return 0;
		}
		/* restore file status flags */
		if ( fcntl(sockfd, F_SETFL, flags) < 0 )
		{
			close(sockfd);
			return 0;
		}
		return sockfd;

}

// if success , return the number of bytes read
// if error, return -1;
ssize_t readline2(int fd, void *vptr, size_t maxlen) 
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	*ptr = 0;
	for (n = 1; n < maxlen; n++) {
again:
		if ( (rc = read(fd, &c, 1)) == 1) {
			*ptr++ = c;
			if (c == '\n') 
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			if (n == 1)
				return(0);	/* EOF, no data read */
			else
				break;	/* EOF, some data was read */
		} else {
			if (errno == EINTR)
				goto again;
			return(-1);		/* error, errno set by read() */
		}
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}



int set_image_filename(const char *filename, const char *path)
{
	if (m_imgNum >= MAXLINE) 
		return 0;

	int i = m_imgNum;

	sprintf(imgName[i],"%s",filename);
	sprintf(imgPath[i],"%s",path);

	m_imgNum++;
	return 1;
}


static int write_image_buf(int socketfd)
{
	unsigned char buffer[10*1024];
	
	struct stat fi;
	int i;
	caddr_t fp;
	int fd;
	char imgPathName[MAXLINE];
	for(i=0;i<m_imgNum;i++)
	{
		memset( g_en64_buf, 0, sizeof(g_en64_buf) );
		sprintf(buffer,"\r\n%s\r\n","--" BOUNDARY);
		//	sprintf(buffer,"\r\n%s\r\n", BOUNDARY);
		write(socketfd,buffer,strlen(buffer));
		sprintf(buffer,"Content-Type: image/jpeg; name=%s\r\n",imgName[i]);
		//sprintf(buffer,"Content-Type: application/zip; name=%s\r\n",imgName[i]);
		//sprintf(buffer,"Content-Type: multipart/mixed; name=%s\r\n",imgName[i]);
		if (write(socketfd,buffer,strlen(buffer)) < 0)
			return 0;

		sprintf(buffer,"Content-disposition: attachment; filename=%s\r\nContent-Transfer-Encoding: base64\r\n\r\n",imgName[i]);
		//sprintf(buffer,"Content-disposition: inline; filename=%s\r\nContent-Transfer-Encoding: base64\r\n\r\n",imgName[i]);
		if (write(socketfd,buffer,strlen(buffer)) < 0)
			return 0;	
		sprintf(imgPathName,"%s/%s",imgPath[i],imgName[i]);
		/*
		if((fd = open(imgPathName, O_RDONLY)) == -1)
		continue;
		fstat(fd, &fi);
		fp = mmap(NULL, fi.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
		if(fp == (caddr_t) -1)
		return 0;
		*/
		printf("%s: encrypt_b64 now\n", __func__);
		int len = encrypt_b64_file_to_buf (g_en64_buf, imgPathName);
		if( len < 0 )
		{
			printf("%s: encrypt_b64 error\n", __func__);
			continue;
		}
		printf("%s: encrypt_b64 ok,len=%d\n", __func__, len);

		static const char *backup_path = "/tmp/image_cache/backup";
		char back_path[256] = {0};
		sprintf(back_path, "%s/%s", backup_path, imgName[i]);
		
		// 备份
		rename(imgPathName, back_path);
		
		// delete file
		unlink(imgPathName);

		if (write(socketfd, (void *)g_en64_buf, len) <= 0 )
		{
			printf("%s: write image error: %s\n", __func__,strerror(errno));
			// if(write(socketfd, (void *) fp, fi.st_size) == -1)
			return 0;
			//munmap(fp, fi.st_size);
		}
	}
	return 1;
}




int send_mail_to_user(const char *smtp_server, const char *passwd, const char *send_from, const char *send_to,const char *body)
{
	struct timeval tv;
	int sockfd;
	int i,cnt,rt;
	int auth_base64_flag;
	unsigned char buffer[1024*1024],*ptr,username[MAXLINE];
	int first_auth = FALSE;

	struct hostent *hent;

	if ( (NULL == smtp_server) || (NULL == send_from) ||
		(NULL == body) || ( NULL ==passwd ) || (NULL == send_to) )
	{
		printf("smtp_server=%s send_from=%s body=%s passwd=%s send_to=%s\n", \
			smtp_server,send_from,body,passwd,send_to);
		return -1;
	}


	//get the username. eg: test@test.com ---> "test"
	strncpy(username,send_from,MAXLINE);
	ptr = username;
	while ( *ptr != '@' && *ptr != '\0' )
		ptr++;
	if ( *ptr == '\0' )
		return -1;	 	
	else if ( *ptr == '@' ) {
		*ptr = '\0';
	} else 
		return -1;

	//printf("%d: test\n", __LINE__);
	sockfd = connect_nonblock(smtp_server,25,20);
	if (sockfd <= 0) {
		mail_error("open socket");
		goto error;
	}

	tv.tv_sec = 10;
	tv.tv_usec = 0;	
	if ( setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		mail_error("setsockopt");
		goto error;
	}	

	//printf("%d: test\n", __LINE__);
	readline2(sockfd,buffer,MAXLINE);
	if ( strncmp(buffer,"220",3) ) {		
		mail_error("not smtp server");
		goto error;
	}

	sprintf(buffer,"EHLO %s\r\n",smtp_server);
	if (write(sockfd,buffer,strlen(buffer)) < 0){
		mail_error("EHLO");
		goto error;
	}

	//printf("%d: test\n", __LINE__);

	auth_base64_flag = 0;
	do {
		rt = readline2(sockfd,buffer,MAXLINE);		
		if (rt <= 0){
			mail_error("readline2");
			goto error;
		}
		if ( !strncmp("250-AUTH",buffer,8) && strstr(buffer,"LOGIN") )
			auth_base64_flag = 1;
	} while (strncmp(buffer,"250 ",4) != 0) ;

	if (auth_base64_flag != 0)
	{
		if ( write(sockfd,"AUTH LOGIN\r\n",12)  < 0 )
			goto error;
		if ( readline2(sockfd,buffer,MAXLINE) < 0)
			goto error;
		if ( 0 != strncmp("334 ",buffer,4) )
			goto error;
		//write the base64 crypted username
		encrypt_b64(buffer,username,strlen(username) );
		strcat(buffer,"\r\n");
		if ( write(sockfd,buffer,strlen(buffer)) < 0)
			goto error;

		if ( readline2(sockfd,buffer,MAXLINE) < 0)
			goto error;
		if ( 0 != strncmp("334 ",buffer,4) )
		{
			first_auth = FALSE;
		}else{
			first_auth = TRUE;
		} 

		//write the base64 crypted passwd
		encrypt_b64(buffer,passwd,strlen(passwd) );
		//  hsg_debug("Tinggui: %s\n",passwd);
		strcat(buffer,"\r\n");
		if ( write(sockfd,buffer,strlen(buffer)) < 0)
			goto error;
		if ( readline2(sockfd,buffer,MAXLINE) < 0)
			goto error;
		if ( 0 != strncmp("235 ",buffer,4) )
		{
			first_auth = FALSE;
		}else{
			first_auth = TRUE;
		}
		// auth ok!
	}


	//printf("%d: test\n", __LINE__);

	// the first auth failed , try use the send_from as the email account
	if ( auth_base64_flag != 0  && first_auth == FALSE )
	{
		if ( write(sockfd,"AUTH LOGIN\r\n",12)  < 0 )
		{
			printf("%d: error\n", __LINE__);
			goto error;
		}
		if ( readline2(sockfd,buffer,MAXLINE) < 0)
		{
			printf("%d: error\n", __LINE__);
			goto error;
		}
		if ( 0 != strncmp("334 ",buffer,4) )
		{
			printf("%d: error\n", __LINE__);
			goto error;
		}
		//write the base64 crypted username
		encrypt_b64(buffer,send_from,strlen(send_from) );
		strcat(buffer,"\r\n");
		if ( write(sockfd,buffer,strlen(buffer)) < 0)
		{
			printf("%d: error\n", __LINE__);
			goto error;
		}
		if ( readline2(sockfd,buffer,MAXLINE) < 0)
		{
			printf("%d: error\n", __LINE__);
			goto error;
		}
		if ( 0 != strncmp("334 ",buffer,4) )
		{
			printf("%d: error\n", __LINE__);
			goto error;
		}

		//write the base64 crypted passwd
		encrypt_b64(buffer,passwd,strlen(passwd) );
		strcat(buffer,"\r\n");
		if ( write(sockfd,buffer,strlen(buffer)) < 0)
		{
			printf("%d: error\n", __LINE__);
			goto error;
		}
		if ( readline2(sockfd,buffer,MAXLINE) < 0)
		{
			printf("%d: error\n", __LINE__);
			goto error;
		}
		if ( 0 != strncmp("235 ",buffer,4) ) 
		{
			printf("%d: error\n", __LINE__);
			goto error;
		}
		// auth ok!
	}


	//printf("%d: test\n", __LINE__);

	sprintf(buffer,"MAIL FROM: <%s>\r\n",send_from);
	write(sockfd,buffer,strlen(buffer));
	readline2(sockfd,buffer,MAXLINE);
	// hsg_debug("Tinggui2: %s, %s",buffer,send_from);
	if (strncmp(buffer,"250",3)) {
		printf("The smtp server didn't accept sender name(%s) \n",send_from);
		goto error;
	}

	sprintf(buffer,"RCPT TO: <%s>\r\n",send_to);
	write(sockfd,buffer,strlen(buffer));
	readline2(sockfd,buffer,MAXLINE);
	printf("We get from smtp server: %s\n",buffer);
	if (strncmp(buffer,"250",3)) {
		printf("The smtp server didn't accept the send_to(%s) ret=%s\n",send_to,buffer);
		goto error;
	}

	//printf("%d: test\n", __LINE__);

	write(sockfd,"DATA\r\n",6);
	readline2(sockfd,buffer,MAXLINE);
	if (strncmp(buffer,"354",3)) {
		printf("send \"DATA\" command error:%s\n",buffer);	 
		goto error; 
	}	 

	printf("start write to server\n");

	//next we will write the header
	//To: example@hotmail.com
	//From: in4s@in4s.com
	//Subject: e-Home Security
	//MIME-Version: 1.0
	//Content-Type: multipart/mixed; boundary="PtErOdAcTyL2285248"
	//#define MIME		"MIME-Version: 1.0"
	//#define CONTENT_TYPE	"Content-Type: multipart/mixed"		  

	sprintf(buffer,"From: %s\r\n",send_from);
	if (write(sockfd,buffer,strlen(buffer)) < 0)
		goto error;

	sprintf(buffer,"To: %s\r\n",send_to);
	if (write(sockfd,buffer,strlen(buffer)) < 0)
		goto error;

	sprintf(buffer,"%s" "\r\n",MIME);
	if (write(sockfd,buffer,strlen(buffer)) < 0)
		goto error;

	sprintf(buffer,"Subject: %s\r\n",MAIL_SUBJECT); 
	if (write(sockfd,buffer,strlen(buffer)) < 0)
		goto error;	

	sprintf(buffer,"%s\r\n",CONTENT_TYPE);
	if (write(sockfd,buffer,strlen(buffer)) < 0)
		goto error;

	sprintf(buffer,"\r\n%s\r\n", "--" BOUNDARY);
	write(sockfd,buffer,strlen(buffer));

	sprintf(buffer,"%s",FIRST_BODY);
	write(sockfd,buffer,strlen(buffer)); 

	if (write(sockfd,body,strlen(body)) < 0)
		goto error;

	printf("text complete, next image\n");

	// 发送附件
	if(!write_image_buf(sockfd))
		goto error;
	printf("image complete\n");

	sprintf(buffer,"\r\n%s\r\n","--" BOUNDARY);
	write(sockfd,buffer,strlen(buffer));
	write(sockfd,"\r\n.\r\n",5);
	write(sockfd,"quit\r\n",6);
	close(sockfd);
	//hsg_debug("Send Mail to list successfully\n");
	return 0;	

error:
	//hsg_debug("Send Mail to list fail\n");
	close(sockfd);
	return -1;	
}

// retur value : =0 mean success ; = -1 mean fail
//int send_mail_to_user(char *smtp_server,char *passwd, char *send_from,const char *send_to,char *body)
int send_mail_to_user_list(char *smtp_server,
						   char *passwd,
						   char *send_from,
						   char **send_to_list,
						   int usertotal,
						   char *body) 
{
	int  ret = 0,d=0;
	int i=0;
	for (i=0; i<usertotal; i++)
	{
		d = send_mail_to_user(smtp_server,passwd, send_from,send_to_list[i],body);
		if( 0 != d )
			printf("send mail to %s error\n", send_to_list[i]);
		ret += d;
	}

	/* unlink all image file here */
	char image[MAXLINE];
	for(i=0;i<m_imgNum;i++)
	{
		sprintf(image,"%s/%s",imgPath[i],imgName[i]);
		unlink(image);
	}
	m_imgNum = 0;

	return (ret != 0) ? -1 : 0;
}

void reset_img_num()
{
	m_imgNum = 0;
}

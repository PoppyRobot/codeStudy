#ifndef _EMAILLIB_H_
#define _EMAILLIB_H_

#include "encb64.h"

#define MAIL_SUBJECT  "Home Security Box Message"

#define CHARSET	 "gbk"
//#define CHARSET "utf-8"
//#define CHARSET "sjis"
#define	BOUNDARY "---Embbed test mail---"

#ifndef MAXLINE
#define MAXLINE		128
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

static char imgName[MAXLINE][MAXLINE];
static char imgPath[MAXLINE][MAXLINE];
static int m_imgNum=0;
int set_image_filename(const char *filename, const char *path);

void reset_img_num();

int send_mail_to_user(const char *smtp_server, const char *passwd, const char *send_from, const char *send_to,const char *body);

int send_mail_to_user_list(char *smtp_server,char *passwd, char *send_from,char **send_to_list, int usertotal, char *body);
	

#endif
	
	


#include "emaillib.h"
#include "encb64.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <time.h>

// 图片缓存目录
const char *IMG_CACHE_DIR = "/tmp/image_cache";

// 邮件服务器地址
const char *MAIL_SERVER_ADDR = "smtp.126.com";

const char *from = "azx12358@126.com";
const char *to = "azx12358@126.com";
char msg[] = "only for test";
char g_passwd[100];
int g_send_mail = 0;

const char *download_file_cmd = "wget http://125.70.86.244:9527/?action=snapshot -O /tmp/image_cache";

//
void call_exec_non_block(const char *cmd) 
{
	if (cmd == NULL)
	{
		return;
	}

	char cmd_buf[1024] = {0};
	//后台执行,非阻塞
	snprintf(cmd_buf, sizeof(cmd_buf), "%s 2>&1", cmd);

	FILE *pp = popen(cmd_buf, "r"); //建立管道

	if (!pp) {
		return;
	}
	
	pclose(pp); //关闭管道
}

// 通过wget命令下载文件到指定目录
void GetPictureFromWeb()
{
	time_t now = time(NULL);
	struct tm * curtime = localtime( &now );
	char ctemp[20];
	strftime(ctemp, 20, "%Y%m%d%H%M%S", curtime);
	
	char tmp_cmd[256] = {0};
	sprintf(tmp_cmd, "%s/Home_monitor%s.JPG", download_file_cmd, ctemp);
	
	// 通过wget命令下载文件到指定目录
	call_exec_non_block(tmp_cmd);
}

/// 获取文件列表，返回找到的文件个数
int GetFileList(const char *path)
{
	DIR              *pDir ;
	struct dirent    *ent  ;
	int               i=0  ;
	char              childpath[512];

	pDir = opendir(path);
	memset(childpath,0,sizeof(childpath));

	reset_img_num();
	int img_num = 0;
	while ((ent = readdir(pDir)) != NULL)
	{
		if (ent->d_type & DT_DIR)
		{
			continue;
		}

		// 取得文件名
		printf("fileName: %s\n", ent->d_name);
		set_image_filename(ent->d_name, IMG_CACHE_DIR);
		img_num++;
		break;
	}
	closedir(pDir);

	return img_num;
}

int daemon_mode()
{
	pid_t pid = fork();

	if (pid == -1) {
		printf("fork error.\n");
		return -1;
	} else if (pid != 0) {
		return 0;
	} 

	setsid();
	signal(SIGHUP, SIG_IGN);

	while (1) {
		pid = fork();

		if (pid != 0) {
			if (waitpid(pid, NULL, WNOHANG) != 0) {
				return 1;
			}
			int code;
			waitpid(pid, &code, 0);
		}else {
			break;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
///
/// 1、遍历图片缓存目录
/// 2、设置发送文件的路径
/// 3、发送邮件
///
//////////////////////////////////////////////////////////////////////////
void *server_thread( void *arg ) 
{
	int img_num = 0;
	GetPictureFromWeb();
	while (1) {
		// 获取文件列表
		if (0 == g_send_mail) {
			img_num = 0;
		} else {
			img_num = GetFileList(IMG_CACHE_DIR);
		}
		if (0 == img_num)
		{
			static time_t fulltimeout = 0;
			time_t timeout = time(NULL);
			if( (timeout - fulltimeout) > 60*10 )
			{
				printf("No image file in cache dir: %s\n", IMG_CACHE_DIR);

				fulltimeout = timeout;
			}

			sleep(60 * 2);  // 5分钟取一张
			GetPictureFromWeb();
			continue;
		}
		
		// 不发送邮件，只截图保存
		if (0 == g_send_mail) {
			continue;
		}
		
		// 发送
		int ret = send_mail_to_user(MAIL_SERVER_ADDR, g_passwd, from, to, msg);
		printf("ret= %d\n", ret);
		sleep(5);
	}

	printf("leaving server thread, calling cleanup function now\n");

	return NULL;
}

int RunCheckPicture() {
	pthread_t threadID;
	/* create thread and pass context to thread function */
	pthread_create(&threadID, NULL, server_thread, NULL);
	pthread_detach(threadID);

	return 0;
}


int main(int argc,char **argv)
{
	if( argc < 4 )
	{
		printf("Usage: a.out [i|x] password sendmail\n");
		return 0;
	}
	memset(g_passwd, 0, sizeof(g_passwd));
	strncpy(g_passwd, argv[2], sizeof(g_passwd));
	printf("g_passwd= %s\n", g_passwd);
	
	g_send_mail = atoi(argv[3]);
	printf("g_send_mail= %d\n", g_send_mail);

	RunCheckPicture();

	while (1)
	{
		sleep(1000);
	}
	

	//if( 'i' == argv[1][0] )
	//	//set_image_filename("test.jpg", "./");
	//	GetFileList(IMG_CACHE_DIR);

	//int i;
	//i = send_mail_to_user(MAIL_SERVER_ADDR, argv[2], from, to, msg);
	//printf("ret=%d\n", i);

	return 0;
}

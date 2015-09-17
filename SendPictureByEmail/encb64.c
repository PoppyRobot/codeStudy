#include  "encb64.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

char s_buff[1024*1024*10];
      
/*                                                                                                                      
int encrypt_b64(char* dest,char* src,size_t size)
{
	int i, tmp = 0, b64_tmp = 0;
	
	while ((size - b64_tmp) >= 3) {
		dest[tmp] = 0x3F & (src[b64_tmp]>>2);
		dest[tmp+1] = ((src[b64_tmp]<<4) & 0x30) | ((src[b64_tmp+1]>>4) & 0x0F);
		dest[tmp+2] = ((src[b64_tmp+1]<<2) & 0x3C) | ((src[b64_tmp+2]>>6) & 0x03);
		dest[tmp+3] = 0x3F & src[b64_tmp+2];
		for (i=0; i<=3; i++) {
			if ((0 <= dest[tmp+i]) && (dest[tmp+i] <= 25))
				dest[tmp+i] += 'A';
			else	
				if (dest[tmp+i] <= 51)
					dest[tmp+i] += 'a' - 26;	       
				else
					if (dest[tmp+i] <= 61)
						dest[tmp+i] += '0' - 52;
			if (dest[tmp+i] == 62)
				dest[tmp+i] = '+';
			if (dest[tmp+i] == 63)
				dest[tmp+i] = '/';
		}
		
		tmp += 4;
		b64_tmp += 3;
	} //end while	
	if (b64_tmp == size) {
		dest[tmp] = '\0';
		return tmp;
	}
	if ((size - b64_tmp) == 1) {    //one
		dest[tmp] = 0x3F & (src[b64_tmp]>>2);
		dest[tmp+1] = (src[b64_tmp]<<4) & 0x30;
		dest[tmp+2] = '=';
	}
	else {    //two
		dest[tmp] = 0x3F & (src[b64_tmp]>>2);
		dest[tmp+1] = ((src[b64_tmp]<<4) & 0x30) | ((src[b64_tmp+1]>>4) & 0x0F);
		dest[tmp+2] = (src[b64_tmp+1]<<2) & 0x3F;	
	}

	for (i=0; i<=(size - b64_tmp); i++) {
		if ((0 <= dest[tmp+i]) && (dest[tmp+i] <= 25))
			dest[tmp+i] += 'A';
		else	
			if (dest[tmp+i] <= 51)
				dest[tmp+i] += 'a' - 26;	       
			else
				if (dest[tmp+i] <= 61)
					dest[tmp+i] += '0' - 52;	//end if
		if (dest[tmp+i] == 62)
			dest[tmp+i] = '+';
		if (dest[tmp+i] == 63)
			dest[tmp+i] = '/';
	}
	
	dest[tmp+3] = '=';
	dest[tmp+4] = '\0';

	return tmp+4;
}
*/

size_t encrypt_b64(char* dest,char* src,size_t size)
{
	int i, tmp = 0, b64_tmp = 0;
	
	if ( dest == NULL || src == NULL )
		return -1;
	
	while ((size - b64_tmp) >= 3) {
		// every 3 bytes of source change to 4 bytes of destination, 4*6 = 3*8
		dest[tmp] = 0x3F & (src[b64_tmp]>>2);
		dest[tmp+1] = ((src[b64_tmp]<<4) & 0x30) | ((src[b64_tmp+1]>>4) & 0x0F);
		dest[tmp+2] = ((src[b64_tmp+1]<<2) & 0x3C) | ((src[b64_tmp+2]>>6) & 0x03);
		dest[tmp+3] = 0x3F & src[b64_tmp+2];
		for (i=0; i<=3; i++) {
			if ( (dest[tmp+i] <= 25) )
				dest[tmp+i] += 'A';
			else	
				if (dest[tmp+i] <= 51)
					dest[tmp+i] += 'a' - 26;	       
				else
					if (dest[tmp+i] <= 61)
						dest[tmp+i] += '0' - 52;
			if (dest[tmp+i] == 62)
				dest[tmp+i] = '+';
			if (dest[tmp+i] == 63)
				dest[tmp+i] = '/';
		}
		
		tmp += 4;
		b64_tmp += 3;
	} //end while	
	if (b64_tmp == size) {
		dest[tmp] = '\0';
		return tmp;
	}
	if ((size - b64_tmp) == 1) {    //one
		dest[tmp] = 0x3F & (src[b64_tmp]>>2);
		dest[tmp+1] = (src[b64_tmp]<<4) & 0x30;
		dest[tmp+2] = '=';
	}
	else {    //two
		dest[tmp] = 0x3F & (src[b64_tmp]>>2);
		dest[tmp+1] = ((src[b64_tmp]<<4) & 0x30) | ((src[b64_tmp+1]>>4) & 0x0F);
		dest[tmp+2] = (src[b64_tmp+1]<<2) & 0x3F;	
	}

	for (i=0; i<=(size - b64_tmp); i++) {
		if  (dest[tmp+i] <= 25)
			dest[tmp+i] += 'A';
		else	
			if (dest[tmp+i] <= 51)
				dest[tmp+i] += 'a' - 26;	       
			else
				if (dest[tmp+i] <= 61)
					dest[tmp+i] += '0' - 52;	//end if
		if (dest[tmp+i] == 62)
			dest[tmp+i] = '+';
		if (dest[tmp+i] == 63)
			dest[tmp+i] = '/';
	}
	
	dest[tmp+3] = '=';
	dest[tmp+4] = '\0';

	return tmp+4;
}


int encrypt_b64_file_to_buf (char* en64_buf, const char* s_file)
{
	int r_size = 1024*1024;
	int s_fd = open(s_file,O_RDONLY);
	//char s_buff[1024*1024];	
	char *ptr ;
	int count;

	if (s_fd < 0)
		return -1;
	if (NULL == s_buff) {
		close(s_fd);
		return -2;
	}
	ptr = en64_buf;
	do {
		int i;
		count = read(s_fd,s_buff,r_size);
		if (count < 0) {
			close(s_fd);
			return -3;
		}
		i = count;
		while (i >= 17*3) {
			encrypt_b64(ptr,&s_buff[count-i],17*4);
			ptr += 17*4;
			ptr += sprintf(ptr,"\n");			
			i -= 17*3;
		}
		if (i > 0) {
			ptr += encrypt_b64(ptr,&s_buff[count-i],i);
			ptr += sprintf(ptr,"\n");			
		}
		if (count < r_size)
			break;
	} while (count == r_size);
	close(s_fd);
	return (ptr-en64_buf);
}

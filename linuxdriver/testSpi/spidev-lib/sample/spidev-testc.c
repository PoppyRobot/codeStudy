#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <spidev_lib.h>
#include "trk.h"
#include <sys/time.h>
#include <bits/time.h>
#include <errno.h>
#include <sched.h>
#include <sys/resource.h>

spi_config_t spi_config;
uint8_t tx_buffer[32];
uint8_t rx_buffer[32];
uint32_t reg_addr_base = 0x00000000;
int max_data_cnt = 400;
int max_irq_cnt  = 200;
int data_ready = 0;

typedef struct 
{
	uint32_t reg_addr;
	int value;
}st_read_dat;

st_read_dat read_data[1024] = {{0}};

int spifd;
#define BB_BASE_ADDR (0x00010000)

#define BB_TRK_SOFT_RESET               (BB_BASE_ADDR + 0x0200)          
#define BB_TRK_CH_STATUS                (BB_BASE_ADDR + 0x0204)          
#define BB_TRK_CH_BIT_SYNC              (BB_BASE_ADDR + 0x0208)          
#define BB_TRK_CH_STATUS                (BB_BASE_ADDR + 0x020C)          
#define BB_TRK_CH_BIT_FIFO              (BB_BASE_ADDR + 0x020C)          
#define BB_TRK_CH_SNR_FIFO              (BB_BASE_ADDR + 0x020C)          
#define BB_TRK_CH_BIT_FIFO_STATUS       (BB_BASE_ADDR + 0x020C)          
#define BB_TRK_CH_SNR_FIFO_STATUS       (BB_BASE_ADDR + 0x020C)          
                                                                         
                                                                         
#define BB_TRK_CH_PARA_ID(x)            (BB_BASE_ADDR + 0x020C +(x<<8))  
#define BB_TRK_CH_M_K(x)                (BB_BASE_ADDR + 0x020C +(x<<8))  
#define BB_TRK_CH_SV_NO(x)              (BB_BASE_ADDR + 0x020C +(x<<8))  
#define BB_TRK_CH_SV_TYPE(x)            (BB_BASE_ADDR + 0x020C +(x<<8))  
#define BB_TRK_CH_FCW(x)                (BB_BASE_ADDR + 0x020C +(x<<8))  
#define BB_TRK_CH_CA_CHIP(x)            (BB_BASE_ADDR + 0x020C +(x<<8))  
#define BB_TRK_CH_CODE_NCO(x)           (BB_BASE_ADDR + 0x020C +(x<<8))  
#define BB_TRK_CH_CARR_NCO(x)           (BB_BASE_ADDR + 0x020C +(x<<8))  
#define BB_TRK_CH_BIT_CNT(x)            (BB_BASE_ADDR + 0x020C +(x<<8))
#define BB_TRK_CH_CARR_LPF(x)           (BB_BASE_ADDR + 0x020C +(x<<8))
#define BB_TRK_CH_CODE_LPF(x)           (BB_BASE_ADDR + 0x020C +(x<<8))
#define BB_TRK_CH_CARR_FCW(x)           (BB_BASE_ADDR + 0x020C +(x<<8))
#define BB_TRK_CH_CODE_FCW(x)           (BB_BASE_ADDR + 0x020C +(x<<8))
#define BB_TRK_CH_CODE_VEL(x)           (BB_BASE_ADDR + 0x020C +(x<<8))
                                                                         
#define BB_CLK_CNT_L                    (BB_BASE_ADDR + 0x020C)
#define BB_CLK_CNT_H                    (BB_BASE_ADDR + 0x020C)

#define BB_BIT_FIFO_DEPTH   4500
#define BB_SNR_FIFO_DEPTH   1024

STR_TRK_INFO g_trk_ctrl[BB_TRK_CH_NUM];

UINT32 origin_bit_fifo[BB_BIT_FIFO_DEPTH];
UINT32 origin_snr_fifo[BB_SNR_FIFO_DEPTH];
UINT32 origin_bit_cnt;
UINT32 origin_snr_cnt;

STR_LOCK_DATA trk_lock_data[BB_TRK_CH_NUM];

UINT32 bit_fifo_buf_len;

void disable_trk_int()
{}

void enable_trk_int()
{}


void trk_int_isr()
{
    UINT32 i;
    UINT32 bit_cnt, s_n_cnt;
    UINT32 value;
    UINT32 ch_status;
    
	disable_trk_int();

    // get origin bit 
    bb_get_reg_value( &bit_cnt, BB_TRK_CH_BIT_FIFO_STATUS);

    for( i=0; i<bit_cnt; i++)
    {
        bb_get_reg_value( &value, BB_TRK_CH_BIT_FIFO);
        origin_bit_fifo[i] = value;
        origin_bit_cnt++;
        if( origin_bit_cnt >= BB_BIT_FIFO_DEPTH)
        {
            break;
        }
    }

    //get origin signal and noise

    bb_get_reg_value( &s_n_cnt, BB_TRK_CH_SNR_FIFO_STATUS);

    for( i=0; i<s_n_cnt; i++)
    {
        bb_get_reg_value( &value, BB_TRK_CH_SNR_FIFO);
        origin_snr_fifo[i] = value;
        origin_snr_cnt++;
        if( origin_snr_cnt >= BB_SNR_FIFO_DEPTH)
        {
            break;
        }
    }

    //get lock data

    bb_get_reg_value( &ch_status, BB_TRK_CH_STATUS);

    for( i=0; i<BB_TRK_CH_NUM; i++)
    {
        if(((1<<i)&ch_status) != 0)
        {
            bb_get_reg_value(&trk_lock_data[i].bit_cnt, BB_TRK_CH_BIT_CNT(i));
            bb_get_reg_value(&trk_lock_data[i].chip_cnt, BB_TRK_CH_CA_CHIP(i));
            bb_get_reg_value(&trk_lock_data[i].carrier_lpf, BB_TRK_CH_CARR_LPF(i));
            bb_get_reg_value(&trk_lock_data[i].code_lpf, BB_TRK_CH_CODE_LPF(i));
            bb_get_reg_value(&trk_lock_data[i].carrier_fcw, BB_TRK_CH_CARR_FCW(i));
            bb_get_reg_value(&trk_lock_data[i].code_fcw, BB_TRK_CH_CODE_FCW(i));
            bb_get_reg_value(&trk_lock_data[i].code_vel, BB_TRK_CH_CODE_VEL(i));
            bb_get_reg_value(&trk_lock_data[i].bit_cnt, BB_TRK_CH_BIT_CNT(i));
            bb_get_reg_value(&trk_lock_data[i].bit_cnt, BB_TRK_CH_BIT_CNT(i));
            trk_lock_data[i].lock_data_usable = TRUE;
        }
    }    
}


/* get data from FPGA */
int bb_get_reg_value(int *p_value, uint32_t reg_addr)
{
	int ret;
	int i;
	memset(tx_buffer, 0, 32);
	memset(rx_buffer, 0, 32);
	uint16_t reg_addr_L15 = reg_addr & 0x7FFF;	
	tx_buffer[0] = 0x00 | ((reg_addr_L15 >> 8) & 0x7F);
	tx_buffer[1] = reg_addr_L15 & 0xFF;
	
	//tx_buffer[2] = 0x98;
	//tx_buffer[3] = 0x88;
	//printf("R:tx_buffer[0]=0x%X tx_buffer[1]=0x%X\n", tx_buffer[0], tx_buffer[1]);
	ret = spi_xfer(spifd, tx_buffer, 6, rx_buffer, 6);
	
	if (ret == 6) {
		*p_value = 0;
		*p_value = rx_buffer[2];
		*p_value = *p_value << 24 | rx_buffer[3];
		*p_value = *p_value << 16 | rx_buffer[4];
		*p_value = *p_value << 8  | rx_buffer[5];
		
		//printf("R:rx_buffer[2]=0x%X rx_buffer[3]=0x%X rx_buffer[4]=0x%X rx_buffer[5]=0x%X\n", rx_buffer[2], rx_buffer[3],rx_buffer[4], rx_buffer[5]);
	}
  /*	
	for (i = 0; i < ret; i++) {
    	printf("%.4X ", rx_buffer[i]);
  	}
 	printf("\n");
	*/
	return 0;
}

/* write data to FPGA */
int bb_set_reg_value(int value, uint32_t reg_addr)
{
	int ret;
    int i;
	memset(tx_buffer, 0, 32);
	memset(rx_buffer, 0, 32);
    uint16_t reg_addr_L15 = reg_addr & 0x7FFF;
    tx_buffer[0] = 0x80 | (reg_addr_L15 >> 8);
    tx_buffer[1] = reg_addr_L15 & 0xFF;
    tx_buffer[2] = value >> 24 & 0xFF;
    tx_buffer[3] = value >> 16 & 0xFF;
	tx_buffer[4] = value >> 8  & 0xFF;
	tx_buffer[5] = value       & 0xFF;
	printf("W:tx_buffer[0]=0x%X tx_buffer[1]=0x%X tx_buffer[2]=0x%X tx_buffer[3]=0x%X tx_buffer[4]=0x%X tx_buffer[5]=0x%X\n", tx_buffer[0], tx_buffer[1], tx_buffer[2], tx_buffer[3],tx_buffer[4], tx_buffer[5]);
    ret = spi_xfer(spifd, tx_buffer, 6, rx_buffer, 6);

#if 0
	printf("reg set:\n");
    for (i = 0; i < ret; i++) {
        printf("%.4X ", rx_buffer[i]);
    }
     printf("\n");
#endif
    return 0;
}

int get_value[1024] = {0};

void test()
{
	// trk_int_isr();

#if 0
	uint32_t reg_addr = 0x00001002;
	int value = 0;
	int value2 = 50;
	int step = 2;
	int i;
	for (i = 0; i < 400; i++) {
		//bb_set_reg_value(value2,  reg_addr);
	//	value2 += 20;
		bb_get_reg_value(&value, reg_addr);
		//printf("reg get value= %d\n", value);
		reg_addr += step;
	}
#endif

#if 1
	printf("\n");
	int value = 0;
	int i;
	static int cnt = 1;
	uint32_t reg_addr;
	reg_addr = reg_addr_base;
	memset(read_data, 0, sizeof(read_data));
	for (i = 0; i < 400; i++) {
		bb_get_reg_value(&value, reg_addr);
		
		read_data[i].reg_addr = reg_addr;
		read_data[i].value = value;
		
		reg_addr++;
		//printf("R:reg_addr=%x value=%d\n", reg_addr, value);
	}
	
	data_ready = 1;
	
	void disable_irq();
	//disable_irq();
#endif
}

struct timeval t_irq_pre;

static unsigned int irq_cnt = 0;
void my_signal_fun(int signum)
{
#ifdef TIME_TRACE
	struct timeval tpstart,tpend; 
	float timeuse; 
	gettimeofday(&tpstart,NULL);
	float t_irq = 1000000 * (tpstart.tv_sec - t_irq_pre.tv_sec) + tpstart.tv_usec-t_irq_pre.tv_usec; 
	t_irq /= 1000000;
	printf("IRQ time:%f ms\n", t_irq * 1000);
	t_irq_pre = tpstart;
#endif	
	irq_cnt++;
	test();

	printf("irq count= %d\n", irq_cnt);
	
#ifdef TIME_TRACE	
	gettimeofday(&tpend,NULL); 
	timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec-tpstart.tv_usec; 
	timeuse /= 1000000; 
	printf("Used Time:%f ms\n", timeuse * 1000);
#endif
}

void disable_irq()
{
	//中断使能寄存器
	int REG_IRQ_ENABLE = 0x7FFF;

	// disable FPGA IRQ
	int irq_enable = 0;
	int irq_value = (irq_enable << 16) + 0;// max_data_cnt;
	bb_set_reg_value(irq_value, REG_IRQ_ENABLE);
	printf("disable irq_value= %d\n", irq_value);
}

void enable_irq()
{
	int REG_IRQ_ENABLE = 0x7FFF;
	// enable FPGA IRQ
	int irq_enable = 0x01;
	int irq_value = (irq_enable << 16) + max_irq_cnt;
	bb_set_reg_value(irq_value, REG_IRQ_ENABLE);
	printf("enable irq_value= %d\n", irq_value);
}

int main(int argc, char *argv[])
{
  //int spifd;
  int ret;
  int i;
  int n = 0;
  int fd;
  int Oflags;
  spi_config.mode = 0;
  spi_config.speed = 10000000;
  spi_config.delay = 0;
  spi_config.bits_per_word = 8;
  
  
  {
#if 0
	struct sched_param mysched;
	mysched.sched_priority = 60;
	if ( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		puts(" ERROR IN SETTING THE SCHEDULER UP");
		perror( "errno" );
		exit( 0 );
	}

	
#endif
int pri_level = getpriority(PRIO_PROCESS, getpid());
	printf("cur priority= %d\n", pri_level);
	getchar();
	setpriority(PRIO_PROCESS, getpid(), -20);
	}

#if 1
  signal(SIGIO, my_signal_fun);

  spifd = spi_open("/dev/spidev4.0", spi_config);
  if (spifd < 0)
  {
    printf("can't open /dev/spidev4.0!\n");
    exit(1);
  }
  printf("device /dev/spidev4.0 open OK.\n");  
  
  memset(tx_buffer, 0, 32);
  memset(rx_buffer, 0, 32);
  
  fd = open("/dev/fpgaIRQ", O_RDWR);
  if (fd < 0)
  {
	 spi_close(spifd);
     printf("can't open /dev/fpgaIRQ!\n");
     exit(1);
  }
  printf("device /dev/fpgaIRQ open OK.\n"); 
  
  fcntl(fd, F_SETOWN, getpid());
  Oflags = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, Oflags | FASYNC);
#endif

	// 测试写6000个数据
	{
		disable_irq();
		
		#if 0
		int i;
		uint32_t reg_addr = reg_addr_base;
		for (i = 0; i < max_data_cnt; i++) {
			// bb_set_reg_value(int value, uint32_t reg_addr)
			bb_set_reg_value(i, reg_addr);
			printf("W:reg_addr=0x%0X value=%d\n", reg_addr, i);
			reg_addr++;
		}
		
		#endif
		
		// enable FPGA IRQ
		//gettimeofday(&t_irq_pre,NULL);
		enable_irq();		
	}
	
//  test();

  while (1) {
	if (data_ready) {
		FILE *fd = fopen("testResult.txt", "a+");
		if (fd == NULL) {
			printf("open file failed.");
			return -1;
		}
	
		int all_data_is_ok = 1;
		static int ok_data_cnt = 0;
		data_ready = 0;
		uint32_t reg_addr = reg_addr_base;
		for (i = 0; i < 400; i++) {
			if (read_data[i].reg_addr != reg_addr || read_data[i].value != i ) {
				all_data_is_ok = 0;
				fprintf(fd, "E-R:reg_addr=0x%0X value=%d\n",  read_data[i].reg_addr, read_data[i].value );
			}
			//printf("R:reg_addr=0x%0X value=%d\n",  read_data[i].reg_addr, read_data[i].value );
			reg_addr++;
		}
		
		if (all_data_is_ok) {
			ok_data_cnt++;
			fprintf(fd, "All Data is OK. totalCnt=%d\n", ok_data_cnt);
			
			if (ok_data_cnt % 1000 == 0) {
				printf("All Data is OK. totalCnt=%d\n", ok_data_cnt);
			}
		}
		
		fflush(fd);
		fclose(fd);
	}
  
  	sleep(100);
  }

  spi_close(spifd);
  close(fd); 
}

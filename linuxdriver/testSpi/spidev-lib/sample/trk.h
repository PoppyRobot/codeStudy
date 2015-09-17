
#ifndef _TRK_H_
#define _TRK_H_


#ifdef __cplusplus
extern "C" {
#endif




#include "Comm_type.h"


typedef struct _STR_MSG
{
    UINT32 buf[32];/*����BITԭʼ����*/
    UINT32 bit_cnt;/*��ǰ����BIT����*/
}STR_MSG, *PSTR_MSG;

typedef struct _STR_LOCK_DATA
{
    UINT32 bit_cnt;/*bit����*/
    UINT32 chip_cnt;/*ms��chip��������16λΪms������32λΪchip����*/
    INT32 carrier_lpf, code_lpf;
    UINT32 code_nco, code_fcw;/*code_nco����α��ʱʹ��*/
    UINT32 carrier_fcw, carrier_nco, carrier_pha_cnt;
    INT16 code_vel;
    BOOL lock_data_usable;
}STR_LOCK_DATA, *PSTR_LOCK_DATA;

typedef struct _STR_TRK_INFO
{
    STR_MSG msg;/*ԭʼ����BIT */
    UINT8 status;/*����ͨ��״̬��IDLE��BIT_SYNC��TRACKING*/
    UINT8 sv_type;
    UINT8 sv_no;
    UINT32 start_time;/*��ʼ�����ʱ��*/
    FLOAT64 signal[64], noise[64]; /*����SNR�õ���ԭʼ�źź�����*/
    UINT32 snr_cnt;/*Signal ��Noise�ĸ��� */
    FLOAT64 hw_doppler;
    UINT32 cn0;
    STR_LOCK_DATA lock_data; /*�жϼ�����������Ϣ*/
    
}STR_TRK_INFO, *PSTR_TRK_INFO;

#define BB_TRK_CH_NUM              32

#define TRK_CH_STATUS_IDLE               0
#define TRK_CH_STATUS_BIT_SYNC           1
#define TRK_CH_STATUS_TRACKING           2

#define BB_CLK_FREQ       10e6

#define BIT_SYNC_TIMEOUT_TH         15000//unit:ms

#define TRK_RESET_BIT_SYNC_TIMEOUT             1
#define TRK_RESET_CN0_LOW                      2
#define TRK_RESET_DOPPLER_ERROR                3
#define TRK_RESET_CROSS_CORRECTION             4
#define TRK_RESET_PR_ERROR                     5

extern void Get_cur_time(FLOAT64* p_time );
extern UINT32 Get_time_diff_ms(PBOOL p_is_big, PFLOAT64 p_time1, PFLOAT64 p_time2);

extern void trk_ctrl();
extern STR_TRK_INFO g_trk_ctrl[BB_TRK_CH_NUM];
extern void trk_int_isr();

#ifdef __cplusplus
}
#endif


#endif//_ACQ_H_


#ifndef __MY_FDS__
#define __MY_FDS__
//引用FDS头文件
#include "fds.h"
#include "common.h"

#define MAX_HIS_CNT                    240
//定义文件ID和该文件包含的记录的KEY
#define DEVICE_FILE                    (0x1000)//文件ID
#define DEVICE_HIS_KEY                 (0x1001)//记录历史数据的个数
#define DEVICE_TIME_KEY                (0x1002)//记录设备时间，该记录存放的文件ID=0X1000
#define HIS_DATA_START                 (0x2000)//历史数据存储起始位
//#define DEVICE_DESP_KEY                   (0x1002)//记录KEY，该记录存放的文件ID=0X1000

//记录历史数据存储个数的数据
typedef struct
{
	uint8_t num;
}__attribute__((aligned(4)))his_cnt_t;

//记录设备时间
typedef struct
{
	uint32_t     localtime;//本地时间，
}__attribute__((aligned(4)))devicetime_t;

void wait_for_fds_ready(void);
void read_hiscnt_record(void);
void update_hisdata_record(void);
void read_hisdata_record(uint8_t num);

void read_devicetime_record(void);
void update_devicetime_record(void);

void My_fds_Init(void);

#endif

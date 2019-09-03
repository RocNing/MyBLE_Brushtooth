#include "my_fds.h"
#include "my_ble.h"
#include "my_ble_uarts.h"

uint8_t cur_his_num = 0;
//记录历史数据存储个数的数据
his_cnt_t his_cnt =
{
	.num = 0,
};

devicetime_t devicetime = //初始化
{
	.localtime = 620650965  //2019年1月11日
};

his_data_t his_data = 
{
	.end_sec = 0,
	.mode = 1,
	.dur_time = 0,
	.run_time = 120,
};
//包含设备基本信息的记录
fds_record_t const m_hiscnt_record =
{
    .file_id           = DEVICE_FILE,        //文件id
    .key               = DEVICE_HIS_KEY,     //文件key
    .data.p_data       = &his_cnt,
    //记录的长度必须以4字节（字）为单位
    .data.length_words = (sizeof(his_cnt) + 3) / sizeof(uint32_t),
};

//包含设备时间的记录
fds_record_t const m_devicetime_record =
{
    .file_id           = DEVICE_FILE,        //文件id
    .key               = DEVICE_TIME_KEY,    //文件key
    .data.p_data       = &devicetime,
    //记录的长度必须以4字节（字）为单位
    .data.length_words = (sizeof(devicetime) + 3) / sizeof(uint32_t),
};

//定义FDS异步操作标志结构体
my_fds_info_t my_fds_info;

//FDS事件处理函数
static void fds_evt_handler(fds_evt_t const * p_evt)
{
    //判断事件类型
    switch (p_evt->id)
    {
        case FDS_EVT_INIT://FDS初始化事件
					  //初始化成功
            if (p_evt->result == FDS_SUCCESS)
            {
                my_fds_info.busy = false;
            }
            break;
        //FDS写记录事件
        case FDS_EVT_WRITE:
        {
            //写记录成功
		    if (p_evt->result == FDS_SUCCESS)
            {
                my_fds_info.busy = false;
            }
        } break;
		//FDS更新记录事件
        case FDS_EVT_UPDATE:
        {
            //写记录成功
		    if (p_evt->result == FDS_SUCCESS)
            {
                my_fds_info.busy = false;
            }
        } break;
		//FDS碎片整理事件
        case FDS_EVT_GC:
        {
            //碎片整理成功
		    if (p_evt->result == FDS_SUCCESS)
            {
				  my_fds_info.busy = false;
            }
        } break;
        default:
            break;
    }
}

//等待FDS初始化完成
void wait_for_fds_ready(void)
{
    while (my_fds_info.busy)
    {
        (void) sd_app_evt_wait();
    }
}
void read_hiscnt_record(void)
{
	ret_code_t rc;
	
	fds_record_desc_t desc = {0};     //定义并初始化记录描述符结构体变量
	fds_find_token_t  tok  = {0};     //定义并初始化记录查找令牌结构体变量
   
	memset(&tok, 0x00, sizeof(fds_find_token_t)); //清零tok，从头查找		
	rc = fds_record_find(DEVICE_FILE, DEVICE_HIS_KEY, &desc, &tok);//在DEVICE_FILE文件中查找记录m_version_record
	//查找到记录后，读取记录内容
	if (rc == FDS_SUCCESS)
	{
		fds_flash_record_t temp = {0};				
		rc = fds_record_open(&desc, &temp);//打开记录读取记录内容
		APP_ERROR_CHECK(rc);
		static his_cnt_t temp_his_cnt;				
		memcpy(&temp_his_cnt, temp.p_data, sizeof(his_cnt_t));//拷贝记录内容
		cur_his_num = temp_his_cnt.num;
		//串口打印记录内容
		NRF_LOG_INFO("Current hisdata num is: %d  ", temp_his_cnt.num);		
		rc = fds_record_close(&desc);  //读取后，关闭记录
		APP_ERROR_CHECK(rc);
	}
}
uint8_t send_his_data[16] = {0};
void read_hisdata_record(uint8_t num)
{
	ret_code_t rc;
	
	fds_record_desc_t desc = {0};     //定义并初始化记录描述符结构体变量
	fds_find_token_t  tok  = {0};     //定义并初始化记录查找令牌结构体变量
   
	memset(&tok, 0x00, sizeof(fds_find_token_t)); //清零tok，从头查找		
	rc = fds_record_find(DEVICE_FILE, HIS_DATA_START+num, &desc, &tok);//在DEVICE_FILE文件中查找记录m_version_record
	if (rc == FDS_SUCCESS)
	{
		fds_flash_record_t temp = {0};				
		rc = fds_record_open(&desc, &temp);//打开记录读取记录内容
		APP_ERROR_CHECK(rc);
		static his_data_t temp_his_data;				
		memcpy(&temp_his_data, temp.p_data, sizeof(his_data_t));//拷贝记录内容
		his_data.end_sec = temp_his_data.end_sec;
		his_data.mode = temp_his_data.mode;
		his_data.dur_time = temp_his_data.dur_time;
		his_data.run_time = temp_his_data.run_time;
		//串口打印记录内容
		NRF_LOG_INFO("Current hisdata is:%d %d %d %d ", his_data.end_sec, his_data.mode, his_data.dur_time, his_data.run_time);		
		rc = fds_record_close(&desc);  //读取后，关闭记录
		APP_ERROR_CHECK(rc);
		
		//ble串口透传上报历史数据	
		*((uint32_t *)&send_his_data[0]) = his_data.end_sec;
		send_his_data[4] = his_data.mode;
		*((uint16_t *)&send_his_data[5]) = his_data.dur_time;
		*((uint16_t *)&send_his_data[7]) = his_data.run_time;
		send_his_data[12] = 0x34;
		send_his_data[13] = 0x56;
		send_his_data[14] = 0x78;
		send_his_data[15] = 0x90;
		//uint16_t send_his_len = sizeof(send_his_data);
		//my_ble_uarts_data_send(send_his_data, &send_his_len);
	}
}

//更新历史数据
void update_hisdata_record(void)
{
	ret_code_t rc;
	
	fds_record_desc_t desc = {0};  //定义并初始化记录描述符结构体变量
	fds_find_token_t  tok  = {0};  //定义并初始化记录查找令牌结构体变量
	
	cur_his_num++;  //历史数据计数累加
	if(cur_his_num > 240)
		cur_his_num = 1;
		
	//更新历史数据记录
	if((my_fds_info.hisdata_record_update == true) && (my_fds_info.busy == false))
	{
		//清零tok，从头查找
		memset(&tok, 0x00, sizeof(fds_find_token_t));
		my_fds_info.busy = true;
		//在DEVICE_FILE文件中查找记录m_version_record
		rc = fds_record_find(DEVICE_FILE, DEVICE_HIS_KEY, &desc, &tok);	
		if (rc == FDS_SUCCESS)
		{			 
			 //更新索引号
			 his_cnt.num = cur_his_num;
			 rc = fds_record_update(&desc, &m_hiscnt_record);
			 APP_ERROR_CHECK(rc);
			 wait_for_fds_ready();
		}
		
		his_data.end_sec = rtc_sec;
		his_data.mode = my_motor.mode;
		his_data.dur_time = my_motor.dur_time;
		his_data.run_time = my_motor.run_time;
		
		//包含历史数据的记录
		fds_record_t const m_hisdata_record =
		{
			.file_id           = DEVICE_FILE,                    //文件id
			.key               = HIS_DATA_START+cur_his_num,     //文件key
			.data.p_data       = &his_data,
			//记录的长度必须以4字节（字）为单位
			.data.length_words = (sizeof(his_data) + 3) / sizeof(uint32_t),
		};
		//清零tok，从头查找
		memset(&tok, 0x00, sizeof(fds_find_token_t));
		my_fds_info.busy = true;
		rc = fds_record_find(DEVICE_FILE, HIS_DATA_START+cur_his_num, &desc, &tok);	
		if (rc == FDS_SUCCESS)//更新
		{	
			rc = fds_record_update(&desc, &m_hisdata_record);
			APP_ERROR_CHECK(rc);
			wait_for_fds_ready();
			NRF_LOG_INFO("Update hisdata!");
		}
		else //没有找到，写入记录
		{
			my_fds_info.busy = true;
			rc = fds_record_write(&desc, &m_hisdata_record);
			APP_ERROR_CHECK(rc);
			wait_for_fds_ready();
			NRF_LOG_INFO("Write hisdata!");
		}
		my_fds_info.hisdata_record_update = false;
	}
}

void read_devicetime_record(void)
{
	ret_code_t rc;
	fds_record_desc_t desc = {0};
	fds_find_token_t  tok  = {0};

	memset(&tok, 0x00, sizeof(fds_find_token_t));
	rc = fds_record_find(DEVICE_FILE, DEVICE_TIME_KEY, &desc, &tok);
	if (rc == FDS_SUCCESS)
	{
		fds_flash_record_t temp = {0};
		rc = fds_record_open(&desc, &temp);
		APP_ERROR_CHECK(rc);
		static devicetime_t temp_devtime;
		memcpy(&temp_devtime, temp.p_data, sizeof(devicetime_t));
		rtc_sec = temp_devtime.localtime;// + 0x386d4380;
		NRF_LOG_INFO("RTC: date=%d\r\n", rtc_sec);
		rc = fds_record_close(&desc);
		APP_ERROR_CHECK(rc);
	}
}

void update_devicetime_record(void)
{
	ret_code_t rc;
	
	fds_record_desc_t desc = {0};  //定义并初始化记录描述符结构体变量
	fds_find_token_t  tok  = {0};  //定义并初始化记录查找令牌结构体变量
		
	//更新记录m_version_record
	if(my_fds_info.busy == false)
	{
		//清零tok，从头查找
		memset(&tok, 0x00, sizeof(fds_find_token_t));
		my_fds_info.busy = true;
		//在DEVICE_FILE文件中查找记录m_version_record
		rc = fds_record_find(DEVICE_FILE, DEVICE_TIME_KEY, &desc, &tok);	
		if (rc == FDS_SUCCESS)
		{
			 //my_fds_info.devicetime_record_update = false;
			 //更新记录m_version_record
			 devicetime.localtime = rtc_sec;
			 rc = fds_record_update(&desc, &m_devicetime_record);
			 APP_ERROR_CHECK(rc);
			 wait_for_fds_ready();
			 NRF_LOG_INFO("RTC update: date=%d\r\n", rtc_sec);
		}
	}
}

void My_fds_Init()
{
	ret_code_t rc;
	
	fds_record_desc_t desc = {0};  //定义并初始化记录描述符结构体变量
	fds_find_token_t  tok  = {0};  //定义并初始化记录查找令牌结构体变量	
	(void)fds_register(fds_evt_handler); //注册FDS事件回调函数接收FS事件
	
	my_fds_info.busy = true;
	rc = fds_init();       //初始化FDS
	APP_ERROR_CHECK(rc);   //用错误处理模块检查函数返回值	
	wait_for_fds_ready();  //FDS初始化是异步的，因此要等待FDS初始化完成
	
	memset(&tok, 0x00, sizeof(fds_find_token_t)); //清零tok，从头查找
	
	rc = fds_record_find(DEVICE_FILE, DEVICE_HIS_KEY, &desc, &tok);	 //在DEVICE_FILE文件中查找记录m_version_record	
	if (rc != FDS_SUCCESS)    //没有查找到m_version_record记录，写入记录
	{
		my_fds_info.busy = true;
		rc = fds_record_write(&desc, &m_hiscnt_record);
		APP_ERROR_CHECK(rc);
		wait_for_fds_ready();
	}
	//读取设备时间
	memset(&tok, 0x00, sizeof(fds_find_token_t));//清零tok，从头查找	
	rc = fds_record_find(DEVICE_FILE, DEVICE_TIME_KEY, &desc, &tok);//在DEVICE_FILE文件中查找记录m_desp_record								
	//没有查找到m_desp_record记录，写入记录
	if (rc != FDS_SUCCESS)
	{
		my_fds_info.busy = true;
		rc = fds_record_write(&desc, &m_devicetime_record);
		APP_ERROR_CHECK(rc);
		wait_for_fds_ready();
	}
}

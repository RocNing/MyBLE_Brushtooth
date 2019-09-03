#include "my_ble.h"


NRF_BLE_GATT_DEF(m_gatt);               //定义名称为m_gatt的GATT模块实例
NRF_BLE_QWR_DEF(m_qwr);                 //定义一个名称为m_qwr的排队写入实例
BLE_ADVERTISING_DEF(m_advertising);     //定义名称为m_advertising的广播模块实例
BLE_UARTS_DEF(m_uarts);                 //定义名称为m_uarts的串口透传服务实例

static ble_uuid_t m_adv_uuids[] =       //定义UUID数组，这里只包含了一个用户自定义UUID，用于ble串口服务             
{
	{BLE_UUID_USR1_SERVICE,                 BLE_UUID_TYPE_BLE},//串口透传服务
};

//定义串口透传服务UUID列表
/*static ble_uuid_t nus_adv_uuids[]          =                                          
{
    {BLE_UUID_UARTS_SERVICE, 				UARTS_SERVICE_UUID_TYPE}
};*/

//该变量用于保存连接句柄，初始值设置为无连接
uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; 

/*************************************本地函数*************************************/
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);//BLE事件处理函数


uint32_t my_ble_uarts_data_send(uint8_t  * p_data, uint16_t  * p_length)
{
	return ble_uarts_data_send(&m_uarts, p_data, p_length, m_conn_handle);
}

uint32_t my_ble_uarts_resp_send(uint8_t  * p_data, uint16_t  * p_length)
{
	return ble_uarts_resp_send(&m_uarts, p_data, p_length, m_conn_handle);
}

my_ble_t my_ble = {
	.is_connected = false,
	.is_ble_con = false,
	.is_send_device_info = false,
	.is_send_his_data = false,
	.device_info_s = 0,
	.his_data_s = 0,
};

//BLE事件处理函数
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code = NRF_SUCCESS;
    //判断BLE事件类型，根据事件类型执行相应操作
    switch (p_ble_evt->header.evt_id)
    {
        //断开连接事件
		case BLE_GAP_EVT_DISCONNECTED:           
			NRF_LOG_INFO("Disconnected.");//打印提示信息
			my_ble.is_connected = false;
			LED_BLE_OFF;
            break;
				
        //连接事件
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected.");	
			my_ble.is_connected = true;
            LED_BLE_ON;//设置指示灯状态为连接状态，即指示灯D2常亮
				    
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;//保存连接句柄
			//将连接句柄分配给排队写入实例，分配后排队写入实例和该连接关联，这样，
			//当有多个连接的时候，通过关联不同的排队写入实例，很方便单独处理各个连接
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            break;
				
        //PHY更新事件
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
			//响应PHY更新规程
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;
				
        //GATT客户端超时事件
        case BLE_GATTC_EVT_TIMEOUT:
            NRF_LOG_DEBUG("GATT Client Timeout.");
		//断开当前连接
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
				
        //GATT服务器超时事件
        case BLE_GATTS_EVT_TIMEOUT:
            NRF_LOG_DEBUG("GATT Server Timeout.");
				    //断开当前连接
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            break;
    }
}



/******************************************DFU相关代码******************************************/
//关机准备处理程序。在关闭过程中，将以1秒的间隔调用此函数，直到函数返回true。当函数返回true时，表示应用程序已准备好复位为DFU模式
static bool app_shutdown_handler(nrf_pwr_mgmt_evt_t event)
{
    switch (event)
    {
        case NRF_PWR_MGMT_EVT_PREPARE_DFU:
            NRF_LOG_INFO("Power management wants to reset to DFU mode.");
            // YOUR_JOB: Get ready to reset into DFU mode
            //
            // If you aren't finished with any ongoing tasks, return "false" to
            // signal to the system that reset is impossible at this stage.
            //
            // Here is an example using a variable to delay resetting the device.
            //
            // if (!m_ready_for_reset)
            // {
            //      return false;
            // }
            // else
            //{
            //
            //    // Device ready to enter
            //    uint32_t err_code;
            //    err_code = sd_softdevice_disable();
            //    APP_ERROR_CHECK(err_code);
            //    err_code = app_timer_stop_all();
            //    APP_ERROR_CHECK(err_code);
            //}
            break;

        default:
            // YOUR_JOB: Implement any of the other events available from the power management module:
            //      -NRF_PWR_MGMT_EVT_PREPARE_SYSOFF
            //      -NRF_PWR_MGMT_EVT_PREPARE_WAKEUP
            //      -NRF_PWR_MGMT_EVT_PREPARE_RESET
            return true;
    }

    NRF_LOG_INFO("Power management allowed to reset to DFU mode.");
    return true;
}

//注册优先级为0的应用程序关闭处理程序
NRF_PWR_MGMT_HANDLER_REGISTER(app_shutdown_handler, 0);

//SoftDevice状态监视者
static void buttonless_dfu_sdh_state_observer(nrf_sdh_state_evt_t state, void * p_context)
{
    if (state == NRF_SDH_EVT_STATE_DISABLED)
    {
	    //表明Softdevice在复位之前已经禁用，告之bootloader启动时应跳过CRC
        nrf_power_gpregret2_set(BOOTLOADER_DFU_SKIP_CRC);

        //进入system off.
        nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
    }
}

//注册SoftDevice状态监视者，用于SoftDevice状态改变或者即将改变时接收SoftDevice事件
NRF_SDH_STATE_OBSERVER(m_buttonless_dfu_state_obs, 0) =
{
    .handler = buttonless_dfu_sdh_state_observer,
};

//获取广播模式、间隔和超时时间
static void advertising_config_get(ble_adv_modes_config_t * p_config)
{
    memset(p_config, 0, sizeof(ble_adv_modes_config_t));

    p_config->ble_adv_fast_enabled  = true;
    p_config->ble_adv_fast_interval = APP_ADV_INTERVAL;
    p_config->ble_adv_fast_timeout  = APP_ADV_DURATION;
}
//断开当前连接，设备准备进入bootloader之前，需要先断开连接
static void disconnect(uint16_t conn_handle, void * p_context)
{
    UNUSED_PARAMETER(p_context);
    //断开当前连接
    ret_code_t err_code = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_WARNING("Failed to disconnect connection. Connection handle: %d Error: %d", conn_handle, err_code);
    }
    else
    {
        NRF_LOG_DEBUG("Disconnected connection handle %d", conn_handle);
    }
}

//DFU事件处理函数。如果需要在DFU事件中执行操作，可以在相应的事件里面加入处理代码
static void ble_dfu_evt_handler(ble_dfu_buttonless_evt_type_t event)
{
    switch (event)
    {
        //该事件指示设备正在准备进入bootloader
	    case BLE_DFU_EVT_BOOTLOADER_ENTER_PREPARE:
        {
            NRF_LOG_INFO("Device is preparing to enter bootloader mode.");
            //防止设备在断开连接时广播
            ble_adv_modes_config_t config;
            advertising_config_get(&config);			  
            config.ble_adv_on_disconnect_disabled = true;//连接断开后设备不自动进行广播					  
            ble_advertising_modes_config_set(&m_advertising, &config);//修改广播配置
			
		    //断开当前已经连接的所有其他绑定设备。在设备固件更新成功（或中止）后，需要在启动时接收服务更改指示
            uint32_t conn_count = ble_conn_state_for_each_connected(disconnect, NULL);
            NRF_LOG_INFO("Disconnected %d links.", conn_count);
            break;
        }
        //该事件指示函数返回后设备即进入bootloader
        case BLE_DFU_EVT_BOOTLOADER_ENTER:
			//如果应用程序有数据需要保存到Flash，通过app_shutdown_handler返回flase以延迟复位，从而保证数据正确写入到Flash
		    LED1_ON;
			LED2_ON;
		    LED3_ON;
			LED4_ON;
			LED5_ON;
            NRF_LOG_INFO("Device will enter bootloader mode.");
            break;
        //该事件指示进入bootloader失败
        case BLE_DFU_EVT_BOOTLOADER_ENTER_FAILED:
		    //进入bootloader失败，应用程序需要采取纠正措施来处理问题
            NRF_LOG_ERROR("Request to enter bootloader mode failed asynchroneously.");
            break;
        //该事件指示发送响应失败
        case BLE_DFU_EVT_RESPONSE_SEND_ERROR:
            NRF_LOG_ERROR("Request to send a response to client failed.");
            //发送响应失败，应用程序需要采取纠正措施来处理问题
            APP_ERROR_CHECK(false);
            break;

        default:
            NRF_LOG_ERROR("Unknown event from ble_dfu_buttonless.");
            break;
    }
}



const unsigned char Num2CharTable[] = "0123456789ABCDEF";
//十六进制数转换为字符串
void HexArrayToString(unsigned char *hexarray,int length,unsigned char *string)
{
    int i = length-1;
	while(i >= 0)
	{
	    *(string++) = Num2CharTable[((hexarray[i] >> 4) & 0x0f)];
		*(string++) = Num2CharTable[(hexarray[i] & 0x0f)];
		i--;
	}
	*string = 0x0;
}


/******************************************初始化相关代码******************************************/
//初始化BLE协议栈
static void ble_stack_init(void)
{
    ret_code_t err_code;
    //请求使能SoftDevice，该函数中会根据sdk_config.h文件中低频时钟的设置来配置低频时钟
	//内部RC时钟，校准间隔16，恒温校准间隔2，精度20ppm
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);
    
    //定义保存应用程序RAM起始地址的变量
    uint32_t ram_start = 0;
	  //使用sdk_config.h文件的默认参数配置协议栈，获取应用程序RAM起始地址，保存到变量ram_start
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    //使能BLE协议栈
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    //注册BLE事件监视者回调函数
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


//GAP参数初始化，该函数配置需要的GAP参数，包括设备名称，外观特征、首选连接参数
static void gap_params_init(void)
{
    ret_code_t              err_code;	  
    ble_gap_conn_params_t   gap_conn_params;//定义连接参数结构体变量
    ble_gap_conn_sec_mode_t sec_mode;
	ble_gap_addr_t          my_addr;        //地址结构体变量,用来保存读取的设备地址和类型
	
	char name[BLE_GAP_DEVNAME_MAX_LEN];      //保存设备名
	err_code = sd_ble_gap_addr_get(&my_addr);//读取设备地址
	memset(name,0,sizeof(name));             //清零 
	strcpy(name,DEVICE_NAME);
	unsigned char str[20];
	HexArrayToString(my_addr.addr,6,str);
	strcat(name,(char *)str);
	
	for(uint8_t i=0;i<6;i++)
	{
		my_ble.mac_addr[i] = my_addr.addr[5-i];
	}
	if(err_code == NRF_SUCCESS) //读取厂商地址成功，log输出地址
	{
		NRF_LOG_INFO("Address Type:%02X\r\n", my_addr.addr_type);      //打印地址类型
		NRF_LOG_INFO("Address:%02X:%02X:%02X:%02X:%02X:%02X\r\n", //打印地址
										my_ble.mac_addr[0],my_ble.mac_addr[1],my_ble.mac_addr[2],
		                                my_ble.mac_addr[3],my_ble.mac_addr[4],my_ble.mac_addr[5]);
	}	
	
	my_addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC; //BLE_GAP_ADDR_TYPE_PUBLIC BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
	err_code =  sd_ble_gap_addr_set(&my_addr);
	APP_ERROR_CHECK(err_code);
	
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&sec_mode);//设置GAP的安全模式，无加密
    //设置GAP设备名称
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)name,
                                          strlen(name));
    //检查函数返回的错误代码
	APP_ERROR_CHECK(err_code);
																				
    //外观特征，384
	err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_REMOTE_CONTROL);
	APP_ERROR_CHECK(err_code); 
																					
    //设置首选连接参数，设置前先清零gap_conn_params
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;//最小连接间隔
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;//最小连接间隔
    gap_conn_params.slave_latency     = SLAVE_LATENCY;    //从机延迟
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT; //监督超时
    //调用协议栈API sd_ble_gap_ppcp_set配置GAP参数
    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
										 
	//err_code = sd_ble_gap_tx_power_set(TX_POWER_LEVEL);
   // APP_ERROR_CHECK(err_code);
																					
}
static uint16_t   m_ble_uarts_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;  
//GATT事件处理函数，该函数中处理MTU交换事件
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    //如果是MTU交换事件
	  if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        //设置串口透传服务的有效数据长度（MTU-opcode-handle）
			  m_ble_uarts_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", m_ble_uarts_max_data_len, m_ble_uarts_max_data_len);
    }
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}

//初始化GATT程序模块
static void gatt_init(void)
{
	//初始化GATT程序模块
	ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
	//检查函数返回的错误代码
	APP_ERROR_CHECK(err_code);
	//设置ATT MTU的大小，这里设置的值为247
	err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
	APP_ERROR_CHECK(err_code);
}

//广播事件处理函数
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    //ret_code_t err_code;
    //判断广播事件类型
    switch (ble_adv_evt)
    {
        //快速广播启动事件：快速广播启动后会产生该事件
		case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.");		
            //LED1_ON;    //设置广播指示灯为正在广播（D1指示灯闪烁）
            break;
        //广播IDLE事件：广播超时后会产生该事件
        case BLE_ADV_EVT_IDLE:	  
            break;

        default:
            break;
    }
}

//广播初始化
static void advertising_init(void)
{
    ret_code_t             err_code;
	
    ble_advertising_init_t init;       //定义广播初始化配置结构体变量   
    memset(&init, 0, sizeof(init));    //配置之前先清零  
    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;//设备名称类型：全称	  
    //init.advdata.include_appearance      = false;                 //是否包含外观：包含
	//init.advdata.include_ble_device_addr = true;				 //是否包含设备地址：包含
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;//Flag:一般可发现模式，不支持BR/EDR 
	
	init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);//广播数据中包含的服务UUID
	init.advdata.uuids_complete.p_uuids  = m_adv_uuids; //指向服务UUID数组入口
	
	//ble串口UUID放到扫描响应里面
	//init.srdata.uuids_complete.uuid_cnt = sizeof(nus_adv_uuids) / sizeof(nus_adv_uuids[0]);
   // init.srdata.uuids_complete.p_uuids  = nus_adv_uuids;
	
    init.config.ble_adv_fast_enabled  = true;//设置广播模式为快速广播
	//设置广播间隔和广播持续时间
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;//0，不超时
    
    init.evt_handler = on_adv_evt;						   //广播事件回调函数
    
    err_code = ble_advertising_init(&m_advertising, &init);//初始化广播
    APP_ERROR_CHECK(err_code);
    //设置连接设置标志，当前SoftDevice版本中（S132 V6.1版本），只能写1。
    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


//排队写入事件处理函数，用于处理排队写入模块的错误
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    //检查错误代码
	  APP_ERROR_HANDLER(nrf_error);
}

//服务初始化，包含初始化排队写入模块和初始化应用程序使用的服务,DFU
static void services_init(void)
{
    ret_code_t         err_code;

	ble_uarts_init_t     uarts_init;          	   //定义串口透传初始化结构体
	ble_dfu_buttonless_init_t dfus_init = {0};     //定义DFU服务初始化结构体
    nrf_ble_qwr_init_t qwr_init = {0};			   //定义排队写入初始化结构体变量
    qwr_init.error_handler = nrf_qwr_error_handler;//排队写入事件处理函数    
	
    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);//初始化排队写入模块
    APP_ERROR_CHECK(err_code);

/*******************加入添加服务的代码*******************/
	//初始化DFU服务
	dfus_init.evt_handler = ble_dfu_evt_handler;
    err_code = ble_dfu_buttonless_init(&dfus_init);
    APP_ERROR_CHECK(err_code);
		
/*------------------以下代码初始化串口透传服务-------------*/
	//清零串口透传服务初始化结构体
	memset(&uarts_init, 0, sizeof(uarts_init));	
    uarts_init.data_handler = uarts_data_handler;      //设置串口透传事件回调函数
    err_code = ble_uarts_init(&m_uarts, &uarts_init);  //初始化串口透传服务
    APP_ERROR_CHECK(err_code);
/*------------------初始化串口透传服务-END-----------------*/
}

//连接参数协商模块事件处理函数
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;
    //判断事件类型，根据事件类型执行动作
	//连接参数协商失败
    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
		//LED3_OFF;
    }
	//连接参数协商成功
	if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_SUCCEEDED)
    {
        //功能代码;
		//LED3_ON;//协商成功，LED3亮
		//协商成功后，读取外围设备首选连接参数
		//定义用来保存读取的外围设备首选连接参数的变量
		ble_gap_conn_params_t   my_gap_conn_params;
		//调用协议栈API sd_ble_gap_ppcp_set配置GAP参数
        err_code = sd_ble_gap_ppcp_get(&my_gap_conn_params);
		//读取成功后，打印出读取的外围设备首选连接参数
		if(err_code == NRF_SUCCESS)
		{
			 NRF_LOG_INFO("min_conn_interval = %d",my_gap_conn_params.min_conn_interval);
			 NRF_LOG_INFO("max_conn_interval = %d",my_gap_conn_params.max_conn_interval);
			 NRF_LOG_INFO("slave_latency = %d",my_gap_conn_params.slave_latency);
			 NRF_LOG_INFO("conn_sup_timeout = %d",my_gap_conn_params.conn_sup_timeout);
		}
    }
}

//连接参数协商模块错误处理事件，参数nrf_error包含了错误代码，通过nrf_error可以分析错误信息
static void conn_params_error_handler(uint32_t nrf_error)
{
    //检查错误代码
	  APP_ERROR_HANDLER(nrf_error);
}

//连接参数协商模块初始化
static void conn_params_init(void)
{
    ret_code_t             err_code;
	  
    ble_conn_params_init_t cp_init;      //定义连接参数协商模块初始化结构体
    memset(&cp_init, 0, sizeof(cp_init));//配置之前先清零
    
    cp_init.p_conn_params                  = NULL;//设置为NULL，从主机获取连接参数
	//连接或启动通知到首次发起连接参数更新请求之间的时间设置为5秒
	cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
	//每次调用sd_ble_gap_conn_param_update()函数发起连接参数更新请求的之间的间隔时间设置为：30秒
	cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
	//放弃连接参数协商前尝试连接参数协商的最大次数设置为：3次
	cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
	//连接参数更新从连接事件开始计时
	cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
	//连接参数更新失败不断开连接
	cp_init.disconnect_on_fail             = false;
	//注册连接参数更新事件句柄
	cp_init.evt_handler                    = on_conn_params_evt;
	//注册连接参数更新错误事件句柄
	cp_init.error_handler                  = conn_params_error_handler;
	//调用库函数（以连接参数更新初始化结构体为输入参数）初始化连接参数协商模块
    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


//启动广播，该函数所用的模式必须和广播初始化中设置的广播模式一样
void advertising_start(void)
{
	//使用广播初始化中设置的广播模式启动广播
	ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
	//检查函数返回的错误代码
   APP_ERROR_CHECK(err_code);
}

void My_ble_Init(void)
{	
	ble_stack_init();   //初始化协议栈	
	gap_params_init();  //配置GAP参数	
	gatt_init();        //初始化GATT	
	advertising_init(); //初始化广播	
	services_init();    //初始化服务	
	conn_params_init(); //连接参数协商初始化
}


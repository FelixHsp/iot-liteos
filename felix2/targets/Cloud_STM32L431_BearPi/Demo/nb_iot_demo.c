/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/
 

#include <stdio.h>
#include "nb_iot/los_nb_api.h"
#include "nb_cmd_ioctl.h"
#include "lcd.h"
#include "bh1750.h"
#include "los_swtmr.h"
#include "stm32l4xx_hal.h"

#define TELECON_IP "180.101.147.115"
#define OCEAN_IP "49.4.85.232"
#define SECURITY_PORT "5684"
#define NON_SECURITY_PORT "5683"
#define DEV_PSKID  "868744031131026"
#define DEV_PSK  "d1e1be0c05ac5b8c78ce196412f0cdb0"

#define cn_buf_len    256   //may be big enough
static char s_report_buf[cn_buf_len];
extern const unsigned char gImage_Huawei_IoT_QR_Code[114720];
uint16_t counter = 0;
short int Lux; 
bool qr_code = true;
int ue_stats[4];

static void Timer1_Callback(UINT32 arg)
{
	qr_code = !qr_code;
	LCD_Clear(WHITE);
	if (qr_code)
		LCD_Show_Image(0,0,240,239,gImage_Huawei_IoT_QR_Code);
	else 
	{
		POINT_COLOR = BLUE;			
		LCD_ShowString(40, 10, 200, 16, 24, "Felix's BearPi");
		LCD_ShowString(15, 50, 210, 16, 24, "LiteOS NB-IoT");
		LCD_ShowString(10, 100, 200, 16, 16, "NCDP_IP:");
		LCD_ShowString(80, 100, 200, 16, 16, OCEAN_IP);
		LCD_ShowString(10, 150, 200, 16, 16, "NCDP_PORT:");
		LCD_ShowString(100, 150, 200, 16, 16, NON_SECURITY_PORT);
		LCD_ShowString(10, 200, 200, 16, 16, "luminance is:");
		LCD_ShowNum(140, 200, Lux, 5, 16);
	}
	
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	int ret;
	switch(GPIO_Pin)
	{
		case KEY1_Pin:	
			key1 = true;
			printf("proceed to get ue_status!\r\n");
			break;
		case KEY2_Pin:
			key2 = true;
			toggle = !toggle;
			HAL_GPIO_TogglePin(Light_GPIO_Port,Light_Pin);
			break;
		default:		
			break;
	}
}

void nb_iot_entry(void);

void demo_nbiot_only(void)
{
#if defined(WITH_AT_FRAMEWORK) && defined(USE_NB_NEUL95_NO_ATINY)
    #define AT_DTLS 0
    #if AT_DTLS
    sec_param_s sec;
    sec.setpsk = 1;
    sec.pskid = DEV_PSKID;
    sec.psk = DEV_PSK;
    #endif
    printf("\r\n=====================================================");
    printf("\r\nSTEP1: Init NB Module( NB Init )");
    printf("\r\n=====================================================\r\n");

#if AT_DTLS
    los_nb_init((const int8_t *)OCEAN_IP, (const int8_t *)SECURITY_PORT, &sec);
		LCD_Clear(WHITE);		   
		POINT_COLOR = RED;			
		LCD_ShowString(40, 50, 200, 16, 24, "IoTCluB BearPi");	
		LCD_ShowString(10, 100, 200, 16, 16, "NCDP_IP:");
		LCD_ShowString(80, 100, 200, 16, 16, OCEAN_IP);
		LCD_ShowString(10, 150, 200, 16, 16, "NCDP_PORT:");
		LCD_ShowString(100, 150, 200, 16, 16, SECURITY_PORT);
#else
    los_nb_init((const int8_t *)OCEAN_IP, (const int8_t *)NON_SECURITY_PORT, NULL);
#endif

#if defined(WITH_SOTA)
    extern void nb_sota_demo(void);
    nb_sota_demo();
#endif
    printf("\r\n=====================================================");
    printf("\r\nSTEP2: Register Command( NB Notify )");
    printf("\r\n=====================================================\r\n");
		los_nb_notify("+NNMI:",strlen("+NNMI:"),nb_cmd_data_ioctl,OC_cmd_match);
    printf("\r\n=====================================================");
    printf("\r\nSTEP3: Report Data to Server( NB Report )");
    printf("\r\n=====================================================\r\n");
		nb_iot_entry();

#else
    printf("Please checkout if open WITH_AT_FRAMEWORK and USE_NB_NEUL95_NO_ATINY\n");
#endif

}



VOID data_collection_task(VOID)
{
	UINT32 uwRet = LOS_OK;
  
	Init_BH1750();									
	while (1)
  {

		Lux=(int)Convert_BH1750();		
		// printf("\r\n******************************BH1750 Value is  %d\r\n",Lux);
		if (!qr_code)
		{
			LCD_ShowString(10, 200, 200, 16, 16, "BH1750 Value is:");
			LCD_ShowNum(140, 200, Lux, 5, 16);
		}
		sprintf(BH1750_send.Lux,"%04X", Lux);
		uwRet=LOS_TaskDelay(2000);
		if(uwRet !=LOS_OK)
		return;
	
  }
}
UINT32 creat_data_collection_task()
{
    UINT32 uwRet = LOS_OK;
    TSK_INIT_PARAM_S task_init_param;
		UINT32 TskHandle;
    task_init_param.usTaskPrio = 0;
    task_init_param.pcName = "data_collection_task";
    task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)data_collection_task;
    task_init_param.uwStackSize = 0x400;

    uwRet = LOS_TaskCreate(&TskHandle, &task_init_param);
    if(LOS_OK != uwRet)
    {
        return uwRet;
    }
    return uwRet;
}

VOID data_report_task(VOID)
{
	UINT32 uwRet = LOS_OK;
	UINT32 msglen = 0;
	UINT32 msgid;
	while(1)
	{
			if (key1)
			{
					key1 = false;
				  uwRet = nb_check_nuestats();
				  if (ue_stats[0] < -10) ue_stats[0] = ue_stats[0] / 10;
				  if (ue_stats[2] > 10) ue_stats[2] = ue_stats[2] / 10;
					msgid = app_msgid_counter;
					sprintf(s_report_buf,"%02d", msgid);
					sprintf(key1_send.rsrp,"%04X", ue_stats[0] & 0x0000FFFF);
				  sprintf(key1_send.ecl,"%04X", ue_stats[1]);
				  sprintf(key1_send.snr,"%04X", ue_stats[2]);
				  sprintf(key1_send.cellid,"%08X", ue_stats[3]);
					memcpy(s_report_buf + 2, &key1_send, sizeof(key1_send));
					msglen = sizeof(key1_send);
					printf("key1 message: %s\r\n",s_report_buf);
					uwRet = los_nb_report((const char*)(&s_report_buf), (msglen + 2) / 2);
					memset(s_report_buf, 0, sizeof(s_report_buf));
			}
			
			if (key2)
			{
					key2 = false;
					msgid = app_msgid_toggle;
					sprintf(s_report_buf,"%02d", msgid);
					sprintf(key2_send.tog,"%04X", toggle);
					memcpy(s_report_buf + 2, &key2_send, sizeof(key2_send));
					msglen = sizeof(key2_send);
					printf("key2 message: %s\r\n",s_report_buf);
					uwRet = los_nb_report((const char*)(&s_report_buf), (msglen + 2) / 2);
					memset(s_report_buf, 0, sizeof(s_report_buf));
			}
			
			msgid = app_msgid_lux;
			sprintf(s_report_buf,"%02d", msgid);
			memcpy(s_report_buf + 2, &BH1750_send, sizeof(BH1750_send));
			msglen = sizeof(BH1750_send);
			// printf("BH1750_send message: %s\r\n",s_report_buf);
			uwRet = los_nb_report((const char*)(&s_report_buf), (msglen + 2) / 2);
			memset(s_report_buf, 0, sizeof(s_report_buf));
			uwRet=LOS_TaskDelay(2000);
			if(uwRet !=LOS_OK)
			return;
	}
}
UINT32 creat_data_report_task()
{
    UINT32 uwRet = LOS_OK;
    TSK_INIT_PARAM_S task_init_param;
	  UINT32 TskHandle;

    task_init_param.usTaskPrio = 1;
    task_init_param.pcName = "data_report_task";
    task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)data_report_task;
    task_init_param.uwStackSize = 0x800;
    uwRet = LOS_TaskCreate(&TskHandle, &task_init_param);
    if(LOS_OK != uwRet)
    {
        return uwRet;
    }
    return uwRet;
}

VOID nb_reply_task(VOID)
{
		UINT32 msglen = 0;
		while (1)
		{				                                               
				LOS_SemPend(reply_sem, LOS_WAIT_FOREVER);
				printf("This is reply_report_task!\n");   
				msglen = strlen(s_resp_buf);
				if(los_nb_report((const char*)(&s_resp_buf), msglen / 2)>=0)
				{
						printf("ocean_send_reply OK!\n");                                                  
				}
				else                                                                                  
				{
						printf("ocean_send_reply Fail!\n"); 	
				}
				memset(s_resp_buf, 0, sizeof(s_resp_buf));
		}
}

UINT32 creat_nb_reply_task()
{
    UINT32 uwRet = LOS_OK;
    TSK_INIT_PARAM_S task_init_param;
	  UINT32 TskHandle;

    task_init_param.usTaskPrio = 0;
    task_init_param.pcName = "nb_reply_task";
    task_init_param.pfnTaskEntry = (TSK_ENTRY_FUNC)nb_reply_task;
    task_init_param.uwStackSize = 0x400;
    uwRet = LOS_TaskCreate(&TskHandle, &task_init_param);
    if(LOS_OK != uwRet)
    {
			return uwRet;
    }

    return uwRet;
}

void nb_iot_entry(void)
{
    UINT32 uwRet = LOS_OK;
		UINT16 swtmr1,swtmr2,swtmr3;
    uwRet = creat_data_collection_task();
    if (uwRet != LOS_OK)
    {
        return ;
    }
		
		uwRet = creat_data_report_task();
    if (uwRet != LOS_OK)
    {
        return ;
    }
		uwRet = LOS_SemCreate(0,&reply_sem);
		if (uwRet != LOS_OK)
    {
        return ;
    }				
		uwRet = creat_nb_reply_task();
    if (uwRet != LOS_OK)
    {
        return ;
    }
		
		uwRet = LOS_SwtmrCreate(8000,LOS_SWTMR_MODE_PERIOD,Timer1_Callback,&swtmr1,1,OS_SWTMR_ROUSES_ALLOW,OS_SWTMR_ALIGN_SENSITIVE);
    if (uwRet != LOS_OK)
    {
        return ;
    }
    uwRet = LOS_SwtmrStart(swtmr1);
		if (uwRet != LOS_OK)
    {
        return ;
    }
		
}

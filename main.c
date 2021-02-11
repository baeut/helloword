#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "lcd.h"
#include "usart2.h"
#include "key.h"
#include "24cxx.h" 
#include "myiic.h"
#include <string.h>
//#include "stmflash.h"  
//ALIENTEK Mini STM32�����巶������11
//TFTLCD��ʾʵ��   
//����֧�֣�www.openedv.com
//������������ӿƼ����޹�˾ 
extern volatile u8 buf01;
extern volatile u8 frame_available;
extern volatile u8 recv_buf[16];
#define Max_num 32
char history[Max_num][4]={0};
static u8 history_read_ptr = 0, history_write_ptr = 0, history_rotated = 0;
static u32 hold_time_len = 0;
static u8 triggered_buttons = 0;

#define EEPROM_LAST_SAVE_POS 0
#define EEPROM_SAVE_POS 1

void eeprom_init() {
	for (int i = 0; i < 256; i++) {
		AT24CXX_WriteOneByte(i, 0);
	}
}

void eeprom_bulk_load() {
	for (u8 i = 0; i < (history_rotated ? Max_num : history_write_ptr); i++) {
		AT24CXX_Read(EEPROM_SAVE_POS + i * 3, (u8 *) history[i], 3);
		history[i][3] = 0;
	}
}

void on_button_one_press() {
	LCD_ShowNum(60,100,history_read_ptr,16,16);
	LCD_ShowString(60,120,200,16,16,"History:"); 
	LCD_ShowString(60,170,200,16,16,"   ");
	LCD_ShowString(60,170,200,16,16,(u8*)history[history_read_ptr]);//��ʾ�������ַ���
	if (history_rotated) {
		if (++history_read_ptr >= Max_num) {
			history_read_ptr = 0;
		}
	} else {
		if (++history_read_ptr >= history_write_ptr) {
			history_read_ptr = 0;
		}
	}
}

void on_button_two_press() {
	LCD_ShowNum(60,100,history_read_ptr,16,16);
	LCD_ShowString(60,120,200,16,16,"History:"); 
	LCD_ShowString(60,170,200,16,16,"   ");
	LCD_ShowString(60,170,200,16,16,(u8*)history[history_read_ptr]);//��ʾ�������ַ���
	if (history_rotated) {
		if (history_read_ptr-- == 0) {
			history_read_ptr = Max_num - 1;
		}
	} else {
		if (history_read_ptr-- == 0) {
			if (history_write_ptr == 0) {
				history_read_ptr = 0;
			} else {
				history_read_ptr = history_write_ptr - 1;
			}
		}
	}
}

void on_button_clear_press() {
	LCD_ShowString(60,150,200,16,16,"Clearing FLASH data.... ");
	eeprom_init();
	LCD_ShowString(60,150,200,16,16,"MEMORY CLEARED          ");
}

void on_button_save_press() {
	strncpy(history[history_write_ptr], (char *) recv_buf, 3);
	history[history_write_ptr][3] = 0;
	LCD_Fill(0,0,239,319,WHITE );//���ȫ��	   
	LCD_ShowString(60,150,200,16,16,"Start Write 24C02....");
	AT24CXX_Write(EEPROM_SAVE_POS + history_write_ptr * 3, (u8*) history[history_write_ptr], 3);
	if (++history_write_ptr >= Max_num) {
		history_rotated = 1;
	}
	AT24CXX_WriteOneByte(EEPROM_LAST_SAVE_POS, (history_rotated ? 0x80u : 0) | history_write_ptr);
	LCD_ShowString(60,150,200,16,16,"24C02 Write Finished!");//��ʾ�������
}

int main(void)
{
	u8 t=0;
	u16 num = 0;
	static u8 num_for_addr = 0;
	delay_init();	    	 //��ʱ������ʼ��	  
	uart_init(9600);	 	//���ڳ�ʼ��Ϊ9600
	USART2_Init(115200);
	LED_Init();		  		//��ʼ����LED���ӵ�Ӳ���ӿ�
 	LCD_Init();
	KEY_Init();          	//��ʼ���밴�����ӵ�Ӳ���ӿ�
	AT24CXX_Init();
	POINT_COLOR=RED; 
	
	for (int i = 8; i < 16; i++) {
		recv_buf[i] = ' ';
	}
	recv_buf[15] = 0;
	
	history_write_ptr = AT24CXX_ReadOneByte(EEPROM_LAST_SAVE_POS);
	if (history_write_ptr == 0xffu) {
		history_write_ptr = 0;
		on_button_clear_press();
	}
	
	history_rotated = 0x80u & history_write_ptr;
	history_write_ptr &= 0x1fu;
	
	eeprom_bulk_load();
	LCD_ShowString(60,150,200,16,16, "History loaded");
		 	
  	while(1) 
	{
		t = KEY_Scan(0);		// ֧������ ���õ���ֵ
		if (triggered_buttons == t) {
			// button holding
			switch (t) {
				case KEY0_PRES:
				case KEY1_PRES:
					++hold_time_len;
					break;
				default:
					// holding not allowed
					hold_time_len = 0;
					break;
			}
			if (hold_time_len > 100u) {
				LED0 = !LED0;
				switch (t) {
					case KEY0_PRES:
						on_button_one_press();
						break;
					case KEY1_PRES:
						on_button_two_press();
						break;
					default:
						break;
				}
				hold_time_len = 0;
			}
		} else {
			LED0 = !LED0;
			// new triggered button
			if (t > 0) {
				// new button press
				switch (t) {
					case KEY0_PRES:
						on_button_one_press();
						break;
					case KEY1_PRES:
						on_button_two_press();
						break;
					case KEY2_PRES:
						on_button_save_press();
						break;
					case WKUP_PRES:
						on_button_clear_press();
						break;
					default:
						break;
				}
			} else {
				hold_time_len = 0;
				LED1 = !LED1;
			}
			triggered_buttons = t;
		}
		POINT_COLOR=RED;	  
		if (frame_available) {
			LCD_ShowString(30,40,200,24,24, (u8*) recv_buf);
			frame_available = 0;
		}
	} 
}

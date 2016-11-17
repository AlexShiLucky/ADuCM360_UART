#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <ADuCM360.h>

#include <WdtLib.h>
#include <ClkLib.h>
#include <DioLib.h>
#include <UrtLib.h>
#include <IntLib.h>
#include <GptLib.h>

#define TRUE		1
#define FALSE		0

volatile uint8_t ucTxBufferEmpty  = 0; // Used to indicate that the UART Tx buffer is empty
volatile uint8_t ucRxBufferFull  = 0; // Used to indicate that the UART Rx buffer is full

volatile uint8_t ucCOMIID0 = 0;
volatile uint8_t ucComRx = 0;

uint32_t dly_cnt = 0;


void delay_ms(int length){
   dly_cnt = 0;
   
   while (length > dly_cnt);
}

void WatchDogInit(void){
	//---------- Disable Watchdog timer resets ----------
   WdtCfg(T3CON_PRE_DIV1, T3CON_IRQ_EN,T3CON_PD_DIS); 
}

void ClockInit(void){
	//---------- Disable clock to unused peripherals ----------
   ClkDis(CLKDIS_DISSPI0CLK|CLKDIS_DISSPI1CLK|CLKDIS_DISI2CCLK|CLKDIS_DISPWMCLK|CLKDIS_DIST0CLK|CLKDIS_DIST1CLK);

   // Select CD0 for CPU clock - 2Mhz clock
   ClkCfg(CLK_CD2,CLK_HF,CLKSYSDIV_DIV2EN_EN,CLK_UDIV);   
   ClkSel(CLK_CD7,CLK_CD7,CLK_CD0,CLK_CD7);     // Select CD0 for UART System clock
}

void GPIOInit(void){
	//---------- I/O Port Configuration ----------
   DioCfg(pADI_GP0, 0x9000);   // P0.7[TX], P0.6[RX]
   DioOen(pADI_GP0, 0xA4); 
   DioPul(pADI_GP0, 0x00);
   
   DioCfg(pADI_GP1, 0x0000);   // GPIO1 Port Function Setting
   DioOen(pADI_GP1, 0xEB);
   DioPul(pADI_GP1, 0x00);

   DioCfg(pADI_GP2, 0x0000);   // GPIO2 Port Function Setting
   DioOen(pADI_GP2, 0xFF);
   DioPul(pADI_GP2, 0x00);
}

void UARTInit(void){
   //Select IO pins for UART.
   UrtCfg(pADI_UART,B1200,COMLCR_WLS_8BITS,COMLCR_PEN_EN);  // setup baud rate for 9600, 8-bits
   UrtMod(pADI_UART,COMMCR_DTR,0);              			// Setup modem bits
   UrtIntCfg(pADI_UART,COMIEN_ERBFI|COMIEN_ETBEI);  		// Setup UART IRQ sources
}

void TIMER0_Init(void){
	//--------- Timer 0 Setup 1 msec -------------
   //GptLd(pADI_TM0, 32);	//32Khz PCLK
   //GptCfg(pADI_TM0,TCON_CLK_LFOSC,TCON_PRE_DIV1,TCON_MOD_PERIODIC|TCON_RLD_DIS|TCON_ENABLE_EN|TCON_UP_DIS);
  GptLd(pADI_TM0, 0x7D);
  GptCfg(pADI_TM0,TCON_CLK_PCLK,TCON_PRE_DIV16,TCON_MOD|TCON_RLD|TCON_ENABLE);
}

void Chip_Initialize(){
   WatchDogInit();
   ClockInit();
   GPIOInit();
   TIMER0_Init();
   UARTInit();
   
   NVIC_EnableIRQ(TIMER0_IRQn);
   NVIC_EnableIRQ(UART_IRQn);
}

void SendChar(uint8_t ch){
	ucTxBufferEmpty = 0;
    UrtTx(pADI_UART, ch);
    while (ucTxBufferEmpty == 0) {};
}

void SendMsg(uint8_t *str){
	uint16_t len =0 , i;

	len = strlen((char*)str);
	for(i=0; i<len; i++){
		SendChar(str[i]);
	}
}

uint8_t* ReadMsg(void){
	uint8_t cnt = 0;
	static uint8_t str[20] = "";
	while(TRUE){
		if(ucRxBufferFull){
			str[cnt++] = ucComRx;
			ucRxBufferFull = FALSE;
			if(ucComRx == 0xff)
				break;
		}
	}
	return str;
}

int main(){
	//uint8_t data[10]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xff};
	//uint8_t sendFlag = FALSE;
	volatile int i;
   	Chip_Initialize();
    while(TRUE){
        DioClr(pADI_GP0,BIT5);
        delay_ms(1000);
        //for(i=0; i< 5000; i++);
        DioSet(pADI_GP0,BIT5);
        //for(i=0; i< 5000; i++);
        delay_ms(1000);
	}
 /*  	while(TRUE){
		if(!sendFlag){
			DioClr(pADI_GP0,PIN5);
			SendMsg(data);
			sendFlag = TRUE;
			DioSet(pADI_GP0,PIN5);
		}
		if(ucRxBufferFull){
			ReadMsg();
			sendFlag = FALSE;
			ucRxBufferFull = FALSE;
		}
		delay_ms(1000);
   	}*/
}
void UART_Int_Handler(){
	ucCOMIID0 = UrtIntSta(pADI_UART);   	// Read UART Interrupt ID register
   	if ((ucCOMIID0 & 0x7) == 0x2){       	// Transmit buffer empty
   		ucTxBufferEmpty = TRUE;
   	}
   	if ((ucCOMIID0 & 0x7) == 0x4){       	// Receive byte
   		ucComRx = UrtRx(pADI_UART);
   		ucRxBufferFull = TRUE;
   	}
}

void GP_Tmr0_Int_Handler(void){
   // Timer0 Interrupt : 1msec
   GptClrInt(pADI_TM0, TSTA_TMOUT);
   dly_cnt++;
}
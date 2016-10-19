#include <stdio.h>
#include <string.h>

#include <ADuCM360.h>

#include <WdtLib.h>
#include <ClkLib.h>
#include <DioLib.h>
#include <IntLib.h>
#include <UrtLib.h>

#define TRUE		1
#define FALSE		0

volatile uint8_t ucTxBufferEmpty  = 0; // Used to indicate that the UART Tx buffer is empty
volatile uint8_t ucRxBufferFull  = 0; // Used to indicate that the UART Rx buffer is full

uint8_t szTemp[128] = "";

volatile uint8_t ucCOMIID0 = 0;
volatile uint8_t ucComRx = 0;

void WatchDogInit(void){
	//---------- Disable Watchdog timer resets ----------
   pADI_WDT->T3CON = 0;
   WdtCfg(T3CON_PRE_DIV1, T3CON_IRQ_EN,T3CON_PD_DIS); 
}

void ClockInit(void){
	//---------- Disable clock to unused peripherals ----------
   ClkDis(CLKDIS_DISSPI0CLK|CLKDIS_DISSPI1CLK|CLKDIS_DISI2CCLK|CLKDIS_DISPWMCLK|CLKDIS_DIST0CLK|CLKDIS_DIST1CLK);

   // Select CD0 for CPU clock - 16Mhz clock
   ClkCfg(CLK_CD0,CLK_HF,CLKSYSDIV_DIV2EN_DIS,CLK_UCLKCG);     
   ClkSel(CLK_CD7,CLK_CD7,CLK_CD0,CLK_CD7);     // Select CD0 for UART System clock
}

void GPIOInit(void){
	//---------- I/O Port Configuration ----------
   DioCfg(pADI_GP0, 0x9500);   // P0.7[TX], P0.6[RX]
   DioOen(pADI_GP0, 0x80); 
   DioPul(pADI_GP0, 0x00);
   
   DioCfg(pADI_GP1, 0x0000);   // GPIO1 Port Function Setting
   DioOen(pADI_GP1, 0xEF);
   DioPul(pADI_GP1, 0x00);

   DioCfg(pADI_GP2, 0x0000);   // GPIO2 Port Function Setting
   DioOen(pADI_GP2, 0xFF);
   DioPul(pADI_GP2, 0x00);
}

void UARTInit(void){
   //Select IO pins for UART.
   pADI_GP0->GPCON |= 0x9000;                   // Configure P0.6/P0.7 for UART
   UrtCfg(pADI_UART,B9600,COMLCR_WLS_8BITS,0);  // setup baud rate for 9600, 8-bits
   UrtMod(pADI_UART,COMMCR_DTR,0);              // Setup modem bits
   UrtIntCfg(pADI_UART,COMIEN_ERBFI|COMIEN_ETBEI|COMIEN_ELSI|COMIEN_EDSSI|COMIEN_EDMAT|COMIEN_EDMAR);  // Setup UART IRQ sources
}

void delay(long int length){
   while(length)
      length--;
}

void Chip_Initialize(){
   WatchDogInit();
   ClockInit();
   GPIOInit();
   UARTInit();
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
	uint8_t str[128] = "";
	while(TRUE){
		if(ucRxBufferFull){
			str[cnt++] = ucComRx;
			ucRxBufferFull = FALSE;
			if(ucComRx == '\0' || ucComRx == '\r' || ucComRx == '\n')
				break;
		}
	}
	return str;
}

int main(){
	//Initialize
   	Chip_Initialize();

   	SendMsg("Input Text\r\n");
   	while(TRUE){
		if(ucRxBufferFull){
			//ReadMsg();
			SendMsg(ReadMsg());
			SendMsg("\r\n");
			SendMsg("Input Text\r\n");
			ucRxBufferFull = FALSE;
		}
   	}
   	return 0;
}

void UART_Int_Handler()
{
	ucCOMIID0 = UrtIntSta(pADI_UART);   	// Read UART Interrupt ID register
   	if ((ucCOMIID0 & 0x7) == 0x2){       	// Transmit buffer empty
   		ucTxBufferEmpty = TRUE;
   	}
   	if ((ucCOMIID0 & 0x7) == 0x4){       	// Receive byte
		ucComRx = UrtRx(pADI_UART);
	   	ucRxBufferFull = TRUE;
   	}
}

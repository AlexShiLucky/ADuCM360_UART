#include <string.h>
#include <ADuCM360.h>

#include <WdtLib.h>
#include <ClkLib.h>
#include <DioLib.h>
#include <IntLib.h>
#include <UrtLib.h>

#include "Shico_Lib.h"

#define TRUE  		1
#define RETURN	'\n'
#define Cap2Low(Ch)	(((Ch) >= 'A') && ((Ch) <= 'Z'))? (Ch-'A'+'a'):Ch


uint8_t szTemp[64] = "";
volatile uint8_t ucComRx = 0;
uint8_t nLen = 0;


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

//------------------------------------------------------------------
void delay (long int length)
{
   while(length)
      length--;
}

void Chip_Initialize()
{
   WatchDogInit();
   ClockInit();
   GPIOInit();
   UARTInit();
   NVIC_EnableIRQ(UART_IRQn);
}

void menu_display(void)
{
	printf("*************************************************************\n");
	printf("**                                                           \n");		
	printf("**   Note :                                                  \n");					 
	printf("**   ADuCM360 EVM board Test Program                         \n");		
	printf("**                                                           \n");		
	printf("** Usage :                                                   \n");					 
	printf("** LoveJongSu]set addr val                                   \n");					 
	printf("** LoveJongSu]get addr                                       \n");					 			 
	printf("**                                                           \n");		
	printf("**                       2013, 10, 23  By Lim Jong Su        \n");	 
	printf("**                                                           \n");		
	printf("*************************************************************\n");
	printf("*           1.  PWM Test                                    *\n");               
	printf("**                                                          *\n");		
	printf("             ขั m.  Main Menu display                      \n");
	printf("***********************************************************\n\n");
}
//------------------------------------------------------------------

int main()
{
	uint8_t d, addr;

	
   	Chip_Initialize();
   
   	while(TRUE){
   		printf("Shico]"); 
		gets(szTemp);
		d=szTemp[0];

		if (d == 'm')	{
			addr = d;
		} else if (d == RETURN ) {			
			addr = 'r';
		} else if (Cap2Low(d) == 'g') {
			MemoryDump((char *)szTemp);
			addr = 'r';
		} else if (Cap2Low(d) == 's') {
			MemorySet((char *)szTemp);
			addr='r';
		} else
		{
			addr = hextoint(szTemp);
		}
		szTemp[0] = 0;
		switch(addr){				
			case 0x1 :
				printf("PWM Test\n");
			break;
		case 'm'  :	
			menu_display();								
			break;
		default:														   
			break;
		}	
   	}
   	return 0;
}



//------------------------------------------------------------------

void UART_Int_Handler()
{
   volatile uint8 ucCOMIID0 = 0;

	ucCOMIID0 = UrtIntSta(pADI_UART);   // Read UART Interrupt ID register
   if ((ucCOMIID0 & 0x7) == 0x2)       // Transmit buffer empty
   {
      ucTxBufferEmpty = 1;
   }
   if ((ucCOMIID0 & 0x7) == 0x4)       // // Receive byte
   {
	   GetStrTmp=UrtRx(pADI_UART);
	   ucWaitForUart = 1;
   }
} 
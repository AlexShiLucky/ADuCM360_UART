/**
 *****************************************************************************
   @example  RTD_Demo.c
   @brief    ADC1 is used to measure an RTD input connected across AIN0 and AIN1.
   - AIN6 is used as the excitation current source for the RTD
   - REFIN+/REFIN- are the ADC external reference inputs 
   - 100ohm PT100 RTD expected connected to AIN0 and AIN1

   - To eliminate drift elements the following is the measurement sequence:
   - 1) Measure AIN1/AGND v Internal 1.2V reference to derive exact Excitation current value
   - 2) Measure AIN0/AGND as a diagnostic - should equal sum of steps 1 and 2.
   - 3) Measure AIN0/AIN1 v Internal Vref to determine voltage across RTD (vRTD)
   - 4) RRTD is determined and final RTD temeprature calculated.

   - The RTD reading is linearized and sent to the UART in a string format.
   - Default Baud rate is 9600
   - EVAL-ADuCM360MKZ or similar hardware is assumed
   - Results will be more accurate if System calibration is added 

   @version V0.2
   @author  ADI
   @date    February 2013

   @par     Revision History:
   - V0.1, September 2012: initial version. 
   - V0.2, February 2013: Fixed a bug in SendString().
              
All files for ADuCM360/361 provided by ADI, including this file, are
provided  as is without warranty of any kind, either expressed or implied.
The user assumes any and all risk from the use of this code.
It is the responsibility of the person integrating this code into an application
to ensure that the resulting application performs as required and is safe.

**/



#include <stdio.h>
#include <string.h>
#include <ADuCM360.h>

#include <..\common\AdcLib.h>
#include <..\common\IexcLib.h>
#include <..\common\DacLib.h>
#include <..\common\UrtLib.h>
#include <..\common\ClkLib.h>
#include <..\common\WutLib.h>
#include <..\common\WdtLib.h>
#include <..\common\GptLib.h>
#include <..\common\I2cLib.h>
#include <..\common\IntLib.h>
#include <..\common\PwmLib.h>
#include <..\common\DioLib.h>

void ADC1INIT(void);                // Init ADC1
void UARTINIT (void);
void SendString(void);              // Transmit string using UART
void IEXCINIT(void);                // Setup Excitation Current sources
void SendResultToUART(void);        // Send measurement results to UART - in ASCII String format
void SendErrorToUART(void);         // Send Error String to UART to indicate PGA overrange occured
float CalculateRTDTemp(float r);    // returns RTD Temperature reading

#define SAMPLENO        0x8         // Number of samples to be taken between channel switching
//RTD constants
#define TMIN (-40)                  // = minimum temperature in degC
#define TMAX (125)                  // = maximum temperature in degC
#define RMIN (84.2707)              // = input resistance in ohms at -40 degC
#define RMAX (147.951)              // = input resistance in ohms at 125 degC
#define NSEG 30                     // = number of sections in table
#define RSEG 2.12269                // = (RMAX-RMIN)/NSEG = resistance  in ohms of each segment
//RTD lookup table
const float C_rtd[] = {-40.0006,-34.6322,-29.2542,-23.8669,-18.4704,-13.0649,-7.65042,-2.22714,3.20489,8.64565,14.0952,
                        19.5536,25.0208,30.497,35.9821,41.4762,46.9794,52.4917,58.0131,63.5436,69.0834,74.6325,80.1909,
                        85.7587,91.3359,96.9225,102.519,108.124,113.74,119.365,124.999};

volatile unsigned char bSendResultToUART = 0;   // Flag used to indicate ADC1 result ready to send to UART	
volatile double long ulADC1Result = 0;          // Variable that ADC1DAT is read into in ADC1 IRQ
volatile unsigned char ucComRx = 0;

float fVRTD = 0.0 ;                       // RTD voltage, 
float fRrtd = 0.0;                        // resistance of the RTD
float fTRTD = 0.0;                        // RTD temperature
float fVAIN0_AGND = 0.0;                  // Measures voltage across Rref	and RTD
float fVAIN1_AGND = 0.0;                  // Measures voltage across Rref
float fIexc0 = 0.0;                       // Measured Excitation current 
volatile unsigned char ucADCERR = 0;      // Used to indicate an ADC error
unsigned char ucSampleNo = 0;             // Used to keep track of when to switch channels
unsigned char ucADCInput;                 // Used to indicate what channel the ADC1 is sampling
volatile long ulADC1DATRtd[SAMPLENO];     // Variable that ADC1DAT is read into when sampling RTD
unsigned long ulADC1CONRtd;               // used to set ADC1CON which sets channel to RTD
unsigned char ucCounter = 0;
// UART-based external variables
unsigned char ucTxBufferEmpty  = 0;       // Used to indicate that the UART Tx buffer is empty
unsigned char szTemp[64] = "";            // Used to store string before printing to UART
unsigned char nLen = 0;
unsigned char i = 0;
unsigned char ucWaitForUart = 0;          // Used by calibration routines to wait for user input

int main (void)
{

   DioOen(pADI_GP1,0x8);                        // Set P1.3 as an output for test purposes
   WdtCfg(T3CON_PRE_DIV1,T3CON_IRQ_EN,T3CON_PD_DIS); // Disable Watchdog timer resets
   //Disable clock to unused peripherals
   ClkDis(CLKDIS_DISSPI0CLK|CLKDIS_DISSPI1CLK|CLKDIS_DISI2CCLK|CLKDIS_DISPWMCLK|CLKDIS_DIST0CLK|CLKDIS_DIST1CLK|CLKDIS_DISDMACLK); // Only enable clock to used blocks
   ClkCfg(CLK_CD0,CLK_HF,CLKSYSDIV_DIV2EN_DIS,CLK_UCLKCG);     // Select CD0 for CPU clock
   ClkSel(CLK_CD7,CLK_CD7,CLK_CD0,CLK_CD7);     // Select CD0 for UART System clock
   AdcGo(pADI_ADC1,ADCMDE_ADCMD_IDLE);          // Place ADC1 in Idle mode
   UARTINIT();                         // Init Uart
   ADC1INIT();                         // Setup ADC1
   IEXCINIT();                         // Setup Excitation current source
   
   NVIC_EnableIRQ(ADC1_IRQn);          // Enable ADC1 and UART interrupt sources
   NVIC_EnableIRQ(UART_IRQn);

   while (1)
   {
      // reset calculation variables
      fVRTD =0;
      fVAIN1_AGND = 0;
      fVAIN0_AGND = 0;
      fIexc0 = 0;


      AdcGo(pADI_ADC1,ADCMDE_ADCMD_IDLE);		 // Put ADC1 into idle mode to allow re-configuration of its control registers
   /* Step 1 - Measure AIN1 v AGND and derive exact Excitation current */
      AdcRng(pADI_ADC1,ADCCON_ADCREF_INTREF,ADCMDE_PGA_G1,ADCCON_ADCCODE_INT); // Set G=1, Select internal reference
      AdcBuf(pADI_ADC1,ADCCFG_EXTBUF_OFF,ADCCON_BUFBYPN|ADCCON_BUFBYPP);       // Turn off input buffers
      AdcPin(pADI_ADC1,ADCCON_ADCCN_AGND,ADCCON_ADCCP_AIN1);                   // Select AIN1 to AGND
      bSendResultToUART = 0;
      AdcGo(pADI_ADC1,ADCMDE_ADCMD_CONT);    // Start ADC1 for continuous conversions
      ucCounter = 0;
      while ( bSendResultToUART == 0)        // Wait for 8x samples to accumulate before calculating result
      {}
      AdcGo(pADI_ADC1,ADCMDE_ADCMD_IDLE);   // Put ADC1 into idle mode to allow re-configuration of its control registers

      for (ucCounter = 0; ucCounter < SAMPLENO; ucCounter++)
      {
         fVAIN1_AGND += (((float)ulADC1DATRtd[ucCounter]*1.2)  / 268435456);
      }
 
      bSendResultToUART = 0;
      fVAIN1_AGND = fVAIN1_AGND/SAMPLENO;    // Average our 8x samples
      fIexc0 = fVAIN1_AGND/5600;             // Calculate Excitation current 

   /* Step 2 - Measure AIN0 v AGND as a check */
      AdcPin(pADI_ADC1,ADCCON_ADCCN_AGND,ADCCON_ADCCP_AIN0); // Select AIN0 to AGND for these measurements
      AdcGo(pADI_ADC1,ADCMDE_ADCMD_CONT);    // Start ADC1 for continuous conversions
      ucCounter = 0;
      while ( bSendResultToUART == 0)        // Wait for 8x samples to accumulate
      {}
      AdcGo(pADI_ADC1,ADCMDE_ADCMD_IDLE);		// Put ADC1 into idle mode to allow re-configuration of its control registers
      for (ucCounter = 0; ucCounter < SAMPLENO; ucCounter++)
      {
         fVAIN0_AGND += (((float)ulADC1DATRtd[ucCounter]*1.2)  / 268435456);	
      } 

      bSendResultToUART = 0;

      fVAIN0_AGND = fVAIN0_AGND/SAMPLENO;		 // Calculate average value

   /* Step 3 - Measure AIN1 v AIN0 for voltage across RTD */
      AdcBuf(pADI_ADC1,ADCCFG_EXTBUF_OFF,ADC_BUF_ON);	// Turn ADC input buffers on for this differential measurement
      AdcRng(pADI_ADC1,ADCCON_ADCREF_INTREF,ADCMDE_PGA_G32,ADCCON_ADCCODE_INT); // Set G=32 to measure across RTD
      AdcPin(pADI_ADC1,ADCCON_ADCCN_AIN1,ADCCON_ADCCP_AIN0);	 // Select AIN0/AIN1 as differential inputs
      AdcGo(pADI_ADC1,ADCMDE_ADCMD_CONT);			// Start ADC1 for continuous conversions
      ucCounter = 0;
      while (	bSendResultToUART == 0)					// Wait for 8x samples to accumulate
      {}
      for (ucCounter = 0; ucCounter < SAMPLENO; ucCounter++)
      {
         fVRTD += (((float)ulADC1DATRtd[ucCounter]*1.2)  / 268435456);
      }

      bSendResultToUART = 0;

      fVRTD = fVRTD/SAMPLENO;                   // Calcualte voltage across RTD
      AdcGo(pADI_ADC1,ADCMDE_ADCMD_IDLE);
   /* Step 4 - Calculate RTD value in ohms and derive final temeprature */
      fRrtd = fVRTD/fIexc0;                     // RTD resistance value
      fTRTD = CalculateRTDTemp(fRrtd);          // RTD temperature
      bSendResultToUART = 0;
      SendResultToUART();                       // Send results to UART
      DioTgl(pADI_GP1,0x8);                     // Toggle LED, P1.3
   }
}

void UARTINIT (void)
{
   //Select IO pins for UART.
   pADI_GP0->GPCON |= 0x3C;                     // Configure P0.1/P0.2 for UART		
//    pADI_GP0->GPCON |= 0x9000;                   // Configure P0.6/P0.7 for UART
   UrtCfg(pADI_UART,B9600,COMLCR_WLS_8BITS,0);  // setup baud rate for 9600, 8-bits
   UrtMod(pADI_UART,COMMCR_DTR,0);              // Setup modem bits
   UrtIntCfg(pADI_UART,COMIEN_ERBFI|COMIEN_ETBEI|COMIEN_ELSI|COMIEN_EDSSI|COMIEN_EDMAT|COMIEN_EDMAR);   // Setup UART IRQ sources
}
void SendString (void)
{
   for ( i = 0 ; i < nLen ; i++ )	// loop to send ADC0 result
   {
      ucTxBufferEmpty = 0;
      UrtTx(pADI_UART,szTemp[i]);
      while (ucTxBufferEmpty == 0)
      {
      }
   }
} 
void SendResultToUART(void)
{
   sprintf ( (char*)szTemp, "Voltage across Rref: %fV \r\n",fVAIN1_AGND );                          
   nLen = strlen((char*)szTemp);
   if (nLen <64)
      SendString();

   sprintf ( (char*)szTemp, "Voltage across RTD : %fV \r\n",fVRTD );                          
   nLen = strlen((char*)szTemp);
   if (nLen <64)
      SendString();

   sprintf ( (char*)szTemp, "RTD Resistance: %fOhms \r\n",fRrtd );                          
   nLen = strlen((char*)szTemp);
   if (nLen <64)
      SendString();

   sprintf ( (char*)szTemp, "Excitation current: %fA \r\n",fIexc0 );                          
   nLen = strlen((char*)szTemp);
   if (nLen <64)
      SendString();

   sprintf ( (char*)szTemp, "RTD Temperature: %fC \r\n\n\n",fTRTD );                          
   nLen = strlen((char*)szTemp);
   if (nLen <64)
      SendString();
}
void SendErrorToUART(void)
{
   sprintf ( (char*)szTemp, "ADC Overvoltage error on ADC1 PGA  \r\n");// Send error message to UART
   nLen = strlen((char*)szTemp);
   if (nLen <64)
      SendString();
}
void ADC1INIT(void)
{
   AdcMski(pADI_ADC1,ADCMSKI_RDY,1);                                 // Enable ADC ready interrupt source		
   AdcFlt(pADI_ADC1,124,14,FLT_NORMAL|ADCFLT_NOTCH2|ADCFLT_CHOP);    // ADC filter set for 3.75Hz update rate with chop on enabled
   AdcRng(pADI_ADC1,ADCCON_ADCREF_INTREF,ADCMDE_PGA_G32,ADCCON_ADCCODE_INT);  // Internal reference selected, Gain of 32, Signed integer output
   // Turn off input buffers to ADC and external reference
   AdcBuf(pADI_ADC1,ADCCFG_EXTBUF_OFF,ADCCON_BUFBYPN|ADCCON_BUFBYPP|ADCCON_BUFPOWP|ADCCON_BUFPOWN); 
   AdcPin(pADI_ADC1,ADCCON_ADCCN_AIN1,ADCCON_ADCCP_AIN0);            // Select AIN0 as postive input and AIN1 as negative input

}
void DACINIT(void)
{
   // Configure DAC output for 0-1.2V output range, Normal 12-bit mode and immediate update.
   DacCfg(DACCON_CLR_Off,DACCON_RNG_IntVref,DACCON_CLK_HCLK,DACCON_MDE_12bit);	  
   DacWr(0,0x1FF0000);                 // Output value of 150mV
}
void IEXCINIT(void)
{
   IexcDat(IEXCDAT_IDAT_200uA,IDAT0En);         // Set output for 200uA
   IexcCfg(IEXCCON_PD_off,IEXCCON_REFSEL_Int,IEXCCON_IPSEL1_Off,IEXCCON_IPSEL0_AIN6); //Setup IEXC) for AIN6
}
float CalculateRTDTemp(float r) 
{
   float t;
   int j;
   j=(int)((r-RMIN)/RSEG);       // determine which coefficients to use
   if (j<0)               // if input is under-range..
      j=0;                // ..then use lowest coefficients
   else if (j>NSEG-1)     // if input is over-range..
      j=NSEG-1;            // ..then use highest coefficients
   t = C_rtd[j]+(r-(RMIN+RSEG*j))*(C_rtd[j+1]-C_rtd[j])/RSEG;
   return t;
}

void WakeUp_Int_Handler(void)
{
  
}
void Ext_Int2_Handler ()
{           

}
void Ext_Int4_Handler ()
{         
 
}
void GP_Tmr0_Int_Handler(void)
{

}

void GP_Tmr1_Int_Handler(void)
{
 
}
void ADC0_Int_Handler()
{
   
}
void ADC1_Int_Handler ()
{
   volatile unsigned int uiADCSTA = 0;
   volatile long ulADC1DAT = 0;

   uiADCSTA = pADI_ADC1->STA;          // read ADC status register
   if ((uiADCSTA & 0x4) == 0x4)        // Check for ADC1TH exceeded error condition
   {
      ucADCERR = 1;
      pADI_ADC1->MSKI &= 0xFB;         // Disable Threshold detection to clear this interrupt source
      pADI_ADC1->PRO = 0;              // Disable comparator
   }
   ulADC1DAT = AdcRd(pADI_ADC1);       // read ADC result register;
   ulADC1DATRtd[ucSampleNo++] = ulADC1DAT;
   if (ucSampleNo > SAMPLENO)
   {
      ucSampleNo = 0;
      bSendResultToUART = 1;           // Set flag to indicate ready to send result to UART
   }
}
void SINC2_Int_Handler()
{
 
}
void UART_Int_Handler ()
{
   volatile unsigned char ucCOMSTA0 = 0;
   volatile unsigned char ucCOMIID0 = 0;

   ucCOMIID0 = UrtIntSta(pADI_UART);   // Read UART Interrupt ID register
   if ((ucCOMIID0 & 0x2) == 0x2)       // Transmit buffer empty
   {
      ucTxBufferEmpty = 1;
   }
   if ((ucCOMIID0 & 0x4) == 0x4)       // Receive byte
   {
      ucWaitForUart = 0;
   }
} 
void I2C0_Slave_Int_Handler(void)
{

}
void PWMTRIP_Int_Handler ()
{           
 
}
void PWM0_Int_Handler()
{

}
void PWM1_Int_Handler ()
{
 
}
void PWM2_Int_Handler()
{
  
}






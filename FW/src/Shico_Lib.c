#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <ADuCM360.h>
#include "Shico_Lib.h"

//#define NULL    ((void *)0)
#define LARGE   64	      /* use 'ABCDEF' instead of 'abcdef' */
#define MOD8_FIRST		0	
#define MOD8_LAST		7
#define MOD8(x)		(0x7&(x))
unsigned char ucTxBufferEmpty  = 0;       // Used to indicate that the UART Tx buffer is empty
unsigned char ucWaitForUart = 0;          // Used by calibration routines to wait for user input
uint8_t GetStrTmp=0;
	
// In case you wanna write data into an address.
void write_reg(unsigned volatile int address, unsigned int data)
{
    unsigned volatile int *ptr = 0x0;
    ptr=(unsigned volatile int *) address;
    *ptr=data;
}

// In case you wanna read data into an address.
unsigned int read_reg(unsigned volatile int address)
{
    unsigned volatile int *ptr = 0;
    ptr=(unsigned volatile int *) address;
    return(*ptr);
}

// In case you wanna set a bit of data at an address.
void set_bit(unsigned volatile int address, unsigned short int bitnumber)
{
    unsigned volatile int *ptr = 0;
    int reg_data,shift=1;

    ptr      =  (unsigned volatile int *) address;
    reg_data =  (*ptr);
    reg_data =  (reg_data)|(shift<<bitnumber);
    (*ptr)   =  reg_data;

}

// In case you wanna clear a bit of data at an address.
void clear_bit(unsigned volatile int address, unsigned short int bitnumber)
{
    unsigned volatile int *ptr = 0;
    unsigned int reg_data,shift=1;

    ptr      =  (unsigned volatile int *) address;
    reg_data =  (*ptr);
    reg_data =  (reg_data) & (~(shift<<bitnumber));
    (*ptr)   =  reg_data;

}

// In case you wanna get a bit of data at an address.
unsigned int get_bit(unsigned volatile int address,unsigned short int bitnumber)
{
    unsigned int reg_data,shift=1;
    unsigned volatile int *ptr = 0;

    ptr      =  (unsigned volatile int *) address;
    reg_data =  (*ptr);
    reg_data =  ( (reg_data)&(shift << bitnumber) ) >> (bitnumber);

    return(reg_data);

} 

//Write Mask operation :
void write_mask(unsigned volatile int address, unsigned int data, unsigned int mask_pattern)
{
	unsigned int data_mask;
    	data_mask=data & (~(mask_pattern));
	write_reg(address,(data_mask|(read_reg(address))));
}

//Read Mask operation :
unsigned int read_mask(unsigned volatile int address,unsigned int mask_pattern)
{
    	return ((read_reg(address)) & (~(mask_pattern)));
}

// Polling flag checking routine :
unsigned int poll_on_set(unsigned volatile int address,unsigned short int bitnumber)
{
        unsigned volatile int *ptr = 0;
        unsigned int bit_data;
        unsigned int shift=1;

        ptr  =  (unsigned volatile int *) address;

        do									
        {
              bit_data =  (*ptr);
              bit_data =  ( (bit_data)&(shift << bitnumber) ) >> (bitnumber);

        }while(bit_data==0);

        if(bit_data == 1 )
        {
            return 1;
        }
        else
        {
            return 0;
        }
}

// Polling flag checking routine including limitation on the polling operation :
unsigned int poll_on_set_limit(unsigned volatile int address,unsigned short int bitnumber, unsigned int count)
{
        unsigned volatile int *ptr = 0;
        unsigned int bit_data;
        unsigned int shift=1;
        unsigned int set_count=0;
	
        ptr  =  (unsigned volatile int *) address;

        do
        {
              bit_data =  (*ptr);
              bit_data =  ( (bit_data)&(shift << bitnumber) ) >> (bitnumber);
	      set_count++;
        }while((bit_data==0) && (set_count < count));

        if(bit_data == 1 )
        {
            return 1;
        }
        else
        {
            return 0;
        }
}

// Checking routine for dedicated polling flag : 
unsigned int poll_on_reset( unsigned volatile int address, unsigned short int bitnumber )
{
    	unsigned int bit_data;

	do
	{
		bit_data=get_bit(address,bitnumber);

	}while(bit_data==1);
	if(bit_data==0)
        {
	    return 1;
	}
	else
	{
	    return 0;
	}
}

///////////////////////////////// STDIO.H ////////////////////////////////////////////
int hextoint(unsigned char *hex)
{
	unsigned char 	r = 0;
	unsigned char		d;

	while (1)
	{
		d = *hex++;
		if (d == 0)
			break;
		r <<= 4;
		if ((d >= '0') && (d <= '9'))
			r += d - '0';
		else if ((d >= 'A') && (d <= 'F'))
			r += d - 'A' + 10;
		else if ((d >= 'a') && (d <= 'f'))
			r += d - 'a' + 10;
		else
			return -1;
	}
	return r;
}

int gets(uint8_t *s)
{
	int len = 0;

	while (1)
	{
		s[len] = getc();

		if (s[len] == '\n' || s[len] == '\r')
		{
			putc('\r');
			putc('\n');
			s[len] = '\0';
			break;
		}
		else if (s[len] == 0x7F || s[len] == 0x08)
		{
			if (len > 0)
			{
				puts("\b \b");
				len--;
			}
		}
		else
		{
			putc(s[len]);
			len++;
		}
	}
	return 1;
}

#if 0
uint8_t getc(void)
{
	uint8_t reg_data = 0, tmp;
	reg_data = *pCOMLSR;
	reg_data = ((reg_data)&(0x1 << 0)) >> (0);
	while (reg_data != 0x1)
	{
		reg_data = *pCOMLSR;
		reg_data = ((reg_data)&(0x1 << 0)) >> (0);
	}
	tmp = *pCOMTX_RX;
#ifndef FermiEmulation_Mode
	__asm{ DSB}
#endif
	return tmp;
}
#else
uint8_t getc(void)
{
	uint8_t tmp;
	
	ucWaitForUart = 0;			
	while(ucWaitForUart == 0);	
	tmp=GetStrTmp;
	return tmp;
}
#endif

void puts(const char* str)
{
	while (*str)
	{
		putc(*str);
		++str;
	}
}

#if 0
void putc(uint8_t Ch)
{
	if (Ch == '\n')
	{
		while ((((read_reg(0x40005014)&(0x1 << 5))) >> (5)) != 0x1);
		*pCOMTX_RX = '\n';
	}
	while ((((read_reg(0x40005014)&(0x1 << 5))) >> (5)) != 0x1);
	*pCOMTX_RX = Ch;
#ifndef FermiEmulation_Mode
	__asm{ DSB}
#endif	
}
#else
void putc(uint8_t Ch)
{
	if (Ch == '\n')
	{
		ucTxBufferEmpty = 0;
		UrtTx(pADI_UART,'\n');
		while(ucTxBufferEmpty == 0);
	}
	ucTxBufferEmpty = 0;		
	UrtTx(pADI_UART,Ch);
	while(ucTxBufferEmpty == 0);	
}
#endif

void printf(const char *fmt, ...)
{
	va_list args;
	char printbuffer[256];

	va_start(args, fmt);

	sprintf(printbuffer, fmt, args);
	va_end(args);	
	puts(printbuffer);
}

int sprintf(char *buf, const char *fmt, va_list args)
{

	unsigned num;
	int base;
	char * str;

	int flags;	      /* flags to number() */

	int field_width = -1;

	for (str = buf; *fmt; ++fmt) {
		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}

		/* process flags */
		flags = 0;
		++fmt;	  /* this also skips first '%' */

		/* default base */
		base = 16;

		switch (*fmt) {
		case '%':
			*str++ = '%';
			continue;

			/* integer number formats - set up the flags and "break" */
		case 'X':
			flags |= LARGE;
		case 'x':
			base = 16;
			break;

		default:
			*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			continue;
		}

		num = va_arg(args, unsigned int);
		str = number(str, num, base, field_width, flags);
	}
	*str = '\0';
	return str - buf;
}

char* number(char * str, unsigned num, int base, int size, int type)
{
	char tmp[66];
	const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	int i, j = 8;

	if (type & LARGE)
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	size -= 2;

	i = 0;
	if (num == 0)
		tmp[i++] = '0';
	else while (num != 0)
		tmp[i++] = digits[do_div(&num, base)];

	while (j - i)
	{
		tmp[i++] = '0';
	}

	while (i-- > 0)
		*str++ = tmp[i];
	while (size-- > 0)
		*str++ = ' ';
	return str;
}


void unsigned_divide(unsigned int dividend, unsigned int divisor,
	unsigned int* p_quotient, unsigned int* p_remainder)
{
	unsigned int t, num_bits;
	unsigned int q, bit, d=0;
	unsigned int i;

	*p_remainder = 0;
	*p_quotient = 0;

	if (divisor == 0)
		return;

	if (divisor > dividend) {
		*p_remainder = dividend;
		return;
	}

	if (divisor == dividend) {
		*p_quotient = 1;
		return;
	}

	num_bits = 32;

	while (*p_remainder < divisor) {
		bit = (dividend & 0x80000000) >> 31;
		*p_remainder = (*p_remainder << 1) | bit;
		d = dividend;
		dividend = dividend << 1;
		num_bits--;
	}

	dividend = d;
	*p_remainder = *p_remainder >> 1;
	num_bits++;

	for (i = 0; i < num_bits; i++) {
		bit = (dividend & 0x80000000) >> 31;
		*p_remainder = (*p_remainder << 1) | bit;
		t = *p_remainder - divisor;
		q = !((t & 0x80000000) >> 31);
		dividend = dividend << 1;
		*p_quotient = (*p_quotient << 1) | q;
		if (q) {
			*p_remainder = t;
		}
	}
}  /* unsigned_divide */

int do_div(unsigned int *n, int base)
{
	unsigned int	quo, res;
	unsigned_divide(*n, base, &quo, &res);

	*n = quo;
	return res;
}

///////////////////////////////  Memory Read/Write  ///////////////////////////////
void MemoryDump(char *str1)
{
	uint32_t memAddr, memSize=0x40;
	char str2[]="", str3[]="";
	char Ch=' ';
	char *ptr=0;
	int i;	

	ptr=strchr(str1,Ch);
	Ch=tolower(*(ptr+2));
	if (Ch == 'x')
	{
		for(i=0;i<8;i++)
		{
			if((*(ptr+3+i)) != ' ')
			{
				str2[i]=*(ptr+3+i);
			} else
			{
				str2[i]='\0';
				break;
			}
		} 
		memAddr=stoi(str2);
		strcpy(str2,(&ptr[0]+3+i));
	} else
	{
		for(i=0;i<8;i++)
		{
			if((*(ptr+1+i)) != ' ')
			{
				str2[i]=*(ptr+1+i);
			} else
			{
				str2[i]='\0';
				break;
			}
		} 
		memAddr=stoi(str2);
		strcpy(str2,(&ptr[0]+1+i));
	}
	Ch=' ';
	ptr=strchr(str2,Ch);
	Ch=tolower(*(ptr+2));
	if (Ch == 'x')
	{
		for(i=0;i<8;i++)
		{
			if(((*(ptr+3+i)) != ' ') && ((*(ptr+3+i)) != '\0'))
			{
				str3[i]=*(ptr+3+i);
			} else
			{
				str3[i]='\0';
				break;
			}
		}
		memSize=stoi(str3);
	} else
	{
		for(i=0;i<8;i++)
		{
			if(((*(ptr+1+i)) != ' ') && ((*(ptr+1+i)) != '\0'))
			{
				str3[i]=*(ptr+1+i);
			} else
			{
				str3[i]='\0';
				break;
			}
		}
		memSize=atoi(str3);
	}
	
	DispMemData (memAddr , memSize);
}

void MemorySet(char *str1)
{
	uint32_t memAddr, memVal;
	char str2[]="", str3[]="";
	char Ch=' ';
	char *ptr=0;
	int i;	

	ptr=strchr(str1,Ch);
	Ch=tolower(*(ptr+2));
	if (Ch == 'x')
	{
		for(i=0;i<8;i++)
		{
			if((*(ptr+3+i)) != ' ')
			{
				str2[i]=*(ptr+3+i);
			} else
			{
				str2[i]='\0';
				break;
			}
		} 
		memAddr=stoi(str2);
		strcpy(str2,(&ptr[0]+3+i));
	} else
	{
		for(i=0;i<8;i++)
		{
			if((*(ptr+1+i)) != ' ')
			{
				str2[i]=*(ptr+1+i);
			} else
			{
				str2[i]='\0';
				break;
			}
		} 
		memAddr=stoi(str2);
		strcpy(str2,(&ptr[0]+1+i));
	}
	Ch=' ';
	ptr=strchr(str2,Ch);
	Ch=tolower(*(ptr+2));
	if (Ch == 'x')
	{
		for(i=0;i<8;i++)
		{
			if(((*(ptr+3+i)) != ' ') && ((*(ptr+3+i)) != '\0'))
			{
				str3[i]=*(ptr+3+i);
			} else
			{
				str3[i]='\0';
				break;
			}
		}
		memVal=stoi(str3);
	} else
	{
		for(i=0;i<8;i++)
		{
			if(((*(ptr+1+i)) != ' ') && ((*(ptr+1+i)) != '\0'))
			{
				str3[i]=*(ptr+1+i);
			} else
			{
				str3[i]='\0';
				break;
			}
		}
		memVal=atoi(str3);
	}
	write_reg(memAddr,memVal);
}

void DispMemData (uint32_t memAddr , uint32_t memSize)
{
	int cnt = 0;
	uint32_t uValue;
	uint32_t *pMem = (uint32_t *)memAddr;
	memSize = memSize/4;
	
	for(cnt=0;cnt<memSize;cnt++)
	{
		if(MOD8(cnt)==MOD8_FIRST)
		{
			printf("0x%x   ",pMem);
		}
		
		uValue=*pMem++;
		if(MOD8(cnt)==MOD8_LAST)
		{
			printf("0x%x\n",uValue);
		}
		else
		{
			printf("0x%x ",uValue);
		}
	}
	printf("\n");
}

//  Conversion of string hex value to int hex value
uint32_t stoi(char *pString)
{
	int cnt ; 
	uint32_t  uLength;
	uint32_t uSum;
	uint32_t auValue[8] = {0,0,0,0,0,0,0,0};


	uLength=strlen(pString);
	if(uLength>8)
	{
		uLength=8;
	}
	
	for(cnt=0;cnt<uLength;cnt++)	// get the values individually 
	{
		*pString=0x20 | (*pString);
		if(*pString >= 'A')		// if it is between A-F
		{
			auValue[uLength-1-cnt]=10+(*pString-'a');
		} 
		else 	// for value between 0 - 9
		{
			auValue[uLength-1-cnt]=*pString-'0';
		}
		pString+=1;
	}

	uSum=0;
	uSum+=(0x10000000*auValue[7]);
	uSum+=(0x1000000*auValue[6]);
	uSum+=(0x100000*auValue[5]);
	uSum+=(0x10000*auValue[4]);
	uSum+=(0x1000*auValue[3]);
	uSum+=(0x100*auValue[2]);
	uSum+=(0x10*auValue[1]);
	uSum+=(auValue[0]);

	return(uSum);
}


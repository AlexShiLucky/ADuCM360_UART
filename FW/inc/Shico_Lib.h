#include <stdint.h>
#include <stdarg.h>

extern unsigned char ucTxBufferEmpty;
extern unsigned char ucWaitForUart;
extern uint8_t GetStrTmp;

void write_reg(unsigned volatile int address, unsigned int data);
unsigned int read_reg(unsigned volatile int address);
void set_bit(unsigned volatile int address, unsigned short int bitnumber);
void clear_bit(unsigned volatile int address, unsigned short int bitnumber);
unsigned int get_bit(unsigned volatile int address,unsigned short int bitnumber);
void write_mask(unsigned volatile int address, unsigned int data, unsigned int mask_pattern);
unsigned int read_mask(unsigned volatile int address,unsigned int mask_pattern);
unsigned int poll_on_set(unsigned volatile int address,unsigned short int bitnumber);
unsigned int poll_on_set_limit(unsigned volatile int address,unsigned short int bitnumber, unsigned int count);
unsigned int poll_on_reset( unsigned volatile int address, unsigned short int bitnumber );

///////////////////////////////// STDIO.H ////////////////////////////////////////////

uint8_t getc(void);
void putc(uint8_t Ch);
void puts(const char* str);
int gets(uint8_t *s);
void menu_display(void);
int hextoint(unsigned char *hex);
void printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, va_list args);
int do_div(unsigned int *n, int base);
void unsigned_divide(unsigned int dividend, unsigned int divisor,
	unsigned int* p_quotient, unsigned int* p_remainder);
char* number(char * str, unsigned num, int base, int size, int type);

///////////////////////////////  Memory Read/Write  ///////////////////////////////
void MemoryDump(char *str1);
void MemorySet(char *str1);
void DispMemData (uint32_t memAddr , uint32_t memSize);
uint32_t stoi(char *pString);

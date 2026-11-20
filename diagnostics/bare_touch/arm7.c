/* Bare ARM7 touchscreen controller adapted from the user's MANDEX port.
 * Reads pen GPIO and SPI ADC independently, bypasses the BlocksDS FIFO
 * and libnds' touchscreen noise filter entirely. */
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
#define R16(a) (*(volatile u16*)(a))
#define SPI_CNT 0x040001C0u
#define SPI_DAT 0x040001C2u
#define EXTKEY  0x04000136u
#define VCOUNT  0x04000006u
typedef struct {volatile u16 bits,px,py,alive,raw_x,raw_y;} Shared;
#define IPC ((Shared*)0x027FF000u)
static void spi_wait(void){u32 t=100000;while((R16(SPI_CNT)&0x80u)&&--t){}}
static u8 xfer(u8 v,u16 flags){R16(SPI_CNT)=0x8000u|flags;R16(SPI_DAT)=v;spi_wait();return (u8)R16(SPI_DAT);}
static u16 adc(u8 command){
 (void)xfer(command,0x0A01);
 u32 hi=xfer(0,0x0A01),lo=xfer(0,0x0201);
 return (u16)((((hi<<8)|lo)>>3)&0x0FFFu);
}
void arm7_main(void){
 u16 count=0;IPC->bits=0;IPC->alive=0;
 for(;;){
  while(R16(VCOUNT)!=200u){}
  u16 e=R16(EXTKEY),bits=0;
  if(!(e&1u))bits|=1u;
  if(!(e&2u))bits|=2u;
  if(!(e&0x40u)){
   bits|=4u; /* pen GPIO, even when ADC invalid */
   u16 rx=adc(0xD1),ry=adc(0x91);
   IPC->raw_x=rx;IPC->raw_y=ry;
   if(rx>0&&ry>0&&rx<4080&&ry<4080){
    IPC->px=rx>>4;
    IPC->py=(u16)(((u32)ry*192u)>>12);
    bits|=8u; /* ADC validated */
   }
  }
  IPC->bits=bits;IPC->alive=++count;
  while(R16(VCOUNT)==200u){}
 }
}

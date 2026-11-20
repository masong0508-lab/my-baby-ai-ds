/* Bare DS ARM9 uses direct hardware registers and shared ARM7 input.
 * TOP: purple idle, green pen GPIO, yellow A button.
 * BOTTOM: blue idle; pink if pen GPIO asserted but ADC invalid; varying
 * teal if valid ADC. Blinking idle color proves CPU is alive. */
typedef unsigned short u16; typedef unsigned int u32;
#define R16(a) (*(volatile u16*)(a))
#define R32(a) (*(volatile u32*)(a))
typedef struct {volatile u16 bits,px,py,alive,raw_x,raw_y;} Shared;
#define IPC ((Shared*)0x027FF000u)
static u16 rgb(int r,int g,int b){return (u16)((r&31)|((g&31)<<5)|((b&31)<<10));}
int main(void){
 R16(0x04000304)=0x0203;
 R32(0x04000000)=0x00010000;
 R32(0x04001000)=0x00010000;
 unsigned frame=0;
 for(;;){
  while(R16(0x04000006)>=192u){}
  while(R16(0x04000006)<192u){}
  int pen=(IPC->bits&4u)!=0,valid=(IPC->bits&8u)!=0;
  int a=(R16(0x04000130)&1u)==0;
  u16 upper=pen?rgb(2,29,6):a?rgb(31,27,0):rgb(17,5,(frame/35u&1u)?24:27);
  u16 lower=pen?(valid?rgb((IPC->px>>3)&31,24,(IPC->py>>3)&31):rgb(31,5,22)):rgb(7,19,26);
  R16(0x05000000)=upper;
  R16(0x05000400)=lower;
  frame++;
 }
}

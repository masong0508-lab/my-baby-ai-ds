/* My Baby A.I.! Nintendo DS dual-engine prototype:
 * 3D nursery on main engine, direct-VRAM bitmap touchscreen on sub engine.
 */
#include <nds.h>
#include <fat.h>
#include <stdint.h>
#include "game.h"
#include "save.h"
#include "world.h"
#include "ui.h"
#define SAVE_BASE "fat:/my_baby_ai"
static Game baby;
static Arcade arc;
static int storage_ok,view=5,selected,reset_step,last_ending,pose;
static const char *message="WELCOME TO DAYCARE 2.0";
static void persist(void){if(storage_ok)(void)save_write(SAVE_BASE,&baby);}
static void care(Care action){
 message=game_care(&baby,action);
 pose=(int)action+1;
 persist();
}
static void end_day(void){
 int e=game_end_day(&baby);
 last_ending=e;
 message=game_ending_name(e);
 if(e!=END_OK)view=4;
 persist();
}
static void begin_game(void){
 game_arcade_start(&baby,&arc,selected);
 if(arc.active){view=2;message="SIX ROUNDS. THREE STRIKES.";}
 else message="LOCKED CABINET.";
}
static void leave_game(void){
 if(arc.finished){
  message=arc.finished==1?"DIAPER DOLLARS EARNED!":"THE GLITCHES WON.";
  persist();
 }else message="BACK TO THE ARCADE.";
 arc.active=0;view=1;
}
int main(void){
 uint32_t frame=0;
 int dirty=1;
 game_init(&baby);
 lcdMainOnTop();
 videoSetMode(MODE_0_3D);
 videoSetModeSub(MODE_5_2D);
 vramSetBankC(VRAM_C_SUB_BG);
 {
  int layer=bgInitSub(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
  ui_init(bgGetGfxPtr(layer));
 }
 world_init();
 storage_ok=fatInitDefault();
 if(storage_ok)(void)save_load(SAVE_BASE,&baby);
 while(pmMainLoop()){
  int down,held,previous=view;
  touchPosition touch;
  swiWaitForVBlank();scanKeys();touchRead(&touch);
  down=keysDown();held=keysHeld();
  if(view==0){
   int action=-1;
   if(down&KEY_A)action=FEED;
   else if(down&KEY_B)action=CHAT;
   else if(down&KEY_X)action=PLAY;
   else if(down&KEY_Y)action=NAP;
   else if(down&KEY_TOUCH)action=ui_nursery_touch(touch.px,touch.py);
   if(action>=0){care((Care)action);dirty=1;}
   else if(down&KEY_START){view=1;message="WELCOME TO THE ARCADE.";dirty=1;}
   else if(down&KEY_SELECT){end_day();dirty=1;}
   else if(down&KEY_TOUCH){
    int nav=ui_home_navigation(touch.px,touch.py);
    if(nav==1){view=1;message="WELCOME TO THE ARCADE.";dirty=1;}
    else if(nav==2){end_day();dirty=1;}
   }else if((held&(KEY_L|KEY_R))==(KEY_L|KEY_R)&&game_reset_unlocked(&baby)){
    view=3;reset_step=0;dirty=1;
   }else if(down&KEY_L){view=6;dirty=1;}
   else if(down&KEY_R){
    baby.safe_visuals^=1;
    message=baby.safe_visuals?"QUIET GRAPHICS, SAME GAMEPLAY.":"VISUAL GLITCHES RESTORED.";
    persist();dirty=1;
   }
  }else if(view==1){
   if(down&KEY_UP){selected=(selected+15)%16;dirty=1;}
   else if(down&KEY_DOWN){selected=(selected+1)%16;dirty=1;}
   else if(down&KEY_LEFT){selected=(selected+15)%16;dirty=1;}
   else if(down&KEY_RIGHT){selected=(selected+1)%16;dirty=1;}
   else if(down&KEY_B){view=0;message="BACK TO THE NURSERY.";dirty=1;}
   else if(down&KEY_X){
    if(game_unlock(&baby,selected)){message="CABINET BOUGHT!";persist();}
    else message="EARN MORE WINS OR CURRENCY.";
    dirty=1;
   }else if(down&KEY_A){begin_game();dirty=1;}
   else if(down&KEY_TOUCH){
    int nav=ui_arcade_navigation(touch.px,touch.py);
    if(nav==1){begin_game();dirty=1;}
    else if(nav==2){
     if(game_unlock(&baby,selected)){message="CABINET BOUGHT!";persist();}
     else message="EARN MORE WINS OR CURRENCY.";
     dirty=1;
    }else if(nav==3){view=0;message="BACK TO THE NURSERY.";dirty=1;}
    else{
     int id=ui_arcade_select(touch.px,touch.py);
     if(id>=0){if(id==selected)begin_game();else selected=id;dirty=1;}
    }
   }
  }else if(view==2){
   int direction=-1;
   if(down&KEY_B){leave_game();dirty=1;}
   else{
    if(down&KEY_UP)direction=0;
    else if(down&KEY_RIGHT)direction=1;
    else if(down&KEY_DOWN)direction=2;
    else if(down&KEY_LEFT)direction=3;
    else if(down&KEY_TOUCH){
     if(touch.py>=176){leave_game();dirty=1;}
     else direction=ui_arcade_touch(touch.px,touch.py);
    }
    if(direction>=0){game_arcade_input(&baby,&arc,direction);dirty=1;}
    if(!arc.finished)game_arcade_tick(&baby,&arc);
    if(arc.finished){leave_game();dirty=1;}
   }
  }else if(view==3){
   if(down&KEY_B){view=0;reset_step=0;message="RESET CANCELED.";dirty=1;}
   else if(!reset_step){if(down&KEY_A){reset_step=1;dirty=1;}}
   else if((down&KEY_X)&&(held&(KEY_L|KEY_R))==(KEY_L|KEY_R)){
    if(storage_ok&&!save_erase(SAVE_BASE)){message="SAVE ERASE FAILED. NO RESET.";}
    else{game_init(&baby);message="COLD BOOT. WHO AM I?";}
    view=0;reset_step=0;dirty=1;
   }
  }else if(view==4){
   if((down&(KEY_A|KEY_B|KEY_START))||(down&KEY_TOUCH)){
    view=0;message="BACK TO DAYCARE. SAME SAVE!";dirty=1;
   }
  }else if(view==5){
   if(down&(KEY_A|KEY_START|KEY_TOUCH)){
    view=0;message="WELCOME BACK, SHAREHOLDER.";dirty=1;
   }
  }else if(view==6){
   if(down&(KEY_B|KEY_START|KEY_A|KEY_TOUCH)){
    view=0;message="COLLECTION CLOSED.";dirty=1;
   }
  }
  world_draw(&baby,frame,pose);
  /* Keep bitmap VRAM writes off the hot path unless UI state changed. */
  if(dirty||previous!=view||(view==2&&frame%9u==0u)){
   ui_draw(&baby,&arc,view,selected,last_ending,reset_step,storage_ok,message);
   dirty=0;
  }
  frame++;
 }
 return 0;
}

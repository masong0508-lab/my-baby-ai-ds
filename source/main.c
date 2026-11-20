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
#include "touch_input.h"
#include "virtual_stylus.h"
#if defined(__BLOCKSDS__)
/* BlocksDS main loop convention: traditional foreground VBlank loop. */
#define GAME_LOOP_ACTIVE() 1
#else
/* Modern devkitPro libnds2/calico asks applications to use pmMainLoop(). */
#define GAME_LOOP_ACTIVE() pmMainLoop()
#endif
#define SAVE_BASE "fat:/my_baby_ai"
static Game baby;
static Arcade arc;
static int storage_ok,view=5,selected,reset_step,last_ending,pose;
static int three_d_ready;
static TouchGate gesture;
static VirtualStylus vpen;
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
 touch_gate_init(&gesture);
 virtual_stylus_init(&vpen);
 lcdMainOnTop();
 /* Title must appear before 3D or filesystem work; this isolates crashes. */
 videoSetMode(MODE_0_2D);
 setBackdropColor(RGB15(15,8,21));
 videoSetModeSub(MODE_5_2D);
 vramSetBankC(VRAM_C_SUB_BG);
 {
  int layer=bgInitSub(3,BgType_Bmp16,BgSize_B16_256x256,0,0);
  ui_init(bgGetGfxPtr(layer));
 }
 /* Always show a visible title before touching the filesystem or heavy 3D.
  * Some older emulators hang during their emulated DLDI/FAT setup. */
 ui_draw(&baby,&arc,view,selected,last_ending,reset_step,storage_ok,message);
 /* Stay in RAM-only mode until the player explicitly requests save access.
  * New and old .nds game saves remain compatible when FAT is available. */
 while(GAME_LOOP_ACTIVE()){
  int down,held,tap,previous=view,virtual_move=0;
  int raw_touch;
  touchPosition touch={0};
  swiWaitForVBlank();
  scanKeys();
  down=keysDown();held=keysHeld();
  raw_touch=(held&KEY_TOUCH)!=0;
  if(raw_touch)touchRead(&touch);
  tap=touch_gate_step(&gesture,raw_touch,touch.px,touch.py);
  ui_touch_observe(raw_touch,touch.px,touch.py,gesture.total_presses);
  /* Compare against the proven button-scanning pattern from the user's
   * Mandex DS bare-metal port. This fallback needs no working ARM7 stylus. */
  if((down&KEY_SELECT)&&(held&KEY_L)){
   virtual_stylus_toggle(&vpen);
   down &= ~(KEY_SELECT|KEY_L); /* not "end day" or "album" */
   message=vpen.enabled?"VIRTUAL PEN: D-PAD MOVE, A TAP":"VIRTUAL PEN DISABLED.";
   dirty=1;
  }
  if(vpen.enabled){
   unsigned move=0;
   if(held&KEY_LEFT)move|=VS_LEFT;
   if(held&KEY_RIGHT)move|=VS_RIGHT;
   if(held&KEY_UP)move|=VS_UP;
   if(held&KEY_DOWN)move|=VS_DOWN;
   virtual_move=virtual_stylus_move(&vpen,move);
   if((down&KEY_A)&&!tap){
    tap=1;
    touch.px=vpen.x;
    touch.py=vpen.y;
   }
   /* D-pad belongs to cursor when it is enabled. X/Y and Start/B remain
    * normal shortcuts. A triggers the existing native tap hitboxes. */
   down &= ~(KEY_A|KEY_LEFT|KEY_RIGHT|KEY_UP|KEY_DOWN);
  }
  if(view==0){
   int action=-1;
   if(down&KEY_A)action=FEED;
   else if(down&KEY_B)action=CHAT;
   else if(down&KEY_X)action=PLAY;
   else if(down&KEY_Y)action=NAP;
   else if(tap)action=ui_nursery_touch(touch.px,touch.py);
   if(action>=0){care((Care)action);dirty=1;}
   else if(down&KEY_START){view=1;message="WELCOME TO THE ARCADE.";dirty=1;}
   else if(down&KEY_SELECT){end_day();dirty=1;}
   else if(tap){
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
   else if(tap){
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
    if(arc.id==0){
     int steer=0;
     if(!vpen.enabled && (held&KEY_LEFT))steer=-1;
     else if(!vpen.enabled && (held&KEY_RIGHT))steer=1;
     else if(raw_touch&&touch.py>=90&&touch.py<173)
      steer=touch.px<128?-1:1;
     else if(vpen.enabled&&(held&KEY_A)&&vpen.y>=90&&vpen.y<173)
      steer=vpen.x<128?-1:1;
     if((tap)&&touch.py>=176){leave_game();dirty=1;}
     else{
      game_arcade_steer(&arc,steer);
      game_arcade_tick(&baby,&arc);
     }
    }else{
     if(down&KEY_UP)direction=0;
     else if(down&KEY_RIGHT)direction=1;
     else if(down&KEY_DOWN)direction=2;
     else if(down&KEY_LEFT)direction=3;
     else if(tap){
      if(touch.py>=176){leave_game();dirty=1;}
      else direction=arc.id==1?ui_diaper_touch(touch.px,touch.py)
                              :ui_arcade_touch(touch.px,touch.py);
     }
     if(direction>=0){game_arcade_input(&baby,&arc,direction);dirty=1;}
     if(arc.active)game_arcade_tick(&baby,&arc);
    }
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
   if((down&(KEY_A|KEY_B|KEY_START))||tap){
    view=0;message="BACK TO DAYCARE. SAME SAVE!";dirty=1;
   }
  }else if(view==5){
   if(down&KEY_SELECT){view=7;dirty=1;}
   else if(down&KEY_X){
    message="CHECKING SAVE STORAGE...";
    ui_draw(&baby,&arc,view,selected,last_ending,reset_step,0,message);
    storage_ok=fatInitDefault();
    if(storage_ok){
     if(save_load(SAVE_BASE,&baby))message="SAVED BABY RESTORED!";
     else message="SAVING ENABLED!";
    }else message="NO SAVE DEVICE - RAM ONLY.";
    dirty=1;
   }else if((down&(KEY_A|KEY_START))||tap){
    /* Successfully displaying the 2D title means ARM9 game code ran. */
    videoSetMode(MODE_0_3D);
    world_init();
    three_d_ready=1;
    view=0;
    message=storage_ok?"WELCOME BACK, SHAREHOLDER.":"RAM MODE: X ON TITLE ENABLES SAVES.";
    dirty=1;
   }
  }else if(view==6){
   if((down&(KEY_B|KEY_START|KEY_A))||tap){
    view=0;message="COLLECTION CLOSED.";dirty=1;
   }
  }else if(view==7){
   if(down&(KEY_B|KEY_SELECT|KEY_START)){view=5;dirty=1;}
  }
  if(three_d_ready)world_draw(&baby,frame,pose);
  /* Keep bitmap VRAM writes off the hot path unless UI state changed. */
  if(dirty||previous!=view||(view==2&&frame%(arc.id==0?3u:9u)==0u)){
   ui_draw(&baby,&arc,view,selected,last_ending,reset_step,storage_ok,message);
   dirty=0;
  }
  if(view==7)ui_touch_diag_update();
  /* Overlay a tiny cursor only when virtual pen is enabled. The bottom BG
   * needn't be fully repainted on every frame, avoiding DraStic flicker. */
  ui_virtual_cursor_update(vpen.enabled,vpen.x,vpen.y,
                           vpen.enabled && (held&KEY_A)!=0);
  (void)virtual_move;
  frame++;
 }
 return 0;
}

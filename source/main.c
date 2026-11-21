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
#include "roughhouse.h"
#include "sfx.h"
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
static int storage_ok,view=5,selected,album_page,reset_step,last_ending,pose;
static int three_d_ready;
static TouchGate gesture;
static VirtualStylus vpen;
static Roughhouse rag;
static uint32_t egg_frame;
static int toy_tool=ROUGHHOUSE_SLAP;
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
 if(arc.active){view=2;message=arc.id==0?"STEADY THE BOTTLE.":arc.id==1?"CHANGE THAT DIAPER!":arc.id==2?"SORT BABY'S TAX BILLS!":arc.id==3?"DODGE CRIB KART CONES!":arc.id==4?"MEMORIZE THE BEAT!":arc.id==5?"MATCH THE BABY SOCKS!":arc.id==6?"TIME THE QUIET LULLABY!":arc.id==7?"CATCH SIX RATTLES!":arc.id==8?"JUMP OR DUCK AT WORK!":arc.id==9?"DRIFT THE STROLLER!":arc.id==10?"SOLVE CEREAL MATH!":arc.id==11?"STACK THOSE BLOCKS!":arc.id==12?"CHECK THE AUDIT NUMBERS!":arc.id==13?"WATER THE RIGHT FLOWER!":arc.id==14?"TRACE THE EVIDENCE!":arc.id==15?"BALANCE THE DAYCARE!":"SIX ROUNDS. THREE STRIKES.";}
 else message="LOCKED CABINET.";
}
/* Scene-specific bitmap refresh. Interactive screens retain fluid redraws;
  * still screens repaint only when state changes or the clock ticks, avoiding
  * needless 256x192 VRAM uploads beside the continuously rendered 3D nursery. */
static unsigned arcade_redraw_period(const Arcade *a){
 switch(a->id){
 case 0:case 3:case 6:case 7:case 8:case 9:case 11:return 3u;
 case 1:case 4:return 4u;
 case 2:return a->touch_active?4u:60u;
 case 5:case 10:case 12:case 13:case 14:case 15:return 60u;
 default:return 4u;
 }
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
 roughhouse_init(&rag);
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
 ui_draw(&baby,&arc,view,view==6?album_page:view==8?((int)rag.charge|(toy_tool==ROUGHHOUSE_KICK?32:0)|((int)rag.hits<<6)|
           (sfx_is_enabled()?(1<<16):0)):view==9?(int)egg_frame:selected,last_ending,reset_step,storage_ok,message);
 /* Sound starts only AFTER the title is visibly present. Small hardware
  * PSG cues are synthesized at the ARM9 game tick; no sample bank/heap. */
 sfx_init();
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
   if(((down&KEY_X)&&(held&KEY_R))||
      (tap&&ui_prank_entry_touch(touch.px,touch.py))){
    view=8;toy_tool=ROUGHHOUSE_SLAP;
    message="SELECT A TOY, THEN TAP THE DOLL.";dirty=1;
   }else{
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
    }else if(down&KEY_L){view=6;album_page=0;dirty=1;}
    else if(down&KEY_R){
     baby.safe_visuals^=1;
     message=baby.safe_visuals?"QUIET GRAPHICS, SAME GAMEPLAY.":"VISUAL GLITCHES RESTORED.";
     persist();dirty=1;
    }
   }
  }else if(view==1){
   if(down&KEY_UP){selected=(selected&~3)|((selected+2)&3);dirty=1;}
   else if(down&KEY_DOWN){selected=(selected&~3)|((selected+2)&3);dirty=1;}
   else if(down&KEY_LEFT){selected=(selected+15)%16;dirty=1;}
   else if(down&KEY_RIGHT){selected=(selected+1)%16;dirty=1;}
   else if(down&KEY_L){selected=(selected+12)%16;dirty=1;}
   else if(down&KEY_R){selected=(selected+4)%16;dirty=1;}
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
     int page=ui_arcade_page_touch(touch.px,touch.py);
     if(page){
      selected=(selected+(page<0?12:4))%16;dirty=1;
     }else{
      int id=ui_arcade_select_page(touch.px,touch.py,selected);
      if(id>=0){if(id==selected)begin_game();else selected=id;dirty=1;}
     }
    }
   }
  }else if(view==2){
   int direction=-1;
   if(down&KEY_B){leave_game();dirty=1;}
   else{
    if(arc.id==0||arc.id==9){
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
     if(arc.id==1){
      int pen_down=vpen.enabled?((held&KEY_A)!=0):raw_touch;
      int pen_x=vpen.enabled?vpen.x:touch.px;
      int pen_y=vpen.enabled?vpen.y:touch.py;
      if(game_arcade_diaper_touch(&baby,&arc,pen_down,pen_x,pen_y))
       dirty=1;
      /* D-pad fallback remains available if the stylus is unavailable. */
      if(!vpen.enabled){
       if(down&KEY_UP)direction=0;
       else if(down&KEY_RIGHT)direction=1;
       else if(down&KEY_DOWN)direction=2;
       else if(down&KEY_LEFT)direction=3;
      }
     }else if(arc.id==2){
      int pen_down=vpen.enabled?((held&KEY_A)!=0):raw_touch;
      int pen_x=vpen.enabled?vpen.x:touch.px;
      int pen_y=vpen.enabled?vpen.y:touch.py;
      if(game_arcade_teether_touch(&baby,&arc,pen_down,pen_x,pen_y))
       dirty=1;
      /* Accessible fallback for controllers without native stylus. */
      if(!vpen.enabled){
       if(down&KEY_LEFT)direction=3;
       else if(down&KEY_RIGHT)direction=1;
      }
     }else if(arc.id==3||arc.id==5){
      if(down&KEY_LEFT)direction=0;
      else if(down&KEY_DOWN)direction=1;
      else if(down&KEY_RIGHT)direction=2;
      else if(tap)direction=arc.id==3?
        ui_kart_touch(touch.px,touch.py):
        ui_sock_touch(touch.px,touch.py);
     }else if(arc.id==6){
      if((down&(KEY_A|KEY_UP))||tap)
       direction=(tap&&ui_nap_touch(touch.px,touch.py)<0)?-1:0;
     }else if(arc.id==4){
      if(down&KEY_UP)direction=0;
      else if(down&KEY_RIGHT)direction=1;
      else if(down&KEY_DOWN)direction=2;
      else if(down&KEY_LEFT)direction=3;
      else if(tap)direction=ui_beatbox_touch(touch.px,touch.py);
     }else if(arc.id==8){
      if(down&KEY_UP)direction=0;
      else if(down&KEY_DOWN)direction=2;
      else if(tap)direction=ui_crawl_touch(touch.px,touch.py);
     }else if(arc.id==10){
      if(down&KEY_LEFT)direction=0;
      else if(down&KEY_DOWN)direction=1;
      else if(down&KEY_RIGHT)direction=2;
      else if(tap)direction=ui_cereal_touch(touch.px,touch.py);
     }else if(arc.id==11){
      if(down&KEY_A)direction=0;
      else if(tap)direction=ui_blocks_touch(touch.px,touch.py);
     }else if(arc.id==12){
      if(down&KEY_LEFT)direction=0;
      else if(down&KEY_RIGHT)direction=1;
      else if(tap)direction=ui_audit_touch(touch.px,touch.py);
     }else if(arc.id==13){
      int pen_down=vpen.enabled?((held&KEY_A)!=0):raw_touch;
      int pen_x=vpen.enabled?vpen.x:touch.px;
      int pen_y=vpen.enabled?vpen.y:touch.py;
      if(game_arcade_garden_touch(&baby,&arc,pen_down,pen_x,pen_y))
       dirty=1;
      if(!vpen.enabled){
       if(down&KEY_LEFT)direction=0;
       else if(down&KEY_DOWN)direction=1;
       else if(down&KEY_RIGHT)direction=2;
      }
     }else if(arc.id==14){
      int pen_down=vpen.enabled?((held&KEY_A)!=0):raw_touch;
      int pen_x=vpen.enabled?vpen.x:touch.px;
      int pen_y=vpen.enabled?vpen.y:touch.py;
      if(game_arcade_court_touch(&baby,&arc,pen_down,pen_x,pen_y))
       dirty=1;
      if(!vpen.enabled){
       if(down&KEY_LEFT)direction=0;
       else if(down&KEY_UP)direction=1;
       else if(down&KEY_RIGHT)direction=2;
      }
     }else if(arc.id==15){
      if(down&KEY_LEFT)direction=0;
      else if(down&KEY_DOWN)direction=1;
      else if(down&KEY_RIGHT)direction=2;
      else if(tap)direction=ui_ceo_touch(touch.px,touch.py);
     }else if(arc.id==7){
      if(down&KEY_LEFT)direction=0;
      else if(down&KEY_DOWN)direction=1;
      else if(down&KEY_RIGHT)direction=2;
      else if(tap)direction=ui_rattle_touch(touch.px,touch.py);
     }else{
      if(down&KEY_UP)direction=0;
      else if(down&KEY_RIGHT)direction=1;
      else if(down&KEY_DOWN)direction=2;
      else if(down&KEY_LEFT)direction=3;
      else if(tap)direction=ui_arcade_touch(touch.px,touch.py);
     }
     if(tap&&touch.py>=176){leave_game();dirty=1;}
     else{
      if(direction>=0&&arc.active){
       game_arcade_input(&baby,&arc,direction);dirty=1;
      }
      if(arc.active)game_arcade_tick(&baby,&arc);
     }
    }
    if(arc.finished){leave_game();dirty=1;}
   }
  }else if(view==8){
   int hit=tap?ui_roughhouse_touch(touch.px,touch.py):0;
   if((down&(KEY_B|KEY_START))||hit==3){
    sfx_stop();
    view=0;message="BACK TO THE NURSERY.";dirty=1;
   }else if(down&KEY_Y){
    sfx_set_enabled(!sfx_is_enabled());
    message=sfx_is_enabled()?"TOYBOX AUDIO ON.":"TOYBOX AUDIO MUTED.";
    dirty=1;
   }else if(hit==ROUGHHOUSE_SLAP||(down&KEY_L)){
    toy_tool=ROUGHHOUSE_SLAP;dirty=1;
   }else if(hit==ROUGHHOUSE_KICK||(down&(KEY_R|KEY_X))){
    toy_tool=ROUGHHOUSE_KICK;dirty=1;
   }else if((down&KEY_A)||hit==4){
    /* Lower LCD contacts move the synthetic toy with different impulses;
     * physical A retains a neutral, accessible centered strike. */
    int sx=hit==4?touch.px:128;
    int sy=hit==4?touch.py:110;
    if(roughhouse_strike_at(&rag,toy_tool,sx,sy)){
     sfx_play(toy_tool==ROUGHHOUSE_KICK?SFX_KICK:SFX_SLAP);
     pose=0;
     message=toy_tool==ROUGHHOUSE_KICK?
             "BOING! THE SYNTHETIC SHELL BOUNCES.":
             "BONK! THAT WAS AN UNNATURAL SOUND.";
     dirty=1;
     if(roughhouse_cracked(&rag)){
      sfx_play(SFX_CRACK);
      (void)game_secret_egg(&baby);
      persist();
      view=9;egg_frame=0;last_ending=END_EGG;
      message="INCUBATION FILE 09 OPENED.";dirty=1;
     }
    }
   }
  }else if(view==9){
   if(down&(KEY_B|KEY_START)){
    sfx_stop();
    view=4;last_ending=END_EGG;dirty=1;
   }else{
    egg_frame++;
    if(egg_frame==EGG_CRACK_FRAME)sfx_play(SFX_UNSEAL);
    else if(egg_frame==EGG_SCARE_FRAME)sfx_play(SFX_SCARE);
    else if(egg_frame==EGG_PUSH_FRAME||
            (egg_frame>EGG_PUSH_FRAME&&
             egg_frame<EGG_SCARE_FRAME-24u&&
             (egg_frame-EGG_PUSH_FRAME)%38u==0u))
     sfx_play(SFX_PULSE);
    if(egg_frame==EGG_CRACK_FRAME||egg_frame==EGG_PUSH_FRAME||
       egg_frame==EGG_SCARE_FRAME||egg_frame==EGG_HOLD_FRAME)dirty=1;
    if(egg_frame>=EGG_TOTAL_FRAMES){
     sfx_stop();
     view=4;last_ending=END_EGG;dirty=1;
    }
   }
  }else if(view==3){
   if(down&KEY_B){view=0;reset_step=0;message="RESET CANCELED.";dirty=1;}
   else if(!reset_step){if(down&KEY_A){reset_step=1;dirty=1;}}
   else if((down&KEY_X)&&(held&(KEY_L|KEY_R))==(KEY_L|KEY_R)){
    if(storage_ok&&!save_erase(SAVE_BASE)){message="SAVE ERASE FAILED. NO RESET.";}
    else{game_init(&baby);roughhouse_init(&rag);message="COLD BOOT. WHO AM I?";}
    view=0;reset_step=0;dirty=1;
   }
  }else if(view==4){
   if((down&(KEY_A|KEY_B|KEY_START))||tap){
    /* Don't strand the toy selectors after secret #9. Ragdoll state is
     * deliberately transient; unlocked endings stay in persistent Game. */
    if(last_ending==END_EGG)roughhouse_init(&rag);
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
    /* A visible bottom title is already drawn. Initialize top 3D only once. */
    if(!three_d_ready){
     videoSetMode(MODE_0_3D);world_init();three_d_ready=1;
    }
    view=0;
    message=storage_ok?"WELCOME BACK, SHAREHOLDER.":"RAM MODE: X ON TITLE ENABLES SAVES.";
    dirty=1;
   }
  }else if(view==6){
   if(down&(KEY_B|KEY_START)){
    view=0;message="COLLECTION CLOSED.";dirty=1;
   }else if((down&KEY_A)&&album_page==2&&
            (baby.ending_mask&(1u<<END_EGG))){
    view=9;egg_frame=0;last_ending=END_EGG;dirty=1;
   }else if(down&KEY_A){
    view=0;message="COLLECTION CLOSED.";dirty=1;
   }else if(down&(KEY_LEFT|KEY_L)){
    if(album_page>0)album_page--;
    dirty=1;
   }else if(down&(KEY_RIGHT|KEY_R)){
    if(album_page<2)album_page++;
    dirty=1;
   }else if(tap){
    int page=ui_album_page_touch(touch.px,touch.py);
    if(page<0){if(album_page>0)album_page--;dirty=1;}
    else if(page>0){if(album_page<2)album_page++;dirty=1;}
    else if(album_page==2&&touch.py>=67&&touch.py<169&&
            (baby.ending_mask&(1u<<END_EGG))){
     view=9;egg_frame=0;last_ending=END_EGG;dirty=1;
    }else if(touch.py>=176){
     view=0;message="COLLECTION CLOSED.";dirty=1;
    }
   }
  }else if(view==7){
   if(down&(KEY_B|KEY_SELECT|KEY_START)){view=5;dirty=1;}
  }
  /* Delay the lightweight top portrait until AFTER the bottom title has
   * been visible for 16 frames. It keeps boot safe on older DS emulators. */
  if(!three_d_ready&&frame>=16u){
   videoSetMode(MODE_0_3D);world_init();three_d_ready=1;
  }
  if(view==8||(view==0&&rag.moving)){
   unsigned old_rebounds=rag.rebound_count;
   roughhouse_step(&rag);
   if(view==8&&rag.rebound_count!=old_rebounds)
    sfx_play(SFX_BOUNCE);
  }
  sfx_tick();
  if(three_d_ready){
   if(view==5||view==7)world_draw_title(frame);
   else if(view==9)world_draw_secret(&baby,egg_frame,&rag);
   else if(view==8||(view==0&&rag.moving))
    world_draw_roughhouse(&baby,frame,&rag);
   else world_draw(&baby,frame,pose);
  }
  /* Keep bitmap VRAM writes off the hot path unless UI state changed. */
  if(dirty||previous!=view||
     (view==2&&frame%arcade_redraw_period(&arc)==0u)){
   ui_draw(&baby,&arc,view,view==6?album_page:view==8?((int)rag.charge|(toy_tool==ROUGHHOUSE_KICK?32:0)|((int)rag.hits<<6)|
           (sfx_is_enabled()?(1<<16):0)):view==9?(int)egg_frame:selected,last_ending,reset_step,storage_ok,message);
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

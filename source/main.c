/* Original DS ARM9 prototype: hardware 3D top, lower console and touch care. */
#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include "game.h"
#include "save.h"
static Game baby;
static Arcade arc;
static int can_save,view,sel,reset_step,last_ending;
static const char *message="WELCOME TO DAYCARE 2.0";
#define SAVE_BASE "fat:/my_baby_ai"
static void oval(float x,float y,float w,float h,float z,int r,int g,int b){
 static const short cs[12]={256,222,128,0,-128,-222,-256,-222,-128,0,128,222};
 static const short sn[12]={0,128,222,256,222,128,0,-128,-222,-256,-222,-128};
 int i;glColor(RGB15(r,g,b));glBegin(GL_TRIANGLES);
 for(i=0;i<12;i++){int j=(i+1)%12;glVertex3f(x,y,z);
 glVertex3f(x+w*cs[i]/256.0f,y+h*sn[i]/256.0f,z);
 glVertex3f(x+w*cs[j]/256.0f,y+h*sn[j]/256.0f,z);}
 glEnd();
}
static void quad(float a,float b,float c,float d,float z,int r,int g,int bl){
 glColor(RGB15(r,g,bl));glBegin(GL_QUADS);
 glVertex3f(a,b,z);glVertex3f(c,b,z);glVertex3f(c,d,z);glVertex3f(a,d,z);glEnd();
}
static void scene(unsigned frame){
 int tier=(int)game_glitch_tier(&baby),blink=(frame%143u)<5,shard=(!baby.safe_visuals&&tier>0&&(frame%97u)<7);
 float sway=((int)(frame/18u%7u)-3)*0.012f;
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();gluLookAt(0,0,4.8,0,0,0,0,1,0);
 glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
 quad(-3,-2,3,2,-1.3,31,28,23);
 quad(-3,-2,3,-1.3,-1.1,24,19,13);
 quad(-2.8,.3,-1.75,1.5,-1.08,26,17,22);
 quad(-2.65,.43,-1.85,1.37,-1.07,31,27,12);
 oval(-2.24,.9,.23,.25,-1.05,31,16,22);
 oval(-2.24,.9,.12,.1,-1.04,31,29,9);
 quad(1.52,-.9,2.85,.5,-1.09,31,27,24);
 quad(1.6,-.65,2.72,-.55,-1.06,21,16,10);
 quad(1.6,-.2,2.72,-.1,-1.06,21,16,10);
 quad(1.6,.28,2.72,.36,-1.06,21,16,10);
 oval(-.1+sway,-.83,.9,.64,.24,30,13,20);
 oval(-.75+sway,-.72,.37,.27,.44,27,16,12);
 oval(.65+sway,-.72,.38,.27,.44,27,16,12);
 oval(-.15+sway,.32,1.04,.99,.52,5,3,4);
 oval(-.08+sway,.19,.92,.88,.56,28,17,12);
 oval(-.18+sway,.94,.83,.30,.59,6,3,4);
 oval(-.81+sway,.72,.17,.23,.69,31,8,18);
 oval(-.40+sway,.29,.23,blink?.027f:.24f,.73,31,29,29);
 oval(.29+sway,.27,.22,blink?.027f:.22f,.73,31,29,29);
 if(!blink){oval(-.36+sway,.28,.112,.15,.76,3,2,3);
 oval(.30+sway+(shard?.16f:0),.29,.125,.155,.76,3,2,3);
 oval(-.4+sway,.35,.037,.041,.79,31,31,31);
 oval(.26+sway,.35,.038,.042,.79,31,31,31);}
 oval(-.02+sway,-.18,.23,.13,.74,13,3,6);
 oval(-.02+sway,-.22,.12,.034,.76,31,18,21);
 oval(.27,-1.24,.4,.43,.89,25,12,4);
 oval(.27,-.92,.3,.3,.9,25,13,5);
 oval(.04,-.71,.14,.13,.90,25,13,5);
 oval(.49,-.71,.14,.13,.90,25,13,5);
 oval(.27,-.98,.17,.12,.92,31,25,17);
 oval(.25,-.94,.05,.05,.94,3,2,2);
 if(shard){quad(-.71,.01,.61,.075,.8,4,31,21);quad(.57,-.1,.76,.55,.78,31,2,19);}
 glFlush(0);
}
static void persist(void){if(can_save)(void)save_write(SAVE_BASE,&baby);}
static void hud(void){
 consoleClear();
 iprintf("MY BABY A.I.! DEMO\nDay %lu   $%u  Favor %u\n",(unsigned long)baby.days,baby.dollars,baby.favors);
 iprintf("Hunger %u Mood %u Energy %u\n",baby.hunger,baby.mood,baby.energy);
 iprintf("Chaos %u  GLITCH %u / T%u\n",baby.chaos,baby.corruption,game_glitch_tier(&baby));
 iprintf("Bad %u  Good %d / 4\n",baby.bad_endings,game_good_ending_count(&baby));
 iprintf("SAVE: %s\n\n",can_save?"FAT":"RAM ONLY");
 if(view==0)iprintf("A Feed  B Chat\nX Play  Y Nap\nSTART Arcade SELECT End day\nTouch: quadrants care\nL+R: optional reset ending\n");
 if(view==1)iprintf("ARCADE 16 CABINETS\n%02d %s\n%s\nUP/DOWN choose\nA Play X Unlock B Back\nWins %lu\n",sel+1,game_names[sel],game_unlocked(&baby,sel)?"UNLOCKED":"LOCKED",(unsigned long)baby.wins);
 if(view==2)iprintf("ARCADE: %s\nROUND %u/6 MISS %u/3\nD-PAD: %s\n%s\nTIMER %u frames\nB exit\n",game_names[arc.id],arc.round+1,arc.mistakes,((const char*[]){"UP","RIGHT","DOWN","LEFT"})[arc.direction],arc.reverse?"GLITCH: REVERSE!":"normal",arc.timer);
 if(view==3)iprintf("SECRET ENDING: RESET\nERASES ONLY THIS GAME SAVE!\nNO OTHER FILES.\n\n%s\nB cancel\n",reset_step?"Hold L+R and press X":"A to continue");
 if(view==4)iprintf("%s ENDING!\n\n%s\n\n%s\nA or B: CONTINUE PLAYING\nProgress is saved.\n",game_is_good_ending(last_ending)?"GOOD":"BAD",game_ending_name(last_ending),game_is_good_ending(last_ending)?"+8 DOLLARS, +1 FAVOR":"Baby survived. Try again!");
 iprintf("\n-------------------------\n%s\n",message);
}
static void care(Care c){message=game_care(&baby,c);persist();}
int main(void){
 unsigned frame=0;game_init(&baby);
 lcdMainOnTop();consoleDemoInit();can_save=fatInitDefault();
 if(can_save)(void)save_load(SAVE_BASE,&baby);
 videoSetMode(MODE_0_3D);glInit();glEnable(GL_ANTIALIAS);
 glClearColor(31,28,27,31);glClearPolyID(63);glClearDepth(0x7fff);glViewport(0,0,255,191);
 glMatrixMode(GL_PROJECTION);glLoadIdentity();gluPerspective(57,256.0/192.0,0.1,20);
 hud();
 while(pmMainLoop()){
  int down,held,prev=view;touchPosition touch;
  swiWaitForVBlank();scanKeys();touchRead(&touch);
  down=keysDown();held=keysHeld();
  if(view==0){
   if(down&KEY_A)care(FEED);
   else if(down&KEY_B)care(CHAT);
   else if(down&KEY_X)care(PLAY);
   else if(down&KEY_Y)care(NAP);
   else if(down&KEY_TOUCH)care(touch.py<96?(touch.px<128?FEED:CHAT):(touch.px<128?PLAY:NAP));
   else if(down&KEY_START){view=1;message="WELCOME TO THE ARCADE.";}
   else if(down&KEY_SELECT){int e=game_end_day(&baby);last_ending=e;message=game_ending_name(e);if(e!=END_OK)view=4;persist();}
   else if((held&(KEY_L|KEY_R))==(KEY_L|KEY_R)&&game_reset_unlocked(&baby)){
    view=3;reset_step=0;message="NO GLITCHES ON THIS SCREEN.";
   }
  }else if(view==1){
   if(down&KEY_UP)sel=(sel+15)%16;
   else if(down&KEY_DOWN)sel=(sel+1)%16;
   else if(down&KEY_B){view=0;message="WELCOME BACK, SHAREHOLDER.";}
   else if(down&KEY_X){if(game_unlock(&baby,sel)){message="CABINET BOUGHT!";persist();}else message="NOT ENOUGH WINS OR MONEY.";}
   else if(down&KEY_A){game_arcade_start(&baby,&arc,sel);if(arc.active){view=2;message="SIX ROUNDS. THREE STRIKES.";}else message="LOCKED CABINET.";}
  }else if(view==2){
   int dir=-1;
   if(down&KEY_B){view=1;arc.active=0;message="TAKING A NAP FROM ARCADE.";}
   else {
    if(down&KEY_UP)dir=0;else if(down&KEY_RIGHT)dir=1;
    else if(down&KEY_DOWN)dir=2;else if(down&KEY_LEFT)dir=3;
    if(dir>=0)game_arcade_input(&baby,&arc,dir);
    if(!arc.finished)game_arcade_tick(&baby,&arc);
    if(arc.finished){view=1;message=arc.finished==1?"DIAPER DOLLARS EARNED!":"THE GLITCHES WON.";persist();}
   }
  }else if(view==3){
   if(down&KEY_B){view=0;reset_step=0;message="RESET CANCELED.";}
   else if(!reset_step){if(down&KEY_A)reset_step=1;}
   else if((down&KEY_X)&&(held&(KEY_L|KEY_R))==(KEY_L|KEY_R)){
    if(can_save&&!save_erase(SAVE_BASE)){message="SAVE ERASE FAILED. NO RESET.";}
    else{game_init(&baby);message="COLD BOOT. WHO AM I?";}
    view=0;reset_step=0;
   }
  }else if(view==4){
   if(down&(KEY_A|KEY_B|KEY_START)){view=0;message="BACK TO DAYCARE. SAME SAVE!";}
  }
  scene(frame);
  if(down||prev!=view||(view==2&&frame%10==0))hud();
  frame++;
 }
 return 0;
}

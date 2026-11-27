/* My Baby A.I.! Nintendo DS dual-engine prototype:
 * 3D nursery on main engine, direct-VRAM bitmap touchscreen on sub engine.
 */
#include <nds.h>
#include <fat.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "arcade_feedback.h"
#include "dialogue.h"
#include "conversation.h"
#include "conversation_save.h"
#include "observer.h"
#include "observer_save.h"
#include "voice.h"
#include "voice_save.h"
#include "mic_input.h"
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
static ArcadeFeedback arcade_feedback;
static uint8_t save_write_failed;
static uint8_t learning_write_failed;
static uint8_t observer_write_failed;
static uint8_t voice_write_failed;
static int storage_ok,view=5,selected,album_page,reset_step,last_ending,pose;
static int talk_selected;
static uint32_t pose_age;
/* Small response state; learned weights persist separately from Game v1.
 * No additional rendering buffer is needed. */
static DialogueMemory memories;
static ConversationProfile learning;
static ConversationSession conversation;
static ObserverProfile shelter;
static ObserverSession observer;
static VoiceProfile voice_profile;
static VoiceCapture voice_capture;
static char voice_word[33]="HELLO",voice_status[96]="TYPE A WORD IN WARDEN, THEN OPEN MIC.";
static unsigned voice_mode; /* 0 idle, 1 train, 2 listen */
static uint32_t voice_started;
static int voice_match=-1,voice_slot=-1;
static uint8_t voice_forget_armed,voice_profile_present;
/* No full-time mic use, training PCM files or changes to v1 Game. */
static char terminal_input[33];
static unsigned terminal_speaker,terminal_key;
static int terminal_sent;
static WorldBabyMotion care_motion;
/* Transient room-tour state: save schema stays exactly the same. */
static int room_focus=-1,room_previous=-1,room_daylight,room_lamp=1;
static int room_attention_anchor;
static uint32_t room_camera_age=16u,room_prop_age=40u;
/* Capture the EXACT in-flight viewpoint before switching focus. Without
 * this, quick prop taps restart from the old preset and jerk the camera. */
static EggCamera room_camera_anchor={.97f,1.05f,3.43f,-.12f,.035f,.17f};
/* Capture both connected animations together before redirecting them.
 * The next head turn starts where the current turn is, just as the
 * camera does; neither jumps back to its previous preset. */
static void room_retarget(int focus){
 room_camera_anchor=world_nursery_camera_from(room_camera_anchor,
                                             room_focus,room_camera_age);
 room_attention_anchor=world_baby_attention_from(room_attention_anchor,
                                                room_focus,room_camera_age);
 room_previous=room_focus;room_focus=focus;room_camera_age=0;
}
static int three_d_ready;
static TouchGate gesture;
static VirtualStylus vpen;
static Roughhouse rag;
static uint32_t egg_frame;
static int egg_paused;
static int toy_tool=ROUGHHOUSE_SLAP;
static const char *message="THE LAST NURSERY. WARDEN IS ONLINE.";
static char room_feedback[64];
static void reset_care_motion(void){
 pose=0;pose_age=0;
 care_motion=(WorldBabyMotion){0};
}
static void reset_care_session(void){
 mic_input_stop();voice_mode=0;voice_match=-1;voice_slot=-1;
 voice_forget_armed=0;voice_profile_present=0;
 voice_profile_reset(&voice_profile);voice_capture_reset(&voice_capture);
 strcpy(voice_word,"HELLO");
 strcpy(voice_status,"TYPE A WORD IN WARDEN, THEN OPEN MIC.");
 dialogue_memory_reset(&memories,baby.days);
 conversation_profile_reset(&learning);
 conversation_session_reset(&conversation,baby.days,baby.rng);
 observer_profile_reset(&shelter,&baby);
 observer_session_reset(&observer,&baby);
 terminal_input[0]='\0';terminal_key=0;terminal_speaker=OBSERVER_MONITOR;
 terminal_sent=0;
 arcade_feedback_reset(&arcade_feedback);
 save_write_failed=0;learning_write_failed=0;observer_write_failed=0;
 voice_write_failed=0;talk_selected=0;
 reset_care_motion();
}
/* Failed writes must NEVER be silent: the game remains fully playable
 * in RAM, but a real UI warning tells the player their latest care/day
 * is not yet on disk. Each later action retries the same two-slot save. */
static int persist(void){
 if(storage_ok){
  if(!save_write(SAVE_BASE,&baby)){
   save_write_failed=1;
   message="SAVE WRITE FAILED. PROGRESS IN RAM!";
   return 0;
  }
  save_write_failed=0;
  /* Learning is optional and bound to this exact, successfully saved
   * v1 Game. A companion failure must never invalidate care progress. */
  learning_write_failed=(uint8_t)!conversation_save_write(SAVE_BASE,&baby,&learning);
  observer_write_failed=(uint8_t)!observer_save_write(SAVE_BASE,&baby,&shelter);
  /* Once a voice profile exists, an explicitly empty profile is real
   * state too. Persist forgetting the final lesson, rather than silently
   * skipping that write. Untouched games still avoid the extra record. */
  voice_write_failed=(uint8_t)((voice_profile_present||voice_profile.used!=0u)&&
      !voice_save_write(SAVE_BASE,&baby,&voice_profile));
  if(learning_write_failed)message="LEARNING NOT SAVED. KEPT IN RAM!";
  else if(observer_write_failed)message="SHELTER MEMORY NOT SAVED. IN RAM!";
  else if(voice_write_failed)message="VOICE LESSONS NOT SAVED. IN RAM!";
  if(learning_write_failed||observer_write_failed||voice_write_failed)return 0;
 }
 return 1;
}
/* Save the CURRENT in-progress baby on an explicit Room Tour request.
 * Unlike title SAVE/LOAD, never load an older file over unsaved progress.
 * Boot remains in RAM mode until the player intentionally opts in. */
static void save_live_baby(void){
 if(!storage_ok){
  message="CHECKING SAVE STORAGE...";
  ui_draw(&baby,&arc,view,selected,last_ending,reset_step,0,message);
  storage_ok=fatInitDefault();
 }
 if(storage_ok){
  if(persist())message="CURRENT BABY AND LEARNING SAVED.";
 }
 else message="NO SAVE DEVICE - RAM ONLY.";
}
/* Apply a care event before saving so compound interactions can commit
 * their complete state together, rather than writing intermediate saves. */
static void care_apply(Care action,uint32_t frame){
 Game before=baby;
 /* Capture the current articulated pose BEFORE care changes tiredness
  * and need-driven fidget. A repeated tap can interrupt another blend. */
 WorldBabyPose from=world_baby_pose(&baby,frame,pose,pose_age,
                                   pose?&care_motion:0);
 message=game_care(&baby,action);
 message=dialogue_contextual_reply(&memories,&baby,action,message);
 conversation_record_care(&conversation,&before,&baby,action);
 (void)observer_record_care(&shelter,&observer,&before,&baby,action);
 if(action!=CHAT){
  const ConversationReply *reply=conversation_care_response(&learning,
                                      &conversation,&baby,action);
  if(reply)message=reply->text;
 }
 if(observer.echo_ready)message=observer.reply;
 pose=(int)action+1;
 pose_age=0; /* restart this care gesture: do not loop it forever */
 world_baby_motion_start(&care_motion,from,&baby,frame,pose);
 /* Short original PSG phrases now match the on-screen head/jaw/arm
  * reactions. The single-channel audio engine handles mute and priority,
  * so neither the egg scare nor old Toybox cues can be affected. */
 switch(action){
 case FEED:sfx_play(SFX_FEED);break;
 case CHAT:sfx_play(SFX_CHAT);break;
 case PLAY:sfx_play(SFX_PLAY);break;
 case NAP:sfx_play(SFX_NAP);break;
 default:break;
 }
}
static int care(Care action,uint32_t frame){
 care_apply(action,frame);
 return persist();
}
/* Questions and feedback are conversation, not another care action:
 * no hidden hunger/energy/corruption cost and no shared RNG advance. */
static void ask_baby(ConversationPrompt prompt,uint32_t frame){
 ConversationProfile before=learning;
 const ConversationReply *reply;
 WorldBabyPose from=world_baby_pose(&baby,frame,pose,pose_age,
                                   pose?&care_motion:0);
 reply=conversation_respond(&learning,&conversation,&baby,prompt);
 if(!reply)return;
 message=reply->text;
 pose=CHAT+1;pose_age=0;
 world_baby_motion_start(&care_motion,from,&baby,frame,pose);
 sfx_play(SFX_CHAT);
 /* Only changed learned preferences require disk writes. Merely reading
  * or following a topic never increments a save sequence or touches FAT. */
 if(memcmp(&before,&learning,sizeof(learning))!=0)(void)persist();
}
static void open_conversation(uint32_t frame){
 view=11;talk_selected=CONVERSATION_HOW;
 ask_baby(CONVERSATION_HOW,frame);
}
/* The terminal has bounded local text entry. Neither a question nor a
 * generated word is a care command, a filesystem command or an attack. */
static void open_observer(void){
 view=12;terminal_speaker=OBSERVER_MONITOR;terminal_key=0;
 terminal_input[0]='\0';terminal_sent=0;
 (void)observer_report(&shelter,&observer,&baby);
}
static void terminal_type(char key){
 size_t length;
 if(!key)return;
 if(terminal_sent){terminal_input[0]='\0';terminal_sent=0;}
 length=strlen(terminal_input);
 if(key=='<'){
  if(length)terminal_input[length-1u]='\0';
 }else if(length<sizeof(terminal_input)-1u){
  terminal_input[length]=key;terminal_input[length+1u]='\0';
 }
}
static void animate_baby_reply(uint32_t frame){
 WorldBabyPose from=world_baby_pose(&baby,frame,pose,pose_age,
                                    pose?&care_motion:0);
 pose=CHAT+1;pose_age=0;
 world_baby_motion_start(&care_motion,from,&baby,frame,pose);
 sfx_play(SFX_CHAT);
}
static void terminal_send(uint32_t frame){
 ObserverProfile before=shelter;
 (void)observer_answer(&shelter,&observer,&baby,terminal_speaker,terminal_input);
 terminal_sent=1;
 if(terminal_speaker==OBSERVER_BABY)animate_baby_reply(frame);
 if(memcmp(&before,&shelter,sizeof(shelter))!=0)(void)persist();
}
/* Speak-to-command is opt-in. A microphone match is a suggestion only;
 * pressing USE forwards it to the already guarded typed terminal. */
static void open_microphone(void){
 size_t n=strlen(terminal_input);
 mic_input_stop();voice_mode=0;voice_match=-1;voice_forget_armed=0;
 if(!n)strcpy(voice_word,"HELLO");
 else memcpy(voice_word,terminal_input,n+1u);
 voice_slot=voice_find(&voice_profile,voice_word);
 if(voice_slot<0)voice_slot=voice_cycle(&voice_profile,-1,1);
 voice_capture_reset(&voice_capture);
 strcpy(voice_status,"A TRAIN. X LISTEN. L/R BROWSE. DEL TWICE.");
 view=13;
}
/* Mic lesson browser is read-only: moving a selection never touches the
 * game RNG, baby's needs, learner weights or the currently typed message. */
static void open_nursery_microphone(void){
 open_observer();
 terminal_speaker=OBSERVER_BABY;
 strcpy(observer.reply,"I AM LISTENING.\nYOU CAN TEACH ME A SHORT PHRASE.");
 open_microphone();
}
static void browse_microphone(int direction){
 int next;
 if(voice_mode)return;
 voice_forget_armed=0;voice_match=-1;
 next=voice_cycle(&voice_profile,voice_slot,direction);
 if(next<0){
  strcpy(voice_status,"NO SAVED WORDS. TYPE A LABEL TO TRAIN.");
  return;
 }
 voice_slot=next;
 strcpy(voice_word,voice_label(&voice_profile,(unsigned)next));
 strcpy(voice_status,"SAVED WORD SELECTED. TRAIN OR LISTEN.");
}
static void forget_microphone(void){
 unsigned erased;
 int next;
 if(voice_mode)return;
 if(voice_slot<0||!voice_label(&voice_profile,(unsigned)voice_slot)[0]){
  voice_forget_armed=0;
  strcpy(voice_status,"NO SAVED WORDS TO FORGET.");
  return;
 }
 if(!voice_forget_armed){
  voice_forget_armed=1;
  strcpy(voice_status,"PRESS DEL AGAIN TO FORGET THIS WORD.");
  return;
 }
 erased=(unsigned)voice_slot;
 voice_forget_armed=0;
 if(!voice_forget(&voice_profile,erased)){
  strcpy(voice_status,"COULD NOT FORGET THE SELECTED WORD.");
  return;
 }
 voice_match=-1;voice_profile_present=1;
 next=voice_cycle(&voice_profile,(int)erased-1,1);
 voice_slot=next;
 if(next>=0)strcpy(voice_word,voice_label(&voice_profile,(unsigned)next));
 else strcpy(voice_word,"HELLO");
 strcpy(voice_status,"LESSON FORGOTTEN. MEMORY SPACE FREED.");
 if(storage_ok)(void)persist();
}
static void start_microphone(unsigned mode,uint32_t frame){
 if(voice_mode){
  /* A second TRAIN/LISTEN press cancels rather than immediately
   * restarting the ARM7 buffer callback while a sample is in flight. */
  mic_input_stop();voice_mode=0;voice_match=-1;
  voice_forget_armed=0;voice_capture_reset(&voice_capture);
  strcpy(voice_status,"CANCELLED. A TRAIN OR X LISTEN AGAIN.");
  return;
 }
 voice_match=-1;voice_forget_armed=0;
 if(mode==1u&&(!voice_word[0]||strlen(voice_word)>VOICE_TEXT)){
  strcpy(voice_status,"TYPE A LABEL OF 1-24 CHARACTERS IN WARDEN.");return;
 }
 if(mode==1u&&voice_profile.used==VOICE_SLOTS&&
    voice_find(&voice_profile,voice_word)<0){
  strcpy(voice_status,"24 WORDS FULL. SELECT ONE AND DEL TWICE.");
  return;
 }
 /* Don't classify the game's own PSG chimes as the player's voice. */
 sfx_stop();
 if(!mic_input_start(&voice_capture)){
  strcpy(voice_status,"MIC UNAVAILABLE. B BACK, USE KEYBOARD.");return;
 }
 voice_mode=mode;voice_started=frame;
 strcpy(voice_status,"WAIT FOR QUIET CALIBRATION, THEN SPEAK.");
}
static int microphone_tick(uint32_t frame){
 unsigned score;int result;
 VoiceMatchStatus reason;
 if(!voice_mode)return 0;
 if(!mic_input_is_active()){
  voice_mode=0;voice_match=-1;
  strcpy(voice_status,"MIC UNAVAILABLE. B BACK, USE KEYBOARD.");
  return 1;
 }
 /* Allow the 2.5-second onset wait and the larger 1.75-second feature
  * window before an independent 5-second wall-clock fallback. */
 if(voice_capture.state<VOICE_READY&&frame-voice_started<300u)return 0;
 mic_input_stop();
 if(voice_capture.state!=VOICE_READY){
  strcpy(voice_status,voice_capture.state==VOICE_TOO_SHORT?
         "TOO SHORT. TRY A LONGER WORD.":voice_capture.state==VOICE_TOO_LONG?
         "TOO LONG. TRY ONE SHORT PHRASE.":"NO CLEAR SPEECH. TRY AGAIN.");
 }else if(voice_mode==1u){
  result=voice_train(&voice_profile,voice_word,&voice_capture);
  if(result==1||result==2){
   voice_slot=voice_find(&voice_profile,voice_word);
   voice_profile_present=1;
   strcpy(voice_status,result==1?
    "FIRST EXAMPLE SAVED. TRAIN THE SAME WORD AGAIN.":
    "ANOTHER EXAMPLE SAVED. READY TO LISTEN.");
   if(storage_ok)(void)persist();
  }else if(result==3)
   strcpy(voice_status,"24 WORDS FULL. DEL TWICE TO FREE A SLOT.");
  else strcpy(voice_status,"LABEL INVALID OR CAPTURE TOO SHORT.");
 }else{
  voice_match=voice_recognize_detailed(&voice_profile,&voice_capture,&score,&reason);
  if(voice_match<0)strcpy(voice_status,
   reason==VOICE_MATCH_EMPTY?"NO LESSONS YET. TRAIN A WORD FIRST.":
   reason==VOICE_MATCH_AMBIGUOUS?"TWO WORDS SOUND ALIKE. RETRAIN OR TYPE.":
   "UNSURE. TRAIN MORE OR SPEAK MORE CLEARLY.");
  else snprintf(voice_status,sizeof(voice_status),"MATCH %s. Y USE TO SEND.",
                voice_label(&voice_profile,(unsigned)voice_match));
 }
 voice_mode=0;return 1;
}
static void terminal_teach(uint32_t frame){
 ObserverProfile before=shelter;
 char lesson[40];
 if(terminal_speaker!=OBSERVER_BABY){
  strcpy(observer.reply,"SELECT BABY WITH TARGET OR Y.\nTYPE A PHRASE, THEN TAP TEACH.");
  return;
 }
 memcpy(lesson,"TEACH ",6u);
 memcpy(lesson+6u,terminal_input,strlen(terminal_input)+1u);
 (void)observer_answer(&shelter,&observer,&baby,terminal_speaker,lesson);
 terminal_sent=1;
 if(memcmp(&before,&shelter,sizeof(shelter))!=0){
  /* A newly stored/reinforced phrase is a visible response, just like
   * a typed SEND, without spending care, advancing RNG or exposure. */
  animate_baby_reply(frame);
  (void)persist();
 }
}
static void observer_pose(int action,uint32_t frame){
 WorldBabyPose from=world_baby_pose(&baby,frame,pose,pose_age,
                                   pose?&care_motion:0);
 pose=action;pose_age=0;
 world_baby_motion_start(&care_motion,from,&baby,frame,pose);
}
/* Soothe is a real social care action with separate costs and results.
 * Use the existing CHAT memory counters and connected regroup pose;
 * a refusal neither consumes RNG nor saves, and creates no fake event. */
static void room_lullaby(uint32_t frame){
 Game before=baby;
 WorldBabyPose from=world_baby_pose(&baby,frame,pose,pose_age,
                                   pose?&care_motion:0);
 if(!game_room_soothe(&baby)){
  message=before.hunger>=70u?"FEED ME BEFORE A LULLABY.":
          before.energy<=15u?"I NEED A NAP FIRST.":
          "I'M ALREADY CALM AND HAPPY.";
  return;
 }
 conversation_record_care(&conversation,&before,&baby,CHAT);
 (void)observer_record_care(&shelter,&observer,&before,&baby,CHAT);
 pose=WORLD_BABY_REGROUP;pose_age=0;
 world_baby_motion_start(&care_motion,from,&baby,frame,pose);
 sfx_play(SFX_LULLABY);
 snprintf(room_feedback,sizeof(room_feedback),"LULLABY: +%u MOOD -%u CHAOS",
          (unsigned)baby.mood-(unsigned)before.mood,
          (unsigned)before.chaos-(unsigned)baby.chaos);
 message=room_feedback;
 (void)persist();
}
static void end_day(void){
 int e=game_end_day(&baby);
 (void)observer_record_event(&shelter,&observer,&baby,OBSERVER_EVENT_DAY,0u);
 if(memories.day!=baby.days)dialogue_memory_reset(&memories,baby.days);
 if(conversation.day!=baby.days)
  conversation_session_reset(&conversation,baby.days,baby.rng);
 last_ending=e;
 message=e==END_OK?"NEXT DAY: +8 HUNGER, -6 ENERGY.":
                         game_ending_name(e);
 if(e!=END_OK)view=4;
 persist();
}
static void begin_game(void){
 game_arcade_start(&baby,&arc,selected);
 arcade_feedback_begin(&arcade_feedback,&arc);
 if(arc.active){view=2;message=arc.id==0?"STEADY THE BOTTLE.":arc.id==1?"CHANGE THAT DIAPER!":arc.id==2?"SORT BABY'S TAX BILLS!":arc.id==3?"DODGE CRIB KART CONES!":arc.id==4?"MEMORIZE THE BEAT!":arc.id==5?"MATCH THE BABY SOCKS!":arc.id==6?"TIME THE QUIET LULLABY!":arc.id==7?"CATCH SIX RATTLES!":arc.id==8?"JUMP OR DUCK AT WORK!":arc.id==9?"DRIFT THE STROLLER!":arc.id==10?"SOLVE CEREAL MATH!":arc.id==11?"STACK THOSE BLOCKS!":arc.id==12?"CHECK THE AUDIT NUMBERS!":arc.id==13?"WATER THE RIGHT FLOWER!":arc.id==14?"TRACE THE EVIDENCE!":arc.id==15?"BALANCE THE DAYCARE!":"SIX ROUNDS. THREE STRIKES.";}
 else message="LOCKED: CHECK GOALS ABOVE.";
}
/* Scene-specific bitmap refresh. Interactive screens retain fluid redraws;
  * still screens repaint only when state changes or the clock ticks, avoiding
  * needless 256x192 VRAM uploads beside the continuously rendered 3D nursery. */
static unsigned arcade_redraw_period(const Arcade *a){
 switch(a->id){
 case 0:case 3:case 6:case 7:case 8:case 9:case 11:return 3u;
 case 1:
  /* Avoid repainting Diaper Dash's large 256x192 changing mat every
   * fourth frame while idle. Native DeSmuME captured blank pink panels
   * halfway through these unnecessary VRAM writes. Active drag/wipe
   * still refreshes frequently; dirty steps render immediately. */
  return a->touch_active?5u:60u;
 case 4:return 4u;
 case 2:return a->touch_active?4u:60u;
 case 5:case 10:case 12:case 13:case 14:case 15:return 60u;
 default:return 4u;
 }
}
static void leave_game(void){
 if(arc.finished){
  dialogue_memory_event(&memories,arc.finished==1?
                         DIALOGUE_EVENT_ARCADE_WIN:DIALOGUE_EVENT_ARCADE_LOSS,
                         baby.days);
  conversation_record_event(&conversation,arc.finished==1?
      CONVERSATION_EVENT_ARCADE_WIN:CONVERSATION_EVENT_ARCADE_LOSS,
      arc.id,baby.days);
  (void)observer_record_event(&shelter,&observer,&baby,arc.finished==1?
      OBSERVER_EVENT_ARCADE_WIN:OBSERVER_EVENT_ARCADE_LOSS,arc.id);
  message=arc.finished==1?"DIAPER DOLLARS EARNED!":"THE GLITCHES WON.";
  persist();
 }else{
  message="BACK TO THE ARCADE.";
  arcade_feedback_reset(&arcade_feedback);
 }
 arc.active=0;view=1;
}
/* Result acting uses the same retargetable rig as ordinary care. Keep the
 * pre-result needs, so a loss's chaos change cannot move the captured neck
 * before its regroup gesture. Only outcomes sample the rig; ordinary
 * game ticks merely retain a small stack copy of the current Game. */
static void react_to_arcade(int event,const Game *before,uint32_t frame){
 switch(event){
 case ARCADE_FEEDBACK_PROGRESS:sfx_play(SFX_ARCADE_PROGRESS);break;
 case ARCADE_FEEDBACK_MISS:sfx_play(SFX_ARCADE_MISS);break;
 case ARCADE_FEEDBACK_WIN:sfx_play(SFX_ARCADE_WIN);break;
 case ARCADE_FEEDBACK_LOSS:sfx_play(SFX_ARCADE_LOSS);break;
 default:return;
 }
 if(event==ARCADE_FEEDBACK_WIN||event==ARCADE_FEEDBACK_LOSS){
  WorldBabyPose from=world_baby_pose(before,frame,pose,pose_age,
                                    pose?&care_motion:0);
  pose=event==ARCADE_FEEDBACK_WIN?WORLD_BABY_CHEER:WORLD_BABY_REGROUP;
  pose_age=0;
  world_baby_motion_start(&care_motion,from,&baby,frame,pose);
 }
}
int main(void){
 uint32_t frame=0;
 int dirty=1;
 game_init(&baby);
 reset_care_session();
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
 ui_draw(&baby,&arc,view,selected,last_ending,reset_step,storage_ok,message);
 /* Sound starts only AFTER the title is visibly present. Small hardware
  * PSG cues are synthesized at the ARM9 game tick; no sample bank/heap. */
 sfx_init();
 /* Stay in RAM-only mode until the player explicitly requests save access.
  * New and old .nds game saves remain compatible when FAT is available. */
 while(GAME_LOOP_ACTIVE()){
  int down,held,tap,previous=view,virtual_move=0,guard_requested=0;
  int raw_touch;
  touchPosition touch={0};
  swiWaitForVBlank();
  scanKeys();
  down=keysDown();held=keysHeld();
  /* Footer feedback changes only on events and expiry, preserving the
   * throttled bitmap refresh instead of repainting every countdown tick. */
  if(arcade_feedback_tick(&arcade_feedback)&&(view==1||view==2))dirty=1;
  raw_touch=(held&KEY_TOUCH)!=0;
  if(raw_touch)touchRead(&touch);
  tap=touch_gate_step(&gesture,raw_touch,touch.px,touch.py);
  /* Some ARM7 runtimes assert KEY_TOUCH before calibrated coordinates arrive.
   * Keep raw pen visibility for diagnostics, but do not steer or drag game
   * objects using an uncalibrated (0,0) or off-screen sample. A real touch
   * at the top-left corner becomes valid after the gate's three-frame grace. */
  int pen_ready=touch_gate_coordinates_ready(&gesture,raw_touch,touch.px,touch.py);
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
   if((!vpen.enabled&&(down&KEY_RIGHT))||
      (observer.windup&&tap&&ui_observer_entry_touch(touch.px,touch.py))){
    guard_requested=1;dirty=1;
   }else if((tap&&ui_observer_entry_touch(touch.px,touch.py))||
            (!vpen.enabled&&(down&KEY_UP))){
    open_observer();dirty=1;
   }else if((tap&&ui_mic_entry_touch(touch.px,touch.py))||
      (!vpen.enabled&&(down&KEY_X)&&(held&KEY_L))){
    open_nursery_microphone();dirty=1;
   }else if((tap&&ui_conversation_entry_touch(touch.px,touch.py))||
      (!vpen.enabled&&(down&KEY_LEFT))){
    open_conversation(frame);dirty=1;
   }else if((tap&&ui_room_entry_touch(touch.px,touch.py))||
      (!vpen.enabled&&(down&KEY_DOWN))){
    /* Room Tour uses its own four large native touch cards. The care
     * buttons, Toybox and save/end-day shortcuts remain unchanged. */
    room_retarget(0);
    view=10;
    room_prop_age=40u;
    message="TOUCH A PROP, OR B TO RETURN.";dirty=1;
   }else if(((down&KEY_X)&&(held&KEY_R))||
      (tap&&ui_prank_entry_touch(touch.px,touch.py))){
    view=8;toy_tool=ROUGHHOUSE_SLAP;
    message="SELECT A TOY, THEN TAP THE DOLL.";dirty=1;
   }else{
    if(down&KEY_A)action=FEED;
    else if(down&KEY_B)action=CHAT;
    else if(down&KEY_X)action=PLAY;
    else if(down&KEY_Y)action=NAP;
    else if(tap)action=ui_nursery_touch(touch.px,touch.py);
    if(action>=0){care((Care)action,frame);dirty=1;}
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
  }else if(view==11){
   int picked=tap?ui_conversation_touch(touch.px,touch.py):-1;
   if(tap&&ui_observer_entry_touch(touch.px,touch.py)){
    open_observer();dirty=1;
   }else if((down&(KEY_B|KEY_START))||picked==6){
    view=0;message="WE CAN TALK AGAIN WHEN YOU LIKE.";dirty=1;
   }else if(picked>=0&&picked<6){
    talk_selected=picked;
    ask_baby((ConversationPrompt)talk_selected,frame);dirty=1;
   }else if((down&KEY_A)||picked==7){
    ask_baby((ConversationPrompt)talk_selected,frame);dirty=1;
   }else if(down&(KEY_LEFT|KEY_RIGHT)){
    talk_selected^=1;dirty=1;
   }else if(down&KEY_UP){
    talk_selected=((talk_selected/2+2)%3)*2+(talk_selected&1);dirty=1;
   }else if(down&KEY_DOWN){
    talk_selected=((talk_selected/2+1)%3)*2+(talk_selected&1);dirty=1;
   }
  }else if(view==12){
   int picked=tap?ui_observer_touch(touch.px,touch.py):-1;
   unsigned page=terminal_key&32u,index=terminal_key&31u;
   if((down&KEY_B)||picked==30){
    view=0;message="WARDEN IS KEEPING WATCH.";dirty=1;
   }else if((down&KEY_Y)||picked==31){
    terminal_speaker^=1u;
    strcpy(observer.reply,terminal_speaker==OBSERVER_BABY?
      "I AM LISTENING.\nYOU CAN TEACH ME A SHORT PHRASE.":
      "WARDEN ONLINE. ASK STATUS, WORLD,\nMEMORY, HELP OR A CALCULATION.");
    dirty=1;
   }else if(picked==32){
    terminal_teach(frame);dirty=1;
   }else if((down&(KEY_X|KEY_START))||picked==33){
    terminal_send(frame);dirty=1;
   }else if((down&KEY_SELECT)||picked==34){
    terminal_key^=32u;dirty=1;
   }else if(picked==35||((down&(KEY_L|KEY_R))&&
                         (held&(KEY_L|KEY_R))==(KEY_L|KEY_R))){
    open_microphone();dirty=1;
   }else if(picked>=36&&picked<40){
    strcpy(terminal_input,ui_observer_quick_prompt(terminal_speaker,
                                                 (unsigned)(picked-36)));
    terminal_sent=0;dirty=1;
   }else if(picked>=0&&picked<30){
    terminal_key=page|(unsigned)picked;
    terminal_type(ui_observer_key(terminal_key));dirty=1;
   }else if(down&KEY_A){
    terminal_type(ui_observer_key(terminal_key));dirty=1;
   }else if(down&KEY_L){terminal_type('<');dirty=1;
   }else if(down&KEY_R){terminal_type(' ');dirty=1;
   }else if(down&KEY_LEFT){terminal_key=page|((index/10u)*10u+(index+9u)%10u);dirty=1;
   }else if(down&KEY_RIGHT){terminal_key=page|((index/10u)*10u+(index+1u)%10u);dirty=1;
   }else if(down&KEY_UP){terminal_key=page|((index+20u)%30u);dirty=1;
   }else if(down&KEY_DOWN){terminal_key=page|((index+10u)%30u);dirty=1;
   }
  }else if(view==13){
   int picked=tap?ui_mic_touch(touch.px,touch.py):0;
   if((down&KEY_B)||picked==4){
    mic_input_stop();voice_mode=0;voice_forget_armed=0;view=12;dirty=1;
   }else if((down&(KEY_LEFT|KEY_L))||picked==5){
    browse_microphone(-1);dirty=1;
   }else if((down&(KEY_RIGHT|KEY_R))||picked==6){
    browse_microphone(1);dirty=1;
   }else if((down&KEY_SELECT)||picked==7){
    forget_microphone();dirty=1;
   }else if((down&KEY_A)||picked==1){
    start_microphone(1u,frame);dirty=1;
   }else if((down&KEY_X)||picked==2){
    start_microphone(2u,frame);dirty=1;
   }else if((down&KEY_Y)||picked==3){
    voice_forget_armed=0;
    if(voice_mode){ /* No use while recording. */ }
    else if(voice_match>=0){
     const char *label=voice_label(&voice_profile,(unsigned)voice_match);
     if(label[0]){
      strcpy(terminal_input,label);
      view=12;terminal_send(frame);dirty=1;
     }
    }else{
     strcpy(voice_status,"NO CONFIDENT MATCH TO USE.");dirty=1;
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
    else message=game_unlocked(&baby,selected)?
       "ALREADY OPEN: PRESS A.":"CHECK WINS, $ AND FAVORS.";
    dirty=1;
   }else if(down&KEY_A){begin_game();dirty=1;}
   else if(tap){
    int nav=ui_arcade_navigation(touch.px,touch.py);
    if(nav==1){begin_game();dirty=1;}
    else if(nav==2){
     if(game_unlock(&baby,selected)){message="CABINET BOUGHT!";persist();}
     else message=game_unlocked(&baby,selected)?
        "ALREADY OPEN: PRESS A.":"CHECK WINS, $ AND FAVORS.";
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
    Game before_arcade=baby;
    unsigned old_round=arc.round,old_mistakes=arc.mistakes,old_phase=arc.phase;
    if(arc.id==0||arc.id==9){
     int steer=0;
     if(!vpen.enabled && (held&KEY_LEFT))steer=-1;
     else if(!vpen.enabled && (held&KEY_RIGHT))steer=1;
     else if(pen_ready&&touch.py>=90&&touch.py<173)
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
      int pen_down=vpen.enabled?((held&KEY_A)!=0):pen_ready;
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
      int pen_down=vpen.enabled?((held&KEY_A)!=0):pen_ready;
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
      int pen_down=vpen.enabled?((held&KEY_A)!=0):pen_ready;
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
      int pen_down=vpen.enabled?((held&KEY_A)!=0):pen_ready;
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
    /* Timed questions and WATCH -> REPEAT must become visible in the
     * same tick that changes their rules. Static cabinets otherwise
     * wait up to 60 frames for their throttled timer repaint. */
    if(arc.round!=old_round||arc.mistakes!=old_mistakes||arc.phase!=old_phase)
     dirty=1;
    /* Observe once AFTER all input/timer work and BEFORE leaving a
     * finished cabinet. Footer exits are cancellations, never results. */
    if(view==2){
     int event=arcade_feedback_observe(&arcade_feedback,&arc);
     if(event!=ARCADE_FEEDBACK_NONE){
      react_to_arcade(event,&before_arcade,frame);dirty=1;
     }
     if(arc.finished){leave_game();dirty=1;}
    }
   }
  }else if(view==8){
   int hit=tap?ui_roughhouse_touch(touch.px,touch.py):0;
   if((!vpen.enabled&&(down&KEY_RIGHT))||
      (observer.windup&&tap&&ui_observer_entry_touch(touch.px,touch.py))){
    guard_requested=1;dirty=1;
   }else if((down&(KEY_B|KEY_START))||hit==3){
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
     (void)observer_record_event(&shelter,&observer,&baby,
        toy_tool==ROUGHHOUSE_KICK?OBSERVER_EVENT_KICK:OBSERVER_EVENT_SLAP,0u);
     sfx_play(toy_tool==ROUGHHOUSE_KICK?SFX_KICK:SFX_SLAP);
     reset_care_motion();
     message=toy_tool==ROUGHHOUSE_KICK?
             "BOING! THE SYNTHETIC SHELL BOUNCES.":
             "BONK! THAT WAS AN UNNATURAL SOUND.";
     dirty=1;
     if(roughhouse_cracked(&rag)){
      sfx_play(SFX_CRACK);
      (void)game_secret_egg(&baby);
      (void)observer_record_event(&shelter,&observer,&baby,OBSERVER_EVENT_REVEAL,0u);
      persist();
      view=9;egg_frame=0;egg_paused=0;last_ending=END_EGG;
      message="INCUBATION FILE 09 OPENED.";dirty=1;
     }else (void)persist();
    }
   }
  }else if(view==9){
   int film_touch=tap?ui_egg_cinematic_touch(touch.px,touch.py):0;
   if((down&(KEY_B|KEY_START))||film_touch==1){
    sfx_stop();
    egg_paused=0;
    view=4;last_ending=END_EGG;dirty=1;
   }else if((down&KEY_X)||film_touch==2){
    /* Pause freezes the camera, drip growth and scare countdown. Yields
     * no extra screen VRAM writes except this one HUD state change. */
    egg_paused=!egg_paused;
    dirty=1;
   }else{
    /* Native stylus and virtual D-pad cursor can now advance scenes too.
     * Advance unpauses on purpose; idle pauses never consume film frames. */
    if((down&KEY_A)||film_touch==3){
     egg_frame=roughhouse_egg_next_act(egg_frame);
     egg_paused=0;dirty=1;
    }else if(!egg_paused)egg_frame++;
    if(!egg_paused){
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
      egg_paused=0;
      view=4;last_ending=END_EGG;dirty=1;
     }
    }
   }
  }else if(view==3){
   if(down&KEY_B){view=0;reset_step=0;message="RESET CANCELED.";dirty=1;}
   else if(!reset_step){if(down&KEY_A){reset_step=1;dirty=1;}}
   else if((down&KEY_X)&&(held&(KEY_L|KEY_R))==(KEY_L|KEY_R)){
    if(storage_ok&&(!voice_save_erase(SAVE_BASE)||
                    !observer_save_erase(SAVE_BASE)||
                    !conversation_save_erase(SAVE_BASE)||
                    !save_erase(SAVE_BASE))){message="SAVE ERASE FAILED. NO RESET.";}
    else{
     game_init(&baby);reset_care_session();roughhouse_init(&rag);
     message="COLD BOOT. WHO AM I?";
    }
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
   else if((down&KEY_X)||
           (tap&&ui_title_save_touch(touch.px,touch.py))){
    /* Touch-only play can now opt into the SAME proven X-key save load.
     * Never initialize FAT automatically at boot or when touching START. */
    message="CHECKING SAVE STORAGE...";
    ui_draw(&baby,&arc,view,selected,last_ending,reset_step,0,message);
    storage_ok=fatInitDefault();
    if(storage_ok){
     if(save_load(SAVE_BASE,&baby)){
      reset_care_session();
      (void)conversation_save_load(SAVE_BASE,&baby,&learning);
      voice_profile_present=(uint8_t)voice_save_load(SAVE_BASE,&baby,&voice_profile);
      message=observer_save_load(SAVE_BASE,&baby,&shelter)?
       "BABY AND SHELTER MEMORY RESTORED!":"BABY LOADED. WARDEN STARTS WATCHING.";
     }
     else message="SAVING ENABLED!";
    }else message="NO SAVE DEVICE - RAM ONLY.";
    dirty=1;
   }else if((down&(KEY_A|KEY_START))||tap){
    /* A visible bottom title is already drawn. Initialize top 3D only once. */
    if(!three_d_ready){
     videoSetMode(MODE_0_3D);world_init();three_d_ready=1;
    }
    view=0;
    message=storage_ok?"WARDEN: YOUR SHELTER IS STILL HIDDEN.":"RAM MODE: X ON TITLE ENABLES SAVES.";
    dirty=1;
   }
  }else if(view==6){
   if(down&(KEY_B|KEY_START)){
    view=0;message="COLLECTION CLOSED.";dirty=1;
   }else if((down&KEY_A)&&album_page==2&&
            (baby.ending_mask&(1u<<END_EGG))){
    view=9;egg_frame=0;egg_paused=0;last_ending=END_EGG;dirty=1;
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
     view=9;egg_frame=0;egg_paused=0;last_ending=END_EGG;dirty=1;
    }else if(touch.py>=176){
     view=0;message="COLLECTION CLOSED.";dirty=1;
    }
   }
  }else if(view==7){
   if(down&(KEY_B|KEY_SELECT|KEY_START)){view=5;dirty=1;}
  }else if(view==10){
   int picked=-1;
   if(down&KEY_X){
    save_live_baby();dirty=1;
   }else if(down&KEY_Y){
    /* Same global mute as Toybox, now accessible without entering the
     * horror mode. Does not spend a tap or change baby care stats. */
    sfx_set_enabled(!sfx_is_enabled());
    message=sfx_is_enabled()?"ROOM AUDIO ON.":"ROOM AUDIO MUTED.";
    dirty=1;
   }else if(down&(KEY_B|KEY_START)){
    room_retarget(-1);
    view=0;message="BACK IN BABY'S NURSERY.";dirty=1;
   }else{
    if(down&KEY_SELECT)picked=8;
    else if(tap)picked=ui_room_touch(touch.px,touch.py);
    else if(!vpen.enabled){
     /* D-pad selects/uses the pictured objects without a touchscreen. */
     if(down&KEY_UP)picked=0;
     else if(down&KEY_RIGHT)picked=1;
     else if(down&KEY_LEFT)picked=2;
     else if(down&KEY_DOWN)picked=3;
     else if(down&KEY_A)picked=room_focus;
    }
    /* The sound toggle and USE labels are also true stylus hitboxes. */
    if(picked==6)picked=room_focus;
    if(picked==8){
     room_lullaby(frame);dirty=1;
    }else if(picked==7){
     save_live_baby();dirty=1;
    }else if(picked==5){
     sfx_set_enabled(!sfx_is_enabled());
     message=sfx_is_enabled()?"ROOM AUDIO ON.":"ROOM AUDIO MUTED.";
     dirty=1;
    }else if(picked==4){
     room_retarget(-1);
     view=0;message="BACK IN BABY'S NURSERY.";dirty=1;
    }else if(picked>=0&&picked<=3){
     if(picked!=room_focus){
      room_retarget(picked);
     }
     room_prop_age=0u; /* every tap restarts an interaction, not a loop */
     if(picked==0){
      room_daylight^=1;
      sfx_play(SFX_ROOM_SWITCH);
      message=room_daylight?"THE WINDOW SHOWS A NEW DAY.":"THE MOON IS WATCHING AGAIN.";
     }else if(picked==1){
      room_lamp^=1;
      sfx_play(SFX_ROOM_SWITCH);
      message=room_lamp?"LAMP ON. BUDGET APPROVED.":"LIGHTS OUT. OVERTIME DENIED.";
     }else if(picked==2){
      /* Never overwrite a failed-write warning with a joke. */
      if(care(PLAY,frame))message="TEDDY RECEIVED A PROMOTION.";
     }else{
      unsigned energy,chaos,glitch;
      care_apply(NAP,frame);
      energy=baby.energy;chaos=baby.chaos;glitch=baby.corruption;
      /* The attached mobile chimes while it performs its one-shot
       * 40-tick swing. It supersedes the ordinary NAP phrase only here. */
      sfx_play(SFX_MOBILE);
      if(game_room_quiet_rest(&baby,room_daylight,room_lamp)){
       dialogue_memory_event(&memories,DIALOGUE_EVENT_QUIET_REST,baby.days);
       conversation_record_event(&conversation,CONVERSATION_EVENT_QUIET_REST,
                                   0,baby.days);
       (void)observer_record_event(&shelter,&observer,&baby,
                                    OBSERVER_EVENT_QUIET_REST,0u);
       /* Quiet rest changes energy again in the same tick; retain the
        * original captured rig against this final sleepy-arm baseline. */
       world_baby_motion_start(&care_motion,care_motion.start,&baby,frame,pose);
       /* Report the real bonus after clamping at 0/100. */
       snprintf(room_feedback,sizeof(room_feedback),
                "QUIET: +%u ENERGY -%u CHAOS -%u GLITCH",
                (unsigned)baby.energy-energy,chaos-(unsigned)baby.chaos,
                glitch-(unsigned)baby.corruption);
       message=room_feedback;
      }else message="CRIB NAP: BOARD APPROVED.";
      /* Save both parts once; persist owns the final failure warning. */
      (void)persist();
     }
     dirty=1;
    }
   }
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
  {
   unsigned old_notice=observer.notice_ticks;
   int active=(view==0||view==8)&&!rag.moving;
   ObserverTick event=observer_tick(&shelter,&observer,&baby,active,guard_requested);
   if(event==OBSERVER_TICK_WINDUP){
    observer_pose(WORLD_BABY_WINDUP,frame);
    message=observer_notice(&shelter,&observer);sfx_play(SFX_PULSE);
   }else if(event==OBSERVER_TICK_HIT){
    observer_pose(WORLD_BABY_LUNGE,frame);
    message=observer_notice(&shelter,&observer);sfx_play(SFX_KICK);
   }else if(event==OBSERVER_TICK_BLOCK){
    observer_pose(WORLD_BABY_REGROUP,frame);
    message=observer_notice(&shelter,&observer);sfx_play(SFX_BOUNCE);
   }
   if(event!=OBSERVER_TICK_NONE){(void)persist();dirty=1;}
   if(old_notice&&!observer.notice_ticks)dirty=1;
   /* Ordinary care remains available during a warning, but must not
    * visually hide the pending attack. Menus and typing pause its clock. */
   if(active&&observer.windup&&pose!=WORLD_BABY_WINDUP)
    observer_pose(WORLD_BABY_WINDUP,frame);
  }
  if(view==13&&voice_mode){
   if(microphone_tick(frame)||frame%8u==0u)dirty=1;
  }
  sfx_tick();
  world_observer_status(observer_stage(&shelter),shelter.integrity,
                        observer.windup!=0u);
  if(three_d_ready){
   if(view==5||view==7)world_draw_title(frame);
   else if(view==9)world_draw_secret(&baby,egg_frame,&rag);
   else if(view==8&&!rag.moving&&
           (pose==WORLD_BABY_WINDUP||pose==WORLD_BABY_LUNGE))
    world_draw_care_from(&baby,frame,pose,pose_age,&care_motion);
   else if(view==8||(view==0&&rag.moving))
    world_draw_roughhouse(&baby,frame,&rag);
   else if(view==0||view==10||view==11||view==12||view==13)
    world_draw_nursery(&baby,frame,pose,pose_age,
                       room_focus,room_previous,room_camera_age,
                       room_daylight,room_lamp,room_prop_age,
                       &room_camera_anchor,room_attention_anchor,
                       pose?&care_motion:0);
   else world_draw_care_from(&baby,frame,pose,pose_age,pose?&care_motion:0);
  }
  /* Keep bitmap VRAM writes off the hot path unless UI state changed. */
  if(dirty||previous!=view||
     (view==2&&frame%arcade_redraw_period(&arc)==0u)){
   /* The actual per-frame draw must receive the Room prop selection and
    * lamp/day flags. Startup UI paint is NOT the Room redraw path. */
   if(view==13)ui_mic_draw(&baby,storage_ok,voice_word,
       voice_mode?voice_capture.state==VOICE_CALIBRATE?
         "MEASURING ROOM NOISE...":voice_capture.state==VOICE_WAIT?
         "LISTENING. SAY YOUR WORD NOW.":"HEARD YOU. FINISH THE WORD.":voice_status,
       voice_match<0?"":voice_label(&voice_profile,(unsigned)voice_match),
       voice_capture.level,voice_profile.used,voice_mode!=0u,
       voice_slot<0?"":voice_label(&voice_profile,(unsigned)voice_slot),
       voice_slot<0?0u:voice_profile.entries[voice_slot].samples,
       voice_forget_armed,terminal_speaker,voice_mode,voice_capture.state);
   else if(view==12)ui_observer_draw(&baby,storage_ok,terminal_speaker,
           terminal_input,observer.reply,terminal_key,observer_stage(&shelter),
           shelter.integrity);
   else ui_draw(&baby,&arc,view,view==6?album_page:
           view==8?((int)rag.charge|(toy_tool==ROUGHHOUSE_KICK?32:0)|((int)rag.hits<<6)|
                     (sfx_is_enabled()?(1<<16):0)):
           view==9?((int)egg_frame|(egg_paused?EGG_FILM_PAUSED_FLAG:0)):
           view==10?((room_focus&3)|(room_daylight?8:0)|
                    (room_lamp?16:0)|(sfx_is_enabled()?0:32)):
           view==11?talk_selected:
           selected,last_ending,reset_step,storage_ok,message);
   {
    int event=arcade_feedback_current(&arcade_feedback);
    if(view==0||view==8){
     if(observer.windup)ui_observer_guard_prompt(shelter.integrity);
     else if(observer.notice_ticks)ui_observer_alert(observer_notice(&shelter,&observer));
    }
    /* Keep pending failures visible across navigation and feedback.
     * Progress failure takes priority over its optional learning file;
     * only a successful save or an explicit new/load clears the flags. */
    if(save_write_failed)ui_save_warning();
    else if(learning_write_failed)ui_learning_save_warning();
    else if(observer_write_failed)ui_observer_save_warning();
    else if(voice_write_failed)ui_voice_save_warning();
    else if(view==2||(view==1&&(event==ARCADE_FEEDBACK_WIN||
                           event==ARCADE_FEEDBACK_LOSS)))
     ui_arcade_feedback(event);
   }
   dirty=0;
  }
  if(view==7)ui_touch_diag_update(world_vertex_count(),
                                  world_polygon_count());
  /* Overlay a tiny cursor only when virtual pen is enabled. The bottom BG
   * needn't be fully repainted on every frame, avoiding DraStic flicker. */
  ui_virtual_cursor_update(vpen.enabled,vpen.x,vpen.y,
                           vpen.enabled && (held&KEY_A)!=0);
  (void)virtual_move;
  /* Allow a full anticipation/gesture/recovery, then return to normal
   * idle. No extra timers, background threads or save-state entries. */
  if(pose){
   if(pose==WORLD_BABY_WINDUP&&observer.windup){
    if(pose_age<24u)pose_age++;
   }else if(pose_age<56u)pose_age++;
   else reset_care_motion();
  }
  if(room_camera_age<16u)room_camera_age++;
  if(room_prop_age<40u)room_prop_age++;
  frame++;
 }
 return 0;
}

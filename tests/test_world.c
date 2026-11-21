#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "world.h"
int main(void){
 Game s;Roughhouse r;int i;unsigned worst_v=0,worst_p=0;
 game_init(&s);world_init();
 world_draw_title(0);
 assert(world_vertex_count()<=1024);
 assert(world_polygon_count()<=350);
 /* The boot portrait must remain light enough for instant DS startup. */
 for(i=1;i<12;i++){
  world_draw_title((uint32_t)(i*11));
  assert(world_vertex_count()<=1024);
  assert(world_polygon_count()<=350);
 }
 for(i=0;i<256;i++){
  s.corruption=(uint8_t)((i&1)?95:5);
  s.safe_visuals=0;
  s.mood=(uint8_t)((i&2)?15:95);
  world_draw(&s,(uint32_t)i,i%5);
  if(world_vertex_count()>worst_v)worst_v=world_vertex_count();
  if(world_polygon_count()>worst_p)worst_p=world_polygon_count();
  /* Keep room for expressive meshes after the continuous-hair art pass.
   * The NDS theoretical limits are 6144 vertices and 2048 polygons. */
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
 }
 /* The five native nursery camera poses must share one coherent room.
  * 16-tick smoothstep travel is continuous, stays within the DS volume,
  * and leaves the default portrait unchanged. */
 {
  EggCamera idle=world_nursery_camera(-1,-1,16u);
  EggCamera window=world_nursery_camera(0,-1,16u);
  EggCamera lamp=world_nursery_camera(1,-1,16u);
  EggCamera teddy_view=world_nursery_camera(2,-1,16u);
  EggCamera crib=world_nursery_camera(3,-1,16u);
  unsigned f;
  /* Composition deliberately looks near the baby and landmark. The
   * earlier far-left targets showed a giant empty sage wall. */
  assert(window.look_x>-.95f && window.look_x<-.75f);
  assert(lamp.look_x>-1.45f && lamp.look_x< -1.25f);
  assert(teddy_view.look_x>-.75f && teddy_view.look_x<-.55f);
  assert(crib.look_x>1.f && crib.look_x<1.3f);
  assert(window.x>.9f && lamp.x>.9f && teddy_view.x>.9f);
  assert(idle.x>.9f&&idle.z>3.4f);
  for(i=0;i<4;i++){
   EggCamera start=world_nursery_camera(i,-1,0u);
   EggCamera end=world_nursery_camera(i,-1,16u);
   EggCamera hold=world_nursery_camera(i,-1,5000u);
   float last=start.x;
   assert(start.x==idle.x&&start.z==idle.z);
   assert(end.x==hold.x&&end.look_x==hold.look_x);
   for(f=1;f<=16;f++){
    EggCamera q=world_nursery_camera(i,-1,f);
    float diff=q.x-last;
    assert(diff<.18f&&diff>-.18f);
    assert(q.z>2.8f&&q.z<3.8f);
    last=q.x;
   }
   world_draw_nursery(&s,130u,3,18u,i,-1,16u,
                      i==0,i!=1,18u,0);
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
  /* Visual-state changes must not alter any saved Game data. */
  {
   Game original=s;
   world_draw_nursery(&s,132u,0,0u,0,-1,16u,1,0,40u,0);
   assert(memcmp(&original,&s,sizeof(s))==0);
   world_draw(&s,133u,0); /* normal care restores night + lamp */
   assert(world_polygon_count()<=1850);
  }
 }
 /* Interrupting a nursery camera move must start at the exact
  * in-flight lens coordinates, not at the previous preset's end point.
  * Rapid window->crib->teddy interactions cannot teleport the view. */
 {
  EggCamera home=world_nursery_camera(-1,-1,16u);
  EggCamera midway=world_nursery_camera_from(home,0,5u);
  EggCamera interrupt=world_nursery_camera_from(midway,3,0u);
  EggCamera first=world_nursery_camera_from(midway,3,1u);
  EggCamera second=world_nursery_camera_from(first,2,0u);
  EggCamera end=world_nursery_camera_from(midway,3,16u);
  EggCamera crib=world_nursery_camera(3,-1,16u);
  assert(interrupt.x==midway.x && interrupt.y==midway.y);
  assert(interrupt.z==midway.z && interrupt.look_x==midway.look_x);
  assert(second.x==first.x && second.look_z==first.look_z);
  assert(first.x-interrupt.x<.08f && interrupt.x-first.x<.08f);
  assert(first.look_x-interrupt.look_x<.08f);
  assert(end.x==crib.x&&end.look_x==crib.look_x);
  world_draw_nursery(&s,222u,0,0u,3,0,1u,0,1,40u,&midway);
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
 }
 /* NAP eyelids now mask two *fixed-thickness* eyes using broad, safe
  * single quads; no progressively collapsed GL_QUAD_STRIP poles that
  * become giant horizontal pixel rulers at half-closure. */
 {
  unsigned age;
  s.mood=95;
  for(age=0u;age<=56u;age++){
   world_draw_nursery(&s,173u+age,4,age,3,-1,16u,
                      0,1,40u,0);
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
 }
 /* A new nursery mobile is physically anchored to the actual crib bar.
  * One 40-tick reaction with no repeating rocking while CRIB is selected.
  * The added meshes must leave headroom below DS 1850-polygon budget. */
 assert(world_crib_mobile_swing(-1,0u)==0);
 assert(world_crib_mobile_swing(0,8u)==0);
 assert(world_crib_mobile_swing(3,0u)==0);
 assert(world_crib_mobile_swing(3,10u)==-12);
 assert(world_crib_mobile_swing(3,22u)==8);
 assert(world_crib_mobile_swing(3,40u)==0);
 assert(world_crib_mobile_swing(3,5000u)==0);
 {
  unsigned tick;
  int prev=world_crib_mobile_swing(3,0u);
  for(tick=1u;tick<50u;tick++){
   int angle=world_crib_mobile_swing(3,tick);
   assert(angle>=-12&&angle<=8);
   assert(angle-prev<=3&&prev-angle<=3);
   prev=angle;
   world_draw_nursery(&s,350u+tick,4,18u,3,-1,16u,
                      0,0,tick,0);
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
  /* The same four hanging pieces exist in regular care and Toybox
   * at rest; film continuity never depends on a room-only prop. */
  world_draw(&s,402u,0);
  assert(world_polygon_count()<=1850);
  roughhouse_init(&r);
  world_draw_roughhouse(&s,403u,&r);
  assert(world_polygon_count()<=1850);
 }
 /* Natural plush animation: single eased bow, held, then recovery.
  * A selected toy must NOT animate endlessly or detach its face. */
 assert(world_teddy_bow_angle(0u)==0);
 assert(world_teddy_bow_angle(8u)==-12);
 assert(world_teddy_bow_angle(17u)==-12);
 assert(world_teddy_bow_angle(40u)==0);
 assert(world_teddy_bow_angle(80u)==0);
 {
  unsigned tick;
  int old=world_teddy_bow_angle(0u);
  for(tick=1u;tick<52u;tick++){
   int v=world_teddy_bow_angle(tick);
   assert(v<=0&&v>=-12);
   assert(v-old<=3 && old-v<=3);
   old=v;
   world_draw_nursery(&s,300u+tick,0,0u,2,-1,16u,0,1,tick,0);
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
 }
 /* Gaze targets rotate the whole head, remain legible in profile and
  * return to neutral on leaving ROOM. */
 assert(world_baby_attention_angle(-1,16u)==0);
 assert(world_baby_attention_angle(0,0u)==0);
 assert(world_baby_attention_angle(0,16u)==-6);
 assert(world_baby_attention_angle(1,16u)==-7);
 assert(world_baby_attention_angle(2,16u)==-5);
 assert(world_baby_attention_angle(3,16u)==6);
 for(i=0;i<4;i++){
  int old=0;
  unsigned tick;
  for(tick=0u;tick<=16u;tick++){
   int v=world_baby_attention_angle(i,tick);
   assert(v>=-7&&v<=6);
   assert(v-old<=2&&old-v<=2);
   old=v;
  }
 }
 /* Arm gestures now rotate one connected shoulder-to-hand structure.
  * FEED reaches, CHAT invites, PLAY reaches farther, NAP folds in. */
 assert(world_baby_arm_reach(0)==7);
 assert(world_baby_arm_reach(1)==28);
 assert(world_baby_arm_reach(2)==45);
 assert(world_baby_arm_reach(3)==57);
 assert(world_baby_arm_reach(4)==-8);
 assert(world_baby_neck_tilt(0,0u)==0);
 assert(world_baby_neck_tilt(4,123u)==-9);
 assert(world_baby_neck_tilt(2,40u)>world_baby_neck_tilt(2,0u));
 assert(world_baby_neck_tilt(3,40u)>world_baby_neck_tilt(3,0u));
 assert(world_baby_neck_tilt(2,0u)==world_baby_neck_tilt(2,80u));
 assert(world_baby_arm_reach(3)>world_baby_arm_reach(2));
 assert(world_baby_arm_reach(2)>world_baby_arm_reach(1));
 /* The same care-age sequence replays consistently no matter when
  * the physical button is pressed. Keep the 80-age curve in range. */
 {
  unsigned age;
  for(age=0;age<80u;age++){
   int chat=world_baby_neck_tilt(2,age);
   int play=world_baby_neck_tilt(3,age);
   assert(chat>=-1&&chat<=7);
   assert(play>=3&&play<=7);
   assert(chat==world_baby_neck_tilt(2,age+80u));
   assert(play==world_baby_neck_tilt(3,age+80u));
  }
 }
 /* No abrupt chest pop at wrap, no idle breathing during Toybox
  * collisions, and no new render geometry at the breath peak. */
 assert(world_baby_breath(0u)==0);
 assert(world_baby_breath(64u)==256);
 assert(world_baby_breath(128u)==0);
 assert(world_baby_breath(192u)==256);
 {
  unsigned age,prev=0;
  for(age=1u;age<=256u;age++){
   unsigned current=(unsigned)world_baby_breath(age);
   assert(current<=256u);
   assert(current-prev<=7u || prev-current<=7u);
   prev=current;
  }
 }
 {
  unsigned baseline,peak;
  s.mood=12;
  /* Frame zero is also the first instant of the automatic eyelid blink:
   * comparing it with 64 measures a blink, not the breathing pass.
   * Frame 128 is the same zero-breath phase, safely outside blink. */
  world_draw_care(&s,128u,0,56u);
  baseline=world_polygon_count();
  world_draw_care(&s,64u,0,56u);
  peak=world_polygon_count();
  assert(baseline==peak && peak<=1850u);
 }
 /* Finite staged animation: anticipatory ease-in, expressive hold,
  * graceful return to idle (not indefinitely repeating CHAT/PLAY forever). */
 assert(world_baby_care_weight(0u)==0);
 assert(world_baby_care_weight(6u)>0);
 assert(world_baby_care_weight(12u)==256);
 assert(world_baby_care_weight(25u)==256);
 assert(world_baby_care_weight(30u)==256);
 assert(world_baby_care_weight(42u)>0);
 assert(world_baby_care_weight(55u)>=0);
 assert(world_baby_care_weight(56u)==0);
 {
  unsigned f;
  int prev=world_baby_care_weight(0);
  for(f=1u;f<58u;f++){
   int weight=world_baby_care_weight(f);
   assert(weight>=0&&weight<=256);
   assert(weight-prev<=35&&prev-weight<=35);
   prev=weight;
  }
 }
 assert(world_baby_speech_open(0u)==0);
 assert(world_baby_speech_open(8u)==0);
 assert(world_baby_speech_open(14u)>=250);
 assert(world_baby_speech_open(20u)==0);
 assert(world_baby_speech_open(26u)>=250);
 assert(world_baby_speech_open(44u)==0);
 assert(world_baby_lid_open(10u,0,25u)==256);
 assert(world_baby_lid_open(10u,4,25u)==0);
 assert(world_baby_lid_open(10u,4,0u)==256);
 assert(world_baby_lid_open(10u,4,56u)==256);
 assert(world_baby_lid_open(3u,0,25u)<80);
 for(i=0;i<58;i++){
  world_draw_care(&s,(uint32_t)(200+i),3,(uint32_t)i);
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
 }
 /* CHAT has three visible syllables at either mood. In particular an
  * unhappy baby must not produce a full babble soundtrack with an
  * unmoving flat mouth when nursery audio is enabled. */
 {
  unsigned sad_idle,sad_chat;
  s.mood=10;
  world_draw_care(&s,15u,0,20u);
  sad_idle=world_polygon_count();
  world_draw_care(&s,15u,2,20u);
  sad_chat=world_polygon_count();
  assert(sad_chat>sad_idle && sad_chat-sad_idle<=25u);
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
 }
 /* Normal care gestures should actually alter the infant's mouth geometry:
  * HAPPY CHAT / PLAY displays a connected, shallow open smile. */
 {
  unsigned idle,expressive;
  s.mood=95;s.safe_visuals=0;
  world_draw(&s,15u,0);
  idle=world_polygon_count();
  world_draw(&s,15u,2);
  expressive=world_polygon_count();
  assert(expressive>idle);
  assert(expressive-idle<=26u);
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
 }
 /* Toybox contact has a brief attached facial recoil: the mouth opens
  * on an actual hit, then closes to its normal relaxed expression.
  * It costs no additional polygons outside the short hit pulse. */
 {
  unsigned relaxed,shocked;
  roughhouse_init(&r);
  s.mood=95;
  world_draw_roughhouse(&s,410u,&r);
  relaxed=world_polygon_count();
  r.impact=9;r.moving=1;
  world_draw_roughhouse(&s,411u,&r);
  shocked=world_polygon_count();
  assert(shocked>relaxed && shocked-relaxed<=25u);
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
  r.impact=0;r.moving=0;
  world_draw_roughhouse(&s,412u,&r);
  assert(world_polygon_count()==relaxed);
 }
 /* Both the new moving doll and the three egg film states MUST fit within
  * hardware command budgets, with no new framebuffer or geometry spike. */
 roughhouse_init(&r);
 assert(roughhouse_strike(&r,ROUGHHOUSE_KICK));
 for(i=0;i<96;i++){
  roughhouse_step(&r);
  world_draw_roughhouse(&s,(uint32_t)i,&r);
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
 }
 /* Fractures and neon-green seam appear progressively ON the actual 3D
  * synthetic character before the secret cutscene. No mesh overload. */
 r.charge=0;
 world_draw_roughhouse(&s,13,&r);
 {
  unsigned shell_base=world_polygon_count();
  r.charge=6;world_draw_roughhouse(&s,13,&r);
  assert(world_polygon_count()==shell_base+2u);
  r.charge=11;world_draw_roughhouse(&s,13,&r);
  assert(world_polygon_count()==shell_base+5u);
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
 }
 for(i=0;i<8;i++){
  static const uint32_t frames[]={0u,EGG_CRACK_FRAME,
             EGG_PUSH_FRAME-1u,EGG_PUSH_FRAME,
             EGG_PUSH_FRAME+24u,EGG_SCARE_FRAME-1u,
             EGG_SCARE_FRAME,EGG_SCARE_FRAME+8u};
  world_draw_secret(&s,frames[i],&r);
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
  if(frames[i]>=EGG_CRACK_FRAME&&frames[i]<EGG_SCARE_FRAME){
   /* Real recessed cavity, a parasite silhouette, two porcelain halves,
    * visibly separate inner lips and specular green-ichor ribbons. */
   assert(world_polygon_count()>=360);
   assert(world_vertex_count()>=730);
  }
  if(frames[i]>=EGG_SCARE_FRAME){
   /* Scene must NOT collapse to a floating head on a featureless plane.
    * Nursery, split porcelain sides, cavity and attached face all remain
    * submitted after the hard cut, but fit the same DS polygon budget. */
   assert(world_polygon_count()>=650);
   /* The organic cheek details add four tiny triangles in place of
    * rectangle strips, while leaving ample DS hardware headroom. */
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
 }
 /* Physics can throw the toy out of frame. The movie introduction is a
  * consistent damaged portrait while preserving the actual source state. */
 {
  Roughhouse original,staged,again;
  roughhouse_init(&original);
  original.x=390;original.y=345;original.z=-192;
  original.roll=155;original.pitch=-130;original.yaw=210;
  original.moving=1;original.charge=ROUGHHOUSE_CRACK_CHARGE;
  staged=world_egg_staged_doll(&original,17u);
  again=world_egg_staged_doll(&original,17u);
  assert(staged.x==0&&staged.y==0&&staged.z==0);
  assert(!staged.moving && !staged.impact);
  assert(staged.roll>=-18&&staged.roll<=18);
  assert(staged.pitch==0 && staged.yaw==0);
  assert(staged.charge==original.charge);
  assert(memcmp(&staged,&again,sizeof(staged))==0);
  assert(original.x==390&&original.y==345&&original.moving);
  world_draw_secret(&s,17u,&original);
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
  roughhouse_init(&original);
  staged=world_egg_staged_doll(&original,0);
  assert(staged.charge>=ROUGHHOUSE_CRACK_CHARGE);
  assert(!staged.moving);
 }
 /* The room darkens monotonically during the SAME physical camera move,
  * with no palette leakage into normal gameplay or the reveal. */
 {
  int old_light=world_egg_nursery_ambient(EGG_CRACK_FRAME);
  unsigned f;
  assert(old_light==0);
  for(f=EGG_CRACK_FRAME+1u;f<EGG_SCARE_FRAME;f++){
   int now=world_egg_nursery_ambient(f);
   assert(now<=old_light && now>=-5);
   old_light=now;
  }
  assert(old_light==-5);
  assert(world_egg_nursery_ambient(EGG_SCARE_FRAME)==0);
  assert(world_egg_nursery_ambient(0u)==0);
 }
 /* A coherent 3D camera crosses film phase boundaries without a jump.
  * Framing retains room landmarks as the lens moves toward the cavity. */
 {
  EggCamera a=world_secret_camera(EGG_CRACK_FRAME);
  EggCamera b=world_secret_camera(EGG_PUSH_FRAME-1u);
  EggCamera c=world_secret_camera(EGG_PUSH_FRAME);
  EggCamera d=world_secret_camera(EGG_SCARE_FRAME-1u);
  EggCamera e=world_secret_camera(EGG_SCARE_FRAME);
  assert(a.x>.85f && a.y>.85f && a.z>3.4f);
  assert(a.z>b.z && b.z>c.z && c.z>d.z);
  assert(b.z-c.z<.05f); /* no teleport at push transition */
  assert(d.z>1.35f && d.z<1.45f);
  /* Two-track camera eases settle at the PUSH transition without
   * changing the actual nursery geometry or losing camera continuity. */
  {
   unsigned f;
   float prior=a.z;
   for(f=EGG_CRACK_FRAME+1u;f<EGG_SCARE_FRAME;f++){
    EggCamera next=world_secret_camera(f);
    float step=prior-next.z;
    assert(step>=0.f&&step<.035f);
    if(f==EGG_PUSH_FRAME||f==EGG_PUSH_FRAME+1u)
     assert(step<.005f);
    prior=next.z;
   }
  }
  assert(e.z>2.5f); /* single intentional jump-scare cut */
  assert(e.look_y<-.09f&&e.look_y>-.15f); /* jaw visible, not cropped */
 }
 /* Connected lower jaw: the tongue and fangs open together and settle
  * without strobing, repeat surprise cuts or frame-dependent randomness. */
 assert(world_egg_jaw_angle(EGG_SCARE_FRAME-1u)==0);
 assert(world_egg_jaw_angle(EGG_SCARE_FRAME)==0);
 assert(world_egg_jaw_angle(EGG_SCARE_FRAME+9u)==18);
 assert(world_egg_jaw_angle(EGG_SCARE_FRAME+22u)==8);
 {
  unsigned f;
  for(f=EGG_SCARE_FRAME+1u;f<EGG_TOTAL_FRAMES;f++){
   int jaw=world_egg_jaw_angle(f);
   int prev=world_egg_jaw_angle(f-1u);
   assert(jaw>=0&&jaw<=18);
   assert(jaw-prev<=2&&prev-jaw<=2);
  }
 }
 /* An opening eye glow grows as the camera approaches, then visibly
  * dims exactly once before the hard cut. Never flash the whole LCD. */
 assert(world_egg_eye_glow(0u)==0);
 assert(world_egg_eye_glow(EGG_CRACK_FRAME)==7);
 assert(world_egg_eye_glow(EGG_PUSH_FRAME)==7);
 assert(world_egg_eye_glow(EGG_SCARE_FRAME-20u)==31);
 assert(world_egg_eye_glow(EGG_SCARE_FRAME-1u)==4);
 assert(world_egg_eye_glow(EGG_SCARE_FRAME)==0);
 {
  unsigned f;
  int prev=world_egg_eye_glow(EGG_PUSH_FRAME);
  float last_approach=0.f;
  for(f=EGG_PUSH_FRAME+1u;f<EGG_SCARE_FRAME;f++){
   int v=world_egg_eye_glow(f);
   float approach=world_egg_embryo_approach(f);
   assert(v>=3&&v<=31);
   assert(v-prev<=1&&prev-v<=2);
   if(f<=EGG_SCARE_FRAME-20u)assert(v>=prev);
   else assert(v<=prev);
   prev=v;
   assert(approach>=last_approach&&approach<=.046f);
   last_approach=approach;
  }
  assert(world_egg_embryo_approach(0u)==0.f);
  assert(world_egg_embryo_approach(EGG_PUSH_FRAME)==0.f);
  assert(world_egg_embryo_approach(EGG_TOTAL_FRAMES)>.044f);
 }
 /* Active embryo, attached sac and a *single* smooth final withdrawal.
  * These deterministic samples should never overflow or leave the egg. */
 {
  unsigned tick;
  float last=0.f;
  assert(world_egg_membrane_pulse(0u)==0.f);
  assert(world_egg_membrane_pulse(EGG_SCARE_FRAME)==0.f);
  assert(world_egg_membrane_pulse(EGG_CRACK_FRAME)==0.f);
  assert(world_egg_membrane_pulse(EGG_CRACK_FRAME+32u)>.027f);
  assert(world_egg_embryo_retreat(EGG_SCARE_FRAME-18u)==0.f);
  assert(world_egg_embryo_retreat(EGG_SCARE_FRAME-1u)>.078f);
  assert(world_egg_embryo_retreat(EGG_SCARE_FRAME)==0.f);
  for(tick=EGG_CRACK_FRAME;tick<EGG_SCARE_FRAME;tick++){
   float pulse=world_egg_membrane_pulse(tick);
   assert(pulse>=0.f&&pulse<=.029f);
   if(tick>=EGG_SCARE_FRAME-18u){
    float retreat=world_egg_embryo_retreat(tick);
    assert(retreat>=last&&retreat<=.080f);
    assert(retreat-last<.007f);
    last=retreat;
   }else assert(world_egg_embryo_retreat(tick)==0.f);
   /* All incremental visuals, including changing egg wings and lens
    * movement, stay under the same real DS command budgets. */
   world_draw_secret(&s,tick,&r);
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
 }
 /* Smooth physical shell split and restrained one-piece skull sway. */
 assert(world_egg_shell_open(EGG_PUSH_FRAME-1u)==0.f);
 assert(world_egg_shell_open(EGG_PUSH_FRAME)==0.f);
 assert(world_egg_shell_open(EGG_SCARE_FRAME)==1.f);
 assert(world_egg_shell_open(EGG_TOTAL_FRAMES)==1.f);
 assert(world_egg_head_tilt(EGG_SCARE_FRAME-1u)==0);
 assert(world_egg_head_tilt(EGG_SCARE_FRAME)==-3);
 assert(world_egg_head_tilt(EGG_SCARE_FRAME+9u)==2);
 {
  unsigned f;
  float prior=0.f;
  int old_tilt=world_egg_head_tilt(EGG_SCARE_FRAME);
  for(f=EGG_PUSH_FRAME+1u;f<EGG_SCARE_FRAME;f++){
   float width=world_egg_shell_open(f);
   assert(width>=prior&&width<=1.f);
   assert(width-prior<.012f);
   prior=width;
  }
  for(f=EGG_SCARE_FRAME+1u;f<EGG_TOTAL_FRAMES;f++){
   int tilt=world_egg_head_tilt(f);
   assert(tilt>=-3&&tilt<=2);
   assert(tilt-old_tilt<=1&&old_tilt-tilt<=1);
   old_tilt=tilt;
  }
 }
 /* The creature lunges ONCE after the final hard camera cut, then settles
  * into extremely small movement. All features use the SAME matrix. */
 {
  float peak=world_egg_creature_depth(EGG_SCARE_FRAME+9u);
  float start=world_egg_creature_depth(EGG_SCARE_FRAME);
  float settle=world_egg_creature_depth(EGG_SCARE_FRAME+22u);
  unsigned f;
  assert(world_egg_creature_depth(EGG_SCARE_FRAME-1u)==0.f);
  assert(start==0.f);
  assert(peak>.30f&&peak<.32f);
  assert(settle>.20f&&settle<.22f);
  for(f=EGG_SCARE_FRAME;f<EGG_TOTAL_FRAMES;f++){
   float motion=world_egg_creature_depth(f);
   assert(motion>=0.f&&motion<=.32f);
   if(f>EGG_SCARE_FRAME+22u)assert(motion>=.212f&&motion<.222f);
   if(f>EGG_SCARE_FRAME){
    float prev=world_egg_creature_depth(f-1u);
    assert(motion-prev<.036f && prev-motion<.009f);
   }
   world_draw_secret(&s,f,&r);
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
  for(f=EGG_CRACK_FRAME;f<EGG_SCARE_FRAME;f++){
   float breath=world_egg_embryo_breath(f);
   assert(breath>=0.f&&breath<=.016f);
  }
  assert(world_egg_embryo_breath(0u)==0.f);
  assert(world_egg_embryo_breath(32u)>.015f);
  assert(world_egg_embryo_breath(64u)==0.f);
 }
 /* Visual glitch tiers must never summon stray cubes or consume extra
  * polygons; corrupted/safe visual modes alter only existing colors. */
 s.corruption=99;s.mood=75;s.safe_visuals=0;
 world_draw(&s,0,0);
 {
  unsigned base_v=world_vertex_count(),base_p=world_polygon_count();
  s.safe_visuals=1;
  world_draw(&s,0,0);
  assert(world_vertex_count()==base_v);
  assert(world_polygon_count()==base_p);
 }
 printf("PASS: distinct matte nursery/eye details, staged egg cavity & face, GPU budget (max %u verts, %u polys)\n",worst_v,worst_p);
 return 0;
}

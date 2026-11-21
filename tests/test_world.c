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

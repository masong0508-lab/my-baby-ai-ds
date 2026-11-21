#include <assert.h>
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
   /* The creature's eye sockets and actual pointed teeth are a scene
    * transition, not the old generic low-poly turquoise egg portrait. */
   assert(world_polygon_count()>=320);
  }
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
 }
 /* The creature lunges ONCE after the final hard camera cut, then settles
  * into extremely small movement. All features use the SAME matrix. */
 {
  float peak=world_egg_creature_depth(EGG_SCARE_FRAME+7u);
  float start=world_egg_creature_depth(EGG_SCARE_FRAME);
  float settle=world_egg_creature_depth(EGG_SCARE_FRAME+15u);
  unsigned f;
  assert(world_egg_creature_depth(EGG_SCARE_FRAME-1u)==0.f);
  assert(start==0.f);
  assert(peak>.17f&&peak<.18f);
  assert(settle>.12f&&settle<.13f);
  for(f=EGG_SCARE_FRAME;f<EGG_TOTAL_FRAMES;f++){
   float motion=world_egg_creature_depth(f);
   assert(motion>=0.f&&motion<=.18f);
   if(f>EGG_SCARE_FRAME+15u)assert(motion>=.127f&&motion<.14f);
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

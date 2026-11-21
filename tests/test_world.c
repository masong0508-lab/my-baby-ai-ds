#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "world.h"
int main(void){
 Game s;int i;unsigned worst_v=0,worst_p=0;
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
 printf("PASS: clean 3D portrait, stable glitch mesh, budget (max %u verts, %u polys)\n",worst_v,worst_p);
 return 0;
}

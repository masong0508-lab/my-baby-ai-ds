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
 for(i=0;i<256;i++){
  s.corruption=(uint8_t)((i&1)?95:5);
  s.safe_visuals=0;
  s.mood=(uint8_t)((i&2)?15:95);
  world_draw(&s,(uint32_t)i,i%5);
  if(world_vertex_count()>worst_v)worst_v=world_vertex_count();
  if(world_polygon_count()>worst_p)worst_p=world_polygon_count();
  /* Reserve headroom for GPU scene submission; avoid living at the DS cap.
   * The NDS theoretical limits are 6144 vertices and 2048 polygons. */
  assert(world_vertex_count()<=5600);
  assert(world_polygon_count()<=1900);
 }
 printf("PASS: 3D budget (max %u vertices / 6144, %u polygons / 2048)\n",worst_v,worst_p);
 return 0;
}

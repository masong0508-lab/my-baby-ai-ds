#include <assert.h>
#include <stdio.h>
#include "virtual_stylus.h"
int main(void){
 VirtualStylus s;int i;
 virtual_stylus_init(&s);
 assert(!s.enabled && s.x==64 && s.y==131);
 assert(!virtual_stylus_move(&s,VS_RIGHT));
 virtual_stylus_toggle(&s);assert(s.enabled);
 assert(virtual_stylus_move(&s,VS_RIGHT));assert(s.x==67);
 assert(!virtual_stylus_move(&s,VS_LEFT|VS_RIGHT));
 /* A sustained D-pad press accelerates; reversing restores precise speed. */
 {
  int old_x=s.x, before, after;
  for(i=0;i<12;i++)virtual_stylus_move(&s,VS_RIGHT);
  before=s.x;
  assert(before-old_x==36); /* first twelve frames still 3px */
  virtual_stylus_move(&s,VS_RIGHT);
  assert(s.x-before==5);
  for(i=0;i<16;i++)virtual_stylus_move(&s,VS_RIGHT);
  after=s.x;
  virtual_stylus_move(&s,VS_RIGHT);
  assert(s.x-after==8);
  after=s.x;
  virtual_stylus_move(&s,VS_LEFT);
  assert(after-s.x==3);
  virtual_stylus_move(&s,0);
  after=s.x;
  virtual_stylus_move(&s,VS_LEFT);
  assert(after-s.x==3);
 }
 for(i=0;i<200;i++)virtual_stylus_move(&s,VS_LEFT|VS_UP);
 assert(s.x==0 && s.y==0);
 for(i=0;i<200;i++)virtual_stylus_move(&s,VS_RIGHT|VS_DOWN);
 assert(s.x==255 && s.y==191);
 virtual_stylus_toggle(&s);
 assert(!virtual_stylus_move(&s,VS_UP));
 assert(s.y==191);
 puts("PASS: virtual stylus movement, bounds, toggling");
 return 0;
}

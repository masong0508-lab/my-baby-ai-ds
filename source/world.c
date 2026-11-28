#include <nds.h>
#include "world.h"

/* Warm, naturally proportioned NDS nursery: geometry is native DS hardware 3D.
 * This pass replaces intentionally grotesque eye/head proportions with
 * smaller embedded facial features, soft material colors and curved contours.
 * Avoid texture VRAM, dynamic allocation or extra framebuffers.
 */
static const float cx[8]={1,.7071f,0,-.7071f,-1,-.7071f,0,.7071f};
static const float cz[8]={0,.7071f,1,.7071f,0,-.7071f,-1,-.7071f};
static const float h[4]={-1,-.54f,.54f,1};
static const float w[4]={0,.842f,.842f,0};
static int limit(int v){return v<0?0:v>31?31:v;}
/* Avoid passing negative degree values into DS fixed-angle conversion.
 * On some ARM builds the signed shift in degreesToAngle can yield an
 * invalid rotation matrix: a tiny NAP lean then flips the entire doll.
 * Wrap to the native [0,360) domain before any model transform. */
static int model_angle(int degrees){
 int wrapped=degrees%360;
 if(wrapped<0)wrapped+=360;
 return degreesToAngle(wrapped);
}
/* Draw-time-only room lighting: no extra geometry, textures or VRAM. */
static int nursery_ambient;
/* Only the interactive Room tour modifies the nursery lighting. Defaults
 * reproduce the approved original care/night composition. */
static int tour_daylight;
static int tour_lamp_on=1;
static unsigned observer_visual_stage,observer_visual_integrity=100u;
static int observer_visual_warning;
void world_observer_status(unsigned stage,unsigned integrity,int warning){
 observer_visual_stage=stage>3u?3u:stage;
 observer_visual_integrity=integrity>100u?100u:integrity;
 observer_visual_warning=warning!=0;
}
/* The DS geometry engine retains its current vertex color. Avoid
 * submitting the same color register command twice in succession.
 * Invalidate at each scene submission, including after a frozen image. */
static unsigned last_vertex_color=~0u;
/* Native geometry FIFO cache. Record only the matrix-free, static nursery
 * prefix on its first draw; subsequent frames DMA the identical fixed-point
 * vertex/color stream. The camera, mobile and baby are always live.
 * A single 32 KiB bounded entry avoids a framebuffer or heap allocation.
 * The host topology shim retains the original unquantized immediate path. */
#if defined(ARM9) && !defined(MBAI_DISABLE_ROOM_FIFO_CACHE)
#define ROOM_FIFO_CACHE 1
#define ROOM_FIFO_WORDS 8192u
static uint32_t room_fifo[ROOM_FIFO_WORDS] __attribute__((aligned(32)));
static unsigned room_fifo_used,room_fifo_group,room_fifo_slot;
static int room_fifo_recording,room_fifo_overflow,room_fifo_valid;
static int room_fifo_ambient,room_fifo_daylight,room_fifo_lamp;
static unsigned room_fifo_stage,room_fifo_integrity;
static int room_fifo_warning;
static unsigned room_fifo_vertices,room_fifo_polygons,room_fifo_last_color;
static void room_fifo_command(unsigned command,unsigned count,uint32_t a,uint32_t b){
 if(!room_fifo_recording||room_fifo_overflow)return;
 if(room_fifo_used+count+(room_fifo_slot==0u)>ROOM_FIFO_WORDS){
  room_fifo_overflow=1;return;
 }
 if(!room_fifo_slot){
  room_fifo_group=room_fifo_used++;
  room_fifo[room_fifo_group]=0; /* unused command bytes are FIFO_NOP */
 }
 room_fifo[room_fifo_group]|=(uint32_t)command<<(8u*room_fifo_slot);
 room_fifo_slot=(room_fifo_slot+1u)&3u;
 if(count)room_fifo[room_fifo_used++]=a;
 if(count==2u)room_fifo[room_fifo_used++]=b;
}
static void room_fifo_begin(int mode){
 room_fifo_command(FIFO_BEGIN,1u,(uint32_t)mode,0);
 glBegin(mode);
}
static void room_fifo_color(unsigned packed){
 room_fifo_command(FIFO_COLOR,1u,packed,0);
 glColor(packed);
}
static void room_fifo_vertex(float x,float y,float z){
 v16 fx=floattov16(x),fy=floattov16(y),fz=floattov16(z);
 if(room_fifo_recording)
  room_fifo_command(FIFO_VERTEX16,2u,(uint32_t)(uint16_t)fx|
                    ((uint32_t)(uint16_t)fy<<16),(uint32_t)(uint16_t)fz);
 glVertex3v16(fx,fy,fz);
}
#define glBegin(mode) room_fifo_begin(mode)
#define glColor(packed) room_fifo_color(packed)
#define glVertex3f(x,y,z) room_fifo_vertex((x),(y),(z))
#endif
static unsigned packed_color(int r,int g,int b,int shift){
 const int delta=shift+nursery_ambient;
 return RGB15(limit(r+delta),limit(g+delta),limit(b+delta));
}
static void submit_color(unsigned packed){
 if(packed!=last_vertex_color){
  glColor(packed);
  last_vertex_color=packed;
 }
}
static void color(int r,int g,int b,int shift){
 submit_color(packed_color(r,g,b,shift));
}
static void face(float a,float b,float c,float d,float e,float f,
                 float g,float i,float j,float k,float l,float m){
 glVertex3f(a,b,c);glVertex3f(d,e,f);glVertex3f(g,i,j);glVertex3f(k,l,m);
}
static unsigned frame_vertices,frame_polygons;
/* The room's opaque, independent quads all use the same model matrix
 * and polygon attributes. Keep adjacent quads in a single hardware
 * primitive batch without reordering a vertex or changing a color.
 * State: 0 outside room; 1 room scope, no batch; 2 open quad batch.
 * Curved strips, triangles and animated transforms are hard barriers. */
static unsigned room_quad_batch;
static void room_quad_begin(void){
 if(room_quad_batch!=2u){
  glBegin(GL_QUADS);
  if(room_quad_batch)room_quad_batch=2u;
 }
}
static void room_quad_end(void){
 if(!room_quad_batch)glEnd();
}
static void room_quad_barrier(void){
 if(room_quad_batch==2u){
  glEnd();
  room_quad_batch=1u;
 }
}
static void block(float x,float y,float z,float width,float height,float depth,
                  int r,int g,int b){
 float l=x-width*.5f,rr=x+width*.5f,bot=y-height*.5f,top=y+height*.5f;
 float bk=z-depth*.5f,fr=z+depth*.5f;
 room_quad_begin();
 color(r,g,b,2);face(l,bot,fr,rr,bot,fr,rr,top,fr,l,top,fr);
 color(r,g,b,-3);face(rr,bot,bk,l,bot,bk,l,top,bk,rr,top,bk);
 color(r,g,b,-2);face(l,bot,bk,l,bot,fr,l,top,fr,l,top,bk);
 color(r,g,b,-6);face(rr,bot,fr,rr,bot,bk,rr,top,bk,rr,top,fr);
 color(r,g,b,4);face(l,top,fr,rr,top,fr,rr,top,bk,l,top,bk);
 color(r,g,b,-8);face(l,bot,bk,rr,bot,bk,rr,bot,fr,l,bot,fr);
 room_quad_end();
 frame_vertices+=24;
 frame_polygons+=6;
}
/* Surface-only decal: ideal for painted floor grain, wallpaper and fabric.
 * Replaces six-faced boxes with one flat quad in dedicated DS hardware.
 */
static void flat_floor(float x,float y,float z,float sx,float sz,
                       int r,int g,int b){
 room_quad_begin();
 color(r,g,b,0);
 face(x-sx*.5f,y,z-sz*.5f,
      x+sx*.5f,y,z-sz*.5f,
      x+sx*.5f,y,z+sz*.5f,
      x-sx*.5f,y,z+sz*.5f);
 room_quad_end();
 frame_vertices+=4;frame_polygons++;
}
static void wall_sticker(float x,float y,float z,float sx,float sy,
                         int r,int g,int b){
 room_quad_begin();
 color(r,g,b,0);
 face(x-sx*.5f,y-sy*.5f,z,
      x+sx*.5f,y-sy*.5f,z,
      x+sx*.5f,y+sy*.5f,z,
      x-sx*.5f,y+sy*.5f,z);
 room_quad_end();
 frame_vertices+=4;frame_polygons++;
}
/* Simple graphic on the inside left wall; the normal back-wall decal
 * cannot be rotated there. One quad per field, not a six-faced box. */
static void side_wall_mark(float x,float y,float z,float sz,float sy,
                           int r,int g,int b){
 room_quad_begin();
 color(r,g,b,0);
 face(x,y-sy*.5f,z-sz*.5f,
      x,y-sy*.5f,z+sz*.5f,
      x,y+sy*.5f,z+sz*.5f,
      x,y+sy*.5f,z-sz*.5f);
 room_quad_end();
 frame_vertices+=4;frame_polygons++;
}
/* DS-native four-corner lighting. The hardware interpolates vertex
 * colors across three quads: warm lamp-side plaster, a colder moonlit
 * upper-right wall and floor. Much richer depth than one beige plane,
 * without lightmaps, alpha buffers, textures or per-pixel CPU drawing. */
static void nursery_light_planes(void){
 /* The window side stays cool while the lamp end warms the lower wall.
  * These are broad architectural gradients, not floating light strips.
  * All three surfaces are separated from their wall/floor boxes. */
 room_quad_begin();
 color(tour_lamp_on?29:23,tour_lamp_on?24:22,21,0);
 glVertex3f(-3.29f,-1.07f,-3.131f);
 color(23,22,25,0);glVertex3f(3.30f,-1.07f,-3.131f);
 color(tour_daylight?23:20,20,tour_daylight?27:25,0);
 glVertex3f(3.30f,2.53f,-3.131f);
 color(tour_daylight?25:24,tour_daylight?25:21,25,0);
 glVertex3f(-3.29f,2.53f,-3.131f);
 room_quad_end();
 room_quad_begin();
 color(23,19,17,0);glVertex3f(-3.28f,-1.237f,3.28f);
 color(22,20,21,0);glVertex3f(3.28f,-1.237f,3.28f);
 color(20,19,23,0);glVertex3f(3.28f,-1.237f,-3.09f);
 color(tour_lamp_on?28:22,23,tour_daylight?24:20,0);
 glVertex3f(-3.28f,-1.237f,-3.09f);
 room_quad_end();
 room_quad_begin();
 color(tour_lamp_on?23:19,24,21,0);
 glVertex3f(-3.340f,-1.07f,3.28f);
 color(19,23,22,0);glVertex3f(-3.340f,2.53f,3.28f);
 color(22,25,25,0);glVertex3f(-3.340f,2.53f,-3.10f);
 color(tour_lamp_on?27:22,25,22,0);
 glVertex3f(-3.340f,-1.07f,-3.10f);
 room_quad_end();
 frame_vertices+=12;frame_polygons+=3;
}
/* Three broad quads make one clipped-corner contact footprint. No alpha
 * blending, ellipsoid poles, texture or long narrow geometry. Nested
 * tones soften the edge while remaining deterministic native DS meshes. */
static void floor_contact(float x,float y,float z,float rx,float rz,
                           int r,int g,int b){
 const float edge=rx*.7071f,depth=rz*.7071f;
 room_quad_begin();
 color(r,g,b,0);
 face(x-edge,y,z-rz,x+edge,y,z-rz,
      x+edge,y,z+rz,x-edge,y,z+rz);
 face(x-rx,y,z-depth,x-edge,y,z-rz,
      x-edge,y,z+rz,x-rx,y,z+depth);
 face(x+edge,y,z-rz,x+rx,y,z-depth,
      x+rx,y,z+depth,x+edge,y,z+rz);
 room_quad_end();
 frame_vertices+=12;frame_polygons+=3;
}
static void baby_contact(float x,float z,float scale){
 /* Center the contact under both boots, not behind the seated hips.
  * The nested shapes travel with the toy and contract during a bounce. */
 floor_contact(x,-1.121f,z,.53f*scale,.31f*scale,21,20,26);
 floor_contact(x,-1.118f,z,.43f*scale,.23f*scale,18,17,23);
}
/* A rail has three visible faces rather than all six box faces. */
static void crib_rail(float x,float y,float z,float width,float height,
                      float depth,int r,int g,int b){
 float lx=x-width*.5f,rx=x+width*.5f,lo=y-height*.5f,hi=y+height*.5f;
 float fr=z+depth*.5f,bk=z-depth*.5f;
 room_quad_begin();
 color(r,g,b,2);face(lx,lo,fr,rx,lo,fr,rx,hi,fr,lx,hi,fr);
 color(r,g,b,-4);face(rx,lo,fr,rx,lo,bk,rx,hi,bk,rx,hi,fr);
 color(r,g,b,4);face(lx,hi,fr,rx,hi,fr,rx,hi,bk,lx,hi,bk);
 room_quad_end();
 frame_vertices+=12;frame_polygons+=3;
}
/* GL_QUAD_STRIP shares ring vertices between adjacent polygons. This
 * reduces ellipsoid submissions from 96 to 54 vertices at the SAME 8
 * angular sides / 24 polygons. Critical for the DS's 6144 vertex cap.
 */
static void ellipse(float x,float y,float z,float rx,float ry,float rz,
                    int red,int green,int blue){
 int a,i,j;
 /* Both middle rings have the same radius. Compute their coordinates
  * once, reuse the two poles, and prepare the six band colors once.
  * Keep the original multiplication grouping and strip winding. */
 const float sx=rx*w[1],sz=rz*w[1];
 float ringx[8],ringz[8],ringy[4];
 unsigned shades[2][3];
 for(i=0;i<8;i++){
  ringx[i]=x+sx*cx[i];
  ringz[i]=z+sz*cz[i];
 }
 for(i=0;i<4;i++)ringy[i]=y+ry*h[i];
 shades[0][0]=packed_color(red,green,blue,1);
 shades[0][1]=packed_color(red,green,blue,-4);
 shades[0][2]=packed_color(red,green,blue,-1);
 shades[1][0]=packed_color(red,green,blue,3);
 shades[1][1]=packed_color(red,green,blue,-2);
 shades[1][2]=shades[0][0];
 room_quad_barrier();
 for(a=0;a<3;a++){
  const unsigned *palette=shades[a==2];
  glBegin(GL_QUAD_STRIP);
  for(i=0;i<=8;i++){
   j=i&7;
   submit_color(palette[j<3?0:j<5?1:2]);
   glVertex3f(a==0?x:ringx[j],ringy[a],a==0?z:ringz[j]);
   glVertex3f(a==2?x:ringx[j],ringy[a+1],a==2?z:ringz[j]);
  }
  glEnd();
 }
 frame_vertices+=54;
 frame_polygons+=24;
}
/* 12-sided, five-band curved surface for the baby's main face.
 * Costs only 60 quads/130 submitted vertices; all other rounded shapes
 * remain on the cheaper eight-sided mesh. No trigonometry per frame.
 * The 5-bit vertex shading is baked from a soft upper-left light source.
 */
static void soft_head(float x,float y,float z,float rx,float ry,float rz,
                      int red,int green,int blue){
 static const float ring_x[12]={1,.866f,.5f,0,-.5f,-.866f,-1,-.866f,-.5f,0,.5f,.866f};
 static const float ring_z[12]={0,.5f,.866f,1,.866f,.5f,0,-.5f,-.866f,-1,-.866f,-.5f};
 static const float v[6]={-1,-.72f,-.28f,.28f,.72f,1};
 static const float radius[6]={0,.66f,.96f,.96f,.66f,0};
 static const int light[12]={0,1,2,3,4,5,2,-1,-3,-4,-3,-1};
 static const unsigned rings[6]={0,1,2,2,1,0};
 float xs[3][12],zs[3][12],ys[6];
 unsigned shades[15];
 int a,i,j;
 /* Only two nonzero radii exist across the six rings. Reuse the
  * coordinates at each adjacent strip and mirrored latitude. */
 for(i=0;i<12;i++){
  xs[0][i]=x;zs[0][i]=z;
 }
 for(a=1;a<=2;a++){
  const float sx=rx*radius[a],sz=rz*radius[a];
  for(i=0;i<12;i++){
   xs[a][i]=x+sx*ring_x[i];
   zs[a][i]=z+sz*ring_z[i];
  }
 }
 for(i=0;i<6;i++)ys[i]=y+ry*v[i];
 for(i=0;i<15;i++)shades[i]=packed_color(red,green,blue,i-7);
 for(a=0;a<5;a++){
  const unsigned lo=rings[a],hi=rings[a+1];
  const unsigned *low_shades=shades+7+(a>=3?2:a==0?-3:0);
  const unsigned *high_shades=shades+7+(a>=2?2:-2);
  glBegin(GL_QUAD_STRIP);
  for(i=0;i<=12;i++){
   j=i==12?0:i;
   submit_color(low_shades[light[j]]);
   glVertex3f(xs[lo][j],ys[a],zs[lo][j]);
   submit_color(high_shades[light[j]]);
   glVertex3f(xs[hi][j],ys[a+1],zs[hi][j]);
  }
  glEnd();
 }
 frame_vertices+=130;
 frame_polygons+=60;
}
/* Connected crown mesh. Unlike the old isolated hair_point triangles, the
 * hair is a single continuous cap with a curved fringe line. Side and rear
 * hair fall lower than the forehead, like a short untidy bob. 36 quads. */
static void hair_cap(float x,float y,float z,float rx,float ry,float rz,
                     int red,int green,int blue){
 static const float ring_x[12]={1,.866f,.5f,0,-.5f,-.866f,-1,-.866f,-.5f,0,.5f,.866f};
 static const float ring_z[12]={0,.5f,.866f,1,.866f,.5f,0,-.5f,-.866f,-1,-.866f,-.5f};
 static const float radius[4]={1,.86f,.55f,0};
 int a,i,j;
 float xs[3][12],zs[3][12],lower_y[12];
 const float top_y[3]={y+ry*.64f,y+ry*.86f,y+ry};
 unsigned shades[2][2];
 /* Adjacent strips share the same crown ring. Reuse coordinates and
  * color conversions, preserving multiplication order and winding. */
 for(a=0;a<3;a++){
  const float sx=rx*radius[a],sz=rz*radius[a];
  for(j=0;j<12;j++){
   xs[a][j]=x+sx*ring_x[j];zs[a][j]=z+sz*ring_z[j];
  }
 }
 for(j=0;j<12;j++)
  lower_y[j]=y+ry*(.40f+(ring_z[j]>0?.14f*ring_z[j]:.03f*ring_z[j]));
 shades[0][0]=packed_color(red,green,blue,2);
 shades[0][1]=packed_color(red,green,blue,-1);
 shades[1][0]=packed_color(red,green,blue,4);
 shades[1][1]=packed_color(red,green,blue,1);
 for(a=0;a<3;a++){
  glBegin(GL_QUAD_STRIP);
  for(i=0;i<=12;i++){
   j=i==12?0:i;
   submit_color(shades[a==2][j>=5]);
   glVertex3f(xs[a][j],a==0?lower_y[j]:top_y[a-1],zs[a][j]);
   glVertex3f(a==2?x:xs[a+1][j],top_y[a],a==2?z:zs[a+1][j]);
  }
  glEnd();
 }
 frame_vertices+=78;frame_polygons+=36;
}
/* Broad overlapping side-swept fringe plates, not detached needle tips.
 * Top edge is tucked toward the scalp; lower edge is in front of skin. */
static void hair_fringe(float x,float y,float z,float sx,float sy,float skew){
 glBegin(GL_QUADS);
 color(17,11,8,1);
 glVertex3f(x-sx*.5f,y+sy*.5f,z-.016f);
 glVertex3f(x+sx*.5f,y+sy*.5f,z-.016f);
 color(15,10,8,-1);
 glVertex3f(x+sx*.43f+skew,y-sy*.5f,z+.009f);
 glVertex3f(x-sx*.43f+skew,y-sy*.5f,z+.009f);
 glEnd();
 frame_vertices+=4;frame_polygons++;
}
/* A local matrix rotation gives arms and feet actual pose direction rather
 * than leaving each ellipsoid upright and visually disconnected. */
static void tilted_ellipse(float x,float y,float z,float rx,float ry,float rz,
                           int degrees,int red,int green,int blue){
 glPushMatrix();
 glTranslatef(x,y,z);
 glRotateZi(model_angle(degrees));
 ellipse(0,0,0,rx,ry,rz,red,green,blue);
 glPopMatrix(1);
}
/* A pixel-budgeted flat tulip decal (stem, two leaves, two petals).
 * Quads are kept to a single plane on the back wall. */
static void tulip(float x,float y,float z,int pink){
 const int r=pink?29:30, g=pink?15:23, b=pink?19:9;
 wall_sticker(x,y-.075f,z,.025f,.22f,11,21,8);
 wall_sticker(x-.06f,y-.125f,z,.105f,.035f,13,24,9);
 wall_sticker(x+.06f,y-.09f,z,.09f,.032f,13,24,9);
 wall_sticker(x-.055f,y+.065f,z,.115f,.12f,r,g,b);
 wall_sticker(x+.055f,y+.065f,z,.115f,.12f,r,g,b);
}
/* Rounded-looking flower silhouettes on the floor: five petals and a center.
 * All six details are one quad each, preserving polygon budget. */
static void rug_flower(float x,float z){
 flat_floor(x-.105f,-1.109f,z,.17f,.14f,28,17,22);
 flat_floor(x+.105f,-1.109f,z,.17f,.14f,28,17,22);
 flat_floor(x,-1.109f,z-.105f,.17f,.14f,28,17,22);
 flat_floor(x,-1.109f,z+.105f,.17f,.14f,28,17,22);
 flat_floor(x-.065f,-1.108f,z-.065f,.14f,.14f,30,21,24);
 flat_floor(x,-1.107f,z,.12f,.11f,31,27,17);
}
/* Thin surface layers read as fabric, eyebrows or facial blush rather than
 * big detached spheres. Each costs a single GPU quad. */
static void soft_mark(float x,float y,float z,float sx,float sy,
                      int r,int g,int b){
 wall_sticker(x,y,z,sx,sy,r,g,b);
}
/* A rounded shallow surface has no hidden back half or collapsed poles.
 * Eight real triangles replace 24-quad wafer-thin ellipsoids in facial
 * details. The GPU interpolates the soft center/edge colors. All rings
 * retain positive area, including the smallest partial blink. */
static void face_oval(float x,float y,float z,float rx,float ry,
                      int r,int g,int b,int edge_shift){
 int i,j;
 glBegin(GL_TRIANGLES);
 for(i=0;i<8;i++){
  j=(i+1)&7;
  color(r,g,b,0);glVertex3f(x,y,z);
  color(r,g,b,edge_shift);
  glVertex3f(x+rx*cx[i],y+ry*cz[i],z-.008f);
  glVertex3f(x+rx*cx[j],y+ry*cz[j],z-.008f);
 }
 glEnd();
 frame_vertices+=24;frame_polygons+=8;
}
/* 12-sided shallow relief for decorations backed by a solid surface.
 * A gently raised center with shaded perimeter gives real depth and
 * smoother silhouettes, without an invisible back half or degenerate
 * ellipsoid poles. 36 vertices/12 nondegenerate triangles instead of
 * 54 vertices/24 quad-strip polygons. */
static void decor_relief(float x,float y,float z,float rx,float ry,float depth,
                         int red,int green,int blue){
 static const float dx[12]={1,.866f,.5f,0,-.5f,-.866f,-1,-.866f,-.5f,0,.5f,.866f};
 static const float dy[12]={0,.5f,.866f,1,.866f,.5f,0,-.5f,-.866f,-1,-.866f,-.5f};
 static const int shade[12]={-2,0,2,3,3,2,0,-2,-3,-4,-4,-3};
 unsigned i,j;
 room_quad_barrier();
 glBegin(GL_TRIANGLES);
 for(i=0;i<12u;i++){
  j=(i+1u)%12u;
  color(red,green,blue,3);glVertex3f(x,y,z+depth);
  color(red,green,blue,shade[i]);
  glVertex3f(x+rx*dx[i],y+ry*dy[i],z);
  color(red,green,blue,shade[j]);
  glVertex3f(x+rx*dx[j],y+ry*dy[j],z);
 }
 glEnd();
 frame_vertices+=36;frame_polygons+=12;
}
/* One shared inset-eye treatment for the nursery and boot portrait.
 * The opening and iris narrow together, so a blink needs no rectangular
 * skin shutter covering the cheek. The gentle rim stays inside the face.
 * Draw the closed-lid crease separately once the opening is below 1/4. */
static void baby_eye(float x,float y,float z,float scale,int lid,int gaze){
 const float shift=(float)gaze*.006f*scale;
 const float aperture=.090f*(float)lid/256.f;
 const float center=y-(.090f-aperture)*.16f*scale;
 const float iris=aperture*.88f<.063f?aperture*.88f:.063f;
 const float pupil=aperture*.77f<.044f?aperture*.77f:.044f;
 face_oval(x,center,z,.124f*scale,(aperture+.010f)*scale,
            24,17,15,2);
 face_oval(x,center,z+.010f,.106f*scale,aperture*scale,
            31,30,27,-1);
 face_oval(x+.011f*scale+shift,center-.006f*scale,z+.021f,
            .057f*scale,iris*scale,11,19,20,-3);
 face_oval(x+.013f*scale+shift,center-.008f*scale,z+.031f,
            .030f*scale,pupil*scale,4,7,9,-1);
 /* A smaller glint disappears during a low blink instead of floating
  * above the narrowed pupil. It remains attached to the same eye. */
 if(lid>=112)
  soft_mark(x-.010f*scale+shift,center+.024f*scale,z+.038f,
             .023f*scale,.018f*scale,31,31,29);
}
/* Low CHAT syllables used a wafer-thin ellipsoid whose pole bands could
 * rasterize as a long horizontal line across the jaw. The same intended
 * aperture is now eight positive-area triangles at the old front depth.
 * The tongue follows the aperture instead of floating below a closed
 * syllable. Both surfaces stay on the shared head/neck transform. */
static void baby_mouth(float open){
 face_oval(0,.252f,.468f,.094f,.085f*open,12,4,9,-1);
 soft_mark(0,.252f-.040f*open,.476f,.095f,.025f*open,30,15,19);
}
unsigned world_vertex_count(void){return frame_vertices;}
unsigned world_polygon_count(void){return frame_polygons;}
/* Four physically anchored hanging charms. One shared parent transform
 * carries each string and its decoration as one attached mobile element.
 * Twelve DS-native quads total: no texture, buffer or heap allocation. */
static void crib_mobile(int tilt){
 int i;
 /* Never let a static primitive cross an animated matrix change. */
 room_quad_barrier();
 for(i=0;i<4;i++){
  float arm=-.38f+(float)i*.26f;
  glPushMatrix();
  glTranslatef(2.28f+arm,1.25f,-1.43f);
  glRotateZi(model_angle(tilt+(i&1?-tilt/4:tilt/5)));
  glBegin(GL_QUADS);
  color(24,23,19,0);
  face(-.018f,-.02f,.025f,.018f,-.02f,.025f,
       .018f,-.24f,.025f,-.018f,-.24f,.025f);
  color(i&1?27:26,i&1?17:26,i&1?22:14,0);
  face(0,-.19f,.047f,.14f,-.32f,.047f,
       0,-.48f,.047f,-.14f,-.32f,.047f);
  color(i&1?30:20,i&1?26:23,i&1?20:29,0);
  face(-.045f,-.30f,.053f,.045f,-.30f,.053f,
       .045f,-.36f,.053f,-.045f,-.36f,.053f);
  glEnd();
  frame_vertices+=12;frame_polygons+=3;
  glPopMatrix(1);
 }
}
/* Pure frame sampler for one intentional slow swing after CRIB is tapped.
 * At 40 ticks it rests. No repeated rocking and no detached ornaments. */
int world_crib_mobile_swing(int focus,uint32_t age){
 int t,e;
 if(focus!=3||age>=40u)return 0;
 if(age<10u){
  t=(int)(age*256u/10u);
  e=(t*t*(768-2*t)+32768)/65536;
  return -12*e/256;
 }
 if(age<22u){
  t=(int)((age-10u)*256u/12u);
  e=(t*t*(768-2*t)+32768)/65536;
  return -12+20*e/256;
 }
 t=(int)((age-22u)*256u/18u);
 e=(t*t*(768-2*t)+32768)/65536;
 return 8-8*e/256;
}
/* Six continuous top panels form a two-tone quilt without floating
 * decals, subpixel depth offsets, textures or a second mattress box.
 * Side faces are the original mattress thickness and footprint. */
static void crib_quilt(void){
 const float left=1.665f,right=2.895f,front=-.815f,back=-2.165f;
 const float bottom=-.53f,top=-.43f;
 int x,z;
 room_quad_begin();
 color(31,20,24,2);face(left,bottom,front,right,bottom,front,right,top,front,left,top,front);
 color(31,20,24,-3);face(right,bottom,back,left,bottom,back,left,top,back,right,top,back);
 color(31,20,24,-2);face(left,bottom,back,left,bottom,front,left,top,front,left,top,back);
 color(31,20,24,-6);face(right,bottom,front,right,bottom,back,right,top,back,right,top,front);
 color(31,20,24,-8);face(left,bottom,back,right,bottom,back,right,bottom,front,left,bottom,front);
 for(z=0;z<3;z++)for(x=0;x<2;x++){
  float a=x==0?left:2.28f,b=x==0?2.28f:right;
  float c=z==0?back:z==1?-1.715f:-1.265f;
  float d=z==0?-1.715f:z==1?-1.265f:front;
  /* Each shared edge uses precisely the same coordinate. Gentle
   * color differences are broad fabric pieces, not thin stripes. */
  color(31,(x+z)&1?23:26,(x+z)&1?25:28,0);
  glVertex3f(a,top,d);glVertex3f(b,top,d);
  color(31,(x+z)&1?23:26,(x+z)&1?25:28,-1);
  glVertex3f(b,top,c);glVertex3f(a,top,c);
 }
 room_quad_end();
 frame_vertices+=44;frame_polygons+=11;
}
/* This prefix has no camera, animated matrix operations or mobile tilt.
 * Leave the primitive closed before recording/replaying its commands. */
static void room_static(void){
 int i;
 /* Floor/walls/skirting have real thickness and meet at the corners. */
 block(0,-1.43,0,7.0,.25,7.0,24,19,14);
 block(0,.67,-3.29,7.0,4.05,.18,29,28,25);
 block(-3.45,.67,0,.18,4.05,7.0,25,27,25);
 nursery_light_planes();
 block(0,-1.15,-3.12,6.9,.16,.12,24,22,19);
 block(-3.32,-1.15,0,.12,.16,6.9,24,22,19);
 /* WARDEN's salvaged wall terminal occupies the former framed painting.
  * Broad opaque surfaces remain legible at native resolution. Its steady
  * alert lamp and bracer bar read actual observer state; no fake telemetry
  * animation, flickering scanlines or extra screen buffer is involved. */
 side_wall_mark(-3.290f,1.34f,-.25f,1.18f,.85f,8,10,14);
 side_wall_mark(-3.268f,1.39f,-.25f,1.01f,.57f,3,10,12);
 side_wall_mark(-3.247f,1.43f,-.25f,.54f,.23f,11,26,21);
 side_wall_mark(-3.226f,1.43f,-.25f,.18f,.17f,3,9,11);
 side_wall_mark(-3.246f,1.03f,-.25f,.72f,.07f,11,14,17);
 if(observer_visual_integrity){
  float extent=.72f*(float)observer_visual_integrity/100.f;
  side_wall_mark(-3.224f,1.03f,-.61f+extent*.5f,extent,.07f,13,27,21);
 }
 side_wall_mark(-3.224f,1.04f,.23f,.11f,.10f,
                 observer_visual_warning?31:observer_visual_stage>=2u?28:12,
                 observer_visual_warning?10:observer_visual_stage>=2u?20:26,
                 observer_visual_warning?10:8);
 /* The old .035/.045 tall wainscot strips projected into unstable
  * one-pixel rulers over the dresser at 256x192. Clean broad painted
  * plaster replaces them: no more floating horizontal lines. */
 /* Continuous nursery floor: removed 14 subpixel strips. Those long
  * narrow decals produced vertical seams across the DS floor image. */
 /* Real prop footprints anchor the cream furniture and orange plush.
  * Their broad low-contrast shapes stay beneath the fixed props. */
 floor_contact(-2.55f,-1.228f,-.48f,.72f,.58f,17,16,17);
 floor_contact(2.28f,-1.228f,-1.49f,.85f,.93f,17,17,20);
 floor_contact(-1.28f,-1.220f,.55f,.39f,.34f,18,17,19);
 /* One darkened ground plane under the play mat gives contact shadow
  * without expensive translucent blending or extra VRAM. */
 flat_floor(.04f,-1.222f,.78f,3.00f,2.58f,20,17,14);
 block(.04,-1.20,.78,2.86,.09,2.43,15,17,23);
 block(.04,-1.145,.78,2.62,.045,2.18,23,23,29);
 for(i=0;i<4;i++)flat_floor(-.89f+i*.56f,-1.112f,1.78f,.13f,.035f,31,25,23);
 /* Muted lavender-and-blush nursery rug keeps the turquoise synthetic
  * baby distinct against the floor instead of washing it out in neon lime.
  * Flowers and crib remain playful without fighting the foreground. */
 /* Big stylized flowers, a defining motif of the 2000s reference. */
 /* The contact shadow now follows the doll during Toybox physics, so it
  * is drawn by each scene, not painted permanently at its spawn point. */
 rug_flower(-.92f,1.32f);
 rug_flower(.92f,1.48f);
 rug_flower(-.33f,1.77f);
 /* Glazed nursery window, mullions, curtains and cornice. */
 block(-1.76,1.15,-3.06,1.95,1.76,.07,29,28,25);
 /* The glazing is inset against its solid frame. Draw only the
  * front surface at the old box's front depth and color. */
 wall_sticker(-1.76f,1.15f,-2.99f,1.65f,1.51f,
              tour_daylight?16:8,tour_daylight?25:14,tour_daylight?31:26);
 /* Dark, moonlit glass deliberately contrasts with the amber dresser
  * lamp. The same recognizable frame, blinds and furniture stay put for
  * the full film dolly. All marks are fixed native quads (no texture). */
 wall_sticker(-1.76f,.67f,-2.978f,1.52f,.42f,
              tour_daylight?10:7,tour_daylight?22:15,tour_daylight?23:19);
 wall_sticker(-1.20f,.85f,-2.977f,.60f,.18f,
              tour_daylight?12:8,tour_daylight?25:18,tour_daylight?22:21);
 wall_sticker(-2.20f,1.56f,-2.978f,.46f,.11f,11,17,25);
 wall_sticker(-2.32f,1.61f,-2.976f,.24f,.10f,9,15,23);
 wall_sticker(-1.41f,1.75f,-2.976f,.39f,.08f,10,16,27);
 /* One recognizable moon disk and a few rain streaks, with bright pink
  * glitch eyes hiding OUTSIDE, not random floating indoor particles. */
 decor_relief(-1.35f,1.54f,-2.964f,
         tour_daylight?.17f:.14f,tour_daylight?.17f:.14f,.016f,
         31,tour_daylight?26:29,tour_daylight?9:22);
 wall_sticker(-2.18f,1.32f,-2.954f,.016f,.24f,15,22,29);
 wall_sticker(-1.95f,1.62f,-2.954f,.014f,.18f,17,23,29);
 wall_sticker(-1.63f,1.12f,-2.954f,.015f,.23f,13,20,29);
 wall_sticker(-1.17f,1.42f,-2.954f,.015f,.16f,15,21,28);
 wall_sticker(-2.06f,1.07f,-2.949f,.06f,.05f,27,8,22);
 wall_sticker(-1.83f,1.07f,-2.949f,.06f,.05f,27,8,22);
 wall_sticker(-1.76,1.15,-2.97,.10,1.57,28,27,24);
 wall_sticker(-1.76,1.15,-2.965,1.71,.10,28,27,24);
 block(-2.63,1.12,-2.81,.27,1.84,.15,25,12,18);
 block(-.89,1.12,-2.81,.27,1.84,.15,25,12,18);
 block(-1.76,2.11,-2.79,2.09,.13,.15,25,12,18);
 /* Picture frame, yellow stars and faux painting. */
 block(.53,1.93,-3.03,.85,.75,.11,21,17,13);
 wall_sticker(.53f,1.93f,-2.945f,.69f,.57f,29,29,25);
 decor_relief(.48f,1.94f,-2.925f,.18f,.22f,.015f,24,22,15);
 block(1.62,1.84,-3.03,.76,.81,.11,21,17,13);
 wall_sticker(1.62f,1.84f,-2.95f,.60f,.64f,23,29,25);
 decor_relief(1.62f,1.85f,-2.92f,.19f,.16f,.02f,27,24,17);
 /* Dresser, three actual drawers and bottle atop it. */
 block(-2.55,-.48,-.48,1.20,1.36,.94,27,24,20);
 block(-2.55,.23,-.48,1.36,.16,1.11,29,26,22);
 for(i=0;i<3;i++){
  const float drawer_y=-.13f-(float)i*.36f;
  /* Wide, intentionally separated cabinet faces instead of six-faced
   * shallow cuboids with subpixel side caps. No coplanar drawer edges.
   * Two FRONT quads at 0.068/0.095 world-Z replace each old 6-face box. */
  wall_sticker(-2.55f,drawer_y,.068f,1.03f,.305f,21,18,16);
  wall_sticker(-2.55f,drawer_y,.095f,.970f,.243f,30,27,23);
  decor_relief(-2.55f,drawer_y,.128f,.073f,.054f,.038f,27,13,18);
 }
 block(-2.55,.42,-.48,.14,.28,.14,29,23,21);
 ellipse(-2.55,.65,-.48,.34,.16,.32,
         tour_lamp_on?31:19,tour_lamp_on?25:18,tour_lamp_on?15:17);
 /* Lamp rim and warm key-light patch: intentional large forms rather than
  * noisy fake bloom or per-frame translucent screen effects. */
 ellipse(-2.55,.76,-.48,.20,.06,.18,
         tour_lamp_on?31:21,tour_lamp_on?30:20,tour_lamp_on?20:19);
 if(tour_lamp_on)
  wall_sticker(-2.73f,.78f,-.012f,.40f,.03f,31,25,18);
 /* REMOVE 5.6-unit, one-pixel wall ribbons. They z-fought with the
  * color-interpolated back wall and made bright 40+ pixel horizontal
  * cracks through the dresser and crib in real DeSmuME captures.
  * Plaster and crib geometry still define the room naturally. */
 /* 3D crib with rails on two visible sides, mattresses and legs. */
 block(2.28,-.64,-1.49,1.49,.20,1.61,29,28,24);
 crib_quilt();
 ellipse(2.20f,-.39f,-1.87f,.34f,.07f,.25f,31,27,28); /* cotton pillow */
 for(i=0;i<5;i++){
  crib_rail(1.76f+i*.26f,-.16,-.64,.075,1.19,.085,30,29,25);
  crib_rail(1.76f+i*.26f,-.16,-2.34,.075,1.19,.085,30,29,25);
 }
 block(2.28,.45,-.64,1.52,.14,.13,30,29,25);
 block(2.28,.45,-2.34,1.52,.14,.13,30,29,25);
 block(1.55,-.14,-1.49,.13,1.13,1.72,30,29,25);
 block(3.03,-.14,-1.49,.13,1.13,1.72,30,29,25);
 /* The nursery's slightly too-cheerful flower decals. */
 /* The flower decals are a full 0.050 units in front of the smoothed
  * wall plane, not 0.009 units apart (which risks depth quantization). */
 tulip(-.44f,.84f,-3.080f,1);
 tulip(.12f,.76f,-3.080f,0);
 tulip(.74f,.70f,-3.080f,1);
 tulip(1.29f,.70f,-3.080f,0);
 /* A single soft daisy on the green side wall. */
 side_wall_mark(-3.328f,1.51f,.38f,.34f,.31f,29,15,20);
 side_wall_mark(-3.327f,1.51f,.38f,.12f,.13f,31,25,10);
 /* Shelf and a row of toy blocks, no giant texture atlas needed. */
 block(2.59,1.06,-3.00,1.09,.13,.38,24,20,16);
 for(i=0;i<4;i++)block(2.16f+i*.26f,1.28,-2.91,.20,.30,.24,19+i*2,20-i,20+i);
 /* Solid mobile support; floating unanchored quads removed. */
 block(2.28,.93,-1.49,.065,.75,.065,24,21,16);
 block(2.28,1.29,-1.49,.91,.07,.10,24,21,16);
 room_quad_barrier();
}
static void room(int mobile_tilt){
 int i;
 room_quad_batch=1u;
#if defined(ROOM_FIFO_CACHE)
 /* All inputs that affect static geometry or its palette are part of the
  * key, including WARDEN's live integrity bar and cinematic lighting. */
 if(room_fifo_valid&&last_vertex_color==~0u&&
    room_fifo_ambient==nursery_ambient&&room_fifo_daylight==tour_daylight&&
    room_fifo_lamp==tour_lamp_on&&room_fifo_stage==observer_visual_stage&&
    room_fifo_integrity==observer_visual_integrity&&
    room_fifo_warning==observer_visual_warning){
  glCallList(room_fifo);
  /* BEGIN commands delimit primitives; END is intentionally omitted
   * from packed lists for compatibility with the DS FIFO parser. */
  glEnd();
  frame_vertices+=room_fifo_vertices;
  frame_polygons+=room_fifo_polygons;
  last_vertex_color=room_fifo_last_color;
 }else{
  unsigned before_v=frame_vertices,before_p=frame_polygons;
  room_fifo_valid=0;
  room_fifo_used=1u;room_fifo_slot=0u;room_fifo_overflow=0;
  room_fifo_recording=last_vertex_color==~0u;
  room_static();
  if(room_fifo_recording&&!room_fifo_overflow&&room_fifo_used>1u){
   room_fifo[0]=room_fifo_used-1u;
   room_fifo_vertices=frame_vertices-before_v;
   room_fifo_polygons=frame_polygons-before_p;
   room_fifo_last_color=last_vertex_color;
   room_fifo_ambient=nursery_ambient;
   room_fifo_daylight=tour_daylight;room_fifo_lamp=tour_lamp_on;
   room_fifo_stage=observer_visual_stage;
   room_fifo_integrity=observer_visual_integrity;
   room_fifo_warning=observer_visual_warning;
   room_fifo_valid=1;
  }
  /* Capacity failure leaves the complete immediate draw intact. */
  room_fifo_recording=0;
 }
#else
 room_static();
#endif
 crib_mobile(mobile_tilt); /* strings originate at the real crib bar */
 for(i=0;i<6;i++)wall_sticker(-.27f+i*.42f,2.34f+(i&1)*.08f,
                                -3.015f,.11,.11,27,25,19);
 /* Individual decorative labels on the shelf's toy blocks. */
 for(i=0;i<4;i++)wall_sticker(2.16f+i*.26f,1.28f,-2.774f,
                               .09f,.13f,27,26,22);
 /* Blocks and balls visible in foreground. */
 block(2.56,-1.04,1.54,.39,.36,.40,25,20,20);
 block(2.56,-.70,1.54,.30,.32,.31,19,24,26);
 block(-2.34,-1.05,1.68,.46,.36,.39,24,21,16);
 ellipse(-2.68,-1.00,1.03,.22,.22,.20,20,26,21);
 room_quad_barrier();
 room_quad_batch=0u;
}
static void teddy(void){
 /* Readable stuffed-bear silhouette: chest, light muzzle, dark stitched face.
  * Limbs overlap the body, with their axes angled outward. */
 /* Keep the plush clearly beside the baby, not centered in front of its
  * legs and romper. Seat it on the floor near the nursery dresser. */
 const float x=-1.28f,y=-.87f,z=.52f;
 ellipse(x,y-.04f,z,.30f,.32f,.24f,27,16,5);
 ellipse(x,y+.17f,z+.01f,.245f,.235f,.215f,28,17,6);
 ellipse(x,y+.38f,z+.05f,.255f,.265f,.23f,28,17,5);
 ellipse(x-.185f,y+.565f,z,.115f,.115f,.105f,27,16,5);
 ellipse(x+.185f,y+.565f,z,.115f,.115f,.105f,27,16,5);
 decor_relief(x-.185f,y+.565f,z+.095f,.055f,.058f,.018f,23,11,7);
 decor_relief(x+.185f,y+.565f,z+.095f,.055f,.058f,.018f,23,11,7);
 ellipse(x,y+.285f,z+.248f,.165f,.12f,.055f,30,25,19);
 decor_relief(x-.09f,y+.43f,z+.255f,.040f,.051f,.03f,5,5,4);
 decor_relief(x+.09f,y+.42f,z+.255f,.040f,.051f,.03f,5,5,4);
 decor_relief(x,y+.295f,z+.311f,.05f,.035f,.026f,7,5,4);
 /* Stitched black button eyes receive tiny fixed catchlights. */
 soft_mark(x-.098f,y+.446f,z+.287f,.018f,.017f,30,25,19);
 soft_mark(x+.082f,y+.436f,z+.287f,.018f,.017f,30,25,19);
 soft_mark(x-.035f,y+.247f,z+.311f,.055f,.016f,8,5,4);
 soft_mark(x+.035f,y+.247f,z+.311f,.055f,.016f,8,5,4);
 tilted_ellipse(x-.28f,y+.015f,z+.06f,.14f,.18f,.15f,-26,28,17,5);
 tilted_ellipse(x+.28f,y+.015f,z+.06f,.14f,.18f,.15f,26,28,17,5);
 ellipse(x-.175f,y-.285f,z+.09f,.13f,.13f,.17f,27,16,5);
 ellipse(x+.175f,y-.285f,z+.09f,.13f,.13f,.17f,27,16,5);
 /* Bright fabric patches separate the toy from the similarly colored floor. */
 face_oval(x,y-.09f,z+.246f,.085f,.085f,30,25,18,-3);
 face_oval(x-.175f,y-.285f,z+.263f,.039f,.0325f,30,25,18,-3);
 face_oval(x+.175f,y-.285f,z+.263f,.039f,.0325f,30,25,18,-3);
}
/* Articulated shoulders. The sleeve, forearm, hand all share ONE local
 * shoulder transform; no independent bobbing parts and no new polygons.
 * A horizontal PLAY reach or CHAT gesture is visible from the front,
 * while NAP naturally folds both arms toward the torso. */
/* The entire face, hair, eyes, mouth and cheeks lean together. NAP
 * nods toward the lap; PLAY and CHAT glance subtly toward the player. */
/* This phase is ACTION AGE, never the global frame clock: two taps
 * must animate identically even if they occur at different times. */
int world_baby_neck_tilt(int action,uint32_t age){
 int phase=(int)(age%80u);
 int sway=phase<40?phase:80-phase;
 if(action==WORLD_BABY_CHEER)return 4;
 if(action==WORLD_BABY_WINDUP)return -6;
 if(action==WORLD_BABY_LUNGE)return 3;
 /* One small acknowledging head tilt, then return toward the player.
  * The common 56-tick envelope brings the attached head back to rest. */
 if(action==WORLD_BABY_REGROUP)
  return age<=18u?-4:age>=34u?2:-4+(int)(age-18u)*6/16;
 if(action==4)return -9;
 if(action==2)return 3+(sway-20)/5;
 if(action==3)return 5+(sway-20)/7;
 return 0;
}
int world_baby_arm_reach(int action){
 if(action==WORLD_BABY_CHEER)return 76;
 if(action==WORLD_BABY_REGROUP)return 20;
 if(action==WORLD_BABY_WINDUP)return 29;
 if(action==WORLD_BABY_LUNGE)return 54;
 return action==3?57:action==2?45:action==1?28:
        action==4?-8:action==5?-6:7;
}
/* Q8 eased animation envelope: deliberate anticipation -> full gesture ->
 * recovery. Frame-accurate rather than endlessly waving after one care tap.
 * Integer only, deterministic, with zero heap and no animation tables. */
static int smooth8(uint32_t age,uint32_t length){
 uint32_t t;
 if(age>=length)return 256;
 if(!length)return 256;
 t=age*256u/length;
 return (int)((t*t*(768u-2u*t)+32768u)/65536u);
}
/* A single shallow celebratory lift, never an endless bounce loop.
 * Its apex is at age 18, it lands by age 40, and both feet stay attached. */
static int baby_cheer_lift(uint32_t age){
 if(age>=40u)return 0;
 return age<=18u?smooth8(age,18u):smooth8(40u-age,22u);
}
/* Action-driven teddy bow: ease in, hold briefly, recover to rest.
 * Never run an endless loop just because TEDDY remains selected. */
int world_teddy_bow_angle(uint32_t age){
 if(age<8u)return -(12*smooth8(age,8u)/256);
 if(age<18u)return -12;
 if(age<40u)return -(12*smooth8(40u-age,22u)/256);
 return 0;
}
/* Both eyes, cheeks and hair track the selected furniture together.
 * Small yaw angles avoid oversized profile distortions on 256x192. */
int world_baby_attention_from(int from,int focus,uint32_t age){
 static const int gazes[4]={-6,-7,-5,6};
 int target=focus>=0&&focus<4?gazes[focus]:0;
 if(age>=16u)return target;
 /* Capture the sampled head angle before changing a Room focus, just
  * like the lens anchor. Restarting every turn from zero made the face
  * snap toward the player when switching props or leaving the room. */
 if(from< -7)from=-7;
 if(from>6)from=6;
 return from+(target-from)*smooth8(age,16u)/256;
}
int world_baby_attention_angle(int focus,uint32_t age){
 return world_baby_attention_from(0,focus,age);
}
/* Quiet respiration stays inside the existing torso mesh: no additional
 * geometry or textures. One 128-tick inhale/exhale cycle uses the same
 * bounded integer easing as the one-shot care gestures. */
static int idle_motion_enabled=1;
void world_set_idle_motion(int enabled){idle_motion_enabled=enabled!=0;}
int world_baby_idle_sway(uint32_t frame,int mood,int energy){
 unsigned phase=frame%240u,half=phase%120u;
 int amplitude=mood>=65&&energy>=40?3:1;
 int eased=smooth8(half<=60u?half:120u-half,60u);
 return (phase<120u?amplitude:-amplitude)*eased/256;
}
int world_baby_breath(uint32_t frame){
 uint32_t phase=frame%128u;
 return smooth8(phase<=64u?phase:128u-phase,64u);
}
/* Missing sleep needs to be seen in the BABY, not merely an ENERGY meter.
 * Even at zero energy the pupils remain inside their normal eye sockets,
 * using rounded positive-area surfaces instead of thin 3D ellipsoids.
 * Fully rested (>54) characters retain their original bright-eyed pose. */
int world_baby_drowsy_lid(int energy){
 if(energy<=0)return 96;
 if(energy>=55)return 256;
 return 96+energy*160/55;
}
/* Deterministic idle impatience, not a repeating care animation.
 * Hunger and chaos raise the amplitude, with only a four-degree maximum
 * motion at the attached neck pivot. No heap, random impulses or polygons. */
int world_baby_need_fidget(int hunger,int chaos,uint32_t frame){
 int amplitude=0,phase,triangle;
 if(hunger>=70)amplitude=1+(hunger-70)/15;
 if(chaos>=70)amplitude+=1+(chaos-70)/25;
 if(amplitude>4)amplitude=4;
 if(!amplitude)return 0;
 phase=(int)(frame%120u);
 triangle=phase<=60?phase:120-phase;
 return (triangle-30)*amplitude/30;
}
int world_baby_care_weight(uint32_t age){
 if(age<12u)return smooth8(age,12u);
 if(age<30u)return 256;
 if(age<56u)return smooth8(56u-age,26u);
 return 0;
}
/* Natural 8-tick blink with continuous eyelid height. NAP closes over
 * the same action envelope; opening occurs during gesture recovery.
 * A closed eye is an existing narrow lid, never a floating eyeball. */
/* Three readable syllable pulses during CHAT. The jaw and tongue share
 * one existing mouth aperture; no separate facial bits bouncing in space. */
int world_baby_speech_open(uint32_t age){
 uint32_t beat;
 if(age<8u||age>=44u)return 0;
 beat=(age-8u)%12u;
 return (int)(beat<=6u?beat:12u-beat)*42;
}
/* Quiet eye contact alternates with two short, eased glances. Iris,
 * pupil and glint share the same offset; sockets never move. */
int world_baby_gaze(uint32_t frame){
 uint32_t phase=frame%480u;
 if(phase>=192u&&phase<248u){
  uint32_t age=phase-192u;
  return -3*smooth8(age<12u?age:age>=44u?56u-age:12u,12u)/256;
 }
 if(phase>=352u&&phase<408u){
  uint32_t age=phase-352u;
  return 3*smooth8(age<12u?age:age>=44u?56u-age:12u,12u)/256;
 }
 return 0;
}
int world_baby_lid_open(uint32_t frame,int action,uint32_t age){
 uint32_t beat=frame%173u;
 /* Every fourth cycle includes a second, naturally spaced blink.
  * Reuse the established eight-tick aperture curve. */
 if((frame/173u)%4u==2u&&beat>=20u&&beat<28u)beat-=20u;
 int openness=256,nap;
 if(beat<8u){
  int blink=beat<4u?256-(int)(beat+1u)*64:(int)(beat-3u)*64;
  if(blink<openness)openness=blink;
 }
 if(action==4){
  nap=256-world_baby_care_weight(age);
  if(nap<openness)openness=nap;
 }
 return openness;
}
/* Sample the actual channels submitted to the rig, not an action label.
 * The Toybox path uses the same original impulse/settle responses, while
 * ordinary care can capture an interrupted gesture before needs change. */
static WorldBabyPose baby_pose_sample(const Game *s,uint32_t frame,
                                      int action,uint32_t age,
                                      const Roughhouse *rag){
 WorldBabyPose p;
 unsigned phase=age%36u,hop=phase<18u?phase:36u-phase;
 int weight=rag?256:(action?world_baby_care_weight(age):0);
 int result=action==WORLD_BABY_CHEER||action==WORLD_BABY_REGROUP||
            action==WORLD_BABY_WINDUP||action==WORLD_BABY_LUNGE;
 int sleepy=!rag&&s->energy<25?-4*(25-(int)s->energy)/25:0;
 int reach=7+(world_baby_arm_reach(action)-7)*weight/256+sleepy;
 /* .00145 * 160 = 29/125. Preserve the original whole-degree wave
  * without doing float work for a tiny transient integer rig. */
 int wave=result?0:((int)hop-9)*29*weight/(125*256);
 /* A settled Toybox shell must not retain an asymmetric impact pose. */
 int recoil=rag&&rag->moving?(rag->vr>3?5:rag->vr< -3?-5:0):0;
 int neck=world_baby_neck_tilt(action,age)*weight/256;
 /* The staged fractured-doll reveal holds recognizable open eyes. */
 int lid=rag&&!rag->moving&&rag->charge>=ROUGHHOUSE_CRACK_CHARGE?
          256:world_baby_lid_open(frame,action,age);
 p.reach_left=(int16_t)(reach+wave+recoil);
 p.reach_right=(int16_t)(reach-wave-recoil);
 p.forward_left=(int16_t)((action==WORLD_BABY_WINDUP?18:
                           action==WORLD_BABY_LUNGE?32:
                           action==WORLD_BABY_CHEER?9:
                           action==WORLD_BABY_REGROUP?12:
                           action==1?13:action==2?8:
                           action==3?10:0)*weight/256);
 p.forward_right=(int16_t)(p.forward_left+(action==2?6*weight/256:0));
 p.lean=(int16_t)((action==WORLD_BABY_WINDUP?-3:
                   action==WORLD_BABY_LUNGE?4:
                   action==WORLD_BABY_CHEER?3:
                   action==WORLD_BABY_REGROUP?-2:
                   action==1?3:action==2?-3:action==3?5:
                   action==4?-3:0)*weight/256);
 if(!rag){
  if(idle_motion_enabled)
   neck+=world_baby_need_fidget(s->hunger,s->chaos,frame)*(256-weight)/256;
  lid=lid*world_baby_drowsy_lid(s->energy)/256;
 }
 if(action==WORLD_BABY_REGROUP){
  /* A soft, reassuring look; do not make an already tired baby droop
   * further. Natural blinking still has priority over this small cap. */
  int soft_lid=256-48*weight/256;
  if(lid>soft_lid)lid=soft_lid;
 }
 if(action==WORLD_BABY_WINDUP||action==WORLD_BABY_LUNGE){
  int narrowed=256-80*weight/256;
  if(lid>narrowed)lid=narrowed;
 }
 if(rag&&rag->moving)neck-=rag->vr/3;
 if(neck>16)neck=16;
 if(neck< -16)neck=-16;
 p.neck=(int16_t)neck;
 p.yaw=(int16_t)((action==WORLD_BABY_CHEER?-1:
                  action==WORLD_BABY_REGROUP?2:
                  action==2?3:action==3?-2:0)*weight/256);
 if(!rag&&idle_motion_enabled)
  p.yaw=(int16_t)(p.yaw+world_baby_idle_sway(frame,s->mood,s->energy)*(256-weight)/256);
 p.lid=(int16_t)lid;
 p.bounce=(int16_t)(action==WORLD_BABY_CHEER?
                    12*weight*baby_cheer_lift(age)/256:
                    action==3?(int)hop*weight:0);
 return p;
}
static int16_t care_channel(int current,int from,int baseline,int keep){
 return (int16_t)(current+(from-baseline)*keep/256);
}
WorldBabyPose world_baby_pose(const Game *s,uint32_t frame,int action,
                              uint32_t age,const WorldBabyMotion *motion){
 WorldBabyPose p=baby_pose_sample(s,frame,action,age,0);
 int keep;
 if(!motion||age>=12u)return p;
 keep=256-smooth8(age,12u);
 /* Add only the decaying difference from the new zero-age baseline.
  * Blending the whole new pose again would double-ease every gesture.
  * At age zero every channel exactly equals the interrupted sample;
  * by age twelve the established hold/recovery curve is unchanged. */
 p.reach_left=care_channel(p.reach_left,motion->start.reach_left,
                           motion->baseline.reach_left,keep);
 p.reach_right=care_channel(p.reach_right,motion->start.reach_right,
                            motion->baseline.reach_right,keep);
 p.forward_left=care_channel(p.forward_left,motion->start.forward_left,
                             motion->baseline.forward_left,keep);
 p.forward_right=care_channel(p.forward_right,motion->start.forward_right,
                              motion->baseline.forward_right,keep);
 p.lean=care_channel(p.lean,motion->start.lean,motion->baseline.lean,keep);
 p.neck=care_channel(p.neck,motion->start.neck,motion->baseline.neck,keep);
 p.yaw=care_channel(p.yaw,motion->start.yaw,motion->baseline.yaw,keep);
 p.lid=care_channel(p.lid,motion->start.lid,motion->baseline.lid,keep);
 p.bounce=care_channel(p.bounce,motion->start.bounce,
                      motion->baseline.bounce,keep);
 /* A natural blink can advance during the handoff; its additive lid
  * correction must never turn a shutter inside-out or open past the eye. */
 if(p.lid<0)p.lid=0;
 if(p.lid>256)p.lid=256;
 return p;
}
void world_baby_motion_start(WorldBabyMotion *motion,WorldBabyPose from,
                             const Game *s,uint32_t frame,int action){
 motion->start=from;
 motion->baseline=baby_pose_sample(s,frame,action,0u,0);
}
/* Rotating hand caps must be real triangles, not quad-strip poles with
 * two coincident vertices carrying different colors. Native DeSmuME
 * captured a 77px brown scanline across the teddy during FEED while an
 * upper hand cap crossed that row. Keep the same silhouette and polygons;
 * only these two articulated hands spend twelve extra vertices apiece. */
static void baby_hand(float x,float y,float z){
 const float rx=.13f,ry=.125f,rz=.13f;
 int cap,i,j,k;
 for(cap=0;cap<2;cap++){
  int ring=cap?2:1,pole=cap?3:0;
  glBegin(GL_TRIANGLES);
  for(i=0;i<8;i++){
   j=i;k=(i+1)&7;
   color(31,24,20,(j<3?2:j<5?-3:0)+(cap?1:-1));
   glVertex3f(x+rx*w[ring]*cx[j],y+ry*h[ring],z+rz*w[ring]*cz[j]);
   /* One pole vertex, with one stable color across adjacent triangles. */
   color(31,24,20,cap?1:-2);
   glVertex3f(x,y+ry*h[pole],z);
   color(31,24,20,(k<3?2:k<5?-3:0)+(cap?1:-1));
   glVertex3f(x+rx*w[ring]*cx[k],y+ry*h[ring],z+rz*w[ring]*cz[k]);
  }
  glEnd();
 }
 glBegin(GL_QUAD_STRIP);
 for(i=0;i<=8;i++){
  j=i&7;
  color(31,24,20,(j<3?2:j<5?-3:0)-1);
  glVertex3f(x+rx*w[1]*cx[j],y+ry*h[1],z+rz*w[1]*cz[j]);
  glVertex3f(x+rx*w[2]*cx[j],y+ry*h[2],z+rz*w[2]*cz[j]);
 }
 glEnd();
 frame_vertices+=66;frame_polygons+=24;
}
static void baby_arm(int direction,int angle,int forward){
 float side=(float)direction;
 glPushMatrix();
 glTranslatef(side*.45f,-.21f,-.025f);
 glRotateZi(model_angle(direction*angle));
 glRotateXi(model_angle(-forward));
 ellipse(0,0,0,.205f,.22f,.195f,17,25,26);
 tilted_ellipse(side*.10f,-.16f,.17f,
                .14f,.18f,.14f,direction*26,31,23,19);
 baby_hand(side*.18f,-.25f,.305f);
 glPopMatrix(1);
}
/* Baby A.I. hero model. One stable chibi silhouette with a connected
 * romper, SHORT hair, embedded eyes and a small closed-mouth smile. The
 * previous dark bob, gaping mouth/teeth, flaring jeans and detached facial
 * layers were the main reason the character looked frightening at 256x192.
 * Only ROM-native primitive meshes; no texture or framebuffer allocation. */
static void baby_model(const Game *s,uint32_t frame,int action,
                       const Roughhouse *rag,uint32_t action_age,
                       int attention,const WorldBabyMotion *motion){
 unsigned beat=frame%120u;
 const WorldBabyPose rig=rag?baby_pose_sample(s,frame,action,action_age,rag):
   world_baby_pose(s,frame,action,action_age,motion);
 /* Rig motion is staged in one short action. Lower-body bounce, shoulder
  * rotation, hand reach, neck reaction and eyelid shape share this curve. */
 const int weight=rag?256:(action?world_baby_care_weight(action_age):0);
 const int hostile=(action==WORLD_BABY_WINDUP||action==WORLD_BABY_LUNGE)
                    &&weight>40;
 const float breath=rag||!idle_motion_enabled?0.f:(float)world_baby_breath(frame)/256.f;
 const float bob=(rag||idle_motion_enabled?
                  ((float)(beat<60u?beat:120u-beat)-30.f)*.00022f:0.f)+
                 (float)rig.bounce*.0022f/256.f;
 /* Primary body rotation comes from the fixed-point toy simulation. The
  * secondary shoulder/neck reactions are tiny and remain physically
  * connected to the parent model rather than becoming floating parts. */
 const float settle=rag?(float)rag->impact*.0025f:0.f;
 const int lid=rig.lid;
 /* Keep the title and Toybox shell gaze stable. The live nursery uses
  * small horizontal eye excursions within the existing sockets. */
 const int gaze=rag||!idle_motion_enabled?0:world_baby_gaze(frame);
 /* Hungry and exhausted infants should look unsettled even when
  * an old positive mood score hasn't yet caught up with their needs.
  * NAP/FEED restores the same face without adding new visual states. */
 const int happy=!hostile&&s->mood>35&&s->hunger<80&&s->energy>15;
 const int glitch=game_glitch_tier(s)>=2&&!s->safe_visuals&&
                  (frame%151u)<12u;
 glPushMatrix();
 glTranslatef(-.12f,bob-settle,.23f);
 glRotateZi(model_angle(rig.lean)); /* torso and limbs ease together */
 /* Telegraph and lunge pivot the EXISTING connected body at its feet.
  * Keep the camera still, the floor contact fixed and all facial parts
  * parented; no extra mesh, screen shake or full-frame flash is needed. */
 if(action==WORLD_BABY_WINDUP||action==WORLD_BABY_LUNGE){
  int pitch=(action==WORLD_BABY_WINDUP?-4:9)*weight/256;
  glTranslatef(0,-1.08f,.36f);
  glRotateXi(model_angle(pitch));
  glTranslatef(0,1.08f,-.36f);
 }
 /* Footed one-piece romper, short legs and soft shoes: no adult denim. */
 ellipse(0,-.75f,-.07f,.46f,.31f,.34f,17,25,26);
 ellipse(-.32f,-.93f,.14f,.245f,.22f,.27f,16,24,26);
 ellipse(.32f,-.93f,.14f,.245f,.22f,.27f,16,24,26);
 ellipse(-.365f,-1.08f,.36f,.235f,.13f,.24f,29,27,22);
 ellipse(.365f,-1.08f,.36f,.235f,.13f,.24f,29,27,22);
 soft_mark(-.365f,-1.105f,.599f,.25f,.035f,23,24,21);
 soft_mark(.365f,-1.105f,.599f,.25f,.035f,23,24,21);
 /* Layered shadow + warm collar replaces the single white floating coin. */
 /* Preserve shoulder, collar and face attachment: only the upper torso
  * expands by a few percent for a legible but restrained breath. */
 soft_head(0,-.32f,-.08f,.49f*(1.f+.024f*breath),
           .46f*(1.f+.016f*breath),.36f,17,25,26);
 ellipse(0,.084f,-.015f,.27f,.11f,.196f,17,22,21);
 /* A nearly paper-flat, overbright collar ellipsoid was emitting a
  * 75-pixel row of RGB15(31,28,23) through the dresser/cheek. That exact
  * color came from the ellipsoid's terminal GL_QUAD_STRIP band.
  * Replace it with a small, volumetric curved neckline, still attached
  * to the romper, but safely away from the rasterizer's thin-pole case. */
 ellipse(0,.117f,.012f,.176f,.112f,.139f,29,25,21);
 face_oval(0,-.39f,.316f,.235f,.225f,30,29,24,-2);
 /* Four inexpensive stitched glints on the romper's central pocket. */
 soft_mark(0,-.30f,.330f,.10f,.044f,glitch?24:27,glitch?17:23,10);
 soft_mark(-.075f,-.34f,.331f,.055f,.026f,30,25,14);
 soft_mark(.072f,-.34f,.331f,.052f,.026f,30,25,14);
 soft_mark(0,-.38f,.332f,.044f,.027f,30,25,14);
 /* Rotating the whole forearm+hand hierarchy preserves contact with
  * the sleeve in every action and during roughhouse recoil. */
 baby_arm(-1,rig.reach_left,rig.forward_left);
 baby_arm(1,rig.reach_right,rig.forward_right);
 /* Neck pivot moves the entire face, eyes, cheeks, hair, and mouth AS ONE.
  * The free head briefly lags the shell's rolling body by at most 16deg. */
 glPushMatrix();
 if(rig.neck){
  /* Pivot below the jaw, rather than spinning the head around its middle. */
  glTranslatef(0,.10f,0);
  glRotateZi(model_angle(rig.neck));
  glTranslatef(0,-.10f,0);
 }
 /* A tiny eye-line turn keeps head and arms reading as one intention.
  * Avoid large yaw: low-poly eyes/ears remain recognizable in profile. */
 if(rig.yaw)glRotateYi(model_angle(rig.yaw));
 if(attention)glRotateYi(model_angle(attention));
 /* Rounded unified face. No second oversized jaw shell, no floating teeth. */
 soft_head(0,.54f,.045f,.61f,.615f,.45f,30,22,19);
 ellipse(-.575f,.49f,.065f,.105f,.145f,.09f,31,23,19);
 ellipse(.575f,.49f,.065f,.105f,.145f,.09f,31,23,19);
 /* Small inset, warm inner-ear contours remain attached to the head
  * through every neck, gaze and roughhouse pose. */
 face_oval(-.590f,.495f,.155f,.043f,.080f,26,16,14,2);
 face_oval(.590f,.495f,.155f,.043f,.080f,26,16,14,2);
 /* Chestnut cap and two small swept tufts; old long side hair removed. */
 hair_cap(0,.54f,.035f,.625f,.63f,.465f,16,11,8);
 hair_fringe(-.19f,.948f,.44f,.29f,.105f,.045f);
 hair_fringe(.16f,.959f,.438f,.28f,.095f,-.04f);
 hair_fringe(-.015f,1.008f,.436f,.16f,.077f,-.025f);
 decor_relief(-.39f,1.018f,.365f,.075f,.069f,.029f,29,14,20);
 /* Front-facing rounded surfaces follow the skin, with a softly shaded
  * rim instead of four stacked full ellipsoids. Closed eyes retain the
  * same gentle crease and the full face stays on the neck transform. */
 if(lid<64){
  /* Two small joined crease segments per eye form a relaxed low arc.
   * These are safe single quads, not collapsed GL_QUAD_STRIP bands. */
  soft_mark(-.251f,.572f,.532f,.092f,.018f,18,11,10);
  soft_mark(-.160f,.565f,.534f,.092f,.018f,18,11,10);
  soft_mark(.159f,.565f,.534f,.092f,.018f,18,11,10);
  soft_mark(.250f,.572f,.532f,.092f,.018f,18,11,10);
 }else{
  baby_eye(-.205f,.575f,.480f,1.f,lid,gaze);
  baby_eye(.205f,.575f,.480f,1.f,lid,gaze);
 }
 /* Warm cheeks, gentle brows, button nose and a tiny restrained smile. */
 face_oval(-.355f,.365f,.411f,.095f,.050f,31,19,19,2);
 face_oval(.355f,.365f,.411f,.095f,.050f,31,19,19,2);
 if(hostile){
  glPushMatrix();glTranslatef(-.205f,.734f,.459f);
  glRotateZi(model_angle(-16));soft_mark(0,0,0,.105f,.018f,17,11,9);
  glPopMatrix(1);
  glPushMatrix();glTranslatef(.205f,.734f,.459f);
  glRotateZi(model_angle(16));soft_mark(0,0,0,.105f,.018f,17,11,9);
  glPopMatrix(1);
 }else{
  soft_mark(-.205f,.734f,.459f,.105f,.014f,17,11,9);
  soft_mark(.205f,.734f,.459f,.105f,.014f,17,11,9);
 }
 ellipse(0,.375f,.467f,.047f,.034f,.019f,31,23,20);
 if(rag&&rag->impact){
  /* Fresh toy-shell collision: a short shocked open mouth is actually
   * tied to the impact pulse. Once it fades, the same model returns
   * to its ordinary expression. Reuse the CHAT mouth geometry, no
   * new material, sprites, or animation allocations. */
  float gasp=.42f+(float)rag->impact/9.f*.48f;
  baby_mouth(gasp);
 }else if((action==2||action==WORLD_BABY_LUNGE||(happy&&action==3)||
           action==WORLD_BABY_CHEER)&&weight>40){
  /* CHAT syllables must remain visible when the baby is unhappy, too:
   * the nursery PSG voice still babbles at low mood. A smaller grim
   * aperture communicates the changed personality without losing
   * sound/mouth synchronization. Happy PLAY retains its wide grin;
   * a completed win gets a modest smile even when needs are low. */
  float speech=action==2?(float)world_baby_speech_open(action_age)/256.f:1.f;
  float open=action==WORLD_BABY_CHEER?.72f:action==2?
              .12f+(happy?.88f:.59f)*speech:1.f;
  baby_mouth(open);
 }else if(happy||(action==WORLD_BABY_REGROUP&&weight>40)){
  /* A loss receives encouragement through the existing closed smile. */
  soft_mark(-.065f,.278f,.444f,.052f,.017f,18,9,11);
  soft_mark(0,.251f,.454f,.081f,.017f,18,9,11);
  soft_mark(.065f,.278f,.444f,.052f,.017f,18,9,11);
 }else{
  /* Low mood is visible on the real baby rather than only a number
   * downstairs. A gentle three-piece inverted mouth stays on the face,
   * replacing the former flat, unreadable one-quad line. */
  soft_mark(-.064f,.241f,.451f,.050f,.016f,18,9,11);
  soft_mark(0,.269f,.453f,.078f,.016f,18,9,11);
  soft_mark(.064f,.241f,.451f,.050f,.016f,18,9,11);
 }
 glPopMatrix(1); /* attached face and hair pivot */
 glPopMatrix(1); /* whole model */
}
void world_init(void){
 glInit();
 /* Edge AA produces dark cracks between adjacent untextured DS polygons. */
 glDisable(GL_ANTIALIAS);
 glClearColor(27,28,28,31);
 glClearPolyID(63);
 glClearDepth(0x7fff);
 glViewport(0,0,255,191);
 glMatrixMode(GL_PROJECTION);glLoadIdentity();
 gluPerspective(51,256.0f/192.0f,.1f,30);
}
/* Four deliberate in-world object angles, plus the original portrait.
 * Every preset looks into the SAME actual 3D nursery, not a picture
 * carousel or a second low-detail room. A 16-tick smoothstep dolly avoids
 * hard teleports when tapping around the furniture. */
/* All views share one physical nursery; a fixed preset table describes
 * eye and target position only. There is no alternate scene geometry. */
static EggCamera nursery_preset(int focus){
 static const EggCamera preset[5]={
  {.97f,1.05f,3.43f,-.12f,.035f,.17f},  /* baby / default */
  /* Keep the doll and the selected prop in a readable two-subject shot,
   * rather than turning so far left that the bare green wall fills
   * half of the already tiny 256x192 DS picture. */
  {.97f,1.06f,3.43f,-.87f,.88f,-1.83f},/* window + baby */
  {.96f,1.03f,3.37f,-1.36f,.37f,-.39f},/* lamp + dresser */
  {.98f,.84f,3.37f,-.65f,-.43f,.39f},  /* teddy + baby */
  {1.29f,1.12f,3.34f,1.21f,-.15f,-.99f} /* crib + baby */
 };
 return preset[focus>=0&&focus<4?focus+1:0];
}
/* Blend FROM the camera's exact current position, not the previous
 * preset. Rapid WINDOW->TEDDY->CRIB taps must never teleport the lens.
 * The whole motion remains a single smooth 16-tick continuous journey. */
EggCamera world_nursery_camera_from(EggCamera from,int focus,uint32_t age){
 EggCamera to=nursery_preset(focus),c;
 float t;
 /* Return the exact destination, not an extra six float operations,
  * after the 16-tick animation completes; helps stable camera equality. */
 if(age>=16u)return to;
 t=(float)age/16.f;
 t=t*t*(3.f-2.f*t);
 c.x=from.x+(to.x-from.x)*t;c.y=from.y+(to.y-from.y)*t;
 c.z=from.z+(to.z-from.z)*t;
 c.look_x=from.look_x+(to.look_x-from.look_x)*t;
 c.look_y=from.look_y+(to.look_y-from.look_y)*t;
 c.look_z=from.look_z+(to.look_z-from.look_z)*t;
 return c;
}
EggCamera world_nursery_camera(int focus,int previous,uint32_t age){
 return world_nursery_camera_from(nursery_preset(previous),focus,age);
}
void world_draw_nursery(const Game *s,uint32_t frame,int activity,
                        uint32_t action_age,int focus,int from,
                        uint32_t camera_age,int daylight,int lamp,
                        uint32_t prop_age,const EggCamera *camera_start,
                        int attention_start,const WorldBabyMotion *motion){
 EggCamera camera=camera_start?
   world_nursery_camera_from(*camera_start,focus,camera_age):
   world_nursery_camera(focus,from,camera_age);
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 gluLookAt(camera.x,camera.y,camera.z,
           camera.look_x,camera.look_y,camera.look_z,0,1,0);
 (void)camera; /* host GL shim intentionally discards its arguments */
 glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
 frame_vertices=0;frame_polygons=0;last_vertex_color=~0u;
 tour_daylight=daylight!=0;
 tour_lamp_on=lamp!=0;
 /* The window is a meaningful daylight source: daylight now gently
  * illuminates furniture and walls rather than changing only blue glass.
  * Pitch-black sleep conditions remain distinctly dim, without tinting
  * the baby or permanently altering care/egg scenes. */
 nursery_ambient=tour_daylight?(tour_lamp_on?2:1):
                 (tour_lamp_on?0:-3);
 room(world_crib_mobile_swing(focus,prop_age));
 /* Reset visual state BEFORE drawing baby: the lamp affects room surfaces,
  * but must not tint the actual baby or leak into Toybox / egg replay. */
 nursery_ambient=0;
 tour_daylight=0;
 tour_lamp_on=1;
 baby_contact(-.12f,.52f,1.f);
 baby_model(s,frame,activity,0,action_age,
            world_baby_attention_from(attention_start,focus,camera_age),motion);
 if(focus==2&&prop_age<40u){
  /* One *finite* teddy bow. Arms, face and plush fabric remain attached
   * beneath the exact same parent transform. */
  glPushMatrix();
  glTranslatef(-1.28f,-.87f,.52f);
  glRotateZi(model_angle(world_teddy_bow_angle(prop_age)));
  glTranslatef(1.28f,.87f,-.52f);
  teddy();
  glPopMatrix(1);
 }else teddy();
 glFlush(0);
}
void world_draw_care(const Game *s,uint32_t frame,int activity,
                     uint32_t action_age){
 world_draw_care_from(s,frame,activity,action_age,0);
}
void world_draw_care_from(const Game *s,uint32_t frame,int activity,
                          uint32_t action_age,const WorldBabyMotion *motion){
 world_draw_nursery(s,frame,activity,action_age,-1,-1,16u,0,1,40u,0,0,motion);
}
/* Compatibility for existing host callers and noninteractive views: they
 * sample the deliberately posed hold, while live gameplay uses action age. */
void world_draw(const Game *s,uint32_t frame,int activity){
 world_draw_care(s,frame,activity,20u);
}

/* The character is a synthetic toy shell, never a real child. The same
 * mesh swings its attached limbs as it tumbles; the integer simulation
 * supplies 1/256-world-unit motion, so it stays deterministic on DS. */
static void draw_roughhouse(const Game *s,uint32_t frame,const Roughhouse *r,int prologue){
 int tilt;
 float elevation=0.f,shadow=.92f;
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 /* Fracture prologue uses a deliberately frontal, slightly raised
  * portrait. Keep ordinary Toybox framing unchanged. This places the
  * eyes and hair in the focal area instead of losing them to perspective. */
 if(prologue)gluLookAt(.28f,.86f,3.15f,-.12f,.25f,.17f,0,1,0);
 else gluLookAt(.97f,1.05f,3.43f,-.12f,.035f,.17f,0,1,0);
 glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
 frame_vertices=0;frame_polygons=0;last_vertex_color=~0u;
 room(0);
 if(r){
  /* The contact moves WITH the synthetic doll and tightens as it falls.
   * Two clipped-corner tones add no texture VRAM or image operations. */
  shadow=1.f-(float)r->y/850.f;
  if(shadow<.60f)shadow=.60f;
  baby_contact(-.12f+(float)r->x/256.f,
                .52f+(float)r->z/256.f,shadow);
 }else baby_contact(-.12f,.52f,1.f);
 glPushMatrix();
 if(r){
  tilt=r->roll<0?-r->roll:r->roll;
  /* Before: an abrupt .46 world-unit teleport after 90 roll units.
   * Now: continuous contact compensation from an upright pose to a roll.
   * The model's feet never suddenly jump when it rotates past the gate. */
  elevation=(float)tilt*.00075f;
  glTranslatef((float)r->x/256.f,(float)r->y/256.f+elevation,
               (float)r->z/256.f);
  glRotateZi(model_angle((int)r->roll/4));
  glRotateXi(model_angle((int)r->pitch/4));
  glRotateYi(model_angle((int)r->yaw/4));
 }
 baby_model(s,frame,r&&r->moving?5:0,r,0u,0,0);
 if(r&&r->charge>=6){
  /* First fracture: the hospital-pastel chest pocket conceals a seam. */
  soft_mark(-.12f,-.27f,.565f,.029f,.20f,11,13,14);
  soft_mark(-.08f,-.34f,.571f,.079f,.033f,9,14,14);
  if(r->charge>=11){
   /* Second stage: green alien ichor appears INSIDE the artificial shell.
    * Three low-cost decals, no gore sprite or simulated fluid particles. */
   soft_mark(-.11f,-.26f,.580f,.027f,.15f,11,30,10);
   soft_mark(-.085f,-.345f,.580f,.060f,.025f,17,31,9);
   soft_mark(-.04f,-.378f,.580f,.028f,.065f,5,25,7);
  }
 }
 glPopMatrix(1);
 teddy();
 glFlush(0);
}
void world_draw_roughhouse(const Game *s,uint32_t frame,const Roughhouse *r){
 draw_roughhouse(s,frame,r,0);
}
/* One pointed tooth costs one triangle and follows the curved mouth.
 * Fixed, tiny per-frame geometry: never a texture atlas or CPU rasterizer. */
static void fang(float x,float y,float z,float width,float height,int upper){
 glBegin(GL_TRIANGLES);
 color(28,30,23,0);
 glVertex3f(x-width*.5f,y,z);
 glVertex3f(x+width*.5f,y,z);
 color(21,28,20,-1);
 glVertex3f(x,y+(upper?height:-height),z);
 glEnd();
 frame_vertices+=3;frame_polygons++;
}
/* Fixed, sculpted porcelain crescent. The old stretched sphere produced
 * tall rectangular pillars in actual 256x192 DeSmuME images. Each wing is
 * now a tapering, irregular shell with a raised inner cut edge, shaded
 * thickness and two short stress fractures. No trig, buffer, or texture.
 * 14 quads and two cracks per side vs. 60-quads per old ellipsoid. */
static void egg_porcelain_wing(int side,float opened){
 static const float y[8]={-1.01f,-.82f,-.57f,-.23f,
                            .13f,.47f,.78f,1.09f};
 static const float inner[8]={.61f,.45f,.37f,.34f,
                                .38f,.45f,.55f,.72f};
 static const float outer[8]={.75f,.96f,1.09f,1.15f,
                                1.12f,1.02f,.88f,.77f};
 static const int shade[8]={-6,-2,1,3,4,3,0,-4};
 float drift=opened*.18f;
 int i;
 /* This strip's convex outer contour and fractured, concave inner edge
  * form a recognizable open egg even when the head is very close. */
 glBegin(GL_QUAD_STRIP);
 for(i=0;i<8;i++){
  color(29,29,24,shade[i]+1);
  glVertex3f(side*(inner[i]+drift),y[i],
             .65f+(i==2||i==5?.018f:0.f));
  color(27,28,24,shade[i]-3);
  glVertex3f(side*(outer[i]+drift),y[i],.39f);
 }
 glEnd();
 frame_vertices+=16;frame_polygons+=7;
 /* Thickness is the dark organic cut beneath the porcelain, NOT an
  * extra white bar drawn across the opening. */
 glBegin(GL_QUAD_STRIP);
 for(i=0;i<8;i++){
  color(7,14,12,shade[i]/2);
  glVertex3f(side*(inner[i]+drift-.034f),y[i],.705f);
  color(15,23,18,shade[i]/2);
  glVertex3f(side*(inner[i]+drift+.067f),y[i],.649f);
 }
 glEnd();
 frame_vertices+=16;frame_polygons+=7;
 /* Deep cracks on the exposed face, with a short zigzag rather than the
  * prior perfectly straight black streaks. Only 3 tiny surface quads. */
 glBegin(GL_QUADS);
 color(12,15,17,0);
 face(side*(.85f+drift),.58f,.689f,
      side*(.88f+drift),.57f,.689f,
      side*(.82f+drift),.30f,.689f,
      side*(.79f+drift),.31f,.689f);
 face(side*(.82f+drift),.31f,.688f,
      side*(.85f+drift),.30f,.688f,
      side*(.75f+drift),.16f,.688f,
      side*(.72f+drift),.17f,.688f);
 color(18,19,18,0);
 face(side*(.96f+drift),-.39f,.689f,
      side*(.99f+drift),-.40f,.689f,
      side*(.92f+drift),-.60f,.689f,
      side*(.89f+drift),-.58f,.689f);
 glEnd();
 frame_vertices+=12;frame_polygons+=3;
}
/* A few broad, uneven acid-green ribbons make the inner edge appear wet.
 * Align directly to the curved shell profile instead of drawing a long
 * floating neon rectangle. Three-dimensional parallax is preserved. */
static void egg_lip_ichor(int side,float opened){
 static const float y[6]={-.73f,-.40f,-.12f,.19f,.48f,.76f};
 static const float lip[6]={.47f,.37f,.35f,.39f,.46f,.54f};
 static const int gleam[6]={-6,-2,2,5,0,-5};
 int i;
 float drift=opened*.18f;
 glBegin(GL_QUAD_STRIP);
 for(i=0;i<6;i++){
  color(3,24,7,gleam[i]);
  glVertex3f(side*(lip[i]+drift-.020f),y[i],.724f);
  color(17,31,9,gleam[i]);
  glVertex3f(side*(lip[i]+drift+.036f),y[i],.729f);
 }
 glEnd();
 frame_vertices+=12;frame_polygons+=5;
}

/* The alien shell leaks tapering dark-green fluid rather than a long
 * constant-width fluorescent rectangle when the camera pushes close.
 * The top of each rivulet stays attached to its inner shell lip.
 * Two colored triangles per side; no new meshes or frame buffers. */
static void egg_ichor_drip(int side,float opened){
 float s=(float)side,drift=opened*.17f;
 float x=.269f+drift;
 float y=side<0?-.205f:-.165f;
 float length=.30f+opened*.46f;
 glBegin(GL_TRIANGLES);
 color(6,27,9,0);
 glVertex3f(s*(x-.064f),y,.719f);
 color(8,29,10,0);
 glVertex3f(s*(x+.040f),y-.028f,.716f);
 color(2,12,5,0);
 glVertex3f(s*(x+.066f),y-length,.688f);
 color(3,18,7,0);
 glVertex3f(s*(x-.025f),y-.090f,.721f);
 color(7,25,9,0);
 glVertex3f(s*(x+.048f),y-.075f,.722f);
 color(2,11,5,0);
 glVertex3f(s*(x+.082f),y-length*.77f,.694f);
 glEnd();
 frame_vertices+=6;frame_polygons+=2;
}
/* Eight hand-placed triangular planes sculpt the brow, cheeks and lower
 * jaw. Deliberate asymmetric light makes the face feel carved instead of a
 * flat turquoise emoji. Native GL vertex colors supply a cheap fake rim
 * light, while the shell/eye palette carries the neon contrast. */
static void alien_face_facets(void){
 glBegin(GL_TRIANGLES);
 /* Brow planes sweep down toward the fixed eye sockets. */
 color(9,20,17,1);glVertex3f(-.72f,.53f,.720f);
 color(16,25,19,2);glVertex3f(-.38f,.65f,.790f);
 color(7,15,16,-2);glVertex3f(-.42f,.42f,.856f);
 color(9,20,17,1);glVertex3f(.72f,.53f,.720f);
 color(16,25,19,2);glVertex3f(.38f,.65f,.790f);
 color(7,15,16,-2);glVertex3f(.42f,.42f,.856f);
 /* Back cheek planes bridge the broad head to the mouth corners. */
 color(7,17,17,-2);glVertex3f(-.89f,.06f,.612f);
 color(12,24,18,1);glVertex3f(-.62f,.06f,.819f);
 color(4,10,12,-2);glVertex3f(-.54f,-.38f,.800f);
 color(7,17,17,-2);glVertex3f(.89f,.06f,.612f);
 color(12,24,18,1);glVertex3f(.62f,.06f,.819f);
 color(4,10,12,-2);glVertex3f(.54f,-.38f,.800f);
 /* Lower jaw panels keep teeth and tongue visually attached to skull. */
 color(6,16,15,0);glVertex3f(-.66f,-.30f,.70f);
 color(12,22,16,1);glVertex3f(-.49f,-.70f,.78f);
 color(4,10,12,-2);glVertex3f(-.23f,-.90f,.69f);
 color(6,16,15,0);glVertex3f(.66f,-.30f,.70f);
 color(12,22,16,1);glVertex3f(.49f,-.70f,.78f);
 color(4,10,12,-2);glVertex3f(.23f,-.90f,.69f);
 glEnd();
 frame_vertices+=18;frame_polygons+=6;
}
/* Tapered organic cheek rivulets in a single connected facial rig.
 * The earlier long screen-aligned bright rectangles read like graphical
 * glitches in native 256x192 DeSmuME. Three small triangles per side
 * produce asymmetric, shaded attachments with no streak-like strip,
 * heap allocation, sprite atlas, framebuffer, or trigonometry. */
static void alien_ichor_rivulet(int side){
 float s=(float)side,dy=side<0?0.f:.035f;
 glBegin(GL_TRIANGLES);
 color(7,26,11,0);
 glVertex3f(s*.625f,-.205f+dy,.798f);
 color(11,27,12,0);
 glVertex3f(s*.707f,-.240f+dy,.795f);
 color(4,15,8,0);
 glVertex3f(s*.745f,-.615f+dy,.776f);
 color(5,18,10,0);
 glVertex3f(s*.660f,-.290f+dy,.803f);
 color(8,26,12,0);
 glVertex3f(s*.731f,-.330f+dy,.793f);
 color(3,13,8,0);
 glVertex3f(s*.792f,-.465f+dy,.761f);
 color(14,29,13,0);
 glVertex3f(s*.680f,-.218f+dy,.811f);
 color(9,22,10,0);
 glVertex3f(s*.697f,-.232f+dy,.811f);
 color(4,14,8,0);
 glVertex3f(s*.736f,-.375f+dy,.799f);
 glEnd();
 frame_vertices+=9;frame_polygons+=3;
}
/* Purpose-built visual hierarchy for the secret BAD ending:
 *   porcelain shell / shadowed inner lip / black incubation chamber /
 *   clearly silhouetted parasite / thin luminous green ichor.
 * All surfaces are hardware vertices (no extra textures, heap, framebuffer,
 * animation buffers or random particles). This is synthetic alien material. */
/* During the slow approach, the same nursery walls, window, crib and
 * floor retain their positions. A continuous lateral dolly creates real
 * parallax; only the final scare intentionally hard-cuts. Camera arithmetic
 * is constant-time and never allocates a new scene or framebuffer. */
/* The room gradually loses light as the egg is approached, but never
 * vanishes: recognizable objects stay in their original 3D positions.
 * A five-step 5-bit palette dim is cheap and leaves contrast for geometry. */
int world_egg_nursery_ambient(uint32_t frame){
 uint32_t elapsed,span;
 if(frame<=EGG_CRACK_FRAME||frame>=EGG_SCARE_FRAME)return 0;
 span=EGG_SCARE_FRAME-EGG_CRACK_FRAME-1u;
 elapsed=frame-EGG_CRACK_FRAME;
 return -(int)((elapsed*5u+span/2u)/span);
}
EggCamera world_secret_camera(uint32_t frame){
 EggCamera c;
 float travel,settle;
 if(frame<EGG_CRACK_FRAME)frame=EGG_CRACK_FRAME;
 if(frame>=EGG_SCARE_FRAME){
  c.x=0.f;c.y=.05f;c.z=2.63f;
  /* The previous upward aim cropped the LOWER jaw off the tiny top LCD.
   * Aim nearer the final dolly's low target; both eyes and lower fangs
   * stay inside the 192-pixel display when the head lunges. */
  c.look_x=0.f;c.look_y=-.11f;c.look_z=.40f;
  return c;
 }
 /* Two deliberately framed camera beats: a first drift from the actual
  * nursery into the shell, then a longer, slower invasive push after the
  * creature wakes. Both segments arrive at rest at PUSH: no lens jerk,
  * new camera, extra framebuffer or random timing on replay. */
 if(frame<=EGG_PUSH_FRAME){
  travel=(float)(frame-EGG_CRACK_FRAME)/
         (float)(EGG_PUSH_FRAME-EGG_CRACK_FRAME);
  settle=.30f*travel*travel*(3.f-2.f*travel);
 }else{
  travel=(float)(frame-EGG_PUSH_FRAME)/
         (float)(EGG_SCARE_FRAME-EGG_PUSH_FRAME);
  settle=.30f+.70f*travel*travel*(3.f-2.f*travel);
 }
 c.x=.94f-.90f*settle;
 c.y=.93f-.83f*settle;
 c.z=3.46f-2.06f*settle;
 c.look_x=-.12f+.12f*settle;
 c.look_y=.035f-.12f*settle;
 c.look_z=.17f+.25f*settle;
 return c;
}
/* A deliberate single lunge on the surprise cut, followed by subtle
 * settling/breathing. Rigidly moves the ENTIRE head with its eyes and fangs,
 * rather than scaling detached facial components or flashing the screen.
 * Pure bounded frame math allows native regression tests and pause/replay. */
float world_egg_creature_depth(uint32_t frame){
 uint32_t elapsed,beat;
 if(frame<EGG_SCARE_FRAME)return 0.f;
 elapsed=frame-EGG_SCARE_FRAME;
 /* One unmistakable lunge: surge for nine frames, then deliberately
  * withdraw. A continuous idle pulse follows, not repeated jump cuts. */
 if(elapsed<=9u)return (float)elapsed*(.31f/9.f);
 if(elapsed<=22u)return .31f-(float)(elapsed-9u)*.0075f;
 beat=(elapsed-22u)%40u;
 return .2125f+(float)(beat<20u?beat:40u-beat)*.0004f;
}
/* A barely perceptible pulse inside the shell; the embryonic eyes and
 * mouth share one pivot so nothing floats away from the skull. */
/* One physical lower-jaw hinge, not individually bouncing teeth.
 * The mouth opens on the first lunge, then rests partially agape. This is
 * deterministic, so pause, replay and frame skipping stay coherent. */
int world_egg_jaw_angle(uint32_t frame){
 uint32_t elapsed,beat;
 if(frame<EGG_SCARE_FRAME)return 0;
 elapsed=frame-EGG_SCARE_FRAME;
 if(elapsed<=9u)return (int)elapsed*2;
 if(elapsed<=22u)return 18-(int)((elapsed-9u)*10u/13u);
 beat=(elapsed-22u)%40u;
 return 8+(int)(beat<20u?beat/10u:(40u-beat)/10u);
}
float world_egg_embryo_breath(uint32_t frame){
 uint32_t phase=frame%64u;
 return (float)(phase<32u?phase:64u-phase)*.0005f;
}
/* The egg is observing the PLAYER, not merely rocking in place.
 * Pure integer gaze keyframes keep both pupils and their catchlights on
 * a shared motion track: look aside, recognize the approaching camera,
 * then lock dead ahead during the lunge. 1 unit is 0.005 world units in
 * the close-up and smaller in the distant embryo shot. */
int world_egg_gaze(uint32_t frame){
 uint32_t age,phase;
 if(frame<EGG_PUSH_FRAME)return 0;
 if(frame<EGG_SCARE_FRAME){
  age=frame-EGG_PUSH_FRAME;
  if(age<12u)return -(int)age;
  if(age<48u)return -12;
  if(age<72u)return -12+(int)((age-48u)/2u);
  if(age<112u)return 0;
  return (int)((age-112u)*8u/
               (EGG_SCARE_FRAME-EGG_PUSH_FRAME-112u));
 }
 age=frame-EGG_SCARE_FRAME;
 if(age<9u)return 8-(int)(age*8u/9u);
 if(age<23u)return 0;
 phase=(age-23u)%64u;
 return (int)((phase<32u?phase:64u-phase)/11u)-1;
}
/* A low-frequency cavity membrane motion. The dark sac remains part of
 * the same fixed geometry; no independent floating object or particles. */
float world_egg_membrane_pulse(uint32_t frame){
 uint32_t phase;
 if(frame<EGG_CRACK_FRAME||frame>=EGG_SCARE_FRAME)return 0.f;
 phase=(frame-EGG_CRACK_FRAME)%64u;
 return (float)(phase<=32u?phase:64u-phase)*(.028f/32.f);
}
/* The creature pulls back for the last eighteen film ticks. The camera
 * continues forward and the green eyes dim once before the hard cut.
 * This motion is smooth, bounded and only affects the whole embryo rig. */
float world_egg_embryo_retreat(uint32_t frame){
 float t;
 if(frame<EGG_SCARE_FRAME-18u||frame>=EGG_SCARE_FRAME)return 0.f;
 t=(float)(frame-(EGG_SCARE_FRAME-18u))/18.f;
 return .080f*t*t*(3.f-2.f*t);
}
/* The eye glow is a deliberate story beat, not a full-screen flash.
 * Dim during fracture, brighten as the camera finds the cavity, then
 * recede for the final 20 frames before the single surprise cut. */
int world_egg_eye_glow(uint32_t frame){
 uint32_t remaining,elapsed;
 int glow;
 if(frame<EGG_CRACK_FRAME)return 0;
 if(frame<EGG_PUSH_FRAME)return 7;
 if(frame>=EGG_SCARE_FRAME)return 0;
 remaining=EGG_SCARE_FRAME-frame;
 if(remaining<=20u)return 3+(int)(remaining*28u/20u);
 elapsed=frame-EGG_PUSH_FRAME;
 glow=7+(int)(elapsed*24u/(EGG_SCARE_FRAME-EGG_PUSH_FRAME-20u));
 return glow>31?31:glow;
}
/* Embryo comes forward inside the shell, not through its porcelain lips.
 * Its whole face, eyes and jaw use the same parent model matrix. */
/* Organic shell breach: cubic smoothstep has zero velocity at each
 * film beat, unlike the old linear opening with abrupt start/stop. */
float world_egg_shell_open(uint32_t frame){
 float t;
 if(frame<=EGG_PUSH_FRAME)return 0.f;
 if(frame>=EGG_SCARE_FRAME)return 1.f;
 t=(float)(frame-EGG_PUSH_FRAME)/
   (float)(EGG_SCARE_FRAME-EGG_PUSH_FRAME);
 return t*t*(3.f-2.f*t);
}
/* A restrained asymmetric whole-head reaction. All eyes/teeth/jaw move
 * under the SAME model matrix; no detached moving features or jitter. */
int world_egg_head_tilt(uint32_t frame){
 uint32_t elapsed,beat;
 if(frame<EGG_SCARE_FRAME)return 0;
 elapsed=frame-EGG_SCARE_FRAME;
 if(elapsed<=9u)return -3+(int)(elapsed*5u/9u);
 if(elapsed<=22u)return 2-(int)((elapsed-9u)*2u/13u);
 beat=(elapsed-22u)%80u;
 return (int)(beat<40u?beat/20u:(80u-beat)/20u)-1;
}
float world_egg_embryo_approach(uint32_t frame){
 float t;
 if(frame<=EGG_PUSH_FRAME)return 0.f;
 if(frame>=EGG_SCARE_FRAME)return .0448f;
 t=(float)(frame-EGG_PUSH_FRAME)/
   (float)(EGG_SCARE_FRAME-EGG_PUSH_FRAME);
 /* Ease both ends instead of snapping from a stationary creature into
  * an immediately moving one, or abruptly halting mid-breath. */
 return .0448f*t*t*(3.f-2.f*t);
}
/* Stage the fracture as a readable close-to-frontal nursery portrait.
 * A sixth hit might have flung the ragdoll behind the camera or turned its
 * hair into an enormous brown disk. That impulse remains in toy physics,
 * but the FILM itself should not inherit a random off-camera silhouette.
 * This copy is transient: never modifies the game state, saves or Toybox. */
Roughhouse world_egg_staged_doll(const Roughhouse *source,uint32_t frame){
 Roughhouse pose={0};
 int beat=(int)(frame%24u);
 if(source)pose=*source;
 if(beat>12)beat=24-beat;
 pose.x=0;pose.y=0;pose.z=0;
 pose.vx=0;pose.vy=0;pose.vz=0;
 pose.vr=0;pose.vp=0;pose.vyaw=0;
 pose.roll=(int16_t)((beat-6)*3);
 pose.pitch=0;pose.yaw=0;
 pose.moving=0;pose.impact=0;
 /* The damaged shell must remain open for the story intro even on a
  * scrapbook replay that has no active physics object. */
 if(pose.charge<ROUGHHOUSE_CRACK_CHARGE)
  pose.charge=ROUGHHOUSE_CRACK_CHARGE;
 return pose;
}
void world_draw_secret(const Game *s,uint32_t frame,const Roughhouse *r){
 unsigned phase;
 int i;
 EggCamera camera;
 float opened=0.f;
 if(frame<EGG_CRACK_FRAME){
  Roughhouse staged=world_egg_staged_doll(r,frame);
  draw_roughhouse(s,frame,&staged,1);
  return;
 }
 phase=frame<EGG_PUSH_FRAME?1u:frame<EGG_SCARE_FRAME?2u:3u;
 camera=world_secret_camera(frame);
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 gluLookAt(camera.x,camera.y,camera.z,
           camera.look_x,camera.look_y,camera.look_z,0,1,0);
 /* The host geometry stub elides gluLookAt arguments entirely. */
 (void)camera;
 glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
 frame_vertices=0;frame_polygons=0;last_vertex_color=~0u;
 /* Keep the real nursery where the player last saw it: the patterned
  * rug, window and crib provide persistent parallax behind the synthetic
  * shell. Final scare still cuts to a stark silhouette for contrast. */
 /* Keep the nursery around the alien egg for the COMPLETE film, not
  * only the first three acts. The previous final cut erased the entire
  * room and made the face look like a floating abstract polygon head. */
 nursery_ambient=phase==3u?-6:world_egg_nursery_ambient(frame);
 room(0);
 nursery_ambient=0; /* never bleed cinematic dimming into care scenes */
 if(phase<3u){
  if(phase==2u)opened=world_egg_shell_open(frame);
  block(0,-1.09f,-.26f,1.91f,.13f,1.45f,9,13,13);
  flat_floor(0,-1.016f,-.23f,1.67f,1.18f,5,7,8);
  /* The dark backing is a SINGLE continuous incubation sac, not pale
   * geometry blocking the center of the egg. */
  soft_head(0,-.14f,-.015f,.79f,.91f,.42f,12,18,18);
  soft_head(0,-.15f,.32f,.50f+world_egg_membrane_pulse(frame),
            .76f,.16f,9,24,13);
  soft_head(0,-.13f,.42f,.42f,.67f,.20f,1,3,7);
  /* A subtle moving silhouette inside makes the chamber feel inhabited. */
  {
   float sway=phase==2u?opened*.032f:0.f;
   float gaze=(float)world_egg_gaze(frame)*.0014f;
   int glow=world_egg_eye_glow(frame);
   glPushMatrix();
   glTranslatef(0.f,world_egg_embryo_breath(frame),
                world_egg_embryo_approach(frame)-
                world_egg_embryo_retreat(frame));
   soft_head(sway,-.17f,.54f,.23f,.42f,.10f,5,11,12);
   /* Dim eyes awaken gradually as the camera drifts toward the recess.
    * Both irises remain parented to the creature, not floating in room
    * space. A momentary dimness before the hard cut builds anticipation. */
   ellipse(-.090f+sway,-.07f,.652f,.055f,.044f,.012f,
           5+glow*19/31,7+glow*23/31,5+glow*12/31);
   ellipse(.096f+sway,-.07f,.652f,.055f,.044f,.012f,
           5+glow*19/31,7+glow*23/31,5+glow*12/31);
   soft_mark(-.086f+sway+gaze,-.066f,.667f,.025f,.042f,3,6,7);
   soft_mark(.099f+sway+gaze,-.066f,.667f,.025f,.042f,3,6,7);
   soft_mark(sway,-.36f,.650f,.083f,.15f,0,1,3);
   /* Catchlights follow the same rise and fall as the eye surfaces,
    * without an extra polygon or high-frequency strobe effect. */
   soft_mark(-.104f+sway+gaze,-.092f,.682f,.038f,.026f,
             7+glow*16/31,9+glow*22/31,4+glow*6/31);
   soft_mark(.078f+sway+gaze,-.092f,.682f,.038f,.026f,
             7+glow*16/31,9+glow*22/31,4+glow*6/31);
   glPopMatrix(1);
  }
  /* Unlike a sphere elongated vertically, the new concave crescent has a
   * genuine pointed tip, chipped inner edges and an acid-lit cavity lip. */
  egg_porcelain_wing(-1,opened);
  egg_porcelain_wing(1,opened);
  egg_lip_ichor(-1,opened);
  egg_lip_ichor(1,opened);
  if(phase==2u){
   /* Attached taper, not two neon rectangles reaching off-screen. */
   egg_ichor_drip(-1,opened);
   egg_ichor_drip(1,opened);
   ellipse(-.28f,-.94f,.64f,.19f,.038f,.050f,8,28,8);
   ellipse(.27f,-.94f,.63f,.14f,.026f,.045f,14,31,10);
  }
 }else{
  /* The EXACT same shell now holds together across the hard camera cut.
   * The hero portrait emerges FROM the cracked cavity; never a floating
   * mask between unrelated white columns. */
  block(0,-1.08f,-.28f,2.10f,.16f,1.44f,10,13,12);
  soft_head(0,-.10f,-.08f,.86f,1.12f,.23f,1,3,7);
  egg_porcelain_wing(-1,1.f);
  egg_porcelain_wing(1,1.f);
  egg_lip_ichor(-1,1.f);
  egg_lip_ichor(1,1.f);
  /* The reveal is a DESIGNED creature portrait rather than overlapping
   * random ellipsoids: cranium, socketed eyes, slit pupils, jaw and teeth.
   * A single parent transform lunges ALL facial components together. */
  glPushMatrix();
  glTranslatef(0.f,0.f,world_egg_creature_depth(frame));
  glRotateZi(model_angle(world_egg_head_tilt(frame)));
  soft_head(0,.085f,.34f,1.00f,1.08f,.51f,5,10,12);
  soft_head(0,.68f,.36f,.66f,.29f,.28f,7,15,14);
  alien_face_facets();
  /* Raised brow masses visually lock the sockets to the head. */
  tilted_ellipse(-.37f,.51f,.79f,.34f,.105f,.055f,-12,8,18,16);
  tilted_ellipse(.38f,.50f,.79f,.34f,.105f,.055f,12,8,18,16);
  /* Deep black eye wells with bright sideways sclera, vertical irises
   * and one highlight apiece. Shadows are OUTSIDE the lit pupils. */
  ellipse(-.36f,.28f,.832f,.286f,.192f,.048f,1,3,6);
  ellipse(.36f,.28f,.832f,.286f,.192f,.048f,1,3,6);
  ellipse(-.36f,.276f,.887f,.195f,.117f,.022f,17,31,7);
  ellipse(.36f,.276f,.887f,.195f,.117f,.022f,17,31,7);
  /* Gaze pulls both irises and catchlights together: first sideways,
   * then straight into the camera as the skull lunges. The original
   * small convergence remains, and both pupils stay inside their
   * bright eye surfaces at every frame (no detached glints). */
  {
   float focus=world_egg_creature_depth(frame)*.08f;
   float gaze=(float)world_egg_gaze(frame)*.005f;
   ellipse(-.346f+focus+gaze,.278f,.918f,.062f,.117f,.010f,1,4,5);
   ellipse(.374f-focus+gaze,.278f,.918f,.062f,.117f,.010f,1,4,5);
   soft_mark(-.398f+focus+gaze,.325f,.934f,.044f,.031f,31,31,24);
   soft_mark(.324f-focus+gaze,.325f,.934f,.044f,.031f,31,31,24);
  }
  /* Nose bridge and cheek masses define where the mouth belongs. */
  soft_mark(0,-.002f,.879f,.074f,.26f,14,22,15);
  ellipse(-.60f,-.18f,.71f,.19f,.17f,.055f,7,16,13);
  ellipse(.60f,-.18f,.71f,.19f,.17f,.055f,7,16,13);
  /* The yawning black mouth is a recognisable cavity with an outer
   * gum line, not a red rectangle or detached floating teeth. */
  ellipse(0,-.47f,.820f,.425f,.415f,.070f,16,5,11);
  ellipse(0,-.47f,.892f,.340f,.344f,.034f,1,2,4);
  /* Layer the oral cavity INTO the face, not a detached overlay: dark
   * throat at back, rosy tongue lower down and a single wet highlight.
   * All parts share the SAME scare-lunge parent transform. */
  ellipse(0,-.50f,.925f,.17f,.212f,.020f,3,1,5);
  /* Upper fangs stay locked to the cranium. */
  for(i=0;i<5;i++){
   float x=-.245f+(float)i*.122f;
   float tip=(i&1)?.18f:.25f;
   fang(x,-.215f,.953f,.085f,tip,0);
  }
  /* Lower fangs, gums and tongue all rotate around one cheek-level hinge.
   * They therefore cannot float apart when the head lunges. */
  glPushMatrix();
  /* A large X rotation pushes the lower gums INTO the dark mouth plane
   * and makes them flash as a bright floating bar on DeSmuME. Keep the
   * lower-jaw assembly in front of the throat and let it drop as ONE
   * connected unit. The visible pitch is deliberately subtle. */
  glTranslatef(0.f,-(float)world_egg_jaw_angle(frame)*.003f,.004f);
  glRotateXi(model_angle(-world_egg_jaw_angle(frame)/6));
  ellipse(0,-.68f,.931f,.195f,.079f,.019f,22,6,13);
  soft_mark(-.04f,-.692f,.952f,.080f,.018f,30,14,21);
  ellipse(0,-.65f,.930f,.195f,.074f,.018f,19,5,13);
  fang(-.23f,-.75f,.956f,.098f,.19f,1);
  fang(-.08f,-.775f,.956f,.082f,.14f,1);
  fang(.09f,-.775f,.956f,.080f,.15f,1);
  fang(.22f,-.75f,.956f,.092f,.18f,1);
  glPopMatrix(1);
  /* Attached sickly cheek and forehead plates create a stronger faceted
   * silhouette like the target art, without multiplying face meshes. */
  soft_mark(-.67f,.07f,.790f,.10f,.15f,7,21,16);
  soft_mark(.67f,.07f,.790f,.10f,.15f,7,21,16);
  /* Removed the obsolete bright center forehead strip: real DeSmuME
   * measured a 28-pixel fluorescent ruler at y=8 with exact RGB15
   * (11,27,12). The broad brow masses and two compact reveal glints
   * already establish the intended skull shape without this artifact. */
  /* Tapered cheek fluid, rather than two bright vertical rulers. The
   * shorter under-cheek reflections connect the rivulets to the skull. */
  alien_ichor_rivulet(-1);
  alien_ichor_rivulet(1);
  soft_mark(-.67f,-.64f,.793f,.19f,.062f,10,31,11);
  soft_mark(.67f,-.56f,.793f,.16f,.054f,11,31,12);
  /* Two short highlights follow the curved brow instead of the old
   * 0.65-unit-wide, subpixel-thin horizontal light ruler. One brief
   * glint on the reveal, never a repeated full-screen flash. */
  if(frame<EGG_SCARE_FRAME+4u){
   soft_mark(-.26f,.735f,.845f,.14f,.065f,28,31,24);
   soft_mark(.23f,.722f,.845f,.12f,.065f,25,29,22);
  }
  glPopMatrix(1);
 }
 glFlush(0);
}
/* Tiny boot/title scene: do not submit the full ~1800-polygon nursery before
 * the player confirms the program reached its first interactive screen. */
/* The lightweight pregame portrait uses the SAME baby design language as
 * the nursery hero. It must remain inexpensive enough to render at boot. */
void world_draw_title(uint32_t frame){
 unsigned phase=frame%100u;
 const float bob=(float)(phase<50u?phase:100u-phase)*.00045f;
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 /* Frame the complete baby rather than a large head above a plinth. */
 gluLookAt(0.0f,.18f,4.35f,0.0f,-.22f,-.12f,0,1,0);
 glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
 frame_vertices=0;frame_polygons=0;last_vertex_color=~0u;
 block(0,-.12f,-1.89f,6.8f,4.4f,.08f,21,27,30);
 block(0,-1.24f,-.68f,6.2f,.16f,2.3f,25,22,29);
 /* Remove the two tall pink pillars that looked attached to its head. */
 wall_sticker(-1.24f,.82f,-1.818f,.39f,.38f,31,26,12);
 wall_sticker(1.26f,.88f,-1.818f,.35f,.35f,31,22,17);
 block(0,-1.10f,-.84f,2.9f,.16f,1.60f,24,20,24);
 glPushMatrix();
 glTranslatef(0,bob,0);
 /* All costume shapes are original native DS 3D geometry. The sleeves
  * overlap the romper, and both mittens attach to their forearms. */
 ellipse(0,-.75f,-.23f,.55f,.40f,.39f,21,29,27);
 tilted_ellipse(-.56f,-.64f,-.17f,.19f,.25f,.19f,-23,21,29,27);
 tilted_ellipse(.56f,-.64f,-.17f,.19f,.25f,.19f,23,21,29,27);
 ellipse(-.74f,-.83f,-.01f,.125f,.13f,.13f,31,23,19);
 ellipse(.74f,-.83f,-.01f,.125f,.13f,.13f,31,23,19);
 block(-.28f,-1.10f,.08f,.32f,.15f,.35f,29,27,22);
 block(.28f,-1.10f,.08f,.32f,.15f,.35f,29,27,22);
 face_oval(0,-.55f,.173f,.20f,.145f,30,29,24,-2);
 soft_head(0,.045f,-.18f,.745f,.75f,.53f,31,24,20);
 hair_cap(0,.045f,-.175f,.76f,.77f,.55f,16,11,8);
 hair_fringe(-.25f,.54f,.30f,.30f,.09f,.025f);
 hair_fringe(.17f,.54f,.29f,.28f,.09f,-.03f);
 hair_fringe(-.015f,.59f,.305f,.16f,.074f,-.027f);
 baby_eye(-.225f,.057f,.333f,1.08f,256,0);
 baby_eye(.225f,.057f,.333f,1.08f,256,0);
 face_oval(-.43f,-.205f,.282f,.080f,.043f,31,19,19,2);
 face_oval(.43f,-.205f,.282f,.080f,.043f,31,19,19,2);
 soft_mark(0,-.27f,.337f,.14f,.023f,18,9,11);
 soft_mark(0,-.254f,.343f,.09f,.013f,31,20,19);
 glPopMatrix(1);
 glFlush(0);
}

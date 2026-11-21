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
static void color(int r,int g,int b,int shift){
 glColor(RGB15(limit(r+shift+nursery_ambient),
               limit(g+shift+nursery_ambient),
               limit(b+shift+nursery_ambient)));
}
static void face(float a,float b,float c,float d,float e,float f,
                 float g,float i,float j,float k,float l,float m){
 glVertex3f(a,b,c);glVertex3f(d,e,f);glVertex3f(g,i,j);glVertex3f(k,l,m);
}
static unsigned frame_vertices,frame_polygons;
static void block(float x,float y,float z,float width,float height,float depth,
                  int r,int g,int b){
 float l=x-width*.5f,rr=x+width*.5f,bot=y-height*.5f,top=y+height*.5f;
 float bk=z-depth*.5f,fr=z+depth*.5f;
 glBegin(GL_QUADS);
 color(r,g,b,2);face(l,bot,fr,rr,bot,fr,rr,top,fr,l,top,fr);
 color(r,g,b,-3);face(rr,bot,bk,l,bot,bk,l,top,bk,rr,top,bk);
 color(r,g,b,-2);face(l,bot,bk,l,bot,fr,l,top,fr,l,top,bk);
 color(r,g,b,-6);face(rr,bot,fr,rr,bot,bk,rr,top,bk,rr,top,fr);
 color(r,g,b,4);face(l,top,fr,rr,top,fr,rr,top,bk,l,top,bk);
 color(r,g,b,-8);face(l,bot,bk,rr,bot,bk,rr,bot,fr,l,bot,fr);
 glEnd();
 frame_vertices+=24;
 frame_polygons+=6;
}
/* Surface-only decal: ideal for painted floor grain, wallpaper and fabric.
 * Replaces six-faced boxes with one flat quad in dedicated DS hardware.
 */
static void flat_floor(float x,float y,float z,float sx,float sz,
                       int r,int g,int b){
 glBegin(GL_QUADS);
 color(r,g,b,0);
 face(x-sx*.5f,y,z-sz*.5f,
      x+sx*.5f,y,z-sz*.5f,
      x+sx*.5f,y,z+sz*.5f,
      x-sx*.5f,y,z+sz*.5f);
 glEnd();
 frame_vertices+=4;frame_polygons++;
}
static void wall_sticker(float x,float y,float z,float sx,float sy,
                         int r,int g,int b){
 glBegin(GL_QUADS);
 color(r,g,b,0);
 face(x-sx*.5f,y-sy*.5f,z,
      x+sx*.5f,y-sy*.5f,z,
      x+sx*.5f,y+sy*.5f,z,
      x-sx*.5f,y+sy*.5f,z);
 glEnd();
 frame_vertices+=4;frame_polygons++;
}
/* Simple graphic on the inside left wall; the normal back-wall decal
 * cannot be rotated there. One quad per field, not a six-faced box. */
static void side_wall_mark(float x,float y,float z,float sz,float sy,
                           int r,int g,int b){
 glBegin(GL_QUADS);
 color(r,g,b,0);
 face(x,y-sy*.5f,z-sz*.5f,
      x,y-sy*.5f,z+sz*.5f,
      x,y+sy*.5f,z+sz*.5f,
      x,y+sy*.5f,z-sz*.5f);
 glEnd();
 frame_vertices+=4;frame_polygons++;
}
/* DS-native four-corner lighting. The hardware interpolates vertex
 * colors across these TWO quads: a warm lamp-side wall, a colder moonlit
 * upper-right wall and floor. Much richer depth than one beige plane,
 * without lightmaps, alpha buffers, textures or per-pixel CPU drawing. */
static void nursery_light_planes(void){
 glBegin(GL_QUADS);
 color(30,24,21,0);glVertex3f(-3.29f,-1.07f,-3.131f);
 color(27,24,26,0);glVertex3f(3.30f,-1.07f,-3.131f);
 color(22,19,27,0);glVertex3f(3.30f,2.53f,-3.131f);
 color(26,19,25,0);glVertex3f(-3.29f,2.53f,-3.131f);
 glEnd();
 glBegin(GL_QUADS);
 color(26,21,18,0);glVertex3f(-3.28f,-1.237f,3.28f);
 color(25,23,23,0);glVertex3f(3.28f,-1.237f,3.28f);
 color(24,22,25,0);glVertex3f(3.28f,-1.237f,-3.09f);
 color(29,23,20,0);glVertex3f(-3.28f,-1.237f,-3.09f);
 glEnd();
 frame_vertices+=8;frame_polygons+=2;
}
/* A rail has three visible faces rather than all six box faces. */
static void crib_rail(float x,float y,float z,float width,float height,
                      float depth,int r,int g,int b){
 float lx=x-width*.5f,rx=x+width*.5f,lo=y-height*.5f,hi=y+height*.5f;
 float fr=z+depth*.5f,bk=z-depth*.5f;
 glBegin(GL_QUADS);
 color(r,g,b,2);face(lx,lo,fr,rx,lo,fr,rx,hi,fr,lx,hi,fr);
 color(r,g,b,-4);face(rx,lo,fr,rx,lo,bk,rx,hi,bk,rx,hi,fr);
 color(r,g,b,4);face(lx,hi,fr,rx,hi,fr,rx,hi,bk,lx,hi,bk);
 glEnd();
 frame_vertices+=12;frame_polygons+=3;
}
/* GL_QUAD_STRIP shares ring vertices between adjacent polygons. This
 * reduces ellipsoid submissions from 96 to 54 vertices at the SAME 8
 * angular sides / 24 polygons. Critical for the DS's 6144 vertex cap.
 */
static void ellipse(float x,float y,float z,float rx,float ry,float rz,
                    int red,int green,int blue){
 int a,i,j;
 for(a=0;a<3;a++){
  glBegin(GL_QUAD_STRIP);
  for(i=0;i<=8;i++){
   j=i&7;
   color(red,green,blue,(j<3?2:j<5?-3:0)+(a==2?1:-1));
   glVertex3f(x+rx*w[a]*cx[j],y+ry*h[a],z+rz*w[a]*cz[j]);
   glVertex3f(x+rx*w[a+1]*cx[j],y+ry*h[a+1],z+rz*w[a+1]*cz[j]);
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
 int a,i,j;
 for(a=0;a<5;a++){
  glBegin(GL_QUAD_STRIP);
  for(i=0;i<=12;i++){
   j=i%12;
   color(red,green,blue,light[j]+(a>=3?2:a==0?-3:0));
   glVertex3f(x+rx*radius[a]*ring_x[j],
              y+ry*v[a],z+rz*radius[a]*ring_z[j]);
   color(red,green,blue,light[j]+(a>=2?2:-2));
   glVertex3f(x+rx*radius[a+1]*ring_x[j],
              y+ry*v[a+1],z+rz*radius[a+1]*ring_z[j]);
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
 for(a=0;a<3;a++){
  glBegin(GL_QUAD_STRIP);
  for(i=0;i<=12;i++){
   float lower;
   j=i%12;
   /* Keep the hairline above the eyes but let it fall behind the ears. */
   /* Short cap: keep the hairline ABOVE the eyes with no cheek-length bob. */
   lower=.40f+(ring_z[j]>0?.14f*ring_z[j]:.03f*ring_z[j]);
   color(red,green,blue,(j<5?2:-1)+(a==2?2:0));
   glVertex3f(x+rx*radius[a]*ring_x[j],
              y+ry*(a==0?lower:a==1?.64f:.86f),
              z+rz*radius[a]*ring_z[j]);
   glVertex3f(x+rx*radius[a+1]*ring_x[j],
              y+ry*(a+1==1?.64f:a+1==2?.86f:1.f),
              z+rz*radius[a+1]*ring_z[j]);
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
unsigned world_vertex_count(void){return frame_vertices;}
unsigned world_polygon_count(void){return frame_polygons;}
static void room(void){
 int i;
 /* Floor/walls/skirting have real thickness and meet at the corners. */
 block(0,-1.43,0,7.0,.25,7.0,24,19,14);
 block(0,.67,-3.29,7.0,4.05,.18,29,28,25);
 block(-3.45,.67,0,.18,4.05,7.0,25,27,25);
 nursery_light_planes();
 block(0,-1.15,-3.12,6.9,.16,.12,24,22,19);
 block(-3.32,-1.15,0,.12,.16,6.9,24,22,19);
 /* The old .035/.045 tall wainscot strips projected into unstable
  * one-pixel rulers over the dresser at 256x192. Clean broad painted
  * plaster replaces them: no more floating horizontal lines. */
 /* Continuous nursery floor: removed 14 subpixel strips. Those long
  * narrow decals produced vertical seams across the DS floor image. */
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
 block(-1.76,1.15,-3.01,1.65,1.51,.04,6,12,24);
 /* Dark, moonlit glass deliberately contrasts with the amber dresser
  * lamp. The same recognizable frame, blinds and furniture stay put for
  * the full film dolly. All marks are fixed native quads (no texture). */
 wall_sticker(-1.76f,.67f,-2.978f,1.52f,.42f,7,15,19);
 wall_sticker(-1.20f,.85f,-2.977f,.60f,.18f,8,18,21);
 wall_sticker(-2.20f,1.56f,-2.978f,.46f,.11f,11,17,25);
 wall_sticker(-2.32f,1.61f,-2.976f,.24f,.10f,9,15,23);
 wall_sticker(-1.41f,1.75f,-2.976f,.39f,.08f,10,16,27);
 /* One recognizable moon disk and a few rain streaks, with bright pink
  * glitch eyes hiding OUTSIDE, not random floating indoor particles. */
 ellipse(-1.35f,1.54f,-2.964f,.14f,.14f,.016f,31,29,22);
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
 block(.53,1.93,-2.96,.69,.57,.03,27,27,23);
 ellipse(.48,1.94,-2.925,.18,.22,.015,24,22,15);
 block(1.62,1.84,-3.03,.76,.81,.11,21,17,13);
 block(1.62,1.84,-2.96,.60,.64,.02,21,27,23);
 ellipse(1.62,1.85,-2.92,.19,.16,.02,27,24,17);
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
  ellipse(-2.55f,drawer_y,.128f,.073f,.054f,.038f,27,13,18);
 }
 block(-2.55,.42,-.48,.14,.28,.14,29,23,21);
 ellipse(-2.55,.65,-.48,.34,.16,.32,31,25,15);
 /* Lamp rim and warm key-light patch: intentional large forms rather than
  * noisy fake bloom or per-frame translucent screen effects. */
 ellipse(-2.55,.76,-.48,.20,.06,.18,31,30,20);
 wall_sticker(-2.73f,.78f,-.012f,.40f,.03f,31,25,18);
 /* REMOVE 5.6-unit, one-pixel wall ribbons. They z-fought with the
  * color-interpolated back wall and made bright 40+ pixel horizontal
  * cracks through the dresser and crib in real DeSmuME captures.
  * Plaster and crib geometry still define the room naturally. */
 /* 3D crib with rails on two visible sides, mattresses and legs. */
 block(2.28,-.64,-1.49,1.49,.20,1.61,29,28,24);
 block(2.28,-.48,-1.49,1.23,.10,1.35,31,20,24);
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
 ellipse(x-.185f,y+.565f,z+.095f,.055f,.058f,.018f,23,11,7);
 ellipse(x+.185f,y+.565f,z+.095f,.055f,.058f,.018f,23,11,7);
 ellipse(x,y+.285f,z+.248f,.165f,.12f,.055f,30,25,19);
 ellipse(x-.09f,y+.43f,z+.255f,.040f,.051f,.03f,5,5,4);
 ellipse(x+.09f,y+.42f,z+.255f,.040f,.051f,.03f,5,5,4);
 ellipse(x,y+.295f,z+.311f,.05f,.035f,.026f,7,5,4);
 soft_mark(x-.035f,y+.247f,z+.311f,.055f,.016f,8,5,4);
 soft_mark(x+.035f,y+.247f,z+.311f,.055f,.016f,8,5,4);
 tilted_ellipse(x-.28f,y+.015f,z+.06f,.14f,.18f,.15f,-26,28,17,5);
 tilted_ellipse(x+.28f,y+.015f,z+.06f,.14f,.18f,.15f,26,28,17,5);
 ellipse(x-.175f,y-.285f,z+.09f,.13f,.13f,.17f,27,16,5);
 ellipse(x+.175f,y-.285f,z+.09f,.13f,.13f,.17f,27,16,5);
 /* Bright fabric patches separate the toy from the similarly colored floor. */
 soft_mark(x,y-.09f,z+.246f,.17f,.17f,30,25,18);
 soft_mark(x-.175f,y-.285f,z+.263f,.078f,.065f,30,25,18);
 soft_mark(x+.175f,y-.285f,z+.263f,.078f,.065f,30,25,18);
}
/* Articulated shoulders. The sleeve, forearm, hand all share ONE local
 * shoulder transform; no independent bobbing parts and no new polygons.
 * A horizontal PLAY reach or CHAT gesture is visible from the front,
 * while NAP naturally folds both arms toward the torso. */
/* The entire face, hair, eyes, mouth and cheeks lean together. NAP
 * nods toward the lap; PLAY and CHAT glance subtly toward the player. */
int world_baby_neck_tilt(int action,uint32_t frame){
 int phase=(int)(frame%80u);
 int sway=phase<40?phase:80-phase;
 if(action==4)return -9;
 if(action==2)return 3+(sway-20)/5;
 if(action==3)return 5+(sway-20)/7;
 return 0;
}
int world_baby_arm_reach(int action){
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
int world_baby_lid_open(uint32_t frame,int action,uint32_t age){
 uint32_t beat=frame%173u;
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
static void baby_arm(int direction,int angle,int forward){
 float side=(float)direction;
 glPushMatrix();
 glTranslatef(side*.45f,-.21f,-.025f);
 glRotateZi(model_angle(direction*angle));
 glRotateXi(model_angle(-forward));
 ellipse(0,0,0,.205f,.22f,.195f,19,26,27);
 tilted_ellipse(side*.10f,-.16f,.17f,
                .14f,.18f,.14f,direction*26,31,23,19);
 ellipse(side*.18f,-.25f,.305f,.13f,.125f,.13f,31,24,20);
 glPopMatrix(1);
}
/* Baby A.I. hero model. One stable chibi silhouette with a connected
 * romper, SHORT hair, embedded eyes and a small closed-mouth smile. The
 * previous dark bob, gaping mouth/teeth, flaring jeans and detached facial
 * layers were the main reason the character looked frightening at 256x192.
 * Only ROM-native primitive meshes; no texture or framebuffer allocation. */
static void baby_model(const Game *s,uint32_t frame,int action,
                       const Roughhouse *rag,uint32_t action_age){
 unsigned beat=frame%120u,arms=action_age%36u;
 const unsigned hop=action_age%36u;
 /* Rig motion is staged in one short action. Lower-body bounce, shoulder
  * rotation, hand reach, neck reaction and eyelid shape share this curve. */
 const int weight=rag?256:(action?world_baby_care_weight(action_age):0);
 const float intensity=(float)weight/256.f;
 const float bob=((float)(beat<60u?beat:120u-beat)-30.f)*.00022f+
                 (action==3?(float)(hop<18u?hop:36u-hop)*.0022f*intensity:0.f);
 const float wave=((float)(arms<18u?arms:36u-arms)-9.f)*.00145f*intensity;
 /* Primary body rotation comes from the fixed-point toy simulation. The
  * secondary shoulder/neck reactions are tiny and remain physically
  * connected to the parent model rather than becoming floating parts. */
 const float settle=rag?(float)rag->impact*.0025f:0.f;
 /* Care IDs: FEED=1, CHAT=2, PLAY=3, NAP=4. Each reaches using
  * a shared shoulder pivot, not isolated upward-moving arm balls. */
 const int base_reach=7+(world_baby_arm_reach(action)-7)*weight/256;
 const int wave_deg=(int)(wave*160.f);
 const int recoil=rag&&rag->moving?(rag->vr>0?5:-5):0;
 const int lean=(action==1?3:action==2?-3:action==3?5:
                 action==4?-3:0)*weight/256;
 const int reach_forward=(action==1?13:action==2?8:
                          action==3?10:0)*weight/256;
 /* The fractured-baby FILM must show recognizable eyes in its first
  * shot. Ordinary care keeps natural blinking, including NAP; the
  * view-only staged damaged doll holds its eyes open for the reveal. */
 const int lid=rag&&!rag->moving&&
               rag->charge>=ROUGHHOUSE_CRACK_CHARGE?
               256:world_baby_lid_open(frame,action,action_age);
 const float lid_height=(float)lid/256.f;
 const int happy=s->mood>35;
 const int glitch=game_glitch_tier(s)>=2&&!s->safe_visuals&&
                  (frame%151u)<12u;
 glPushMatrix();
 glTranslatef(-.12f,bob-settle,.23f);
 glRotateZi(model_angle(lean)); /* torso and limbs ease together */
 /* Footed one-piece romper, short legs and soft shoes: no adult denim. */
 ellipse(0,-.75f,-.07f,.46f,.31f,.34f,19,26,27);
 ellipse(-.32f,-.93f,.14f,.245f,.22f,.27f,18,25,27);
 ellipse(.32f,-.93f,.14f,.245f,.22f,.27f,18,25,27);
 ellipse(-.365f,-1.08f,.36f,.235f,.13f,.24f,30,28,23);
 ellipse(.365f,-1.08f,.36f,.235f,.13f,.24f,30,28,23);
 soft_mark(-.365f,-1.07f,.599f,.10f,.017f,21,27,24);
 soft_mark(.365f,-1.07f,.599f,.10f,.017f,21,27,24);
 /* Layered shadow + warm collar replaces the single white floating coin. */
 soft_head(0,-.32f,-.08f,.49f,.46f,.36f,19,27,27);
 ellipse(0,.084f,-.015f,.27f,.11f,.196f,17,22,21);
 /* A nearly paper-flat, overbright collar ellipsoid was emitting a
  * 75-pixel row of RGB15(31,28,23) through the dresser/cheek. That exact
  * color came from the ellipsoid's terminal GL_QUAD_STRIP band.
  * Replace it with a small, volumetric curved neckline, still attached
  * to the romper, but safely away from the rasterizer's thin-pole case. */
 ellipse(0,.117f,.012f,.176f,.112f,.139f,29,25,21);
 ellipse(0,-.39f,.284f,.235f,.225f,.035f,29,30,26);
 /* Four inexpensive stitched glints on the romper's central pocket. */
 soft_mark(0,-.30f,.330f,.10f,.044f,glitch?24:27,glitch?17:23,10);
 soft_mark(-.075f,-.34f,.331f,.055f,.026f,30,25,14);
 soft_mark(.072f,-.34f,.331f,.052f,.026f,30,25,14);
 soft_mark(0,-.38f,.332f,.044f,.027f,30,25,14);
 /* Rotating the whole forearm+hand hierarchy preserves contact with
  * the sleeve in every action and during roughhouse recoil. */
 baby_arm(-1,base_reach+wave_deg+recoil,reach_forward);
 baby_arm(1,base_reach-wave_deg-recoil,
          reach_forward+(action==2?6*weight/256:0));
 /* Neck pivot moves the entire face, eyes, cheeks, hair, and mouth AS ONE.
  * The free head briefly lags the shell's rolling body by at most 16deg. */
 glPushMatrix();
 {
  int neck=world_baby_neck_tilt(action,frame)*weight/256;
  if(rag&&rag->moving)neck-=rag->vr/3;
  if(neck>16)neck=16;
  if(neck< -16)neck=-16;
  if(neck){
   /* Pivot below the jaw, rather than spinning the head around its middle. */
   glTranslatef(0,.10f,0);
   glRotateZi(model_angle(neck));
   glTranslatef(0,-.10f,0);
  }
 }
 /* A tiny eye-line turn keeps head and arms reading as one intention.
  * Avoid large yaw: low-poly eyes/ears remain recognizable in profile. */
 if(action==2||action==3)
  glRotateYi(model_angle((action==2?3:-2)*weight/256));
 /* Rounded unified face. No second oversized jaw shell, no floating teeth. */
 soft_head(0,.54f,.045f,.61f,.615f,.45f,30,22,19);
 ellipse(-.575f,.49f,.065f,.105f,.145f,.09f,31,23,19);
 ellipse(.575f,.49f,.065f,.105f,.145f,.09f,31,23,19);
 /* Chestnut cap and two small swept tufts; old long side hair removed. */
 hair_cap(0,.54f,.035f,.625f,.63f,.465f,16,11,8);
 hair_fringe(-.19f,.948f,.44f,.29f,.105f,.045f);
 hair_fringe(.16f,.959f,.438f,.28f,.095f,-.04f);
 hair_fringe(-.015f,1.008f,.436f,.16f,.077f,-.025f);
 ellipse(-.39f,1.018f,.365f,.075f,.069f,.029f,29,14,20);
 /* Sclera and pupils lie just above the curved facial surface, not
  * stacked .10 units out in space. Each shallow eye has a tiny highlight. */
 if(lid>32){
  /* Eye sockets/iris/pupils form a clear expression at only 256x192. */
  ellipse(-.205f,.579f,.462f,.135f,.113f*lid_height,.014f,22,15,14);
  ellipse(.205f,.579f,.462f,.135f,.113f*lid_height,.014f,22,15,14);
  ellipse(-.205f,.575f,.477f,.113f,.099f*lid_height,.015f,31,30,28);
  ellipse(.205f,.575f,.477f,.113f,.099f*lid_height,.015f,31,30,28);
  ellipse(-.188f,.565f,.493f,.066f,.072f*lid_height,.015f,10,19,22);
  ellipse(.218f,.565f,.493f,.066f,.072f*lid_height,.015f,10,19,22);
  ellipse(-.185f,.560f,.509f,.040f,.054f*lid_height,.011f,3,7,10);
  ellipse(.221f,.560f,.509f,.040f,.054f*lid_height,.011f,3,7,10);
  soft_mark(-.208f,.602f,.526f,.031f,.023f*lid_height,31,31,29);
  soft_mark(.202f,.602f,.526f,.031f,.023f*lid_height,31,31,29);
 }else{
  soft_mark(-.205f,.575f,.480f,.169f,.016f,15,10,9);
  soft_mark(.205f,.575f,.480f,.169f,.016f,15,10,9);
 }
 /* Warm cheeks, gentle brows, button nose and a tiny restrained smile. */
 ellipse(-.355f,.365f,.397f,.102f,.058f,.016f,31,18,19);
 ellipse(.355f,.365f,.397f,.102f,.058f,.016f,31,18,19);
 soft_mark(-.205f,.734f,.459f,.105f,.014f,17,11,9);
 soft_mark(.205f,.734f,.459f,.105f,.014f,17,11,9);
 ellipse(0,.375f,.467f,.047f,.034f,.019f,31,23,20);
 if(happy&&(action==2||action==3)&&weight>40){
  /* Syllabic CHAT jaw, or one broad PLAY smile. Both return to the
   * original closed grin after the one-shot action recovery. */
  float speech=action==2?(float)world_baby_speech_open(action_age)/256.f:1.f;
  float open=.26f+.74f*speech;
  ellipse(0,.252f,.451f,.094f,.085f*open,.020f,12,4,9);
  soft_mark(0,.212f,.476f,.095f,.025f*open,30,15,19);
 }else if(happy){
  soft_mark(-.065f,.278f,.444f,.052f,.017f,18,9,11);
  soft_mark(0,.251f,.454f,.081f,.017f,18,9,11);
  soft_mark(.065f,.278f,.444f,.052f,.017f,18,9,11);
 }else soft_mark(0,.259f,.451f,.084f,.015f,18,9,11);
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
void world_draw_care(const Game *s,uint32_t frame,int activity,
                     uint32_t action_age){
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 /* This is a room with true depth and perspective, not a flat orthographic collage. */
 /* More frontal portrait: avoids exaggerated protruding facial geometry. */
 gluLookAt(.97f,1.05f,3.43f,-.12f,.035f,.17f,0,1,0);
 glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
 frame_vertices=0;frame_polygons=0;
 room();
 /* A muted plum contact shadow belongs ON the lavender rug, below the
  * cream shoes. The former olive green quad hovered at y=-1.105 and
  * read as a glowing green wedge stuck between the infant's legs. */
 flat_floor(-.10f,-1.124f,.31f,.84f,.255f,18,16,23);
 baby_model(s,frame,activity,0,action_age);
 teddy();
 glFlush(0);
}
/* Compatibility for existing host callers and noninteractive views: they
 * sample the deliberately posed hold, while live gameplay uses action age. */
void world_draw(const Game *s,uint32_t frame,int activity){
 world_draw_care(s,frame,activity,20u);
}

/* The character is a synthetic toy shell, never a real child. The same
 * mesh swings its attached limbs as it tumbles; the integer simulation
 * supplies 1/256-world-unit motion, so it stays deterministic on DS. */
void world_draw_roughhouse(const Game *s,uint32_t frame,const Roughhouse *r){
 int tilt;
 float elevation=0.f,shadow=.92f;
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 gluLookAt(.97f,1.05f,3.43f,-.12f,.035f,.17f,0,1,0);
 glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
 frame_vertices=0;frame_polygons=0;
 room();
 if(r){
  /* The shadow moves WITH the synthetic doll and tightens as it falls.
   * One floor quad adds no texture VRAM or per-frame image operations. */
  shadow=1.f-(float)r->y/850.f;
  if(shadow<.60f)shadow=.60f;
  flat_floor(-.10f+(float)r->x/256.f,-1.124f,
             .31f+(float)r->z/256.f,
             .84f*shadow,.255f*shadow,18,16,23);
 }else flat_floor(-.10f,-1.124f,.31f,.84f,.255f,18,16,23);
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
 baby_model(s,frame,r&&r->moving?5:0,r,0u);
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
 travel=(float)(frame-EGG_CRACK_FRAME)/
        (float)(EGG_SCARE_FRAME-EGG_CRACK_FRAME);
 /* Hold the wide shot long enough to identify the original nursery;
  * start a slow continuous lateral track through the shell opening. */
 settle=travel*travel*(3.f-2.f*travel);
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
 uint32_t elapsed;
 if(frame<=EGG_PUSH_FRAME)return 0.f;
 elapsed=frame-EGG_PUSH_FRAME;
 if(elapsed>EGG_SCARE_FRAME-EGG_PUSH_FRAME)elapsed=EGG_SCARE_FRAME-EGG_PUSH_FRAME;
 return (float)elapsed*.00032f;
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
  world_draw_roughhouse(s,frame,&staged);
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
 frame_vertices=0;frame_polygons=0;
 /* Keep the real nursery where the player last saw it: the patterned
  * rug, window and crib provide persistent parallax behind the synthetic
  * shell. Final scare still cuts to a stark silhouette for contrast. */
 /* Keep the nursery around the alien egg for the COMPLETE film, not
  * only the first three acts. The previous final cut erased the entire
  * room and made the face look like a floating abstract polygon head. */
 nursery_ambient=phase==3u?-6:world_egg_nursery_ambient(frame);
 room();
 nursery_ambient=0; /* never bleed cinematic dimming into care scenes */
 if(phase<3u){
  if(phase==2u)opened=world_egg_shell_open(frame);
  block(0,-1.09f,-.26f,1.91f,.13f,1.45f,9,13,13);
  flat_floor(0,-1.016f,-.23f,1.67f,1.18f,5,7,8);
  /* The dark backing is a SINGLE continuous incubation sac, not pale
   * geometry blocking the center of the egg. */
  soft_head(0,-.14f,-.015f,.79f,.91f,.42f,12,18,18);
  soft_head(0,-.15f,.32f,.50f,.76f,.16f,9,24,13);
  soft_head(0,-.13f,.42f,.42f,.67f,.20f,1,3,7);
  /* A subtle moving silhouette inside makes the chamber feel inhabited. */
  {
   float sway=phase==2u?opened*.032f:0.f;
   int glow=world_egg_eye_glow(frame);
   glPushMatrix();
   glTranslatef(0.f,world_egg_embryo_breath(frame),
                world_egg_embryo_approach(frame));
   soft_head(sway,-.17f,.54f,.23f,.42f,.10f,5,11,12);
   /* Dim eyes awaken gradually as the camera drifts toward the recess.
    * Both irises remain parented to the creature, not floating in room
    * space. A momentary dimness before the hard cut builds anticipation. */
   ellipse(-.090f+sway,-.07f,.652f,.055f,.044f,.012f,
           5+glow*19/31,7+glow*23/31,5+glow*12/31);
   ellipse(.096f+sway,-.07f,.652f,.055f,.044f,.012f,
           5+glow*19/31,7+glow*23/31,5+glow*12/31);
   soft_mark(-.086f+sway,-.066f,.667f,.025f,.042f,3,6,7);
   soft_mark(.099f+sway,-.066f,.667f,.025f,.042f,3,6,7);
   soft_mark(sway,-.36f,.650f,.083f,.15f,0,1,3);
   /* Catchlights follow the same rise and fall as the eye surfaces,
    * without an extra polygon or high-frequency strobe effect. */
   soft_mark(-.104f+sway,-.092f,.682f,.038f,.026f,
             7+glow*16/31,9+glow*22/31,4+glow*6/31);
   soft_mark(.078f+sway,-.092f,.682f,.038f,.026f,
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
   float drip=opened*.29f;
   soft_mark(-.269f-opened*.17f,-.31f-drip*.46f,.686f,
             .057f,.12f+drip,5,29,10);
   soft_mark(.277f+opened*.17f,-.26f-drip*.33f,.686f,
             .043f,.14f+drip*.72f,12,31,7);
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
  /* Pupils subtly converge toward the camera during the one-shot
   * lunge while remaining wholly inside their parent eye sockets. */
  {
   float focus=world_egg_creature_depth(frame)*.08f;
   ellipse(-.346f+focus,.278f,.918f,.062f,.117f,.010f,1,4,5);
   ellipse(.374f-focus,.278f,.918f,.062f,.117f,.010f,1,4,5);
   soft_mark(-.398f+focus,.325f,.934f,.044f,.031f,31,31,24);
   soft_mark(.324f-focus,.325f,.934f,.044f,.031f,31,31,24);
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
  soft_mark(0,.615f,.787f,.22f,.035f,11,27,12);
  /* Sickly reflective slime follows the facial contour rather than
   * obscuring pupils and teeth with arbitrary green rectangles. */
  soft_mark(-.77f,-.40f,.764f,.068f,.39f,8,29,11);
  soft_mark(.77f,-.30f,.765f,.061f,.31f,9,28,10);
  soft_mark(-.67f,-.64f,.793f,.19f,.062f,10,31,11);
  soft_mark(.67f,-.56f,.793f,.16f,.054f,11,31,12);
  /* A single reveal glint, never an eye-burning strobe loop. */
  if(frame<EGG_SCARE_FRAME+4u)
   soft_mark(0,.78f,.840f,.65f,.033f,30,31,26);
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
 /* Centered, closer hero framing: a cheerful toy-shop portrait rather
  * than an isolated 80px head floating in a flat gray 3D viewport. */
 gluLookAt(0.0f,.18f,4.10f,0.0f,-.05f,-.12f,0,1,0);
 glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
 frame_vertices=0;frame_polygons=0;
 /* Two-plane matte studio set, entirely ROM-native solid polygons.
  * Backdrop is BEHIND the portrait, so it cannot hide any facial pieces. */
 block(0,-.12f,-1.89f,6.8f,4.4f,.08f,21,27,30);
 block(0,-1.24f,-.68f,6.2f,.16f,2.3f,25,22,29);
 block(-1.83f,.05f,-1.71f,.27f,2.7f,.12f,28,20,27);
 block(1.83f,.05f,-1.71f,.27f,2.7f,.12f,27,21,29);
 wall_sticker(-1.24f,.82f,-1.818f,.39f,.38f,31,26,12);
 wall_sticker(1.26f,.88f,-1.818f,.35f,.35f,31,22,17);
 block(0,-.97f,-.84f,2.9f,.20f,1.60f,24,20,24);
 /* Animation only transforms the baby; title decorations stay fixed. */
 glPushMatrix();
 glTranslatef(0,bob,0);
 ellipse(0,-.80f,-.23f,.57f,.36f,.39f,23,29,27);
 soft_head(0,.045f,-.18f,.745f,.75f,.53f,31,24,20);
 hair_cap(0,.045f,-.175f,.76f,.77f,.55f,16,11,8);
 hair_fringe(-.25f,.54f,.30f,.30f,.09f,.025f);
 hair_fringe(.17f,.54f,.29f,.28f,.09f,-.03f);
 hair_fringe(-.015f,.59f,.305f,.16f,.074f,-.027f);
 ellipse(-.225f,.057f,.335f,.106f,.10f,.020f,30,29,27);
 ellipse(.225f,.057f,.335f,.106f,.10f,.020f,30,29,27);
 ellipse(-.215f,.049f,.354f,.054f,.062f,.015f,5,7,11);
 ellipse(.239f,.049f,.354f,.054f,.062f,.015f,5,7,11);
 soft_mark(-.23f,.086f,.372f,.024f,.019f,31,31,30);
 soft_mark(.216f,.086f,.372f,.024f,.019f,31,31,30);
 soft_mark(-.43f,-.205f,.275f,.138f,.07f,31,18,19);
 soft_mark(.43f,-.205f,.275f,.138f,.07f,31,18,19);
 soft_mark(0,-.27f,.337f,.14f,.023f,18,9,11);
 soft_mark(0,-.254f,.343f,.09f,.013f,31,20,19);
 glPopMatrix(1);
 glFlush(0);
}

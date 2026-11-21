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
static void color(int r,int g,int b,int shift){
 glColor(RGB15(limit(r+shift),limit(g+shift),limit(b+shift)));
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
 glRotateZ(degreesToAngle(degrees));
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
 flat_floor(x-.105f,-1.109f,z,.17f,.14f,10,18,6);
 flat_floor(x+.105f,-1.109f,z,.17f,.14f,10,18,6);
 flat_floor(x,-1.109f,z-.105f,.17f,.14f,10,18,6);
 flat_floor(x,-1.109f,z+.105f,.17f,.14f,10,18,6);
 flat_floor(x-.065f,-1.108f,z-.065f,.14f,.14f,11,20,6);
 flat_floor(x,-1.107f,z,.12f,.11f,30,26,5);
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
 block(0,-1.37,0,7.0,.25,7.0,24,19,14);
 block(0,.67,-3.23,7.0,4.05,.18,29,28,25);
 block(-3.45,.67,0,.18,4.05,7.0,25,27,25);
 block(0,-1.15,-3.12,6.9,.16,.12,24,22,19);
 block(-3.32,-1.15,0,.12,.16,6.9,24,22,19);
 /* Wainscot panel on the large side wall; a few restrained surfaces give
  * the room structure, unlike the old solid saturated wall plane. */
 side_wall_mark(-3.33f,.20f,-.96f,2.05f,.045f,21,24,21);
 side_wall_mark(-3.33f,-.57f,-.96f,2.05f,.035f,21,24,21);
 /* Removed two hairline-width vertical wainscot details that alias. */
 /* Continuous nursery floor: removed 14 subpixel strips. Those long
  * narrow decals produced vertical seams across the DS floor image. */
 /* One darkened ground plane under the play mat gives contact shadow
  * without expensive translucent blending or extra VRAM. */
 flat_floor(.04f,-1.222f,.78f,3.00f,2.58f,20,17,14);
 block(.04,-1.20,.78,2.86,.09,2.43,15,22,8);
 block(.04,-1.145,.78,2.62,.045,2.18,25,29,11);
 for(i=0;i<4;i++)flat_floor(-.89f+i*.56f,-1.112f,1.78f,.13f,.035f,18,25,9);
 /* Big stylized flowers, a defining motif of the 2000s reference. */
 rug_flower(-.92f,1.32f);
 rug_flower(.92f,1.48f);
 rug_flower(-.33f,1.77f);
 /* Glazed nursery window, mullions, curtains and cornice. */
 block(-1.76,1.15,-3.06,1.95,1.76,.07,29,28,25);
 block(-1.76,1.15,-3.01,1.65,1.51,.04,16,24,29);
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
  block(-2.55,-.13f-i*.36f,.011,1.05,.28,.05,29,27,23);
  ellipse(-2.55,-.13f-i*.36f,.063,.059,.055,.04,28,13,18);
 }
 block(-2.55,.42,-.48,.14,.28,.14,29,23,21);
 ellipse(-2.55,.65,-.48,.34,.16,.32,31,27,13);
 /* Warm white wall panels and a subtle horizontal chair rail ground
  * the room visually without large textures or extra 3D geometry. */
 wall_sticker(.35f,-.69f,-3.125f,5.65f,.035f,23,23,20);
 wall_sticker(.36f,-1.01f,-3.125f,5.65f,.04f,24,23,21);
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
 tulip(-.44f,.84f,-3.122f,1);
 tulip(.12f,.76f,-3.122f,0);
 tulip(.74f,.70f,-3.122f,1);
 tulip(1.29f,.70f,-3.122f,0);
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
/* Baby A.I. hero model. One stable chibi silhouette with a connected
 * romper, SHORT hair, embedded eyes and a small closed-mouth smile. The
 * previous dark bob, gaping mouth/teeth, flaring jeans and detached facial
 * layers were the main reason the character looked frightening at 256x192.
 * Only ROM-native primitive meshes; no texture or framebuffer allocation. */
static void baby_model(const Game *s,uint32_t frame,int action){
 unsigned beat=frame%120u,arms=frame%104u;
 const float bob=((float)(beat<60u?beat:120u-beat)-30.f)*.00028f;
 const float wave=((float)(arms<52u?arms:104u-arms)-26.f)*.0011f;
 const int blink=(frame%173u)<5u||(action==4&&(frame%125u)<42u);
 const int happy=s->mood>35;
 const int glitch=game_glitch_tier(s)>=2&&!s->safe_visuals&&
                  (frame%151u)<12u;
 glPushMatrix();
 glTranslatef(-.12f,bob,.23f);
 /* Footed one-piece romper, short legs and soft shoes: no adult denim. */
 ellipse(0,-.75f,-.07f,.46f,.31f,.34f,19,26,27);
 ellipse(-.32f,-.93f,.14f,.245f,.22f,.27f,18,25,27);
 ellipse(.32f,-.93f,.14f,.245f,.22f,.27f,18,25,27);
 ellipse(-.365f,-1.08f,.36f,.235f,.13f,.24f,30,28,23);
 ellipse(.365f,-1.08f,.36f,.235f,.13f,.24f,30,28,23);
 /* Curved mint romper and cream collar meet behind the head. */
 soft_head(0,-.32f,-.08f,.49f,.46f,.36f,19,27,27);
 ellipse(0,.09f,.0f,.22f,.10f,.205f,31,26,21);
 ellipse(0,-.39f,.284f,.245f,.25f,.035f,29,30,26);
 soft_mark(0,-.31f,.329f,.11f,.048f,glitch?25:30,glitch?17:18,21);
 /* Sleeves overlap the torso and the skin overlaps each sleeve; hands
  * always remain attached while the animation cycles. */
 tilted_ellipse(-.45f,-.21f,-.025f,.205f,.22f,.195f,-20,19,26,27);
 tilted_ellipse(.45f,-.21f,-.025f,.205f,.22f,.195f,20,19,26,27);
 tilted_ellipse(-.55f,-.37f+wave,.15f,.14f,.18f,.14f,-26,31,23,19);
 tilted_ellipse(.55f,-.37f-wave,.15f,.14f,.18f,.14f,26,31,23,19);
 ellipse(-.63f,-.46f+wave,.28f,.13f,.125f,.13f,31,24,20);
 ellipse(.63f,-.46f-wave,.28f,.13f,.125f,.13f,31,24,20);
 /* Rounded unified face. No second oversized jaw shell, no floating teeth. */
 soft_head(0,.54f,.045f,.61f,.615f,.45f,30,22,19);
 ellipse(-.575f,.49f,.065f,.105f,.145f,.09f,31,23,19);
 ellipse(.575f,.49f,.065f,.105f,.145f,.09f,31,23,19);
 /* Chestnut cap and two small swept tufts; old long side hair removed. */
 hair_cap(0,.54f,.035f,.625f,.63f,.465f,16,11,8);
 hair_fringe(-.19f,.948f,.44f,.29f,.105f,.045f);
 hair_fringe(.16f,.959f,.438f,.28f,.095f,-.04f);
 ellipse(-.39f,1.018f,.365f,.075f,.069f,.029f,29,14,20);
 /* Sclera and pupils lie just above the curved facial surface, not
  * stacked .10 units out in space. Each shallow eye has a tiny highlight. */
 if(!blink){
  ellipse(-.205f,.575f,.472f,.100f,.094f,.015f,30,29,27);
  ellipse(.205f,.575f,.472f,.100f,.094f,.015f,30,29,27);
  ellipse(-.190f,.565f,.485f,.052f,.061f,.015f,5,7,11);
  ellipse(.220f,.565f,.485f,.052f,.061f,.015f,5,7,11);
  soft_mark(-.208f,.602f,.508f,.028f,.023f,31,31,30);
  soft_mark(.202f,.602f,.508f,.028f,.023f,31,31,30);
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
 if(happy){
  /* Three shallow strokes make a smile, not a heavy straight frown. */
  soft_mark(-.065f,.278f,.444f,.052f,.017f,18,9,11);
  soft_mark(0,.251f,.454f,.081f,.017f,18,9,11);
  soft_mark(.065f,.278f,.444f,.052f,.017f,18,9,11);
 }else soft_mark(0,.259f,.451f,.084f,.015f,18,9,11);
 glPopMatrix(1);
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
void world_draw(const Game *s,uint32_t frame,int activity){
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 /* This is a room with true depth and perspective, not a flat orthographic collage. */
 /* More frontal portrait: avoids exaggerated protruding facial geometry. */
 gluLookAt(.97f,1.05f,3.43f,-.12f,.035f,.17f,0,1,0);
 glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
 frame_vertices=0;frame_polygons=0;
 room();
 baby_model(s,frame,activity);
 teddy();
 glFlush(0);
}

/* Tiny boot/title scene: do not submit the full ~1800-polygon nursery before
 * the player confirms the program reached its first interactive screen. */
/* The lightweight pregame portrait uses the SAME baby design language as
 * the nursery hero. It must remain inexpensive enough to render at boot. */
void world_draw_title(uint32_t frame){
 (void)frame;
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
 ellipse(0,-.80f,-.23f,.57f,.36f,.39f,23,29,27);
 soft_head(0,.045f,-.18f,.745f,.75f,.53f,31,24,20);
 hair_cap(0,.045f,-.175f,.76f,.77f,.55f,16,11,8);
 hair_fringe(-.25f,.54f,.30f,.30f,.09f,.025f);
 hair_fringe(.17f,.54f,.29f,.28f,.09f,-.03f);
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
 glFlush(0);
}

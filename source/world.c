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
   lower=.01f+(ring_z[j]>0?.52f*ring_z[j]:.08f*ring_z[j]);
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
 color(8,5,4,1);
 glVertex3f(x-sx*.5f,y+sy*.5f,z-.045f);
 glVertex3f(x+sx*.5f,y+sy*.5f,z-.045f);
 color(7,5,4,-1);
 glVertex3f(x+sx*.43f+skew,y-sy*.5f,z+.015f);
 glVertex3f(x-sx*.43f+skew,y-sy*.5f,z+.015f);
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
 side_wall_mark(-3.33f,-.17f,-1.99f,.035f,.73f,21,24,21);
 side_wall_mark(-3.33f,-.17f,.07f,.035f,.73f,21,24,21);
 /* Floor boards, big soft nursery rug and stitched edging. */
 for(i=0;i<8;i++)flat_floor(-3.1f+i*.84f,-1.227f,0,.025,6.7,19,14,11);
 /* Stagger plank-end seams instead of a tiled/checkerboard grid. */
 for(i=0;i<3;i++){
  flat_floor(-1.72f,-1.227f,-2.45f+i*1.70f,3.25f,.024f,21,16,12);
  flat_floor(2.12f,-1.227f,-1.59f+i*1.70f,2.70f,.024f,21,16,12);
 }
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
 /* A crib mobile, star decals and frame accents make the room feel lived in. */
 block(2.28,.93,-1.49,.065,.75,.065,24,21,16);
 block(2.28,1.29,-1.49,.91,.07,.10,24,21,16);
 wall_sticker(2.01,.88,-1.02,.19,.23,28,25,18);
 wall_sticker(2.55,.79,-1.02,.19,.23,20,25,24);
 wall_sticker(2.24,.68,-1.02,.15,.17,26,20,20);
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
 const float x=.02f,y=-.79f,z=1.27f;
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
static void baby_model(const Game *s,uint32_t frame,int action){
 /* The reference is uneasy because the infant smiles a little too steadily,
  * not because of supernatural effects or physically impossible anatomy. */
 const float bob=(float)((int)(frame%120u)-60)*.00020f;
 const float wave=(float)((int)(frame%105u)-52)*.0013f;
 const int blink=(frame%153u)<4u||(action==4&&(frame%110u)<55u);
 const int glitched=game_glitch_tier(s)>=2&&!s->safe_visuals&&frame%121u<13u;
 const int happy=s->mood>35;
 glPushMatrix();
 glTranslatef(-.12f,bob,.22f);
 /* Dark, baggy denim. Legs fan outward, teddy occupying the empty space. */
 ellipse(0,-.81f,-.08f,.53f,.26f,.40f,7,9,15);
 ellipse(-.39f,-.96f,.24f,.29f,.23f,.34f,7,9,15);
 ellipse(.39f,-.96f,.24f,.29f,.23f,.34f,7,9,15);
 ellipse(-.44f,-1.105f,.47f,.26f,.125f,.25f,6,7,9);
 ellipse(.44f,-1.105f,.47f,.26f,.125f,.25f,6,7,9);
 /* A little exposed pink sock band breaks up the dark shoes. */
 soft_mark(-.43f,-1.02f,.695f,.26f,.045f,29,16,19);
 soft_mark(.43f,-1.02f,.695f,.26f,.045f,29,16,19);
 /* Dusty pink short-sleeved top and a cheap appliqué detail. */
 soft_head(0,-.295f,-.07f,.54f,.49f,.37f,29,15,18);
 ellipse(0,.13f,.02f,.25f,.09f,.23f,30,21,17);
 /* Angled sleeves and overlapping forearms form a continuous arm pose. */
 tilted_ellipse(-.48f,-.155f,.025f,.21f,.26f,.22f,-24,28,14,17);
 tilted_ellipse(.48f,-.155f,.025f,.21f,.26f,.22f,24,28,14,17);
 tilted_ellipse(-.605f,-.355f+wave,.26f,.155f,.23f,.18f,-29,28,19,15);
 tilted_ellipse(.605f,-.355f-wave,.26f,.155f,.23f,.18f,29,28,19,15);
 ellipse(-.685f,-.455f+wave,.435f,.155f,.14f,.15f,30,22,17);
 ellipse(.685f,-.455f-wave,.435f,.155f,.14f,.15f,30,22,17);
 /* A deliberately blurry sewn teddy-face mark: tiny and low-poly. */
 soft_mark(0,-.32f,.308f,.31f,.19f,29,19,18);
 soft_mark(-.09f,-.28f,.323f,.045f,.045f,21,12,10);
 soft_mark(.09f,-.28f,.323f,.045f,.045f,21,12,10);
 soft_mark(0,-.37f,.329f,.11f,.032f,21,12,10);
 /* One coherent face, with a shallow jaw/cheek transition. */
 soft_head(0,.525f,.035f,.645f,.665f,.49f,29,21,17);
 ellipse(0,.275f,.30f,.42f,.18f,.17f,29,21,17);
 /* Continuous dark scalp, rounded bob sides and tucked fringe panels. */
 hair_cap(0,.535f,-.07f,.68f,.68f,.53f,7,5,4);
 ellipse(-.575f,.555f,-.09f,.13f,.32f,.22f,8,5,4);
 ellipse(.575f,.555f,-.09f,.13f,.32f,.22f,8,5,4);
 ellipse(-.615f,.43f,.15f,.11f,.14f,.105f,27,18,14);
 ellipse(.615f,.43f,.15f,.11f,.14f,.105f,27,18,14);
 hair_fringe(-.29f,.925f,.474f,.29f,.16f,.05f);
 hair_fringe(.005f,.94f,.474f,.31f,.15f,.055f);
 hair_fringe(.295f,.94f,.45f,.25f,.125f,.02f);
 /* A small, partly embedded hair clip survives the hair redesign. */
 ellipse(-.505f,1.015f,.35f,.087f,.13f,.035f,28,9,16);
 ellipse(-.505f,1.035f,.386f,.042f,.067f,.015f,31,20,24);
 /* Smaller eyes are recessed toward the skin, not pasted on as bubbles. */
 ellipse(-.215f,.565f,.533f,.107f,blink?.014f:.079f,.020f,18,12,10);
 ellipse(.215f,.572f,.533f,.107f,blink?.014f:.079f,.020f,18,12,10);
 if(!blink){
  ellipse(-.215f,.565f,.548f,.09f,.072f,.014f,29,28,26);
  ellipse(.215f,.572f,.548f,.09f,.072f,.014f,29,28,26);
  ellipse(-.195f,.561f,.562f,.05f,.056f,.014f,5,4,4);
  ellipse(.235f+(glitched?.045f:0),.568f,.562f,.05f,.056f,.014f,5,4,4);
  soft_mark(-.205f,.603f,.579f,.025f,.025f,31,30,28);
  soft_mark(.226f,.609f,.579f,.025f,.025f,31,30,28);
 }else{
  soft_mark(-.215f,.565f,.551f,.20f,.016f,15,9,8);
  soft_mark(.215f,.572f,.551f,.20f,.016f,15,9,8);
 }
 soft_mark(-.22f,.755f,.485f,.19f,.023f,14,9,8);
 soft_mark(.22f,.759f,.485f,.19f,.023f,14,9,8);
 soft_mark(-.405f,.34f,.455f,.125f,.055f,30,17,16);
 soft_mark(.405f,.34f,.455f,.125f,.055f,30,17,16);
 ellipse(0,.395f,.511f,.067f,.045f,.029f,30,23,19);
 /* Layered shallow mouth cavity: dark interior, lip rims, recessed teeth.
  * A small steady open grin is unsettling without a floating black sticker. */
 ellipse(.006f,.205f,.504f,happy?.156f:.112f,happy?.074f:.043f,
         .023f,9,3,5);
 ellipse(.004f,.259f,.520f,happy?.158f:.114f,.022f,.019f,24,12,14);
 ellipse(.006f,.15f,.518f,happy?.143f:.10f,.023f,.018f,26,13,15);
 if(happy){
  soft_mark(0,.231f,.535f,.135f,.017f,30,27,25);
  ellipse(.008f,.163f,.526f,.076f,.020f,.009f,26,11,14);
 }
 glPopMatrix(1);
 if(glitched)block(.55f,.64f,.91f,.10f,.035f,.04f,8,29,23);
}
void world_init(void){
 glInit();
 glEnable(GL_ANTIALIAS);
 glClearColor(27,28,28,31);
 glClearPolyID(63);
 glClearDepth(0x7fff);
 glViewport(0,0,255,191);
 glMatrixMode(GL_PROJECTION);glLoadIdentity();
 gluPerspective(55,256.0f/192.0f,.1f,30);
}
void world_draw(const Game *s,uint32_t frame,int activity){
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 /* This is a room with true depth and perspective, not a flat orthographic collage. */
 gluLookAt(1.42f,1.30f,3.26f,-.12f,.08f,.18f,0,1,0);
 glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
 frame_vertices=0;frame_polygons=0;
 room();
 baby_model(s,frame,activity);
 teddy();
 glFlush(0);
}

/* Tiny boot/title scene: do not submit the full ~1800-polygon nursery before
 * the player confirms the program reached its first interactive screen. */
void world_draw_title(uint32_t frame){
 (void)frame;
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 gluLookAt(0.0f,0.3f,5.5f,0.0f,0.0f,0.0f,0,1,0);
 glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
 frame_vertices=0;frame_polygons=0;
 block(0.0f,-.82f,-.9f,3.0f,.22f,1.6f,24,17,20);
 ellipse(0.0f,.05f,-.2f,.85f,.91f,.57f,27,17,12);
 ellipse(-.29f,.12f,.33f,.20f,.24f,.08f,31,30,30);
 ellipse(.29f,.12f,.33f,.20f,.24f,.08f,31,30,30);
 ellipse(-.29f,.12f,.41f,.10f,.14f,.04f,3,2,3);
 ellipse(.29f,.12f,.41f,.10f,.14f,.04f,3,2,3);
 ellipse(0,-.32f,.36f,.20f,.11f,.06f,13,4,6);
 glFlush(0);
}

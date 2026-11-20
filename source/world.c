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
 block(.04,-1.20,.78,2.86,.09,2.43,18,20,18);
 block(.04,-1.145,.78,2.62,.045,2.18,27,28,25);
 for(i=0;i<5;i++)flat_floor(-.88f+i*.45f,-1.112f,1.78,.15,.04,26,24,18);
 /* Glazed nursery window, mullions, curtains and cornice. */
 block(-1.76,1.15,-3.06,1.95,1.76,.07,29,28,25);
 block(-1.76,1.15,-3.01,1.65,1.51,.04,16,24,29);
 wall_sticker(-1.76,1.15,-2.97,.10,1.57,28,27,24);
 wall_sticker(-1.76,1.15,-2.965,1.71,.10,28,27,24);
 block(-2.63,1.12,-2.81,.27,1.84,.15,22,18,18);
 block(-.89,1.12,-2.81,.27,1.84,.15,22,18,18);
 block(-1.76,2.11,-2.79,2.09,.13,.15,22,18,18);
 /* Picture frame, yellow stars and faux painting. */
 block(.53,1.93,-3.03,.85,.75,.11,21,17,13);
 block(.53,1.93,-2.96,.69,.57,.03,27,27,23);
 ellipse(.48,1.94,-2.925,.18,.22,.015,24,22,15);
 block(1.62,1.84,-3.03,.76,.81,.11,21,17,13);
 block(1.62,1.84,-2.96,.60,.64,.02,21,27,23);
 ellipse(1.62,1.85,-2.92,.19,.16,.02,27,24,17);
 /* Dresser, three actual drawers and bottle atop it. */
 block(-2.55,-.48,-.48,1.20,1.36,.94,21,19,16);
 block(-2.55,.23,-.48,1.36,.16,1.11,26,24,21);
 for(i=0;i<3;i++){
  block(-2.55,-.13f-i*.36f,.011,1.05,.28,.05,27,25,22);
  ellipse(-2.55,-.13f-i*.36f,.063,.059,.055,.04,20,16,11);
 }
 block(-2.55,.42,-.48,.19,.25,.17,29,28,25);
 ellipse(-2.55,.61,-.48,.22,.24,.22,30,27,23);
 /* Warm white wall panels and a subtle horizontal chair rail ground
  * the room visually without large textures or extra 3D geometry. */
 wall_sticker(.35f,-.69f,-3.125f,5.65f,.035f,23,23,20);
 wall_sticker(.36f,-1.01f,-3.125f,5.65f,.04f,24,23,21);
 /* 3D crib with rails on two visible sides, mattresses and legs. */
 block(2.28,-.64,-1.49,1.49,.20,1.61,25,22,18);
 block(2.28,-.48,-1.49,1.23,.10,1.35,29,27,25);
 ellipse(2.20f,-.39f,-1.87f,.34f,.07f,.25f,30,29,27); /* cotton pillow */
 for(i=0;i<5;i++){
  crib_rail(1.76f+i*.26f,-.16,-.64,.075,1.19,.085,28,26,22);
  crib_rail(1.76f+i*.26f,-.16,-2.34,.075,1.19,.085,28,26,22);
 }
 block(2.28,.45,-.64,1.52,.14,.13,27,25,21);
 block(2.28,.45,-2.34,1.52,.14,.13,27,25,21);
 block(1.55,-.14,-1.49,.13,1.13,1.72,27,25,21);
 block(3.03,-.14,-1.49,.13,1.13,1.72,27,25,21);
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
 float x=.89,y=-.80f,z=1.19f;
 ellipse(x,y,z,.28,.32,.25,19,14,10);
 ellipse(x,y+.38f,z+.04f,.25,.25,.23,21,16,11);
 ellipse(x-.22f,y+.57f,z,.12,.12,.12,22,16,11);
 ellipse(x+.22f,y+.57f,z,.12,.12,.12,22,16,11);
 ellipse(x,y+.30f,z+.25f,.13,.10,.055,29,25,21);
 ellipse(x-.09f,y+.43f,z+.255f,.038,.047,.03,6,5,4);
 ellipse(x+.09f,y+.43f,z+.255f,.038,.047,.03,6,5,4);
 ellipse(x,y+.32f,z+.31f,.052,.035,.027,5,4,4);
 ellipse(x-.27f,y-.07f,z,.13,.17,.15,20,15,11);
 ellipse(x+.27f,y-.07f,z,.13,.17,.15,20,15,11);
 ellipse(x-.16f,y-.27f,z+.08f,.13,.13,.17,21,15,11);
 ellipse(x+.16f,y-.27f,z+.08f,.13,.13,.17,21,15,11);
}
static void baby_model(const Game *s,uint32_t frame,int action){
 float bob=(float)((int)(frame%96u)-48)*.00035f;
 float wave=(float)((int)(frame%105u)-52)*.00125f;
 int blink=(frame%149u)<5u||(action==4&&(frame%110u)<55u);
 int tier=game_glitch_tier(s);
 int glitched=tier>=2&&!s->safe_visuals&&(frame%121u<13u);
 int happy=s->mood>42;
 glPushMatrix();
 glTranslatef(-.12f,bob,.22f);
 /* Seated toddler, softly rounded but no giant floating boots or wrists. */
 ellipse(0,-.79f,.00f,.53f,.27f,.41f,11,13,18);
 ellipse(-.30f,-.94f,.23f,.25f,.21f,.30f,12,15,20);
 ellipse(.30f,-.94f,.23f,.25f,.21f,.30f,12,15,20);
 ellipse(-.33f,-1.09f,.46f,.24f,.12f,.23f,26,24,24);
 ellipse(.33f,-1.09f,.46f,.24f,.12f,.23f,26,24,24);
 /* Muted teal knit top with cream bib and soft fabric neck seam. */
 soft_head(0,-.29f,-.07f,.50f,.50f,.38f,15,23,22);
 ellipse(0,.16f,.04f,.25f,.13f,.23f,26,22,18);
 ellipse(0,-.37f,.294f,.27f,.32f,.048f,29,27,23);
 ellipse(0,-.61f,.31f,.31f,.055f,.04f,21,25,22);
 /* Sewn-on pocket and two tiny buttons are flatter than the old 3D logo. */
 soft_mark(0,-.38f,.345f,.20f,.12f,25,23,21);
 soft_mark(-.055f,-.29f,.346f,.031f,.031f,17,14,12);
 soft_mark(.055f,-.29f,.346f,.031f,.031f,17,14,12);
 /* Sleeves and forearms connect continuously. Activity gives gentle motion. */
 ellipse(-.49f,-.15f,-.015f,.22f,.26f,.23f,15,22,22);
 ellipse(.49f,-.15f,-.015f,.22f,.26f,.23f,15,22,22);
 ellipse(-.60f,-.39f+wave,.16f,.17f,.24f,.19f,28,20,16);
 ellipse(.60f,-.39f-wave,.16f,.17f,.24f,.19f,28,20,16);
 ellipse(-.62f,-.55f+wave,.29f,.18f,.14f,.16f,30,22,18);
 ellipse(.62f,-.55f-wave,.29f,.18f,.14f,.16f,30,22,18);
 /* Hair at the rear: chestnut, not black, leaving visible forehead. */
 soft_head(0,.59f,-.095f,.64f,.65f,.50f,12,8,6);
 /* One smoother facial mesh makes the head less blocky without increasing
  * every other object in the nursery to a prohibitively expensive topology. */
 soft_head(0,.54f,.04f,.625f,.61f,.525f,29,21,17);
 ellipse(-.62f,.46f,.05f,.13f,.155f,.14f,27,18,15);
 ellipse(.62f,.46f,.05f,.13f,.155f,.14f,27,18,15);
 /* Asymmetric locks sit above the forehead, not across the eyes. */
 ellipse(-.41f,1.02f,.115f,.23f,.115f,.29f,13,8,6);
 ellipse(.04f,1.095f,-.005f,.39f,.125f,.30f,13,9,7);
 ellipse(.46f,1.01f,.055f,.18f,.135f,.24f,11,7,6);
 ellipse(-.55f,.78f,.055f,.085f,.19f,.14f,12,8,6);
 ellipse(.55f,.78f,.055f,.085f,.19f,.14f,12,8,6);
 /* Small fabric hairclip instead of the oversized hot-pink disk. */
 ellipse(-.47f,1.025f,.33f,.085f,.075f,.025f,23,15,16);
 soft_mark(-.47f,1.025f,.365f,.042f,.055f,30,26,24);
 /* Smaller, embedded eyes have lids/irises rather than bulging frog-eyes.
  * 0.125-radius eyeballs, ~half the old exaggerated size. */
 ellipse(-.225f,.57f,.548f,.145f,blink?.018f:.129f,.027f,20,15,13);
 ellipse(.225f,.57f,.548f,.145f,blink?.018f:.129f,.027f,20,15,13);
 if(!blink){
  ellipse(-.225f,.57f,.565f,.126f,.112f,.023f,30,29,27);
  ellipse(.225f,.57f,.565f,.126f,.112f,.023f,30,29,27);
  ellipse(-.213f,.563f,.589f,.062f,.078f,.018f,11,15,14);
  ellipse(.213f+(glitched?.065f:0),.563f,.589f,.062f,.078f,.018f,11,15,14);
  ellipse(-.214f,.560f,.606f,.035f,.057f,.013f,5,5,6);
  ellipse(.214f+(glitched?.065f:0),.560f,.606f,.035f,.057f,.013f,5,5,6);
  soft_mark(-.233f,.612f,.625f,.034f,.035f,31,31,31);
  soft_mark(.194f+(glitched?.065f:0),.612f,.625f,.034f,.035f,31,31,31);
 }else{
  soft_mark(-.225f,.566f,.580f,.27f,.025f,15,9,8);
  soft_mark(.225f,.566f,.580f,.27f,.025f,15,9,8);
 }
 /* Graceful eyelid contours, low-saturation cheeks, tiny nose and mouth. */
 soft_mark(-.225f,.764f,.523f,.24f,.031f,14,9,8);
 soft_mark(.225f,.764f,.523f,.24f,.031f,14,9,8);
 soft_mark(-.42f,.345f,.435f,.16f,.070f,29,17,17);
 soft_mark(.42f,.345f,.435f,.16f,.070f,29,17,17);
 ellipse(0,.393f,.582f,.084f,.069f,.055f,30,23,19);
 ellipse(0,.228f,.575f,happy?.14f:.10f,happy?.042f:.024f,.018f,17,9,9);
 if(happy)soft_mark(0,.199f,.588f,.13f,.018f,29,20,19);
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
 gluLookAt(2.30f,1.70f,4.08f,-.08f,.04f,-.22f,0,1,0);
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

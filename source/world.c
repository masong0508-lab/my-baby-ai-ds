#include <nds.h>
#include "world.h"

/* Intentionally ugly-cute NDS-era art: 3D bodies, furniture, camera depth.
 * Entire scene uses DS polygon hardware; no software framebuffer or texture.
 * Ellipsoids are 8-sided and have only three rings (24 quads each).
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
unsigned world_vertex_count(void){return frame_vertices;}
unsigned world_polygon_count(void){return frame_polygons;}
static void room(void){
 int i;
 /* Floor/walls/skirting have real thickness and meet at the corners. */
 block(0,-1.37,0,7.0,.25,7.0,24,17,11);
 block(0,.67,-3.23,7.0,4.05,.18,30,25,24);
 block(-3.45,.67,0,.18,4.05,7.0,28,23,24);
 block(0,-1.15,-3.12,6.9,.16,.12,31,18,24);
 block(-3.32,-1.15,0,.12,.16,6.9,31,18,24);
 /* Floor boards, big soft nursery rug and stitched edging. */
 for(i=0;i<8;i++)block(-3.1f+i*.84f,-1.227f,0,.025,.014,6.7,16,10,7);
 for(i=0;i<5;i++)block(0,-1.227f,-3.05f+i*1.40f,6.8,.014,.025,18,12,8);
 block(.04,-1.20,.78,2.86,.09,2.43,10,16,22);
 block(.04,-1.145,.78,2.62,.045,2.18,22,26,30);
 for(i=0;i<5;i++)block(-.88f+i*.45f,-1.112f,1.78,.15,.015,.04,30,22,14);
 /* Glazed nursery window, mullions, curtains and cornice. */
 block(-1.76,1.15,-3.06,1.95,1.76,.07,31,28,20);
 block(-1.76,1.15,-3.01,1.65,1.51,.04,12,23,29);
 block(-1.76,1.15,-2.97,.10,1.57,.05,31,28,18);
 block(-1.76,1.15,-2.97,1.71,.10,.05,31,28,18);
 block(-2.63,1.12,-2.81,.27,1.84,.15,26,10,17);
 block(-.89,1.12,-2.81,.27,1.84,.15,26,10,17);
 block(-1.76,2.11,-2.79,2.09,.13,.15,29,16,20);
 /* Picture frame, yellow stars and faux painting. */
 block(.53,1.93,-3.03,.85,.75,.11,19,12,6);
 block(.53,1.93,-2.96,.69,.57,.03,30,23,25);
 ellipse(.48,1.94,-2.925,.18,.22,.015,29,23,7);
 block(1.62,1.84,-3.03,.76,.81,.11,22,13,6);
 block(1.62,1.84,-2.96,.60,.64,.02,21,27,17);
 ellipse(1.62,1.85,-2.92,.19,.16,.02,31,26,8);
 /* Dresser, three actual drawers and bottle atop it. */
 block(-2.55,-.48,-.48,1.20,1.36,.94,26,15,19);
 block(-2.55,.23,-.48,1.36,.16,1.11,31,21,24);
 for(i=0;i<3;i++){
  block(-2.55,-.13f-i*.36f,.011,1.05,.28,.05,31,25,24);
  ellipse(-2.55,-.13f-i*.36f,.063,.059,.055,.04,20,16,5);
 }
 block(-2.55,.42,-.48,.19,.25,.17,29,28,24);
 ellipse(-2.55,.61,-.48,.22,.24,.22,31,27,16);
 /* 3D crib with rails on two visible sides, mattresses and legs. */
 block(2.28,-.64,-1.49,1.49,.20,1.61,28,17,15);
 block(2.28,-.48,-1.49,1.23,.10,1.35,30,24,20);
 for(i=0;i<5;i++){
  block(1.76f+i*.26f,-.16,-.64,.075,1.19,.085,31,27,20);
  block(1.76f+i*.26f,-.16,-2.34,.075,1.19,.085,31,27,20);
 }
 block(2.28,.45,-.64,1.52,.14,.13,30,25,18);
 block(2.28,.45,-2.34,1.52,.14,.13,30,25,18);
 block(1.55,-.14,-1.49,.13,1.13,1.72,30,25,18);
 block(3.03,-.14,-1.49,.13,1.13,1.72,30,25,18);
 /* Shelf and a row of toy blocks, no giant texture atlas needed. */
 block(2.59,1.06,-3.00,1.09,.13,.38,29,19,15);
 for(i=0;i<4;i++)block(2.16f+i*.26f,1.28,-2.91,.20,.30,.24,17+i*3,18-i,22+i);
 /* Blocks and balls visible in foreground. */
 block(2.56,-1.04,1.54,.39,.36,.40,26,13,23);
 block(2.56,-.70,1.54,.30,.32,.31,16,23,30);
 block(-2.34,-1.05,1.68,.46,.36,.39,29,20,8);
 ellipse(-2.68,-1.00,1.03,.22,.22,.20,18,26,11);
}
static void teddy(void){
 float x=.89,y=-.80f,z=1.19f;
 ellipse(x,y,z,.28,.32,.25,22,12,4);
 ellipse(x,y+.38f,z+.04f,.25,.25,.23,24,14,6);
 ellipse(x-.22f,y+.57f,z,.12,.12,.12,25,14,6);
 ellipse(x+.22f,y+.57f,z,.12,.12,.12,25,14,6);
 ellipse(x,y+.30f,z+.25f,.13,.10,.055,31,26,20);
 ellipse(x-.09f,y+.43f,z+.255f,.038,.047,.03,4,3,2);
 ellipse(x+.09f,y+.43f,z+.255f,.038,.047,.03,4,3,2);
 ellipse(x,y+.32f,z+.31f,.052,.035,.027,3,2,2);
 ellipse(x-.27f,y-.07f,z,.13,.17,.15,23,13,5);
 ellipse(x+.27f,y-.07f,z,.13,.17,.15,23,13,5);
 ellipse(x-.16f,y-.27f,z+.08f,.13,.13,.17,24,13,5);
 ellipse(x+.16f,y-.27f,z+.08f,.13,.13,.17,24,13,5);
}
static void baby_model(const Game *s,uint32_t frame,int action){
 float bob=(float)((int)(frame%80u)-40)*.0009f;
 float waving=(float)((int)(frame%110u)-55)*.002f;
 int blink=(frame%143u)<5u;
 int glitched=game_glitch_tier(s)>=2&&!s->safe_visuals&&(frame%121u<13u);
 int happy=s->mood>42;
 glPushMatrix();
 glTranslatef(-.12f,bob,.22f);
 /* Jeans, legs, socks, soft cloth torso. */
 ellipse(0,-.74f,.00f,.61,.30,.46,9,12,19);
 ellipse(-.39,-.91,.20,.30,.25,.36,9,12,19);
 ellipse(.39,-.91,.20,.30,.25,.36,9,12,19);
 ellipse(-.43,-1.07,.42,.27,.14,.25,30,20,25);
 ellipse(.43,-1.07,.42,.27,.14,.25,30,20,25);
 ellipse(0,-.32f,-.05f,.53,.54,.39,30,14,19);
 ellipse(0,.05f,.16f,.28,.075,.22,31,24,26);
 /* Sleeves, arms and hands animate as separate masses. */
 ellipse(-.54,-.15,.04,.25,.27,.29,30,13,18);
 ellipse(.54,-.15,.04,.25,.27,.29,30,13,18);
 ellipse(-.72,-.43+waving*(action==2?2:1),.20,.19,.29,.18,27,17,12);
 ellipse(.72,-.43-waving*(action==2?2:1),.20,.19,.29,.18,27,17,12);
 ellipse(-.77,-.68+waving,.27,.19,.15,.19,28,18,13);
 ellipse(.77,-.68-waving,.27,.19,.15,.19,28,18,13);
 /* Sweater bunny embroidery, raised above the clothing. */
 ellipse(0,-.37,.36,.21,.16,.025,30,25,24);
 ellipse(-.12,-.27,.36,.09,.14,.022,30,25,24);
 ellipse(.12,-.27,.36,.09,.14,.022,30,25,24);
 ellipse(-.07,-.37,.387,.021,.026,.013,5,4,5);
 ellipse(.06,-.37,.387,.021,.026,.013,5,4,5);
 /* Huge asymmetric, unmistakable head with realistic surface depth. */
 ellipse(0,.61,-.10,.72,.77,.56,5,3,4);
 ellipse(0,.50,.08,.66,.71,.54,28,17,12);
 ellipse(-.65,.39,.06,.15,.18,.18,26,16,11);
 ellipse(.65,.39,.06,.15,.18,.18,26,16,11);
 ellipse(-.42,1.05,.13,.35,.22,.34,6,4,4);
 ellipse(.09,1.10,.08,.52,.22,.36,6,4,4);
 ellipse(.50,.96,.10,.22,.24,.31,6,4,4);
 ellipse(-.65,.73,.07,.13,.34,.17,5,4,4);
 ellipse(.63,.71,.07,.12,.31,.17,5,4,4);
 /* Pink hair clip and shiny, intentionally overlarge DS-era eyes. */
 ellipse(-.56,.98,.37,.12,.18,.075,30,9,18);
 ellipse(-.55,.96,.435,.060,.10,.017,31,19,25);
 ellipse(-.29,.57,.613,.20,blink?.022f:.20f,.071,31,29,29);
 ellipse(.29,.57,.613,.20,blink?.022f:.20f,.071,31,29,29);
 if(!blink){
  ellipse(-.27,.57,.685,.106,.14,.043,3,2,3);
  ellipse(.29+(glitched?.16f:0),.57,.685,.112,.14,.043,3,2,3);
  ellipse(-.30,.64,.724,.037,.040,.020,31,31,31);
  ellipse(.25+(glitched?.16f:0),.64,.724,.037,.040,.020,31,31,31);
 }
 ellipse(0,.30,.64,.14,.12,.085,28,18,14);
 ellipse(-.40,.29,.58,.12,.08,.022,31,15,18);
 ellipse(.40,.29,.58,.12,.08,.022,31,15,18);
 ellipse(0,.08,.615,happy?.23f:.15f,happy?.12f:.055f,.035,13,4,7);
 if(happy)ellipse(0,.035,.64,.115,.034,.016,31,17,20);
 glPopMatrix(1);
 if(glitched)block(.55,.64,.96,.22,.13,.10,1,30,20);
}
void world_init(void){
 glInit();
 glEnable(GL_ANTIALIAS);
 glClearColor(26,24,27,31);
 glClearPolyID(63);
 glClearDepth(0x7fff);
 glViewport(0,0,255,191);
 glMatrixMode(GL_PROJECTION);glLoadIdentity();
 gluPerspective(55,256.0f/192.0f,.1f,30);
}
void world_draw(const Game *s,uint32_t frame,int activity){
 glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 /* This is a room with true depth and perspective, not a flat orthographic collage. */
 gluLookAt(3.30f,2.35f,6.55f,0,.08f,-.25f,0,1,0);
 glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
 frame_vertices=0;frame_polygons=0;
 room();
 baby_model(s,frame,activity);
 teddy();
 glFlush(0);
}

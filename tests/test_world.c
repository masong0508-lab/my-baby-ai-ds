#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <nds.h>
#include "world.h"
/* Capture actual submitted triangle vertices before the no-op transforms.
 * A rigid model transform preserves triangle area. This catches repeated
 * hand-cap poles without pretending the host shim emulates DS pixels. */
static int primitive_mode;
static unsigned primitive_vertices,submitted_hand_caps;
static float triangle_points[3][3];
static int capture_geometry;
static uint32_t geometry_hash;
void test_gl_begin(int mode){
 primitive_mode=mode;primitive_vertices=0;
}
void test_gl_vertex(float x,float y,float z){
 if(capture_geometry){
  uint32_t bits;
  memcpy(&bits,&x,sizeof(bits));geometry_hash=(geometry_hash^bits)*16777619u;
  memcpy(&bits,&y,sizeof(bits));geometry_hash=(geometry_hash^bits)*16777619u;
  memcpy(&bits,&z,sizeof(bits));geometry_hash=(geometry_hash^bits)*16777619u;
 }
 if(primitive_mode==GL_TRIANGLES){
  unsigned n=primitive_vertices%3u;
  triangle_points[n][0]=x;triangle_points[n][1]=y;triangle_points[n][2]=z;
  if(n==2u){
   float ax=triangle_points[1][0]-triangle_points[0][0];
   float ay=triangle_points[1][1]-triangle_points[0][1];
   float az=triangle_points[1][2]-triangle_points[0][2];
   float bx=x-triangle_points[0][0],by=y-triangle_points[0][1];
   float bz=z-triangle_points[0][2];
   float cx=ay*bz-az*by,cy=az*bx-ax*bz,cz=ax*by-ay*bx;
   assert(cx*cx+cy*cy+cz*cz>1.e-12f);
   /* Facial patches also use nondegenerate triangles now. Identify the
    * existing hand-cap poles explicitly instead of assuming every
    * triangle in the whole character must belong to a hand. The host
    * shim intentionally observes coordinates before rigid transforms. */
   {
    unsigned p;
    for(p=0;p<3u;p++){
     float px=triangle_points[p][0],py=triangle_points[p][1];
     float pz=triangle_points[p][2];
     if(((px>-.180001f&&px<-.179999f)||
         (px>.179999f&&px<.180001f))&&
        ((py>-.375001f&&py<-.374999f)||
         (py>-.125001f&&py<-.124999f))&&
        pz>.304999f&&pz<.305001f){
      submitted_hand_caps++;
      break;
     }
    }
   }
  }
 }
 primitive_vertices++;
}
void test_gl_end(void){
 if(primitive_mode==GL_TRIANGLES)assert(primitive_vertices%3u==0u);
}
static void assert_baby_pose_equal(WorldBabyPose a,WorldBabyPose b){
 assert(a.reach_left==b.reach_left&&a.reach_right==b.reach_right);
 assert(a.forward_left==b.forward_left&&a.forward_right==b.forward_right);
 assert(a.lean==b.lean&&a.neck==b.neck&&a.yaw==b.yaw);
 assert(a.lid==b.lid&&a.bounce==b.bounce);
}
static void assert_baby_pose_bounds_to(WorldBabyPose p,int reach_max){
 assert(p.reach_left>=-14&&p.reach_left<=reach_max);
 assert(p.reach_right>=-14&&p.reach_right<=reach_max);
 assert(p.forward_left>=0&&p.forward_left<=13);
 assert(p.forward_right>=0&&p.forward_right<=14);
 assert(p.lean>=-3&&p.lean<=5);
 assert(p.neck>=-16&&p.neck<=16);
 assert(p.yaw>=-2&&p.yaw<=3);
 assert(p.lid>=0&&p.lid<=256);
 assert(p.bounce>=0&&p.bounce<=4608);
}
static void assert_baby_pose_bounds(WorldBabyPose p){
 assert_baby_pose_bounds_to(p,59);
}
static void assert_baby_channel_step(int a,int b,int limit){
 assert(a-b<=limit&&b-a<=limit);
}
static void assert_baby_pose_step_to(WorldBabyPose a,WorldBabyPose b,
                                     int reach_step){
 /* Even the widest PLAY -> sleepy NAP reversal must cross its range
  * over the 12-tick handoff. A reset would jump roughly fifty degrees. */
 assert_baby_channel_step(a.reach_left,b.reach_left,reach_step);
 assert_baby_channel_step(a.reach_right,b.reach_right,reach_step);
 assert_baby_channel_step(a.forward_left,b.forward_left,3);
 assert_baby_channel_step(a.forward_right,b.forward_right,3);
 assert_baby_channel_step(a.lean,b.lean,2);
 assert_baby_channel_step(a.neck,b.neck,3);
 assert_baby_channel_step(a.yaw,b.yaw,2);
 /* The existing natural blink advances up to 64 aperture units/tick;
  * an interrupted NAP can reopen at the same time (32 more units). */
 assert_baby_channel_step(a.lid,b.lid,100);
 assert_baby_channel_step(a.bounce,b.bounce,600);
}
static void assert_baby_pose_step(WorldBabyPose a,WorldBabyPose b){
 assert_baby_pose_step_to(a,b,11);
}
static uint32_t baby_geometry(const Game *s,int action,unsigned age){
 geometry_hash=2166136261u;capture_geometry=1;
 world_draw_care(s,128u,action,age);
 capture_geometry=0;
 return geometry_hash;
}
int main(void){
 Game s;Roughhouse r;int i;unsigned worst_v=0,worst_p=0;
 game_init(&s);world_init();
 /* WARDEN's physical screen clamps damaged/unknown status without
  * changing the baby or exceeding the normal scene budget. */
 {
  Game original=s;
  unsigned stage;
  for(stage=0;stage<6u;stage++){
   world_observer_status(stage,stage?120u:0u,(int)(stage&1u));
   world_draw(&s,128u,0);
   assert(world_vertex_count()<=4500);
   assert(world_polygon_count()<=1700);
   assert(memcmp(&s,&original,sizeof(s))==0);
  }
  world_observer_status(0u,100u,0);
 }
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
  /* Facial surfaces replace wasteful thin full ellipsoids. Preserve
   * the recovered nursery headroom, below the broader 5000/1850 gate
   * used by the optional egg film and moving toy scenes. */
  assert(world_vertex_count()<=4500);
  assert(world_polygon_count()<=1700);
 }
 /* The five native nursery camera poses must share one coherent room.
  * 16-tick smoothstep travel is continuous, stays within the DS volume,
  * and leaves the default portrait unchanged. */
 {
  EggCamera idle=world_nursery_camera(-1,-1,16u);
  EggCamera window=world_nursery_camera(0,-1,16u);
  EggCamera lamp=world_nursery_camera(1,-1,16u);
  EggCamera teddy_view=world_nursery_camera(2,-1,16u);
  EggCamera crib=world_nursery_camera(3,-1,16u);
  unsigned f;
  /* Composition deliberately looks near the baby and landmark. The
   * earlier far-left targets showed a giant empty sage wall. */
  assert(window.look_x>-.95f && window.look_x<-.75f);
  assert(lamp.look_x>-1.45f && lamp.look_x< -1.25f);
  assert(teddy_view.look_x>-.75f && teddy_view.look_x<-.55f);
  assert(crib.look_x>1.f && crib.look_x<1.3f);
  assert(window.x>.9f && lamp.x>.9f && teddy_view.x>.9f);
  assert(idle.x>.9f&&idle.z>3.4f);
  for(i=0;i<4;i++){
   EggCamera start=world_nursery_camera(i,-1,0u);
   EggCamera end=world_nursery_camera(i,-1,16u);
   EggCamera hold=world_nursery_camera(i,-1,5000u);
   float last=start.x;
   assert(start.x==idle.x&&start.z==idle.z);
   assert(end.x==hold.x&&end.look_x==hold.look_x);
   for(f=1;f<=16;f++){
    EggCamera q=world_nursery_camera(i,-1,f);
    float diff=q.x-last;
    assert(diff<.18f&&diff>-.18f);
    assert(q.z>2.8f&&q.z<3.8f);
    last=q.x;
   }
   world_draw_nursery(&s,130u,3,18u,i,-1,16u,
                      i==0,i!=1,18u,0,0,0);
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
  /* Visual-state changes must not alter any saved Game data. */
  {
   Game original=s;
   world_draw_nursery(&s,132u,0,0u,0,-1,16u,1,0,40u,0,0,0);
   assert(memcmp(&original,&s,sizeof(s))==0);
   world_draw(&s,133u,0); /* normal care restores night + lamp */
   assert(world_polygon_count()<=1850);
  }
 }
 /* Interrupting a nursery camera move must start at the exact
  * in-flight lens coordinates, not at the previous preset's end point.
  * Rapid window->crib->teddy interactions cannot teleport the view. */
 {
  EggCamera home=world_nursery_camera(-1,-1,16u);
  EggCamera midway=world_nursery_camera_from(home,0,5u);
  EggCamera interrupt=world_nursery_camera_from(midway,3,0u);
  EggCamera first=world_nursery_camera_from(midway,3,1u);
  EggCamera second=world_nursery_camera_from(first,2,0u);
  EggCamera end=world_nursery_camera_from(midway,3,16u);
  EggCamera crib=world_nursery_camera(3,-1,16u);
  assert(interrupt.x==midway.x && interrupt.y==midway.y);
  assert(interrupt.z==midway.z && interrupt.look_x==midway.look_x);
  assert(second.x==first.x && second.look_z==first.look_z);
  assert(first.x-interrupt.x<.08f && interrupt.x-first.x<.08f);
  assert(first.look_x-interrupt.look_x<.08f);
  assert(end.x==crib.x&&end.look_x==crib.look_x);
  world_draw_nursery(&s,222u,0,0u,3,0,1u,0,1,40u,&midway,0,0);
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
 }
 /* NAP openings use positive-area rounded triangles and then closed
  * creases; no rectangular shutter or collapsed GL_QUAD_STRIP pole
  * that becomes a horizontal pixel ruler at half-closure. */
 {
  unsigned age;
  s.mood=95;
  for(age=0u;age<=56u;age++){
   world_draw_nursery(&s,173u+age,4,age,3,-1,16u,
                      0,1,40u,0,0,0);
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
 }
 /* A new nursery mobile is physically anchored to the actual crib bar.
  * One 40-tick reaction with no repeating rocking while CRIB is selected.
  * The added meshes must leave headroom below DS 1850-polygon budget. */
 assert(world_crib_mobile_swing(-1,0u)==0);
 assert(world_crib_mobile_swing(0,8u)==0);
 assert(world_crib_mobile_swing(3,0u)==0);
 assert(world_crib_mobile_swing(3,10u)==-12);
 assert(world_crib_mobile_swing(3,22u)==8);
 assert(world_crib_mobile_swing(3,40u)==0);
 assert(world_crib_mobile_swing(3,5000u)==0);
 {
  unsigned tick;
  int prev=world_crib_mobile_swing(3,0u);
  for(tick=1u;tick<50u;tick++){
   int angle=world_crib_mobile_swing(3,tick);
   assert(angle>=-12&&angle<=8);
   assert(angle-prev<=3&&prev-angle<=3);
   prev=angle;
   world_draw_nursery(&s,350u+tick,4,18u,3,-1,16u,
                      0,0,tick,0,0,0);
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
  /* The same four hanging pieces exist in regular care and Toybox
   * at rest; film continuity never depends on a room-only prop. */
  world_draw(&s,402u,0);
  assert(world_polygon_count()<=1850);
  roughhouse_init(&r);
  world_draw_roughhouse(&s,403u,&r);
  assert(world_polygon_count()<=1850);
 }
 /* Natural plush animation: single eased bow, held, then recovery.
  * A selected toy must NOT animate endlessly or detach its face. */
 assert(world_teddy_bow_angle(0u)==0);
 assert(world_teddy_bow_angle(8u)==-12);
 assert(world_teddy_bow_angle(17u)==-12);
 assert(world_teddy_bow_angle(40u)==0);
 assert(world_teddy_bow_angle(80u)==0);
 {
  unsigned tick;
  int old=world_teddy_bow_angle(0u);
  for(tick=1u;tick<52u;tick++){
   int v=world_teddy_bow_angle(tick);
   assert(v<=0&&v>=-12);
   assert(v-old<=3 && old-v<=3);
   old=v;
   world_draw_nursery(&s,300u+tick,0,0u,2,-1,16u,0,1,tick,0,0,0);
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
 }
 /* Gaze targets rotate the whole head, remain legible in profile and
  * return to neutral on leaving ROOM. */
 assert(world_baby_attention_angle(-1,16u)==0);
 assert(world_baby_attention_angle(0,0u)==0);
 assert(world_baby_attention_angle(0,16u)==-6);
 assert(world_baby_attention_angle(1,16u)==-7);
 assert(world_baby_attention_angle(2,16u)==-5);
 assert(world_baby_attention_angle(3,16u)==6);
 for(i=0;i<4;i++){
  int old=0;
  unsigned tick;
  for(tick=0u;tick<=16u;tick++){
   int v=world_baby_attention_angle(i,tick);
   assert(v>=-7&&v<=6);
   assert(v-old<=2&&old-v<=2);
   old=v;
  }
 }
 /* A room retarget starts at the current head angle, not at neutral.
  * Exercise every prop/neutral pair, plus interruptions at EVERY tick:
  * rapid WINDOW->CRIB->TEDDY and BACK retain the visible pose. */
 {
  int from_focus,to_focus;
  for(from_focus=-1;from_focus<4;from_focus++){
   int from=world_baby_attention_angle(from_focus,16u);
   for(to_focus=-1;to_focus<4;to_focus++){
    int target=world_baby_attention_angle(to_focus,16u),previous=from;
    unsigned age;
    assert(world_baby_attention_from(from,to_focus,0u)==from);
    assert(world_baby_attention_from(from,to_focus,16u)==target);
    assert(world_baby_attention_from(from,to_focus,UINT32_MAX)==target);
    for(age=0u;age<=16u;age++){
     int sampled=world_baby_attention_from(from,to_focus,age),next_focus;
     assert(sampled>=-7&&sampled<=6);
     assert(sampled-previous<=2&&previous-sampled<=2);
     if(target>=from)assert(sampled>=previous&&sampled<=target);
     else assert(sampled<=previous&&sampled>=target);
     for(next_focus=-1;next_focus<4;next_focus++){
      int restart=world_baby_attention_from(sampled,next_focus,0u);
      assert(restart==sampled); /* no one-frame head jump on reversal */
     }
     previous=sampled;
    }
   }
  }
  /* Snapshot a genuinely incomplete turn, then immediately reverse and
   * return to the player while the original camera is still travelling. */
  {
   int lamp=world_baby_attention_from(0,1,11u);
   int toward_crib=world_baby_attention_from(lamp,3,3u);
   int towards_baby=world_baby_attention_from(toward_crib,-1,1u);
   assert(lamp<0&&toward_crib<0);
   assert(world_baby_attention_from(lamp,3,0u)==lamp);
   assert(world_baby_attention_from(toward_crib,-1,0u)==toward_crib);
   assert(towards_baby>=toward_crib&&towards_baby<=0);
   assert(world_baby_attention_from(toward_crib,-1,16u)==0);
  }
  /* Attention changes only an existing transform: same mesh submissions,
   * same saved needs, no geometry growth during either turn direction. */
  {
   Game original=s;
   unsigned vertices,polygons;
   world_draw_nursery(&s,128u,0,56u,3,1,6u,0,1,40u,0,0,0);
   vertices=world_vertex_count();polygons=world_polygon_count();
   world_draw_nursery(&s,128u,0,56u,3,1,6u,0,1,40u,0,-7,0);
   assert(world_vertex_count()==vertices&&world_polygon_count()==polygons);
   assert(vertices<=5000&&polygons<=1850);
   assert(memcmp(&original,&s,sizeof(s))==0);
  }
 }
 /* Arm gestures now rotate one connected shoulder-to-hand structure.
  * FEED reaches, CHAT invites, PLAY reaches farther, NAP folds in. */
 assert(world_baby_arm_reach(0)==7);
 assert(world_baby_arm_reach(1)==28);
 assert(world_baby_arm_reach(2)==45);
 assert(world_baby_arm_reach(3)==57);
 assert(world_baby_arm_reach(4)==-8);
 assert(world_baby_neck_tilt(0,0u)==0);
 assert(world_baby_neck_tilt(4,123u)==-9);
 assert(world_baby_neck_tilt(2,40u)>world_baby_neck_tilt(2,0u));
 assert(world_baby_neck_tilt(3,40u)>world_baby_neck_tilt(3,0u));
 assert(world_baby_neck_tilt(2,0u)==world_baby_neck_tilt(2,80u));
 assert(world_baby_arm_reach(3)>world_baby_arm_reach(2));
 assert(world_baby_arm_reach(2)>world_baby_arm_reach(1));
 /* Hostile acting has its own connected, finite pose. A displayed windup
  * can be held at its settled age by the application, then be captured
  * exactly when a lunge, guard or ordinary care interrupts it. */
 {
  Game sample=s,original;
  WorldBabyMotion motion;
  WorldBabyPose idle,windup,lunge;
  unsigned age;
  int action;
  sample.energy=80;sample.mood=85;sample.hunger=20;sample.chaos=10;
  original=sample;
  idle=world_baby_pose(&sample,128u,0,0u,0);
  windup=world_baby_pose(&sample,128u,WORLD_BABY_WINDUP,24u,0);
  lunge=world_baby_pose(&sample,128u,WORLD_BABY_LUNGE,24u,0);
  assert(windup.forward_left>idle.forward_left);
  assert(lunge.forward_left>windup.forward_left);
  assert(windup.lid<idle.lid&&lunge.lid<idle.lid);
  assert(windup.neck!=lunge.neck);
  for(action=WORLD_BABY_WINDUP;action<=WORLD_BABY_LUNGE;action++){
   for(age=0u;age<=56u;age++){
    WorldBabyPose p=world_baby_pose(&sample,128u,action,age,0);
    assert(p.reach_left>=0&&p.reach_left<=54);
    assert(p.reach_right>=0&&p.reach_right<=54);
    assert(p.forward_left>=0&&p.forward_left<=32);
    assert(p.forward_right>=0&&p.forward_right<=32);
    assert(p.lid>=0&&p.lid<=256);
    world_draw_care(&sample,128u,action,age);
    assert(world_vertex_count()<=4500);
    assert(world_polygon_count()<=1700);
   }
   assert_baby_pose_equal(world_baby_pose(&sample,128u,action,56u,0),idle);
   assert_baby_pose_equal(world_baby_pose(&sample,128u,action,UINT32_MAX,0),idle);
  }
  world_baby_motion_start(&motion,windup,&sample,128u,WORLD_BABY_LUNGE);
  assert_baby_pose_equal(world_baby_pose(&sample,128u,WORLD_BABY_LUNGE,0u,&motion),windup);
  world_baby_motion_start(&motion,lunge,&sample,128u,WORLD_BABY_REGROUP);
  assert_baby_pose_equal(world_baby_pose(&sample,128u,WORLD_BABY_REGROUP,0u,&motion),lunge);
  assert(memcmp(&sample,&original,sizeof(sample))==0);
 }
 /* Completed rounds have their own positive, finite acting. CHEER's
  * raised hands and single lift differ from PLAY; REGROUP offers low
  * hands and one small head tilt without a fall, shake or prolonged sulk. */
 assert(WORLD_BABY_CHEER==7&&WORLD_BABY_REGROUP==8);
 {
  Game baby,original;
  WorldBabyPose cheer,regroup,idle;
  int action;
  unsigned age;
  game_init(&baby);original=baby;
  idle=world_baby_pose(&baby,128u,0,0u,0);
  cheer=world_baby_pose(&baby,128u,WORLD_BABY_CHEER,18u,0);
  regroup=world_baby_pose(&baby,128u,WORLD_BABY_REGROUP,18u,0);
  assert(cheer.reach_left==76&&cheer.reach_right==76);
  assert(cheer.forward_left==9&&cheer.forward_right==9);
  assert(cheer.neck==4&&cheer.bounce==3072&&cheer.lid==256);
  assert(regroup.reach_left==20&&regroup.reach_right==20);
  assert(regroup.forward_left==12&&regroup.forward_right==12);
  assert(regroup.neck==-4&&regroup.bounce==0&&regroup.lid==208);
  for(action=WORLD_BABY_CHEER;action<=WORLD_BABY_REGROUP;action++){
   WorldBabyPose previous=world_baby_pose(&baby,128u,action,0u,0);
   assert_baby_pose_equal(previous,idle);
   for(age=1u;age<=180u;age++){
    WorldBabyPose p=world_baby_pose(&baby,128u,action,age,0);
    assert_baby_pose_bounds_to(p,76);
    assert_baby_pose_step_to(previous,p,13);
    if(age<=12u){
     assert(p.reach_left>=previous.reach_left);
     assert(p.forward_left>=previous.forward_left);
    }else if(age<=30u)assert(p.reach_left==previous.reach_left);
    else assert(p.reach_left<=previous.reach_left);
    if(action==WORLD_BABY_CHEER){
     if(age<=18u)assert(p.bounce>=previous.bounce);
     else assert(p.bounce<=previous.bounce);
     if(age>=40u)assert(p.bounce==0);
    }else{
     assert(p.bounce==0);
     if(age<=12u)assert(p.lid<=previous.lid);
     if(age>=30u)assert(p.lid>=previous.lid);
     if(age>=19u&&age<=34u)assert(p.neck>=previous.neck);
    }
    if(age>=56u)assert_baby_pose_equal(p,idle);
    submitted_hand_caps=0;
    world_draw_care(&baby,128u,action,age);
    assert(submitted_hand_caps==32u);
    assert(world_vertex_count()<=4702u&&world_polygon_count()<=1808u);
    previous=p;
   }
   assert_baby_pose_equal(world_baby_pose(&baby,128u,action,UINT32_MAX,0),idle);
  }
  assert(memcmp(&baby,&original,sizeof(baby))==0);
  /* A comforting expression must not worsen existing tired eyes. */
  baby.energy=0;
  idle=world_baby_pose(&baby,128u,0,0u,0);
  regroup=world_baby_pose(&baby,128u,WORLD_BABY_REGROUP,18u,0);
  assert(regroup.lid==idle.lid&&regroup.lid>=96);
 }
 /* Check the actual submitted face geometry at low mood. Both outcomes
  * retain their positive expression; ordinary idle still reflects needs.
  * The shim records geometry only, not the DS's transformed pixels. */
 {
  Game baby;
  uint32_t bright_regroup,bright_cheer,bright_idle,dull_idle;
  game_init(&baby);
  bright_regroup=baby_geometry(&baby,WORLD_BABY_REGROUP,18u);
  bright_cheer=baby_geometry(&baby,WORLD_BABY_CHEER,18u);
  bright_idle=baby_geometry(&baby,0,0u);
  baby.mood=0;
  dull_idle=baby_geometry(&baby,0,0u);
  assert(bright_idle!=dull_idle);
  assert(baby_geometry(&baby,WORLD_BABY_REGROUP,18u)==bright_regroup);
  assert(baby_geometry(&baby,WORLD_BABY_CHEER,18u)==bright_cheer);
  assert(baby_geometry(&baby,WORLD_BABY_REGROUP,56u)==dull_idle);
  assert(baby_geometry(&baby,WORLD_BABY_CHEER,56u)==dull_idle);
 }
 /* Every care/result pairing, every interruption age, and sleepy needs.
  * Result-to-care, care-to-result and repeated results all begin at the
  * sampled rig and join their new curve at12, including blink boundaries. */
 {
  static const int actions[]={0,1,2,3,4,WORLD_BABY_CHEER,WORLD_BABY_REGROUP};
  static const uint8_t energies[]={0,24,54,100};
  unsigned from,to,need,old_age,age;
  for(from=0u;from<sizeof(actions)/sizeof(actions[0]);from++){
   for(to=1u;to<sizeof(actions)/sizeof(actions[0]);to++){
    if(actions[from]<WORLD_BABY_CHEER&&actions[to]<WORLD_BABY_CHEER)continue;
    for(need=0u;need<sizeof(energies)/sizeof(energies[0]);need++){
     for(old_age=0u;old_age<=56u;old_age++){
      Game baby,original;
      WorldBabyMotion motion;
      WorldBabyPose captured,previous;
      uint32_t frame=164u+old_age;
      game_init(&baby);
      baby.energy=energies[need];baby.hunger=92;baby.chaos=96;
      captured=world_baby_pose(&baby,frame,actions[from],old_age,0);
      if(actions[to]<=4)(void)game_care(&baby,(Care)(actions[to]-1));
      original=baby;
      world_baby_motion_start(&motion,captured,&baby,frame,actions[to]);
      previous=world_baby_pose(&baby,frame,actions[to],0u,&motion);
      assert_baby_pose_equal(previous,captured);
      for(age=0u;age<=56u;age++){
       WorldBabyPose p=world_baby_pose(&baby,frame+age,actions[to],age,&motion);
       assert_baby_pose_bounds_to(p,76);
       assert_baby_pose_step_to(previous,p,13);
       if(age>=12u)
        assert_baby_pose_equal(p,world_baby_pose(&baby,frame+age,
                                                actions[to],age,0));
       if(need==0u&&old_age==18u){
        submitted_hand_caps=0;
        world_draw_care_from(&baby,frame+age,actions[to],age,&motion);
        assert(submitted_hand_caps==32u);
        assert(world_vertex_count()<=4702u&&world_polygon_count()<=1808u);
       }
       previous=p;
      }
      assert_baby_pose_equal(previous,world_baby_pose(&baby,frame+56u,0,0u,0));
      assert(memcmp(&baby,&original,sizeof(baby))==0);
     }
    }
   }
  }
 }
 /* A genuine arcade finish changes mood/chaos before the new pose starts.
  * Capture before the decisive input and ensure acting never increments
  * wins/losses, advances RNG, or applies another care command. */
 {
  int won;
  for(won=0;won<=1;won++){
   Game baby,finished;
   Arcade arcade;
   WorldBabyPose captured;
   WorldBabyMotion motion;
   unsigned round,age,attempts=won?5u:2u;
   int reaction=won?WORLD_BABY_CHEER:WORLD_BABY_REGROUP;
   game_init(&baby);baby.mood=20;baby.chaos=68;baby.hunger=92;
   game_arcade_start(&baby,&arcade,5);
   for(round=0u;round<attempts;round++){
    int key=game_arcade_expected(&arcade);
    game_arcade_input(&baby,&arcade,won?key:(key+1)%3);
    assert(arcade.active);
   }
   captured=world_baby_pose(&baby,128u,3,18u,0);
   {
    int key=game_arcade_expected(&arcade);
    game_arcade_input(&baby,&arcade,won?key:(key+1)%3);
   }
   assert(!arcade.active&&arcade.finished==(won?1:2));
   assert(baby.wins==(unsigned)won&&baby.losses==(unsigned)!won);
   finished=baby;
   world_baby_motion_start(&motion,captured,&baby,128u,reaction);
   assert_baby_pose_equal(world_baby_pose(&baby,128u,reaction,0u,&motion),
                          captured);
   for(age=0u;age<=56u;age++)
    world_draw_care_from(&baby,128u+age,reaction,age,&motion);
   assert(memcmp(&finished,&baby,sizeof(baby))==0);
  }
 }
 /* Alternating and repeated outcomes may themselves interrupt a blend.
  * Rapid care afterwards must still capture the current animated rig. */
 {
  static const int sequence[]={WORLD_BABY_CHEER,WORLD_BABY_CHEER,
    WORLD_BABY_REGROUP,WORLD_BABY_REGROUP,3,4,WORLD_BABY_CHEER,1,
    WORLD_BABY_REGROUP,2};
  Game baby;
  WorldBabyMotion motion={{0},{0}};
  WorldBabyPose displayed;
  unsigned event,tick,age=0u;
  uint32_t frame=128u;
  int action=0;
  game_init(&baby);baby.energy=5;baby.hunger=93;baby.chaos=96;
  displayed=world_baby_pose(&baby,frame,0,0u,0);
  for(event=0u;event<256u;event++){
   WorldBabyPose captured=world_baby_pose(&baby,frame,action,age,
                                          action?&motion:0);
   action=sequence[event%(sizeof(sequence)/sizeof(sequence[0]))];
   if(action<=4)(void)game_care(&baby,(Care)(action-1));
   world_baby_motion_start(&motion,captured,&baby,frame,action);age=0u;
   assert_baby_pose_equal(world_baby_pose(&baby,frame,action,age,&motion),
                          captured);
   for(tick=0u;tick<1u+event%12u;tick++){
    WorldBabyPose current=world_baby_pose(&baby,frame,action,age,&motion);
    assert_baby_pose_bounds_to(current,76);
    assert_baby_pose_step_to(displayed,current,13);
    displayed=current;frame++;age++;
   }
  }
  for(;age<=56u;age++,frame++){
   WorldBabyPose current=world_baby_pose(&baby,frame,action,age,&motion);
   assert_baby_pose_step_to(displayed,current,13);
   displayed=current;
  }
  assert_baby_pose_equal(displayed,world_baby_pose(&baby,frame-1u,0,0u,0));
 }
 /* Regression: held PLAY has a wide reach and raised body. Starting
  * FEED/CHAT/NAP previously reset those channels to idle in one frame.
  * The renderer now consumes the same compact rig we capture here. */
 {
  Game rested;
  WorldBabyPose play,feed,idle;
  WorldBabyMotion motion;
  game_init(&rested);
  assert(sizeof(WorldBabyPose)==18u&&sizeof(WorldBabyMotion)==36u);
  play=world_baby_pose(&rested,128u,3,18u,0);
  assert(play.reach_left==59&&play.reach_right==55);
  assert(play.forward_left==10&&play.forward_right==10);
  assert(play.lean==5&&play.neck==5&&play.yaw==-2);
  assert(play.lid==256&&play.bounce==4608);
  world_baby_motion_start(&motion,play,&rested,128u,1);
  assert_baby_pose_equal(world_baby_pose(&rested,128u,1,0u,&motion),play);
  feed=world_baby_pose(&rested,134u,1,6u,&motion);
  assert(feed.reach_left>28&&feed.reach_left<play.reach_left);
  assert(feed.reach_right>28&&feed.reach_right<play.reach_right);
  assert(feed.bounce>0&&feed.bounce<play.bounce);
  assert_baby_pose_equal(world_baby_pose(&rested,140u,1,12u,&motion),
                         world_baby_pose(&rested,140u,1,12u,0));
  idle=world_baby_pose(&rested,184u,0,0u,0);
  assert_baby_pose_equal(world_baby_pose(&rested,184u,1,56u,&motion),idle);
 }
 /* Every care pair (including repeated taps), every old interruption
  * age, and both sides of the sleepy arm/eye thresholds. Capturing occurs
  * BEFORE game_care, so the start pose survives its hunger/energy changes.
  * Frames also cross the automatic blink: masks remain within their eyes. */
 {
  static const uint8_t energies[]={0,18,24,30,54,55,100};
  unsigned need,old_age,age;
  int old_action,new_action;
  for(need=0u;need<sizeof(energies)/sizeof(energies[0]);need++){
   for(old_action=0;old_action<=4;old_action++){
    for(new_action=1;new_action<=4;new_action++){
     for(old_age=0u;old_age<=56u;old_age++){
      Game before,after,unchanged;
      WorldBabyMotion motion;
      WorldBabyPose captured,previous,first;
      uint32_t frame=164u+old_age;
      game_init(&before);
      before.energy=energies[need];before.hunger=92;before.chaos=96;
      captured=world_baby_pose(&before,frame,old_action,old_age,0);
      after=before;
      (void)game_care(&after,(Care)(new_action-1));
      unchanged=after;
      world_baby_motion_start(&motion,captured,&after,frame,new_action);
      first=world_baby_pose(&after,frame,new_action,0u,&motion);
      assert_baby_pose_equal(first,captured);
      /* main increments frame/pose_age after drawing. Its next input
       * tick advances the old curve normally, then preserves THAT pose. */
      if(old_age){
       WorldBabyPose displayed=world_baby_pose(&before,frame-1u,
                                                old_action,old_age-1u,0);
       assert_baby_pose_step(displayed,first);
      }
      previous=first;
      for(age=0u;age<=56u;age++){
       WorldBabyPose sampled=world_baby_pose(&after,frame+age,
                                             new_action,age,&motion);
       assert_baby_pose_bounds(sampled);
       assert_baby_pose_step(previous,sampled);
       if(age>=12u)
        assert_baby_pose_equal(sampled,world_baby_pose(&after,frame+age,
                                                       new_action,age,0));
       if(need==0u&&old_age==18u){
        submitted_hand_caps=0;
        world_draw_care_from(&after,frame+age,new_action,age,&motion);
        assert(submitted_hand_caps==32u);
        assert(world_vertex_count()<=4702u&&world_polygon_count()<=1808u);
       }
       previous=sampled;
      }
      assert_baby_pose_equal(previous,
                             world_baby_pose(&after,frame+56u,0,0u,0));
      assert_baby_pose_equal(world_baby_pose(&after,frame,0,UINT32_MAX,&motion),
                             world_baby_pose(&after,frame,0,UINT32_MAX,0));
      assert(memcmp(&after,&unchanged,sizeof(after))==0);
     }
    }
   }
  }
 }
 /* A start from unchanged idle must retain every existing anticipation,
  * hold and recovery sample: applying a second ease would slow it down. */
 {
  Game rested;
  int action;
  unsigned age;
  game_init(&rested);
  for(action=1;action<=4;action++){
   WorldBabyMotion motion;
   WorldBabyPose idle=world_baby_pose(&rested,167u,0,56u,0);
   world_baby_motion_start(&motion,idle,&rested,167u,action);
   for(age=0u;age<=58u;age++)
    assert_baby_pose_equal(world_baby_pose(&rested,167u+age,action,age,&motion),
                           world_baby_pose(&rested,167u+age,action,age,0));
  }
 }
 /* Re-interrupt the IN-FLIGHT blend (not its original endpoint): rapid
  * alternating/repeated care cannot accumulate offsets or jump to idle.
  * Include Room quiet-rest's extra needs update before final baselining. */
 {
  Game baby;
  WorldBabyMotion motion={{0},{0}};
  WorldBabyPose displayed;
  unsigned tap,tick,age=0u;
  uint32_t frame=128u;
  int action=0;
  game_init(&baby);baby.energy=5;baby.hunger=93;baby.chaos=96;
  displayed=world_baby_pose(&baby,frame,0,0u,0);
  for(tap=0u;tap<512u;tap++){
   unsigned duration=1u+(tap%12u);
   WorldBabyPose captured=world_baby_pose(&baby,frame,action,age,
                                          action?&motion:0);
   action=1+(int)((tap/2u)%4u); /* each action repeats before switching */
   (void)game_care(&baby,(Care)(action-1));
   if(action==4)(void)game_room_quiet_rest(&baby,0,0);
   world_baby_motion_start(&motion,captured,&baby,frame,action);
   age=0u;
   assert_baby_pose_equal(world_baby_pose(&baby,frame,action,0u,&motion),
                          captured);
   for(tick=0u;tick<duration;tick++){
    WorldBabyPose current=world_baby_pose(&baby,frame,action,age,&motion);
    assert_baby_pose_bounds(current);
    assert_baby_pose_step(displayed,current);
    displayed=current;
    frame++;age++;
   }
  }
  for(;age<=56u;age++,frame++){
   WorldBabyPose current=world_baby_pose(&baby,frame,action,age,&motion);
   assert_baby_pose_step(displayed,current);
   displayed=current;
  }
  assert_baby_pose_equal(displayed,world_baby_pose(&baby,frame-1u,0,0u,0));
  /* Care retargeting remains a rig-only change inside the moving Room
   * camera; the saved Game and the furniture attention sampler are pure. */
  {
   Game original=baby;
   int attention=world_baby_attention_from(-7,3,6u);
   world_draw_nursery(&baby,frame,action,6u,3,1,6u,0,0,6u,0,-7,&motion);
   assert(world_vertex_count()<=4702u&&world_polygon_count()<=1808u);
   assert(memcmp(&baby,&original,sizeof(baby))==0);
   assert(world_baby_attention_from(-7,3,6u)==attention);
  }
 }
 /* Both rotating hands retain their complete 8-face top and bottom caps.
  * Reverting them to collapsed quad-strip poles removes these triangles
  * and can reintroduce the brown FEED scanline seen in native DeSmuME. */
 {
  int action;
  unsigned age;
  for(action=0;action<=4;action++){
   for(age=0u;age<=56u;age++){
    submitted_hand_caps=0;
    world_draw_care(&s,128u,action,age);
    assert(submitted_hand_caps==32u); /* 2 hands x 2 caps x 8 faces */
    assert(world_vertex_count()<=5000&&world_polygon_count()<=1850);
   }
  }
 }
 /* The same care-age sequence replays consistently no matter when
  * the physical button is pressed. Keep the 80-age curve in range. */
 {
  unsigned age;
  for(age=0;age<80u;age++){
   int chat=world_baby_neck_tilt(2,age);
   int play=world_baby_neck_tilt(3,age);
   assert(chat>=-1&&chat<=7);
   assert(play>=3&&play<=7);
   assert(chat==world_baby_neck_tilt(2,age+80u));
   assert(play==world_baby_neck_tilt(3,age+80u));
  }
 }
 /* Live baby needs must read as physical feelings on the existing rig.
  * A fully rested infant stays wide-eyed; a depleted baby looks sleepy
  * and hungry without ever disappearing or changing the save format. */
 assert(world_baby_drowsy_lid(-100)==96);
 assert(world_baby_drowsy_lid(0)==96);
 assert(world_baby_drowsy_lid(55)==256);
 assert(world_baby_drowsy_lid(100)==256);
 assert(world_baby_drowsy_lid(20)<world_baby_drowsy_lid(40));
 {
  int energy;
  for(energy=0;energy<=100;energy++){
   int lid=world_baby_drowsy_lid(energy);
   assert(lid>=96&&lid<=256);
   if(energy)assert(lid>=world_baby_drowsy_lid(energy-1));
  }
 }
 assert(world_baby_need_fidget(35,12,0u)==0);
 assert(world_baby_need_fidget(70,12,0u)==-1);
 assert(world_baby_need_fidget(35,70,0u)==-1);
 assert(world_baby_need_fidget(100,100,0u)==-4);
 assert(world_baby_need_fidget(100,100,60u)==4);
 {
  unsigned frame;
  int old=world_baby_need_fidget(100,100,0u);
  for(frame=1u;frame<=250u;frame++){
   int current=world_baby_need_fidget(100,100,frame);
   assert(current>=-4&&current<=4);
   assert(current-old<=1&&old-current<=1);
   assert(current==world_baby_need_fidget(100,100,frame+120u));
   old=current;
  }
 }
 {
  Game baseline=s;
  unsigned rested,exhausted,distressed;
  s.mood=90;s.energy=80;s.hunger=35;s.chaos=12;
  world_draw_care(&s,128u,0,56u);
  rested=world_polygon_count();
  assert(world_vertex_count()<=5000&&rested<=1850);
  s.energy=5;s.hunger=92;s.chaos=96;
  world_draw_care(&s,128u,0,56u);
  exhausted=world_polygon_count();
  /* Drowsy eyes reshape the same safe surfaces. They can omit two
   * catchlights, but no longer add rectangular skin shutters. */
  assert(exhausted<=rested&&rested-exhausted<=2u);
  assert(exhausted<=1850);
  assert(world_vertex_count()<=5000);
  /* A care gesture must still look coherent when sleep is low. */
  world_draw_care(&s,145u,2,20u);
  distressed=world_polygon_count();
  assert(distressed<=1850&&world_vertex_count()<=5000);
  s=baseline;
 }
 /* No abrupt chest pop at wrap, no idle breathing during Toybox
  * collisions, and no new render geometry at the breath peak. */
 assert(world_baby_breath(0u)==0);
 assert(world_baby_breath(64u)==256);
 assert(world_baby_breath(128u)==0);
 assert(world_baby_breath(192u)==256);
 {
  unsigned age,prev=0;
  for(age=1u;age<=256u;age++){
   unsigned current=(unsigned)world_baby_breath(age);
   assert(current<=256u);
   assert(current-prev<=7u || prev-current<=7u);
   prev=current;
  }
 }
 {
  unsigned baseline,peak;
  s.mood=12;
  /* Frame zero is also the first instant of the automatic eyelid blink:
   * comparing it with 64 measures a blink, not the breathing pass.
   * Frame 128 is the same zero-breath phase, safely outside blink. */
  world_draw_care(&s,128u,0,56u);
  baseline=world_polygon_count();
  world_draw_care(&s,64u,0,56u);
  peak=world_polygon_count();
  assert(baseline==peak && peak<=1850u);
 }
 /* Finite staged animation: anticipatory ease-in, expressive hold,
  * graceful return to idle (not indefinitely repeating CHAT/PLAY forever). */
 assert(world_baby_care_weight(0u)==0);
 assert(world_baby_care_weight(6u)>0);
 assert(world_baby_care_weight(12u)==256);
 assert(world_baby_care_weight(25u)==256);
 assert(world_baby_care_weight(30u)==256);
 assert(world_baby_care_weight(42u)>0);
 assert(world_baby_care_weight(55u)>=0);
 assert(world_baby_care_weight(56u)==0);
 {
  unsigned f;
  int prev=world_baby_care_weight(0);
  for(f=1u;f<58u;f++){
   int weight=world_baby_care_weight(f);
   assert(weight>=0&&weight<=256);
   assert(weight-prev<=35&&prev-weight<=35);
   prev=weight;
  }
 }
 assert(world_baby_speech_open(0u)==0);
 assert(world_baby_speech_open(8u)==0);
 assert(world_baby_speech_open(14u)>=250);
 assert(world_baby_speech_open(20u)==0);
 assert(world_baby_speech_open(26u)>=250);
 assert(world_baby_speech_open(44u)==0);
 assert(world_baby_lid_open(10u,0,25u)==256);
 assert(world_baby_lid_open(10u,4,25u)==0);
 assert(world_baby_lid_open(10u,4,0u)==256);
 assert(world_baby_lid_open(10u,4,56u)==256);
 assert(world_baby_lid_open(3u,0,25u)<80);
 for(i=0;i<58;i++){
  world_draw_care(&s,(uint32_t)(200+i),3,(uint32_t)i);
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
 }
 /* CHAT has three visible syllables at either mood. In particular an
  * unhappy baby must not produce a full babble soundtrack with an
  * unmoving flat mouth when nursery audio is enabled. */
 {
  unsigned sad_idle,sad_chat;
  s.mood=10;
  world_draw_care(&s,15u,0,20u);
  sad_idle=world_polygon_count();
  world_draw_care(&s,15u,2,20u);
  sad_chat=world_polygon_count();
  assert(sad_chat>sad_idle && sad_chat-sad_idle<=25u);
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
 /* Toybox contact has a brief attached facial recoil: the mouth opens
  * on an actual hit, then closes to its normal relaxed expression.
  * It costs no additional polygons outside the short hit pulse. */
 {
  unsigned relaxed,shocked;
  roughhouse_init(&r);
  s.mood=95;
  world_draw_roughhouse(&s,410u,&r);
  relaxed=world_polygon_count();
  r.impact=9;r.moving=1;
  world_draw_roughhouse(&s,411u,&r);
  shocked=world_polygon_count();
  assert(shocked>relaxed && shocked-relaxed<=25u);
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
  r.impact=0;r.moving=0;
  world_draw_roughhouse(&s,412u,&r);
  assert(world_polygon_count()==relaxed);
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
   /* Scene must NOT collapse to a floating head on a featureless plane.
    * Nursery, split porcelain sides, cavity and attached face all remain
    * submitted after the hard cut, but fit the same DS polygon budget. */
   assert(world_polygon_count()>=650);
   /* The organic cheek details add four tiny triangles in place of
    * rectangle strips, while leaving ample DS hardware headroom. */
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
 }
 /* Physics can throw the toy out of frame. The movie introduction is a
  * consistent damaged portrait while preserving the actual source state. */
 {
  Roughhouse original,staged,again;
  roughhouse_init(&original);
  original.x=390;original.y=345;original.z=-192;
  original.roll=155;original.pitch=-130;original.yaw=210;
  original.moving=1;original.charge=ROUGHHOUSE_CRACK_CHARGE;
  staged=world_egg_staged_doll(&original,17u);
  again=world_egg_staged_doll(&original,17u);
  assert(staged.x==0&&staged.y==0&&staged.z==0);
  assert(!staged.moving && !staged.impact);
  assert(staged.roll>=-18&&staged.roll<=18);
  assert(staged.pitch==0 && staged.yaw==0);
  assert(staged.charge==original.charge);
  assert(memcmp(&staged,&again,sizeof(staged))==0);
  assert(original.x==390&&original.y==345&&original.moving);
  world_draw_secret(&s,17u,&original);
  assert(world_vertex_count()<=5000);
  assert(world_polygon_count()<=1850);
  roughhouse_init(&original);
  staged=world_egg_staged_doll(&original,0);
  assert(staged.charge>=ROUGHHOUSE_CRACK_CHARGE);
  assert(!staged.moving);
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
  /* Two-track camera eases settle at the PUSH transition without
   * changing the actual nursery geometry or losing camera continuity. */
  {
   unsigned f;
   float prior=a.z;
   for(f=EGG_CRACK_FRAME+1u;f<EGG_SCARE_FRAME;f++){
    EggCamera next=world_secret_camera(f);
    float step=prior-next.z;
    assert(step>=0.f&&step<.035f);
    if(f==EGG_PUSH_FRAME||f==EGG_PUSH_FRAME+1u)
     assert(step<.005f);
    prior=next.z;
   }
  }
  assert(e.z>2.5f); /* single intentional jump-scare cut */
  assert(e.look_y<-.09f&&e.look_y>-.15f); /* jaw visible, not cropped */
 }
 /* Uncanny eye contact: the TWO pupils and catchlights move together,
  * with subpixel-small motion at distance and one deliberate lock-on
  * before the scare lunge. No frame-to-frame random saccades. */
 assert(world_egg_gaze(0u)==0);
 assert(world_egg_gaze(EGG_PUSH_FRAME)==0);
 assert(world_egg_gaze(EGG_PUSH_FRAME+12u)==-12);
 assert(world_egg_gaze(EGG_PUSH_FRAME+48u)==-12);
 assert(world_egg_gaze(EGG_PUSH_FRAME+72u)==0);
 assert(world_egg_gaze(EGG_SCARE_FRAME-1u)==7);
 assert(world_egg_gaze(EGG_SCARE_FRAME)==8);
 assert(world_egg_gaze(EGG_SCARE_FRAME+9u)==0);
 {
  unsigned tick;
  int old=world_egg_gaze(EGG_PUSH_FRAME);
  for(tick=EGG_PUSH_FRAME+1u;tick<EGG_TOTAL_FRAMES;tick++){
   int gaze=world_egg_gaze(tick),diff=gaze-old;
   assert(gaze>=-12&&gaze<=8);
   assert(diff>=-1&&diff<=1);
   old=gaze;
   world_draw_secret(&s,tick,&r);
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
 }
 /* Connected lower jaw: the tongue and fangs open together and settle
  * without strobing, repeat surprise cuts or frame-dependent randomness. */
 assert(world_egg_jaw_angle(EGG_SCARE_FRAME-1u)==0);
 assert(world_egg_jaw_angle(EGG_SCARE_FRAME)==0);
 assert(world_egg_jaw_angle(EGG_SCARE_FRAME+9u)==18);
 assert(world_egg_jaw_angle(EGG_SCARE_FRAME+22u)==8);
 {
  unsigned f;
  for(f=EGG_SCARE_FRAME+1u;f<EGG_TOTAL_FRAMES;f++){
   int jaw=world_egg_jaw_angle(f);
   int prev=world_egg_jaw_angle(f-1u);
   assert(jaw>=0&&jaw<=18);
   assert(jaw-prev<=2&&prev-jaw<=2);
  }
 }
 /* An opening eye glow grows as the camera approaches, then visibly
  * dims exactly once before the hard cut. Never flash the whole LCD. */
 assert(world_egg_eye_glow(0u)==0);
 assert(world_egg_eye_glow(EGG_CRACK_FRAME)==7);
 assert(world_egg_eye_glow(EGG_PUSH_FRAME)==7);
 assert(world_egg_eye_glow(EGG_SCARE_FRAME-20u)==31);
 assert(world_egg_eye_glow(EGG_SCARE_FRAME-1u)==4);
 assert(world_egg_eye_glow(EGG_SCARE_FRAME)==0);
 {
  unsigned f;
  int prev=world_egg_eye_glow(EGG_PUSH_FRAME);
  float last_approach=0.f;
  for(f=EGG_PUSH_FRAME+1u;f<EGG_SCARE_FRAME;f++){
   int v=world_egg_eye_glow(f);
   float approach=world_egg_embryo_approach(f);
   assert(v>=3&&v<=31);
   assert(v-prev<=1&&prev-v<=2);
   if(f<=EGG_SCARE_FRAME-20u)assert(v>=prev);
   else assert(v<=prev);
   prev=v;
   assert(approach>=last_approach&&approach<=.046f);
   last_approach=approach;
  }
  assert(world_egg_embryo_approach(0u)==0.f);
  assert(world_egg_embryo_approach(EGG_PUSH_FRAME)==0.f);
  assert(world_egg_embryo_approach(EGG_TOTAL_FRAMES)>.044f);
 }
 /* Active embryo, attached sac and a *single* smooth final withdrawal.
  * These deterministic samples should never overflow or leave the egg. */
 {
  unsigned tick;
  float last=0.f;
  assert(world_egg_membrane_pulse(0u)==0.f);
  assert(world_egg_membrane_pulse(EGG_SCARE_FRAME)==0.f);
  assert(world_egg_membrane_pulse(EGG_CRACK_FRAME)==0.f);
  assert(world_egg_membrane_pulse(EGG_CRACK_FRAME+32u)>.027f);
  assert(world_egg_embryo_retreat(EGG_SCARE_FRAME-18u)==0.f);
  assert(world_egg_embryo_retreat(EGG_SCARE_FRAME-1u)>.078f);
  assert(world_egg_embryo_retreat(EGG_SCARE_FRAME)==0.f);
  for(tick=EGG_CRACK_FRAME;tick<EGG_SCARE_FRAME;tick++){
   float pulse=world_egg_membrane_pulse(tick);
   assert(pulse>=0.f&&pulse<=.029f);
   if(tick>=EGG_SCARE_FRAME-18u){
    float retreat=world_egg_embryo_retreat(tick);
    assert(retreat>=last&&retreat<=.080f);
    assert(retreat-last<.007f);
    last=retreat;
   }else assert(world_egg_embryo_retreat(tick)==0.f);
   /* All incremental visuals, including changing egg wings and lens
    * movement, stay under the same real DS command budgets. */
   world_draw_secret(&s,tick,&r);
   assert(world_vertex_count()<=5000);
   assert(world_polygon_count()<=1850);
  }
 }
 /* Smooth physical shell split and restrained one-piece skull sway. */
 assert(world_egg_shell_open(EGG_PUSH_FRAME-1u)==0.f);
 assert(world_egg_shell_open(EGG_PUSH_FRAME)==0.f);
 assert(world_egg_shell_open(EGG_SCARE_FRAME)==1.f);
 assert(world_egg_shell_open(EGG_TOTAL_FRAMES)==1.f);
 assert(world_egg_head_tilt(EGG_SCARE_FRAME-1u)==0);
 assert(world_egg_head_tilt(EGG_SCARE_FRAME)==-3);
 assert(world_egg_head_tilt(EGG_SCARE_FRAME+9u)==2);
 {
  unsigned f;
  float prior=0.f;
  int old_tilt=world_egg_head_tilt(EGG_SCARE_FRAME);
  for(f=EGG_PUSH_FRAME+1u;f<EGG_SCARE_FRAME;f++){
   float width=world_egg_shell_open(f);
   assert(width>=prior&&width<=1.f);
   assert(width-prior<.012f);
   prior=width;
  }
  for(f=EGG_SCARE_FRAME+1u;f<EGG_TOTAL_FRAMES;f++){
   int tilt=world_egg_head_tilt(f);
   assert(tilt>=-3&&tilt<=2);
   assert(tilt-old_tilt<=1&&old_tilt-tilt<=1);
   old_tilt=tilt;
  }
 }
 /* The creature lunges ONCE after the final hard camera cut, then settles
  * into extremely small movement. All features use the SAME matrix. */
 {
  float peak=world_egg_creature_depth(EGG_SCARE_FRAME+9u);
  float start=world_egg_creature_depth(EGG_SCARE_FRAME);
  float settle=world_egg_creature_depth(EGG_SCARE_FRAME+22u);
  unsigned f;
  assert(world_egg_creature_depth(EGG_SCARE_FRAME-1u)==0.f);
  assert(start==0.f);
  assert(peak>.30f&&peak<.32f);
  assert(settle>.20f&&settle<.22f);
  for(f=EGG_SCARE_FRAME;f<EGG_TOTAL_FRAMES;f++){
   float motion=world_egg_creature_depth(f);
   assert(motion>=0.f&&motion<=.32f);
   if(f>EGG_SCARE_FRAME+22u)assert(motion>=.212f&&motion<.222f);
   if(f>EGG_SCARE_FRAME){
    float prev=world_egg_creature_depth(f-1u);
    assert(motion-prev<.036f && prev-motion<.009f);
   }
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

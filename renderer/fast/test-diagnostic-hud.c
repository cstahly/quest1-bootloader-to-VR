/* Off-device raster/geometry regression using the owner's private optical mesh.
 * Never opens framebuffer, OpenXR, or recovery devices. */
#define HUD_RECOVERY_STATUS_PATH "test-hud-recovery-status"
#define CAMERA_FEED_PATH "test-camera-feed"
#define CAMERA_PEEK_MAP_PATH "test-camera-peek-map"
#define QUEST_INK_PATH "test-spatial-ink"
#define main diagnostic_main
#include "monterey-head-mesh.c"
#undef main
#include <assert.h>
int main(int argc,char **argv) {
 assert(argc==3);
 /* Short click remains FX; long hold emits exactly one reset and no FX. */
 camera_controls_middle(10,0);camera_controls_middle(11,1);
 camera_controls_middle(11.5,0);assert(camera_controls.mode==1&&!camera_controls.recenter);
 camera_controls_middle(12,1);camera_controls_middle(13,1);assert(camera_controls.recenter);
 camera_controls_middle(14,1);assert(!camera_controls.recenter);
 camera_controls_middle(14.1,0);assert(camera_controls.mode==1);
 XrQuaternionf test_origin={0,0,0,1},forward={0,0,0,1};
 XrVector3f test_position_origin={0,0,0},here={1,2,3};
 positional_mode=1;
 assert(!recenter_view(forward,here,1,0,&test_origin,&test_position_origin));
 assert(test_position_origin.x==0);
 assert(recenter_view(forward,here,1,1,&test_origin,&test_position_origin));
 XrVector3f reset_position=rotate(conjugate(test_origin),subtract(here,test_position_origin));
 assert(fabsf(reset_position.x)+fabsf(reset_position.y)+fabsf(reset_position.z)<1e-6f);
 reset_position=rotate(conjugate(test_origin),subtract((XrVector3f){1.5,2,3},test_position_origin));
 assert(fabsf(reset_position.x-.5f)<1e-6f);
 XrQuaternionf tilted={sinf(.2f),0,0,cosf(.2f)};
 assert(recenter_view(tilted,here,1,1,&test_origin,&test_position_origin));
 assert(test_origin.x==0&&test_origin.z==0); /* Gravity stays up. */
 positional_mode=0;camera_controls.mode=0;
 tracking_reset_path="test-tracking-reset";
 assert(access(tracking_reset_path,F_OK)!=0);
 assert(tracking_reset_request());assert(tracking_reset_request());
 assert(access(tracking_reset_path,F_OK)==0);assert(unlink(tracking_reset_path)==0);
 tracking_reset_path=NULL;
 assert(quest_lens_load(&lens,argv[1]));quest_lens_prepare_fast(&lens);
 fb.stride=2880;fb.bytes=2880*1600*4;fb.pixels=calloc(1,fb.bytes);assert(fb.pixels);
 hud.fps=72;hud.draw=8.3;hud.wait=5.6;
 for(int i=0;i<90;i++)hud.intervals[i]=(i%17==0)?27.8:13.9;
 /* A publish can occur between sampling now and opening the status file.
  * This previously caused a healthy scene to exit after a few seconds. */
 FILE *status=fopen(HUD_RECOVERY_STATUS_PATH,"w");assert(status);
 fprintf(status,"%d 0 400 100.01 2\n",getpid());assert(!fclose(status));
 double remaining;unsigned resets;
 assert(hud_recovery(100,&remaining,&resets)&&resets==2&&remaining==300);
 assert(!hud_recovery(103,&remaining,&resets)); /* Stale status must still fail. */
 hud_draw(100,32,0);
 assert(unlink(HUD_RECOVERY_STATUS_PATH)==0);
 assert(hud.count>1000&&hud.count<HUD_PIXELS);
 unsigned left=0,right=0;
 for(unsigned i=0;i<hud.count;i++){
  assert(hud.pixels[i].index<2880*1600);
  if(hud.pixels[i].index%2880<1440)left++;else right++;
 }
 assert(left&&right);
 assert(hud_glyph('Z')&&hud_glyph('?')&&!hud_glyph(' '));
 struct quest_camera_feed *testfeed=calloc(1,sizeof(*testfeed));assert(testfeed);
 testfeed->magic=QUEST_CAMERA_MAGIC;testfeed->version=1;testfeed->width=320;testfeed->height=240;testfeed->cameras=4;testfeed->publication=1;
 for(int i=0;i<4;i++){testfeed->timestamp_ns[i]=99950000000ULL;testfeed->frames[i]=10;memset(testfeed->pixels[i],40+i*60,QUEST_CAMERA_PIXELS);}
 FILE *feedfile=fopen(CAMERA_FEED_PATH,"wb");assert(feedfile);assert(fwrite(testfeed,1,sizeof(*testfeed),feedfile)==sizeof(*testfeed));assert(!fclose(feedfile));
 XrQuaternionf facing_left={0,sinf(-42.f*.01745329252f/2),0,cosf(-42.f*.01745329252f/2)};
 camera_panel_draw(100,facing_left);assert(camera_panel.valid&&camera_panel.seen);
 /* Composite source selection and binocular wall geometry. */
 unsigned ph[4]={0x51464c52,1,320,240};size_t map_count=320*240;
 unsigned *peek_map=calloc(map_count,sizeof(unsigned));assert(peek_map);
 peek_map[0]=UINT_MAX;peek_map[1]=3*QUEST_CAMERA_PIXELS;
 FILE *pf=fopen(CAMERA_PEEK_MAP_PATH,"wb");assert(pf);assert(fwrite(ph,sizeof(ph),1,pf)==1);
 assert(fwrite(peek_map,sizeof(unsigned),map_count,pf)==map_count);assert(!fclose(pf));free(peek_map);
 camera_floor_update();assert(camera_peek.valid&&camera_peek.pixels[0]==0);
 assert(camera_peek.pixels[1]>camera_peek.pixels[2]);
 memset(fb.pixels,0,fb.bytes);
 camera_panel_draw(100,facing_left);
 unsigned floor_eyes[2]={0},shade=camera_peek.pixels[2]*0x010101;
 for(unsigned i=0;i<2880*1600;i++)if(fb.pixels[i]==shade)floor_eyes[i%2880<1440]++;
 assert(floor_eyes[0]>1000&&floor_eyes[1]>1000);
 XrVector3f wall_top=camera_world_point(4,.5f,0),wall_bottom=camera_world_point(4,.5f,1);
 assert(wall_top.x<-.5f&&wall_top.y>wall_bottom.y);
 assert(fabsf(wall_top.z-wall_bottom.z)<1e-6f); /* Upright, not floor. */
 camera_controls.mode=1;camera_floor_update();assert(camera_peek.pixels[2]==0);
 camera_controls.mode=0;camera_floor_update();assert(camera_peek.pixels[2]>0);
 memset(fb.pixels,0,fb.bytes);camera_panel_draw(100,facing_left);hud_draw(100,32,0);
 FILE *out=fopen(argv[2],"wb");assert(out);fprintf(out,"P6\n2880 1600\n255\n");
 /* Undo panel rotation for a readable optical-raster inspection artifact. */
 for(int y=0;y<1600;y++)for(int x=0;x<2880;x++){
  unsigned c=fb.pixels[(1599-y)*2880+2879-x];unsigned char rgb[]={c>>16,c>>8,c};
  assert(fwrite(rgb,1,3,out)==3);
 }
 assert(!fclose(out));printf("HUD: %u cached pixels, both eyes, bounds/capacity/font checks passed\n",hud.count);
 camera_panel.valid=0;camera_floor_update();assert(!camera_peek.valid);
 camera_panel.valid=1;assert(!unlink(CAMERA_PEEK_MAP_PATH));
 free(camera_peek.map);free(camera_peek.pixels);camera_peek.map=NULL;camera_peek.pixels=NULL;
 /* Head aim records distinct world points and persists completed strokes. */
 (void)unlink(QUEST_INK_PATH);ink=(__typeof__(ink)){0};camera_controls.distance=2.5f;camera_controls.draw=1;
 ink_draw(100,(XrQuaternionf){0,0,0,1});
 ink_draw(100.04,(XrQuaternionf){0,sinf(.1f),0,cosf(.1f)});
 camera_controls.draw=0;ink_draw(100.08,(XrQuaternionf){0,0,0,1});
 assert(ink.count==2&&ink.points[0].start&&!ink.points[1].start&&!ink.dirty);
 assert(fabsf(ink.points[0].point.z+2.5f)<.001f&&fabsf(ink.points[1].point.x)>.1f);
 ink=(__typeof__(ink)){0};ink_load();assert(ink.count==2);assert(!unlink(QUEST_INK_PATH));
 camera_panel_draw(101,facing_left);assert(!camera_panel.valid); /* No misleading frozen feed. */
 feedfile=fopen(CAMERA_FEED_PATH,"wb");assert(feedfile);fputs("truncated",feedfile);assert(!fclose(feedfile));
 camera_panel_draw(102,facing_left);assert(!camera_panel.valid);assert(!unlink(CAMERA_FEED_PATH));
 free(testfeed);
 free(fb.pixels);fb.pixels=NULL;return 0;
}

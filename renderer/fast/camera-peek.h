/* Small mono composite texture for the floor monitor. Both eyes share the
 * same camera selection; scene geometry supplies binocular panel placement.
 * The chosen-depth stitching remains approximate, not depth-aware passthrough. */
#ifndef QUEST_CAMERA_PEEK_H
#define QUEST_CAMERA_PEEK_H
#ifndef CAMERA_PEEK_MAP_PATH
#define CAMERA_PEEK_MAP_PATH "/var/lib/monado/monterey/floor-map.bin"
#endif
static struct {unsigned *map,publication;unsigned char *pixels;int tried,valid;} camera_peek;
static int camera_peek_load(void){
 if(camera_peek.tried)return camera_peek.map!=NULL;
 camera_peek.tried=1;FILE *f=fopen(CAMERA_PEEK_MAP_PATH,"rb");if(!f)return 0;
 unsigned header[4];size_t count=320*240;
 unsigned *map=malloc(count*sizeof(unsigned));int ok=map&&fread(header,sizeof(header),1,f)==1;
 ok=ok&&header[0]==0x51464c52&&header[1]==1&&header[2]==320&&header[3]==240;
 ok=ok&&fread(map,sizeof(unsigned),count,f)==count&&fgetc(f)==EOF;fclose(f);
 if(ok)for(size_t i=0;i<count;i++)if(map[i]!=UINT_MAX&&map[i]>=4*QUEST_CAMERA_PIXELS){ok=0;break;}
 if(ok)camera_peek.pixels=calloc(count,1);
 if(!ok||!camera_peek.pixels){free(map);return 0;}
 camera_peek.map=map;return 1;
}
static void camera_floor_update(void){
 camera_peek.valid=0;
 if(!camera_peek_load()||!camera_panel.valid)return;
 camera_peek.valid=1;
 if(camera_peek.publication!=camera_panel.feed.publication){
  unsigned char tone[256];for(int v=0;v<256;v++)tone[v]=(unsigned char)lroundf(powf(fmaxf(0,(v-4.f)/251),1.f/2.2f)*255);
  const unsigned char *source=&camera_panel.feed.pixels[0][0];
  for(unsigned i=0;i<320*240;i++)camera_peek.pixels[i]=camera_peek.map[i]==UINT_MAX?0:tone[source[camera_peek.map[i]]];
  camera_peek.publication=camera_panel.feed.publication;
 }
}
#endif

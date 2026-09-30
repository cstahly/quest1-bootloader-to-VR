/* Read-only V4L2 capability/format queries; no SET_FMT, buffers, or STREAMON.
 * Run one explicit device per invocation under an external timeout. */
#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <linux/videodev2.h>
#include <linux/media.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
int main(int argc,char **argv) {
 if(argc!=2){fprintf(stderr,"usage: query-video /dev/videoN-or-mediaN\n");return 2;}
 alarm(5);
 int fd=open(argv[1],O_RDONLY|O_NONBLOCK|O_CLOEXEC);
 if(fd<0){perror("open query device");return 1;}
 struct media_device_info media={0};
 if(ioctl(fd,MEDIA_IOC_DEVICE_INFO,&media)==0){
  printf("MEDIA driver=%.*s model=%.*s bus=%.*s version=%u\n",16,media.driver,32,media.model,32,media.bus_info,media.media_version);
  struct media_entity_desc entity={.id=MEDIA_ENT_ID_FLAG_NEXT};
  unsigned count=0;
  while(count++<128&&ioctl(fd,MEDIA_IOC_ENUM_ENTITIES,&entity)==0){
   printf("ENTITY id=%u name=%.*s type=%#x pads=%u links=%u dev=%u:%u\n",entity.id,32,entity.name,entity.type,entity.pads,entity.links,entity.v4l.major,entity.v4l.minor);
   entity.id|=MEDIA_ENT_ID_FLAG_NEXT;
  }
  close(fd);return 0;
 }
 struct v4l2_capability caps={0};
 if(ioctl(fd,VIDIOC_QUERYCAP,&caps)<0){perror("QUERYCAP");close(fd);return 1;}
 printf("VIDEO driver=%.*s card=%.*s bus=%.*s caps=%#x device_caps=%#x\n",16,caps.driver,32,caps.card,32,caps.bus_info,caps.capabilities,caps.device_caps);
 const enum v4l2_buf_type types[]={V4L2_BUF_TYPE_VIDEO_CAPTURE,V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE};
 for(unsigned t=0;t<sizeof(types)/sizeof(types[0]);t++){
  for(unsigned i=0;i<64;i++){
   struct v4l2_fmtdesc fmt={.index=i,.type=types[t]};
   if(ioctl(fd,VIDIOC_ENUM_FMT,&fmt)<0)break;
   unsigned f=fmt.pixelformat;
   printf("FORMAT type=%u index=%u fourcc=%c%c%c%c description=%.*s flags=%#x\n",fmt.type,i,f&255,(f>>8)&255,(f>>16)&255,(f>>24)&255,32,fmt.description,fmt.flags);
  }
  struct v4l2_format fmt={.type=types[t]};
  if(ioctl(fd,VIDIOC_G_FMT,&fmt)==0){
   if(t==0)printf("CURRENT %ux%u stride=%u image_bytes=%u\n",fmt.fmt.pix.width,fmt.fmt.pix.height,fmt.fmt.pix.bytesperline,fmt.fmt.pix.sizeimage);
   else printf("CURRENT_MULTIPLANE %ux%u planes=%u\n",fmt.fmt.pix_mp.width,fmt.fmt.pix_mp.height,fmt.fmt.pix_mp.num_planes);
  }
 }
 close(fd);return 0;
}

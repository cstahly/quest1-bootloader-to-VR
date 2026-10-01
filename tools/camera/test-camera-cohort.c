#include <assert.h>
#include <stdio.h>
#include "camera-feed.h"
int main(void){
 struct quest_camera_feed f={0};assert(!quest_camera_feed_synchronized(&f));
 for(unsigned c=0;c<4;c++){f.frames[c]=10;f.timestamp_ns[c]=1000000000+c*100000;}
 assert(quest_camera_feed_synchronized(&f));
 f.frames[0]=12;f.timestamp_ns[0]+=33333333;
 assert(!quest_camera_feed_synchronized(&f));
 for(unsigned c=1;c<4;c++){f.frames[c]=12;f.timestamp_ns[c]+=33333333;}
 assert(quest_camera_feed_synchronized(&f));
 f.timestamp_ns[3]=f.timestamp_ns[0]+1000001;assert(!quest_camera_feed_synchronized(&f));
 puts("camera cohort: empty, camera-zero-first race, completion and skew rejection PASS");
}

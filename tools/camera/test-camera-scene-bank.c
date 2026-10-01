#include <assert.h>
#include <stdio.h>
#include "camera-scene-bank.h"
static unsigned le16(const unsigned char *p){return p[0]+256U*p[1];}
int main(void){
    struct quest_scene_bank_setting settings[4];unsigned char packet[21];
    for(unsigned c=0;c<4;c++)settings[c]=quest_scene_bank_requested(1,0,12000,16+c);
    quest_scene_bank_packet(packet,0,settings);
    for(unsigned c=0;c<4;c++){
        assert(le16(packet+2*c)==12000&&le16(packet+8+2*c)==16+c&&packet[16+c]==1);
    }
    assert(packet[20]==0);
    for(unsigned c=0;c<4;c++)settings[c]=quest_scene_bank_requested(1,1,8000,240);
    quest_scene_bank_packet(packet,1,settings);
    for(unsigned c=0;c<4;c++)assert(le16(packet+2*c)==38&&le16(packet+8+2*c)==48&&packet[16+c]==2);
    assert(packet[20]==1);
    for(unsigned c=0;c<4;c++)settings[c]=quest_scene_bank_requested(0,1,8000,240);
    quest_scene_bank_packet(packet,1,settings);
    for(unsigned c=0;c<4;c++)assert(le16(packet+2*c)==8000&&le16(packet+8+2*c)==240&&packet[16+c]==1);
    for(unsigned high=0;high<16;high++)for(unsigned low=0;low<16;low++){
        assert(quest_camera_scene_selected(128,(high<<4)|low)==(low==1));
        assert(!quest_camera_scene_selected(130,(high<<4)|low));
    }
    /* Selection has no exposure or parity input:988us quantized scene remains
     * scene; a long controller exposure still cannot leak into the feed. */
    assert(quest_camera_scene_selected(128,0x31));
    assert(!quest_camera_scene_selected(128,0x32));
    assert(!quest_camera_scene_selected(128,0x34));
    assert(!quest_camera_scene_selected(128,0x30));
    assert(quest_scene_readback_match(100000001,1,7999,48,8000,48)==-1);
    assert(quest_scene_readback_match(100000002,1,7999,48,8000,48)==1);
    assert(quest_scene_readback_match(100000002,1,7981,48,8000,48)==1);
    assert(quest_scene_readback_match(100000002,1,8019,48,8000,48)==1);
    assert(quest_scene_readback_match(100000002,1,7980,48,8000,48)==0);
    assert(quest_scene_readback_match(100000002,1,8020,48,8000,48)==0);
    assert(quest_scene_readback_match(100000002,1,7999,47,8000,48)==0);
    assert(quest_scene_readback_match(100000002,1,988,48,1000,48)==1);
    assert(quest_scene_readback_match(1,2,7999,48,8000,48)==-1);
    assert(quest_scene_readback_match(100000002,0,7999,48,8000,48)==-1);
    assert(quest_camera_scene_settings_valid(988,16));
    assert(quest_camera_scene_settings_valid(981,240));
    assert(quest_camera_scene_settings_valid(12019,48));
    assert(!quest_camera_scene_settings_valid(980,48));
    assert(!quest_camera_scene_settings_valid(38,48));
    assert(!quest_camera_scene_settings_valid(12020,48));
    assert(!quest_camera_scene_settings_valid(8000,15));
    assert(!quest_camera_scene_settings_valid(8000,241));
    puts("scene bank: exact packets/default identity, tags/flags/all nibble classes and exposure-independent selection PASS");
}

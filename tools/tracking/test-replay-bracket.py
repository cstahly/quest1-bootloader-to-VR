#!/usr/bin/env python3
"""Compile the real replay feeder against a fake VIT to check scheduling only.
Usage: test-replay-bracket.py DIRECTORY_CONTAINING_vit_interface.h
Does not load Basalt, access sensors, or establish estimator correctness.
"""
import os
import pathlib
import struct
import subprocess
import sys
import tempfile

include = pathlib.Path(sys.argv[1]).resolve()
source = pathlib.Path(__file__).with_name('replay-vit.cpp').resolve()
stub = r'''
#define VIT_INTERFACE_IMPLEMENTATION
#include <vit_interface.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
struct vit_pose { vit_pose_data_t data{}; };
struct vit_tracker {
 unsigned cameras=0,next=0;int64_t imu=-1,frame=-1;bool bracket=false;
 std::deque<vit_pose *> poses;FILE *log=nullptr;
};
vit_result_t vit_tracker_create(const vit_config_t *c,vit_tracker_t **out){
 if(std::strcmp(c->file,"normal")&&std::strcmp(c->file,"bracket"))return VIT_ERROR_INVALID_VALUE;
 auto *t=new vit_tracker;t->cameras=c->cam_count;t->bracket=!std::strcmp(c->file,"bracket");
 t->log=std::fopen(std::getenv("FAKE_VIT_LOG"),"w");if(!t->log)return VIT_ERROR_INVALID_VALUE;
 *out=t;return VIT_SUCCESS;
}
void vit_tracker_destroy(vit_tracker_t *t){std::fclose(t->log);delete t;}
vit_result_t vit_tracker_start(vit_tracker_t *){return VIT_SUCCESS;}
vit_result_t vit_tracker_stop(vit_tracker_t *){return VIT_SUCCESS;}
vit_result_t vit_tracker_enable_extension(vit_tracker_t *,vit_tracker_extension_t,bool){return VIT_SUCCESS;}
vit_result_t vit_tracker_push_imu_sample(vit_tracker_t *t,const vit_imu_sample_t *s){
 if(s->timestamp<t->imu)return VIT_ERROR_INVALID_VALUE;
 t->imu=s->timestamp;std::fprintf(t->log,"I %lld\n",(long long)s->timestamp);return VIT_SUCCESS;
}
vit_result_t vit_tracker_push_img_sample(vit_tracker_t *t,const vit_img_sample_t *s){
 if(s->cam_index!=t->next||s->size!=76800||s->width!=320||s->height!=240)return VIT_ERROR_INVALID_VALUE;
 if(!t->next)t->frame=s->timestamp;
 if(s->timestamp!=t->frame||(t->bracket&&t->imu<=s->timestamp))return VIT_ERROR_INVALID_VALUE;
 if(s->data[0]!=s->cam_index+1||s->data[76799]!=s->cam_index+1)return VIT_ERROR_INVALID_VALUE;
 std::fprintf(t->log,"C%u %lld\n",s->cam_index,(long long)s->timestamp);
 if(++t->next==t->cameras){t->next=0;auto *p=new vit_pose;p->data.timestamp=s->timestamp;p->data.ow=1;t->poses.push_back(p);}
 return VIT_SUCCESS;
}
vit_result_t vit_tracker_pop_pose(vit_tracker_t *t,vit_pose_t **p){
 *p=nullptr;if(!t->poses.empty()){*p=t->poses.front();t->poses.pop_front();}return VIT_SUCCESS;
}
void vit_pose_destroy(vit_pose_t *p){delete p;}
vit_result_t vit_pose_get_data(const vit_pose_t *p,vit_pose_data_t *out){*out=p->data;return VIT_SUCCESS;}
vit_result_t vit_pose_get_features(const vit_pose_t *,uint32_t,vit_pose_features_t *out){*out={};return VIT_SUCCESS;}
'''

def event(kind, time, count=4):
    payload = bytes(24) if kind == 'I' else b''.join(bytes([i+1])*76800 for i in range(count))
    return struct.pack('<cQI', kind.encode(), time, len(payload)) + payload

with tempfile.TemporaryDirectory() as temporary:
    root = pathlib.Path(temporary)
    (root/'fake.cpp').write_text(stub)
    binary = root/'replay'
    subprocess.run(['c++', '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-I'+str(include), str(source), str(root/'fake.cpp'), '-pthread',
                    '-o', str(binary)], check=True)
    clean = {k: v for k, v in os.environ.items() if not k.startswith('QUEST_REPLAY_')}
    subprocess.run([sys.executable, str(source.with_name('test-replay-input.py')), str(binary)],
                   env=clean, check=True)
    for count in (2, 4):
        events = [('I', 1), ('C', 10), ('I', 10), ('I', 11), ('I', 15),
                  ('C', 20), ('I', 21), ('I', 22), ('I', 23)]
        recording = root/'events.bin'
        recording.write_bytes(b''.join(event(k,t,count) for k,t in events))
        for mode in ('normal', 'bracket'):
            log, output = root/'push.log', root/'poses.csv'
            env = dict(clean, FAKE_VIT_LOG=str(log))
            if mode == 'bracket':
                env['QUEST_REPLAY_BRACKET_IMU'] = '1'
            result = subprocess.run([str(binary), mode, str(recording), str(output),
                                     str(root/'features.csv'), str(count)],
                                    env=env, capture_output=True, text=True, timeout=5)
            assert result.returncode == 0, result.stderr
            expected = ['I 1']
            cameras = lambda t: [f'C{i} {t}' for i in range(count)]
            if mode == 'bracket':
                expected += ['I 10', 'I 11'] + cameras(10) + ['I 15', 'I 21'] + cameras(20)
            else:
                expected += cameras(10) + ['I 10', 'I 11', 'I 15'] + cameras(20) + ['I 21']
            assert log.read_text().splitlines() == expected
            assert '2 trailing IMU samples omitted' in result.stderr
            assert [row.split(',')[0] for row in output.read_text().splitlines()[1:]] == ['10', '20']
            print(f'{mode} {count} cameras: ordering, copied pixels, poses, trailing cutoff PASS')

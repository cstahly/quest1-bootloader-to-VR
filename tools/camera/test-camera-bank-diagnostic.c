#include <assert.h>
#include <stdio.h>
#include "camera-bank-diagnostic.h"
int main(void){
    static unsigned char pixels[4][320*240];
    unsigned us[4]={4000,4000,4000,4000},gain[4]={16,16,16,16};
    struct quest_exposure normal_group[4];
    quest_exposure_group_next(pixels,us,gain,normal_group);
    assert(normal_group[0].us>=1000&&normal_group[0].gain>=16);
    assert(quest_bank_diagnostic_valid(0,0,0,0,0,1,0));
    assert(quest_bank_diagnostic_valid(1,1,4000,2000,1,0,1));
    assert(!quest_bank_diagnostic_valid(1,0,4000,0,1,0,1));
    assert(!quest_bank_diagnostic_valid(1,1,4000,2000,0,0,1));
    assert(!quest_bank_diagnostic_valid(1,1,4000,2000,1,1,1));
    assert(!quest_bank_diagnostic_valid(1,1,4000,2000,1,0,0));
    assert(!quest_bank_diagnostic_valid(1,1,4001,2000,1,0,1));
    assert(!quest_bank_diagnostic_valid(1,1,4000,1999,1,0,1));
    assert(!quest_bank_diagnostic_valid(1,1,4000,4000,1,0,1));
    struct quest_bank_diagnostic d={.enabled=0,.us={4000,2000}};
    struct quest_exposure normal={8000,240,10};
    struct quest_exposure out=quest_bank_requested(&d,0,normal);
    assert(out.us==8000&&out.gain==240&&out.brightness==10);
    d.enabled=1;
    out=quest_bank_requested(&d,0,normal);assert(out.us==4000&&out.gain==16);
    out=quest_bank_requested(&d,1,normal);assert(out.us==2000&&out.gain==16);
    assert(quest_bank_observed(&d,4009,16)==1);
    assert(quest_bank_observed(&d,1995,16)==2);
    assert(quest_bank_observed(&d,1995,32)==0);
    assert(quest_bank_observed(&d,1900,16)==0);
    assert((quest_bank_observed(&d,4009,16)|quest_bank_observed(&d,1995,16))==3);
    d.step_enabled=1;d.first_request_ns=100;
    assert(!quest_bank_step_due(&d,99,1));
    assert(!quest_bank_step_due(&d,3000000099ULL,1));
    assert(!quest_bank_step_due(&d,3000000100ULL,0));
    assert(quest_bank_step_due(&d,3000000100ULL,1));
    d.step_applied=1;
    assert(!quest_bank_step_due(&d,6000000100ULL,1));
    out=quest_bank_requested(&d,0,normal);assert(out.us==8000&&out.gain==16);
    out=quest_bank_requested(&d,1,normal);assert(out.us==8000&&out.gain==16);
    puts("bank diagnostic: fail-closed configuration, safe settings, default identity and readback masks PASS");
}

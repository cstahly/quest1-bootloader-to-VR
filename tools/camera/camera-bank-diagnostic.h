/* Separate temporary exposure-bank identification mode; never enabled by default. */
#ifndef QUEST_CAMERA_BANK_DIAGNOSTIC_H
#define QUEST_CAMERA_BANK_DIAGNOSTIC_H
#include "exposure-policy.h"
struct quest_bank_diagnostic {
    unsigned enabled,us[2],step_enabled,step_applied;
    unsigned long long first_request_ns;
};
static inline int quest_bank_step_due(const struct quest_bank_diagnostic *d,
 unsigned long long now,unsigned initial_verified){
    return d->enabled&&d->step_enabled&&!d->step_applied&&initial_verified&&d->first_request_ns&&
           now>=d->first_request_ns&&now-d->first_request_ns>=3000000000ULL;
}
static inline int quest_bank_diagnostic_valid(unsigned present0,unsigned present1,
 unsigned us0,unsigned us1,unsigned trace,unsigned automatic,unsigned control){
    if(!present0&&!present1)return 1;
    return present0&&present1&&trace&&!automatic&&control&&
           us0>=2000&&us0<=4000&&us1>=2000&&us1<=4000&&us0!=us1;
}
static inline struct quest_exposure quest_bank_requested(const struct quest_bank_diagnostic *d,
 unsigned bank,struct quest_exposure normal){
    if(d->enabled)return (struct quest_exposure){d->step_applied?8000:d->us[bank],16,0};
    return normal;
}
static inline unsigned quest_bank_observed(const struct quest_bank_diagnostic *d,unsigned us,unsigned gain){
    unsigned matched=0;
    if(!d->enabled||gain!=16)return 0;
    for(unsigned bank=0;bank<2;bank++)if(us+19>=d->us[bank]&&us<=d->us[bank]+19)matched|=1U<<bank;
    return matched;
}
#endif

#include <assert.h>
#include <stdio.h>
#include "recovery-deadline.h"
int main(void) {
    struct recovery_hold h={0}; double deadline=300;
    recovery_input(&h,1,0); assert(!recovery_hold_ready(&h,10)); /* Held at attach. */
    recovery_input(&h,0,10); recovery_input(&h,1,11);
    assert(!recovery_hold_ready(&h,12.99)); assert(recovery_hold_ready(&h,13));
    assert(!recovery_hold_ready(&h,100)); /* Stuck trigger cannot keep renewing. */
    assert(recovery_renew(13,&deadline)); assert(deadline==313);
    recovery_input(&h,0,20); recovery_input(&h,1,21);
    assert(recovery_hold_ready(&h,23)); assert(recovery_renew(23,&deadline));
    assert(deadline==323); assert(!recovery_renew(323,&deadline));
    assert(!recovery_renew(324,&deadline)); assert(deadline==323);
    h=(struct recovery_hold){0}; /* Lost input sync requires a fresh release. */
    recovery_input(&h,1,40); assert(!recovery_hold_ready(&h,50));
    puts("Recovery hold, release, stuck-input, and expiry precedence tests passed");
}

/* Monotonic recovery policy. A held/stuck trigger can renew only once. */
#ifndef OCULUS_RECOVERY_DEADLINE_H
#define OCULUS_RECOVERY_DEADLINE_H
struct recovery_hold { int released, down, fired; double pressed; };
static void recovery_input(struct recovery_hold *h, int down, double now) {
    if (!down) { h->released=1; h->down=0; h->fired=0; return; }
    if (h->released && !h->down) { h->down=1; h->pressed=now; }
}
static int recovery_hold_ready(struct recovery_hold *h, double now) {
    if (h->released && h->down && !h->fired && now-h->pressed>=2.0) {
        h->fired=1; return 1;
    }
    return 0;
}
static int recovery_renew(double now, double *deadline) {
    if (now>=*deadline) return 0; /* Expiration wins even if input is queued. */
    *deadline=now+300.0; return 1;
}
#endif

/* Read-only service-locator probe. Build with upstream servreg_loc.c and libqrtr. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libqrtr.h>
#include "servreg_loc.h"
int main(int argc, char **argv)
{
    if (argc != 3) { fprintf(stderr, "usage: %s NODE PORT\n", argv[0]); return 2; }
    int fd = qrtr_open(0);
    if (fd < 0) { perror("qrtr_open"); return 1; }
    /* QMI request header + string TLV. Upstream request descriptor is decode-only:
       its VAR_LEN_ARRAY string has no QMI_DATA_LEN entry for libqrtr encoding. */
    const unsigned char request[] = {0, 1, 0, 33, 0, 10, 0, 1, 7, 0,
                                    'w', 'l', 'a', 'n', '/', 'f', 'w'};
    ssize_t n;
    if (qrtr_sendto(fd, strtoul(argv[1], NULL, 0), strtoul(argv[2], NULL, 0), request, sizeof(request)) < 0) {
        perror("send domain query"); close(fd); return 1;
    }
    if (qrtr_poll(fd, 3000) <= 0) { fprintf(stderr, "domain query timed out\n"); close(fd); return 1; }
    char buf[4096]; struct sockaddr_qrtr from; socklen_t sl = sizeof(from);
    n = recvfrom(fd, buf, sizeof(buf), 0, (void *)&from, &sl);
    struct qrtr_packet in;
    static struct servreg_loc_get_domain_list_resp resp;
    unsigned txn = 0;
    if (n < 0 || qrtr_decode(&in, buf, n, &from) < 0 ||
        qmi_decode_message(&resp, &txn, &in, QMI_RESPONSE, SERVREG_LOC_GET_DOMAIN_LIST,
                           servreg_loc_get_domain_list_resp_ei) < 0 || txn != 1 || resp.result.result ||
        !resp.domain_list_valid || resp.domain_list_len != 1) {
        fprintf(stderr, "invalid service-locator response\n"); close(fd); return 1;
    }
    const struct servreg_loc_domain_list_entry *entry = &resp.domain_list[0];
    printf("wlan/fw -> %.*s, instance %u\n", 255, entry->name, entry->instance_id);
    int ok = !strcmp(entry->name, "msm/modem/wlan_pd") && entry->instance_id == 180;
    close(fd); return ok ? 0 : 1;
}

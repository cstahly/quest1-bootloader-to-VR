/* Local IPC TFTP smoke test. Only reads a fixture and writes a RAM-root fixture. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libqrtr.h>
static int receive(int fd, unsigned char *buf, struct sockaddr_qrtr *from) {
    if (qrtr_poll(fd, 3000) <= 0) return -1;
    socklen_t len = sizeof(*from);
    return recvfrom(fd, buf, 1024, 0, (void *)from, &len);
}
int main(int argc, char **argv) {
    if (argc != 3) return 2;
    unsigned node = strtoul(argv[1], NULL, 0), port = strtoul(argv[2], NULL, 0);
    const char *paths[] = {"/readonly/firmware/image/quest-tftp-check.txt",
                           "/readwrite/quest-write-check.txt",
                           "/readonly/firmware/image/quest-tftp-check.txt"};
    const char value[] = "quest-ram-test\n";
    for (int test = 0; test < 3; test++) {
        int fd = qrtr_open(0); if (fd < 0) return 1;
        unsigned char req[512] = {0}, buf[1024]; struct sockaddr_qrtr from;
        req[1] = test == 0 ? 1 : 2;
        size_t len = strlen(paths[test]); memcpy(req + 2, paths[test], len + 1);
        memcpy(req + len + 3, "octet", 6);
        if (qrtr_sendto(fd, node, port, req, len + 9) < 0) return 1;
        int n = receive(fd, buf, &from);
        if (test == 0) {
            if (n != 4 + sizeof(value) - 1 || memcmp(buf, "\0\3\0\1", 4) ||
                memcmp(buf + 4, value, sizeof(value) - 1)) {
                fprintf(stderr, "read fixture failed: n=%d opcode=%d\n", n, n>1?buf[1]:-1); return 1;
            }
            unsigned char ack[] = {0, 4, 0, 1};
            qrtr_sendto(fd, from.sq_node, from.sq_port, ack, sizeof(ack));
        } else if (test == 1) {
            if (n != 4 || memcmp(buf, "\0\4\0\0", 4)) {
                fprintf(stderr, "write handshake failed: n=%d opcode=%d\n", n, n>1?buf[1]:-1); return 1;
            }
            unsigned char data[64] = {0, 3, 0, 1}; memcpy(data + 4, value, sizeof(value)-1);
            qrtr_sendto(fd, from.sq_node, from.sq_port, data, 4 + sizeof(value)-1);
            n = receive(fd, buf, &from);
            if (n != 4 || memcmp(buf, "\0\4\0\1", 4)) return 1;
        } else if (n < 4 || buf[0] || buf[1] != 5) {
            fprintf(stderr, "readonly write was not rejected\n"); return 1;
        }
        close(fd);
    }
    /* Cross a full block and an exact block boundary; catches premature EOF. */
    for (int size = 1024; size <= 1500; size += 476) {
        int fd = qrtr_open(0); if (fd < 0) return 1;
        unsigned char req[256] = {0, 1}, buf[1024];
        struct sockaddr_qrtr from;
        const char *path = size == 1024 ? "/readonly/firmware/image/exact.bin" :
                                        "/readonly/firmware/image/multi.bin";
        size_t plen = strlen(path);
        memcpy(req + 2, path, plen + 1); memcpy(req + plen + 3, "octet", 6);
        if (qrtr_sendto(fd, node, port, req, plen + 9) < 0) return 1;
        int total = 0;
        for (int block = 1; block <= 4; block++) {
            int n = receive(fd, buf, &from);
            if (n < 4 || buf[0] || buf[1] != 3 || buf[2] || buf[3] != block) {
                fprintf(stderr, "multiblock %d failed at block %d, n=%d\n", size, block, n); return 1;
            }
            for (int i = 4; i < n; i++) if (buf[i] != (unsigned char)((total + i - 4) % 251)) return 1;
            total += n - 4;
            unsigned char ack[] = {0, 4, 0, (unsigned char)block};
            qrtr_sendto(fd, from.sq_node, from.sq_port, ack, sizeof(ack));
            if (n < 516) break;
        }
        close(fd);
        if (total != size) return 1;
    }
    puts("PASS: firmware read, RAM write, firmware write rejected, multiblock and exact-boundary reads");
    return 0;
}

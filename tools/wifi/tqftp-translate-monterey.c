/* Bounded file roots for a manual Monterey TFTP bring-up test.
 * Stock firmware is read-only; every writable file stays below a fresh /run dir.
 * No remoteproc sysfs dependency and no persistent/factory-storage writes.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef QUEST_TFTP_FIRMWARE
#define QUEST_TFTP_FIRMWARE "/run/quest-wifi-firmware"
#endif
#ifndef QUEST_TFTP_FALLBACK
#define QUEST_TFTP_FALLBACK "/lib/firmware"
#endif
#ifndef QUEST_TFTP_WRITABLE
#define QUEST_TFTP_WRITABLE "/run/quest-tftp-write"
#endif
static int rooted_open(const char *root, const char *relative, int flags)
{
    if (!relative[0] || relative[0] == '/' || strlen(relative) > 1024) {
        errno = EACCES; return -1;
    }
    char *path = strdup(relative), *component = path;
    if (!path) return -1;
    int dir = open(root, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (dir < 0) { free(path); return -1; }
    int result = -1;
    for (;;) {
        char *slash = strchr(component, '/');
        if (slash) *slash = 0;
        if (!component[0] || !strcmp(component, ".") || !strcmp(component, "..")) {
            errno = EACCES; break;
        }
        if (!slash) {
            result = openat(dir, component, flags | O_NOFOLLOW | O_CLOEXEC, 0600);
            if (result >= 0) {
                struct stat st;
                if (fstat(result, &st) || !S_ISREG(st.st_mode)) {
                    close(result); result = -1; errno = EACCES;
                }
            }
            break;
        }
        int next = openat(dir, component, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (next < 0 && errno == ENOENT && (flags & O_CREAT)) {
            if (mkdirat(dir, component, 0700) < 0 && errno != EEXIST) break;
            next = openat(dir, component, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        }
        if (next < 0) break;
        close(dir); dir = next; component = slash + 1;
    }
    int saved = errno;
    close(dir); free(path); errno = saved; return result;
}
int translate_open(const char *path, int flags)
{
    const char *ro[] = {"/readonly/firmware/image/", "/readonly/vendor/firmware/",
                        "/readonly/vendor/firmware_mnt/image/", NULL};
    const char *relative = NULL;
    for (int i = 0; ro[i]; i++)
        if (!strncmp(path, ro[i], strlen(ro[i]))) { relative = path + strlen(ro[i]); break; }
    if (!relative && !strncmp(path, "/readonly/firmware/modem_pr/", strlen("/readonly/firmware/modem_pr/")))
        relative = path + strlen("/readonly/firmware/");
    if (relative) {
        if (flags != O_RDONLY) { errno = EACCES; return -1; }
        int fd = rooted_open(QUEST_TFTP_FIRMWARE, relative, O_RDONLY);
        if (fd < 0 && errno == ENOENT) fd = rooted_open(QUEST_TFTP_FALLBACK, relative, O_RDONLY);
        return fd;
    }
    if (!strncmp(path, "/readwrite/", 11))
        return rooted_open(QUEST_TFTP_WRITABLE, path + 11, flags);
    errno = EACCES; return -1;
}

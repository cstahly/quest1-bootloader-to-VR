# Downstream Monterey service mapper

Upstream source: https://github.com/linux-msm/pd-mapper at
`5ecd2fe926aca7abfe40724177f63b942cff3947`.

Apply `0001-explicit-map-files.patch` and build with `make` against libqrtr and
liblzma (Alpine: qrtr-dev, xz-dev). The patch adds an explicit `--maps FILE.jsn ...`
mode because the default upstream discovery requires `/sys/class/remoteproc`,
which this downstream kernel does not expose. Existing default behavior remains.
Stock manifests stay outside Git.

For a manual transport test only, run with
`LD_PRELOAD=/usr/lib/preload/libqipcrtr4msmipc.so` and the owner's `modemr.jsn` and
`modemuw.jsn` file paths. This publishes service-locator responses; it does not
start a remote processor or load firmware. It is not installed or enabled at boot.

Build `tools/wifi/query-pd-mapper.c` with upstream `servreg_loc.c`, the upstream
include directory and `-lqrtr`. Obtain the mapper's node and port from
`/sys/kernel/debug/msm_ipc_router/dump_servers` (service `0x40`), then pass those
as NODE PORT under the same preload. The probe requests `wlan/fw` and validates
exactly `msm/modem/wlan_pd`, instance 180. It has a three-second reply timeout.
A successful lookup is not proof that WLAN firmware is running.

Validation so far: ARM build passed; invalid arguments and missing maps reject;
stock maps parse. Bounded on-device daemon smoke test starts under the IPC bridge,
which logs a getsockname warning. After stock IRSC initialization, the on-device end-to-end lookup passed: wlan/fw maps to msm/modem/wlan_pd, instance 180. Without IRSC initialization, sendmsg blocks inside the kernel before the probe reply timeout can apply.

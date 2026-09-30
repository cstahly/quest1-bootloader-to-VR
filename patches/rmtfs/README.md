# Read-only RMTFS diagnostic

Base: linux-msm/rmtfs v1.3. The patch makes service output unbuffered and requires
-r, rejecting -s before it can start a remote processor. It does not change NV
storage behavior. Compile the normal Makefile and run only from /run, with
-P -r -v and libqipcrtr4msmipc preload. Redirect stdout/stderr to /dev/kmsg for
crash-persistent diagnostics. This is not an installed/autostart replacement.

The read-only probe tools/wifi/query-rmtfs.c opens each of the four standard NV
paths, requests allocation metadata, verifies the live 0xfca00000/2MiB buffer,
then closes. It issues no IOVEC requests and returns no partition contents.
The installed RMTFS service passed this probe on 2026-09-30.

## Relative buffer offsets — verified on hardware

Stock Monterey modem requests its first sector into offset0x200. Default upstream
absolute-address checking rejects it, and the modem crashes immediately afterward.
Patch0002 adds explicit RMTFS_MSM8998_RELATIVE_OFFSETS=1 mode, with subtraction-based
bounds checks for negative lengths and range overflow. Default absolute mode is
preserved. ARM unit test test-sharedmem.c passes; query-rmtfs --relative-read passes
on hardware. With this fix the modem stays ONLINE through the guarded boot, and
TFTP serves WLAN firmware successfully. Preserve -r (RAM shadows for NV writes).

The modem also asks for optional modem_fsg_oem_1 and _2, absent in the partition map;
those requests remain rejected. They did not prevent the successful modem/WLAN-PD
startup and must not be mapped to arbitrary partitions.

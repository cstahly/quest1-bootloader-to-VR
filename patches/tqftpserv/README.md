# Monterey manual TFTP test

Base: linux-msm/tqftpserv c2559a26098f6d2b36a946ef0b6ad02223264c3e.
Apply the patch, compile tqftpserv.c with tools/wifi/tqftp-translate-monterey.c
instead of upstream translate.c and link -lqrtr. On the device use
LD_PRELOAD=/usr/lib/preload/libqipcrtr4msmipc.so after stock IRSC initialization.

This is a manual diagnostic, not a production service. Reads are restricted to
staged/installed firmware and writes to a fresh mode-0700 /run/quest-tftp-write.
No persist, modemst, factory calibration, or other partition is writable through
this translator. Do not substitute the upstream default write root.

Validation: tools/wifi/test-tqftp-roots.py passes on host; query-tqftp.c passes on
Quest for read/write/rejected firmware write and multi-block/exact-boundary reads.
One guarded modem start with this server still hit the early modem fatal;
Wi-Fi is not working yet. No service autostart installed.

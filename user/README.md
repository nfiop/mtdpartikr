# mtdpartd, utests and libmtdpartctl

`mtdpartd` is an MTD partitioning daemon. It works by opening a TCP/IP
(or a Unix) socket for receiving control messages from a client.

`utests` is suite of userspace unit tests which are designed to test
the `mtdpartikr` kernel module.

`libmtdpartctl` is a small C library shim, designed to hide very low level
parts of the `mtdpartctl` device interface - the ioctl interface and its
hardcoded name definitions. The library is used within each unit test and
the `mtdpartd` daemon.


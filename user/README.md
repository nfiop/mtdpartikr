# mtdpartctl, utests and libmtdpartctl

`mtdpartctl` is an MTD partitioning utility. It works by either opening
a REPL for receiving control messages from a client or receiving direct
commands from the command-line.

`utests` is suite of userspace unit tests which are designed to test
the `mtdpartikr` kernel module.

`libmtdpartctl_if` is a small C library shim, designed to hide very low
level parts of the `mtdpartctl` device interface - the ioctl interface
and its hardcoded name definitions. The library is used within each unit
test and the `mtdpartctl` utility.


# Unit tests

There are unit-tests (also called utests).

They are meant to check the basic functionality of an `mtdpartctl`
device.

It should be run on a test QEMU VM, with a nandsim module being loaded
for a controlled test environment.

These are the tests:

## `add_invalid_context_part`

This utest is testing an attempt to add an invalid partition to an
`mtdpartctl` device.

It tries to add such partition, each time on a different limitation:
- An offset which is unaligned to erase block size
- A length which is unaligned to erase block size
- Offset + length which are greater than the MTD device length, in bytes
- Offset + length which are overflowing a UINT64_MAX 
- A length of 0 bytes

Intended result - all tests should fail with -EINVAL error code.

## `add_invalid_new_part`

This utest is testing adding a basic partition to an MTD master device
from an `mtdpartctl` device.
The request has invalid parameters such as:
- Offset + length which are greater than the MTD device length, in bytes
- Offset + length which are overflowing a UINT64_MAX 

Intended result - fail to complete request.

## `add_part_no_context`

This utest is testing adding a basic partition to an MTD master device
from an `mtdpartctl` device.

Intended result - partition added successfully.

## `create_empty_context`

This utest is testing an attempt to create partitions on an MTD master
device from an empty context of an `mtdpartctl` device.

Intended result - fail to complete request.

## `create_nandsim_parts`

This utest is testing adding a set of partitions from a context of an
`mtdpartctl` device.

Intended result - a set of MTD partitions is created.

## `create_nandsim_parts_while_holding_ref`

This utest is testing an attempt to create a set of partitions from a
context of an `mtdpartctl` device, while there's an MTD user on the
MTD master device.

Intended result - fail to complete request.

## `del_non_existing_part`

This utest is testing an attempt to delete an MTD partition with an
index of non-existing partition.

Intended result - fail to complete request.

## `del_part`

This utest is testing an attempt to delete an MTD partition with an
index of existing partition.

Intended result - ioctl returns 0, a given partition is deleted.

## `get_info`

This utest is testing the `MTDPARTCTL_IOC_GET_INFO` ioctl.

Intended result - ioctl returns 0, a given struct is filled with info.

## `invalid_ioctl`

This utest is testing invoking an invalid ioctl.

Intended result - ioctl returns -EINVAL.

## `restart_context`

This utest is testing the `MTDPARTCTL_IOC_RESTART_CONTEXT` ioctl.

Intended result - `mtdpartctl` device with empty context.

## `restart_mtd`

This utest is testing the `MTDPARTCTL_IOC_DELETE_MTD_PARTITIONS` ioctl.
The corresponding MTD master device should be cleared from MTD partitions.

Intended result - MTD master device with no partitions.

## `restart_mtd_while_holding_ref`

This utest is testing the `MTDPARTCTL_IOC_DELETE_MTD_PARTITIONS` ioctl.
The ioctl is invoked while there's another user to the MTD master device.

Intended result - fail to complete request.

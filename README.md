# mtdpartikr - MTD partitioning in kernel runtime

`mtdpartikr` is a component in the nfiop project, which is intended for
providing a capability of partitioning an MTD device in kernel runtime.

`mtdpartikr` is also a kernel module which registers new character devices 
for master MTD devices, to allow userspace to create MTD partitions in kernel
runtime, without rebooting, after the boot process is completed.

It accomplishes re-partitioning of MTDs by spawning its own proxy MTDs which
are backed by existing MTDs, and with tight control over the lifetime model
of the proxy MTDs, the kernel module is able to respawn the proxy MTDs with
new partition tables when such operation is needed.

Together with `mtdpartctl`, it serves as a vital component in the nfiop 
ecosystem, where otherwise it would be much harder to handle different parts
of a flash chip separately. 

`mtdpartikr` is designed specifically for flash devices and can filter based 
on the MTD type - for now, either a NAND or NOR flash device.

### On the word "partikr"

`mtdpartikr` is pronounced as "partiker" (`em-tee-dee-par-tee-ker`).

"partiker" is not a word in English, but its sound might be catchy and the
resemblence to the word of "party" could have a somewhat joyful meaning to
the name as well.

## Prepare for compiling

If not clean, clean the build directory
```sh
./build.sh clean
```

## Cross compile (with buildroot)

Simply run (with adjustments to your buildroot path):
```sh
./build.sh --buildroot ../buildroot/
```

## Why do you write this module?

Sadly, there's no real easy way to define multiple MTD partitions after
the boot process is done.

Traditional embedded platforms have hardcoded offsets for MTD partitions
or might include them in a device tree blob, for "convenience".

There's the `mtdpart` utility in the `mtd-utils` package that might
serve some of the offered functionality of this module.

This kernel module, together with additional userspace code, are meant
to allow dynamic partitioning in kernel runtime, after the boot process
is completed.

It offers a mechanism to invoke a controlled procedure of defining new
MTD partitions on an MTD device, with the ability of controlling flags
like MTD_WRITABLE and MTD_POWERUP_LOCK which to the best of my knowledge
are not available on the `mtdpart` utility.

## How do I use this module?

This module is not supposed to be installed alone on an embedded machine
or a platform that has an MTD device (or even multiple).

It should be paired with the rest of the `nfiop` ecosystem, serving as a
complementary component for `ufedm` for slicing a backing MTD device to
logical chunks for differential processing.

See instructions about [testing](TESTING.md) to learn more about actual
usage.

## Limitations

- `mtdpartikr` doesn't intend to support unaligned offsets. You must
  read the MTD erasesize (also available from our ioctl interface) and
  calculate an aligned offset.

  The reason for this is to ensure erase semantics are always valid on
  all platforms and drivers.

- `mtdpartikr` doesn't support batch-removal for partition of a backing MTD.
  If needed, delete them one after the other (with index of 0, repeated
  for N partitions) with the MTDPARTCTL_IOC_DEL_MTD_PARTITION ioctl.

  Likewise, we don't support batch-insertion for partition for a backing MTD,
  for the same reason.

  The whole `mtdpartctl` recipe mechanism is intended for the proxy MTD,
  which we do have a control over its lifetime model, so these features are
  supported.

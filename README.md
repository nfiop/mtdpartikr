# mtdpartikr - MTD partitioning in kernel runtime

`mtdpartikr` is a kernel module which registers new character devices for
master MTD devices, to allow userspace to create MTD partitions in kernel
runtime, without rebooting, after the boot process is completed.

It is intended specifically for flash devices and can filter based on
the MTD type - for now, either a NAND or NOR flash device.

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

## Run on a development board

The kernel module will be in `build/kmod` directory now.

You can then upload it to the Olimex LIME2 board and load it as usual.

## Testing with `nandsim` on your local machine

With `nandsim`, you can run tests on any platform (x86, ARM64, RISC-V, you name it...) -
it's a powerful utility for those lacking the physical hardware and want to verify
the capabilities of this module.

On x86 machine (probably what you have near your desk), just compile with a normal gcc:
```
./build.sh
```

Load `nandsim` like so (I chose a standard NAND flash chip to simulate):
```sh
modprobe nandsim first_id_byte=0x20 second_id_byte=0xaa third_id_byte=0x00 \
fourth_id_byte=0x15 pagesize=2048 oobpagesize=64 eraseblock_size=131072
```

You should see a kernel log output similar to this:
```
nand: ST Micro NAND 256MiB 1,8V 8-bit
nand: 256 MiB, SLC, erase size: 128 KiB, page size: 2048, OOB size: 64
```

Then attach this driver & specify the corresponding MTD index:
```sh
insmod build/kmod/mtdpartikr.ko mtds=0
```

`nandsim` can technically simulate almost any NAND flash chip you might
think of.

### Running in a VM

Running this kernel module during development stage could be dangerous
to the system stability.

To prevent a kernel crash on your host, you can run a QEMU VM with
`nandsim` being automatically loaded with your Linux image and a custom
built initramfs containing the `build` directory inside.

See `run-nandsim-vm.sh` for more details, but something like this should
get you started:

```sh
KERNEL=/boot/vmlinuz-linux ./run-nandsim-vm.sh
```

Adjust the `KERNEL` to your environment and make sure you
have read permissions on both files.

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

## Limitations

- `mtdpartikr` doesn't intend to support unaligned offsets. You must
  read the MTD erasesize (also available from our ioctl interface) and
  calculate an aligned offset.

  The reason for this is to ensure erase semantics are always valid on
  all platforms and drivers.

- `mtdpartikr` doesn't provide a way to batch-remove multiple MTD partitions
  nor "reset" an MTD device back to a state of no partitions.

  The kernel doesn't support such mechanism easily, so I didn't bother
  to build something complicated when the solution is to either reboot
  or, (unsafely due a possible userspace race condition) delete each
  partition on your risk.

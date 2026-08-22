/*
 * SPDX-License-Identifier: GPL-2.0-only OR MIT
 * Copyright (c) 2026 Liav A
 */

package main

// NOTE: There's a twin to this file called get_info.c -
// the idea behind this is to ensure we have a unit-test for
// ensuring we can compile libmtdpartctl with a Go program correctly.

/*
#include "dev.h"
*/
import "C"

import "fmt"
import "os"

func main() {
    f, err := os.Open("/dev/mtdpartctl0")
    if err != nil {
        panic(err)
    }
    defer f.Close()

    fd := f.Fd() // uintptr


    ctx := C.struct_mtdpartctl_info{
    }
    fmt.Println(int(C.mtdpartctl_get_info(C.int(fd), &ctx)))
}

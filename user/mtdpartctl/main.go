/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) 2026 Liav A
 */

package main

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

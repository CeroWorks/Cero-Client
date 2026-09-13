//go:build windows

package main

import (
    "os"
    "syscall"
    "unsafe"
)

func enableANSI() bool {
    kernel32 := syscall.NewLazyDLL("kernel32.dll")
    getConsoleMode := kernel32.NewProc("GetConsoleMode")
    setConsoleMode := kernel32.NewProc("SetConsoleMode")

    h := syscall.Handle(os.Stdout.Fd())
    var mode uint32
    r, _, _ := getConsoleMode.Call(uintptr(h), uintptr(unsafe.Pointer(&mode)))
    if r == 0 {
        return false
    }
    mode |= 0x0004
    r, _, _ = setConsoleMode.Call(uintptr(h), uintptr(mode))
    return r != 0
}
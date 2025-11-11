//go:build !windows
// +build !windows

package main

import (
	"os/exec"
	"syscall"

	"github.com/wailsapp/wails/v2/pkg/runtime"
)

func (u *Updater) runApp(appPath string) error {
	cmd := exec.Command(appPath)

	cmd.SysProcAttr = &syscall.SysProcAttr{Setsid: true}

	if err := cmd.Start(); err != nil {
		return err
	}

	runtime.Quit(u.ctx)
	return nil
}

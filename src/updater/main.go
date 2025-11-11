package main

import (
	"embed"

	"github.com/wailsapp/wails/v2"
	"github.com/wailsapp/wails/v2/pkg/options"
	"github.com/wailsapp/wails/v2/pkg/options/assetserver"
)

//go:embed all:frontend/dist
var assets embed.FS

func main() {
	updater := NewUpdater()

	err := wails.Run(&options.App{
		Title:       "CeroUpdater",
		Width:       400,
		Height:      120,
		AssetServer: &assetserver.Options{Assets: assets},
		OnStartup:   updater.Startup,
		Bind:        []interface{}{updater},
	})
	if err != nil {
		println("Error:", err.Error())
	}
}

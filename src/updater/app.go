package main

import (
	"archive/zip"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"runtime"

	wailsruntime "github.com/wailsapp/wails/v2/pkg/runtime"
)

type Updater struct {
	ctx context.Context
}

type Update struct {
	Version     string `json:"version"`
	FileLinux   string `json:"fileLinux"`
	FileWindows string `json:"fileWindows"`
}

const updateJSONURL = "http://play.arcadiafr.fr:3000/latest.json"

func NewUpdater() *Updater {
	return &Updater{}
}

func (u *Updater) Startup(ctx context.Context) {
	u.ctx = ctx
}

func (u *Updater) FetchLatestVersion() (string, error) {
	resp, err := http.Get(updateJSONURL)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()
	var update Update
	if err := json.NewDecoder(resp.Body).Decode(&update); err != nil {
		return "", err
	}
	return update.Version, nil
}

func (u *Updater) FileExists() bool {
	home, _ := os.UserHomeDir()
	path := filepath.Join(home, ".cero", "version.txt")
	_, err := os.Stat(path)
	return !os.IsNotExist(err)
}

func (u *Updater) ReadVersionFile() (string, error) {
	home, _ := os.UserHomeDir()
	path := filepath.Join(home, ".cero", "version.txt")
	data, err := os.ReadFile(path)
	if err != nil {
		return "", err
	}
	return string(data), nil
}

func (u *Updater) StartUpdate() string {
	home, _ := os.UserHomeDir()
	appDir := filepath.Join(home, ".cero")
	os.MkdirAll(appDir, os.ModePerm)

	resp, err := http.Get(updateJSONURL)
	if err != nil {
		return fmt.Sprintf("Erreur fetch update: %v", err)
	}
	defer resp.Body.Close()
	var update Update
	if err := json.NewDecoder(resp.Body).Decode(&update); err != nil {
		return fmt.Sprintf("Erreur decode update: %v", err)
	}

	var zipFileName string
	if runtime.GOOS == "windows" {
		zipFileName = update.FileWindows
	} else {
		zipFileName = update.FileLinux
	}

	zipURL := fmt.Sprintf("http://play.arcadiafr.fr:3000/updates/%s", zipFileName)
	localZip := filepath.Join(appDir, "update.zip")

	if err := u.downloadWithProgress(localZip, zipURL); err != nil {
		return fmt.Sprintf("Erreur téléchargement: %v", err)
	}

	if err := unzip(localZip, appDir); err != nil {
		return fmt.Sprintf("Erreur extraction: %v", err)
	}
	os.Remove(localZip)

	versionFile := filepath.Join(appDir, "version.txt")
	os.WriteFile(versionFile, []byte(update.Version), 0644)

	return "Mise à jour terminée !"
}

func (u *Updater) downloadWithProgress(dest, url string) error {
	out, err := os.Create(dest)
	if err != nil {
		return err
	}
	defer out.Close()

	resp, err := http.Get(url)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	size := resp.ContentLength
	var downloaded int64
	buf := make([]byte, 4096)
	for {
		n, err := resp.Body.Read(buf)
		if n > 0 {
			out.Write(buf[:n])
			downloaded += int64(n)
			percent := int(float64(downloaded) / float64(size) * 100)
			wailsruntime.EventsEmit(u.ctx, "progress", percent)
		}
		if err != nil {
			if err == io.EOF {
				break
			}
			return err
		}
	}
	wailsruntime.EventsEmit(u.ctx, "progress", 100)
	return nil
}

func unzip(src, dest string) error {
	r, err := zip.OpenReader(src)
	if err != nil {
		return err
	}
	defer r.Close()

	for _, f := range r.File {
		fpath := filepath.Join(dest, f.Name)

		if f.FileInfo().IsDir() {
			os.MkdirAll(fpath, os.ModePerm)
			continue
		}

		if err := os.MkdirAll(filepath.Dir(fpath), os.ModePerm); err != nil {
			return err
		}

		inFile, err := f.Open()
		if err != nil {
			return err
		}
		outFile, err := os.OpenFile(fpath, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, f.Mode())
		if err != nil {
			inFile.Close()
			return err
		}
		io.Copy(outFile, inFile)
		inFile.Close()
		outFile.Close()
	}
	return nil
}

func (u *Updater) Finish() error {
	home, _ := os.UserHomeDir()
	appDir := filepath.Join(home, ".cero")

	appPath := filepath.Join(appDir, "ceroclient")
	if runtime.GOOS == "windows" {
		appPath += ".exe"
	}

	return u.runApp(appPath)
}

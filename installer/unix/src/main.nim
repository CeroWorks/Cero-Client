{.passC: gorge("pkg-config --cflags gtk4").}
{.passL: gorge("pkg-config --libs gtk4").}
{.passL: gorge("pkg-config --libs gmodule-2.0").}
when defined(linux):
  {.passL: "-lselinux".}

import std/[httpclient, os, strutils, strformat, osproc]

const OS_SUFFIX = when defined(linux): "linux"
                  elif defined(freebsd): "freebsd"
                  else: "unknown"

const ARCH_SUFFIX = when defined(amd64): "x86_64"
                    else: "unknown"

const GITHUB_RELEASES_BASE = "https://github.com/CeroWorks/Cero-Client/releases/latest/download"
const GITHUB_RAW_BASE = "https://raw.githubusercontent.com/CeroWorks/Cero-Client/main"

const LICENSE_URL = fmt"{GITHUB_RAW_BASE}/THIRD_PARTY_LICENSES.txt"
const ICON_URL = fmt"{GITHUB_RAW_BASE}/assets/favicon.ico"

type
  GtkApplication {.importc: "GtkApplication", header: "<gtk/gtk.h>".} = object
  GtkWidget {.importc: "GtkWidget", header: "<gtk/gtk.h>".} = object
  GApplication {.importc: "GApplication", header: "<gtk/gtk.h>".} = object
  GtkBox {.importc: "GtkBox", header: "<gtk/gtk.h>".} = object
  GtkButton {.importc: "GtkButton", header: "<gtk/gtk.h>".} = object
  GtkLabel {.importc: "GtkLabel", header: "<gtk/gtk.h>".} = object

proc gtk_application_new(app_id: cstring, flags: cint): ptr GtkApplication
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_application_window_new(app: ptr GtkApplication): ptr GtkWidget
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_window_set_title(window: ptr GtkWidget, title: cstring)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_window_set_default_size(window: ptr GtkWidget, w, h: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_window_present(window: ptr GtkWidget)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_widget_add_css_class(widget: ptr GtkWidget, class_name: cstring)
  {.importc, header: "<gtk/gtk.h>".}

proc gtk_widget_set_margin_top(widget: ptr GtkWidget, margin: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_widget_set_margin_bottom(widget: ptr GtkWidget, margin: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_widget_set_halign(widget: ptr GtkWidget, align: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_widget_set_valign(widget: ptr GtkWidget, align: cint)
  {.importc, header: "<gtk/gtk.h>".}

proc gtk_box_new(orientation: cint, spacing: cint): ptr GtkWidget
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_button_new_with_label(label: cstring): ptr GtkWidget
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_label_new(text: cstring): ptr GtkWidget
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_label_set_text(label: ptr GtkLabel, text: cstring)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_box_append(box: ptr GtkBox, child: ptr GtkWidget)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_window_set_child(window: ptr GtkWidget, child: ptr GtkWidget)
  {.importc, header: "<gtk/gtk.h>".}

proc g_signal_connect_data(instance: pointer, detailed_signal: cstring,
                            c_handler: pointer, data: pointer,
                            destroy_data: pointer, flags: cint): culong
  {.importc, header: "<glib-object.h>".}
proc g_application_run(app: pointer, argc: cint, argv: ptr cstring): cint
  {.importc, header: "<gio/gio.h>".}

proc g_application_quit(app: pointer)
  {.importc, header: "<gio/gio.h>".}

proc gtk_css_provider_new(): pointer
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_css_provider_load_from_string(provider: pointer, css: cstring)
  {.importc, header: "<gtk/gtk.h>".}
proc gdk_display_get_default(): pointer
  {.importc, header: "<gdk/gdk.h>".}
proc gtk_style_context_add_provider_for_display(display: pointer, provider: pointer, priority: cuint)
  {.importc, header: "<gtk/gtk.h>".}

const GTK_STYLE_PROVIDER_PRIORITY_APPLICATION: cuint = 600
const GTK_ORIENTATION_VERTICAL: cint = 1
const GTK_ALIGN_CENTER: cint = 3

const CSS = """
window.cero-installer {
  background-color: alpha(@theme_bg_color, 0.85);
}
button.cero-install-btn {
  padding: 10px 20px;
  font-weight: bold;
}
label.cero-status {
  font-style: italic;
  color: @theme_fg_color;
}
"""

var statusLabel: ptr GtkLabel
var globalApp: pointer = nil

proc updateStatus(msg: string) =
  echo msg
  if statusLabel != nil:
    gtk_label_set_text(statusLabel, msg.cstring)

proc downloadFile(url, dest: string) =
  var client = newHttpClient()
  client.headers = newHttpHeaders({"User-Agent": "CeroClient-Installer/0.1"})
  client.downloadFile(url, dest)
  client.close()

proc performInstallation() =
  let homeDir = getHomeDir()
  let destDir = homeDir / ".ceroclient"
  let bootstrapperPath = destDir / "ceroclient-bootstrapper"

  let iconDir = homeDir / ".local/share/icons/hicolor/256x256/apps"
  let iconFile = iconDir / "ceroclient.png"
  let iconIco = destDir / "ceroclient.ico"
  let licenseFile = destDir / "THIRD_PARTY_LICENSES.txt"

  let desktopFile = homeDir / ".local/share/applications/ceroclient.desktop"
  let binDir = homeDir / ".local/bin"
  let linkPath = binDir / "ceroclient"

  updateStatus("Preparing directories...")
  createDir(destDir)
  createDir(iconDir)
  createDir(binDir)

  let zipName = fmt"CeroClient-bootstrapper-{OS_SUFFIX}-{ARCH_SUFFIX}.zip"
  let fileUrl = fmt"{GITHUB_RELEASES_BASE}/{zipName}"
  let zipPath = destDir / zipName

  updateStatus(fmt"Downloading {zipName}...")

  try:
    downloadFile(fileUrl, zipPath)
  except Exception as e:
    updateStatus(fmt"Error downloading: {e.msg}")
    return

  updateStatus("Download complete. Extracting...")
  if findExe("unzip").len > 0:
    discard execCmd(fmt"unzip -o {zipPath} -d {destDir} 2>/dev/null")
    removeFile(zipPath)
  else:
    updateStatus("Error: 'unzip' command not found.")
    return

  if not fileExists(bootstrapperPath):
    updateStatus(fmt"Error: Bootstrapper binary not found in {destDir}")
    return

  setFilePermissions(bootstrapperPath, {fpUserRead, fpUserWrite, fpUserExec, fpGroupRead, fpGroupExec, fpOthersRead, fpOthersExec})
  updateStatus("Bootstrapper extracted.")

  updateStatus("Downloading licenses...")
  try:
    downloadFile(LICENSE_URL, licenseFile)
  except:
    updateStatus("Warning: Could not download licenses.")

  updateStatus("Downloading icon...")
  try:
    downloadFile(ICON_URL, iconIco)
  except:
    updateStatus("Warning: Could not download icon.")

  updateStatus("Converting icon...")
  let magickPath = findExe("magick")
  let convertPath = findExe("convert")
  if magickPath.len > 0:
    discard execCmd(fmt"{magickPath} {iconIco} -resize 256x256 {iconFile} 2>/dev/null")
  elif convertPath.len > 0:
    discard execCmd(fmt"{convertPath} {iconIco} -resize 256x256 {iconFile} 2>/dev/null")
  else:
    copyFile(iconIco, iconFile)

  updateStatus("Creating executable link...")
  if symlinkExists(linkPath):
    removeFile(linkPath)
  createSymlink(bootstrapperPath, linkPath)

  updateStatus("Creating desktop entry...")
  let desktopContent = fmt"""[Desktop Entry]
Version=1.0
Type=Application
Name=Cero Client
GenericName=Cero Client
Comment=Launch the Cero Client
Exec={bootstrapperPath}
Icon={iconFile}
Terminal=false
Categories=Game;
StartupNotify=true
StartupWMClass=CeroClient
Keywords=Cero;Client;Minecraft;"""

  writeFile(desktopFile, desktopContent)
  setFilePermissions(desktopFile, {fpUserRead, fpUserWrite, fpUserExec, fpGroupRead, fpGroupExec, fpOthersRead, fpOthersExec})

  updateStatus("Refreshing system caches...")
  if findExe("update-desktop-database").len > 0:
    discard execCmd(fmt"update-desktop-database {homeDir}/.local/share/applications 2>/dev/null")
  if findExe("gtk-update-icon-cache").len > 0:
    discard execCmd(fmt"gtk-update-icon-cache {homeDir}/.local/share/icons/hicolor 2>/dev/null")

    updateStatus("Launching bootstrapper...")

    discard execCmd(fmt"nohup '{bootstrapperPath}' > /dev/null 2>&1 &")

    updateStatus("Installation finished! Closing installer...")

  if globalApp != nil:
    g_application_quit(globalApp)

proc onInstallButtonClicked(btn: ptr GtkButton, userData: pointer) {.cdecl.} =
  performInstallation()

proc onActivate(app: ptr GApplication, userData: pointer) {.cdecl.} =
  globalApp = cast[pointer](app)

  let provider = gtk_css_provider_new()
  gtk_css_provider_load_from_string(provider, CSS.cstring)
  gtk_style_context_add_provider_for_display(
    gdk_display_get_default(), provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION)

  let window = gtk_application_window_new(cast[ptr GtkApplication](app))
  gtk_window_set_title(window, "CeroClient Installer")

  gtk_window_set_default_size(window, 400, 200)
  gtk_widget_add_css_class(window, "cero-installer")

  let mainBox = cast[ptr GtkBox](gtk_box_new(GTK_ORIENTATION_VERTICAL, 20))

  gtk_widget_set_halign(cast[ptr GtkWidget](mainBox), GTK_ALIGN_CENTER)
  gtk_widget_set_valign(cast[ptr GtkWidget](mainBox), GTK_ALIGN_CENTER)
  gtk_widget_set_margin_top(cast[ptr GtkWidget](mainBox), 20)
  gtk_widget_set_margin_bottom(cast[ptr GtkWidget](mainBox), 20)

  let installBtn = gtk_button_new_with_label("Install CeroClient")
  gtk_widget_add_css_class(installBtn, "cero-install-btn")

  let statusWidget = gtk_label_new("Ready to install.")
  gtk_widget_add_css_class(statusWidget, "cero-status")
  statusLabel = cast[ptr GtkLabel](statusWidget)

  discard g_signal_connect_data(installBtn, "clicked", onInstallButtonClicked, nil, nil, 0)

  gtk_box_append(mainBox, installBtn)
  gtk_box_append(mainBox, statusWidget)

  gtk_window_set_child(window, cast[ptr GtkWidget](mainBox))
  gtk_window_present(window)

proc main() =
  let app = gtk_application_new("fr.ceroworks.installer", 0)
  discard g_signal_connect_data(app, "activate", onActivate, nil, nil, 0)
  discard g_application_run(cast[pointer](app), 0, nil)

main()

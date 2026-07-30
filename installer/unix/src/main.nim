{.passC: gorge("pkg-config --cflags gtk4").}
{.passL: gorge("pkg-config --libs gtk4").}
{.passL: gorge("pkg-config --libs gmodule-2.0").}

import std/[httpclient, os, strutils, strformat, osproc, locks]

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
proc gtk_widget_set_sensitive(widget: ptr GtkWidget, sensitive: cint)
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

proc g_timeout_add(interval: cuint, function: proc (data: pointer): cint {.cdecl.}, data: pointer): cuint
  {.importc, header: "<glib.h>".}

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

var
  statusLabel: ptr GtkLabel
  installBtnRef: ptr GtkWidget
  globalApp: pointer = nil
  statusLock: Lock
  pendingStatus: string = ""
  installDone = false
  installThread: Thread[void]

initLock(statusLock)

proc updateStatus(msg: string) {.gcsafe.} =
  echo msg
  {.cast(gcsafe).}:
    withLock statusLock:
      pendingStatus = msg

proc markDone() {.gcsafe.} =
  {.cast(gcsafe).}:
    withLock statusLock:
      installDone = true

proc pollUiUpdates(data: pointer): cint {.cdecl.} =
  var msg = ""
  var done = false
  withLock statusLock:
    msg = pendingStatus
    done = installDone

  if statusLabel != nil and msg.len > 0:
    gtk_label_set_text(statusLabel, msg.cstring)

  if done:
    if globalApp != nil:
      g_application_quit(globalApp)
    return 0

  return 1

proc downloadFile(url, dest: string) =
  var client = newHttpClient()
  client.headers = newHttpHeaders({"User-Agent": "CeroClient-Installer/0.1"})
  client.downloadFile(url, dest)
  client.close()

proc sha256OfFile(path: string): string =
  let (output, code) = execCmdEx(fmt"sha256sum {quoteShell(path)}")
  if code != 0:
    return ""
  result = output.splitWhitespace()[0].toLowerAscii()

proc lookupChecksum(checksumsTxt, filename: string): string =
  for line in checksumsTxt.splitLines():
    let parts = line.splitWhitespace()
    if parts.len >= 2 and parts[1].strip(chars = {'*'}) == filename:
      return parts[0].toLowerAscii()
  return ""

proc performInstallation() {.thread.} =
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

  updateStatus("Préparation des dossiers...")
  createDir(destDir)
  createDir(iconDir)
  createDir(binDir)

  let zipName = fmt"CeroClient-bootstrapper-{OS_SUFFIX}-{ARCH_SUFFIX}.zip"
  let fileUrl = fmt"{GITHUB_RELEASES_BASE}/{zipName}"
  let zipPath = destDir / zipName

  updateStatus(fmt"Téléchargement de {zipName}...")
  try:
    downloadFile(fileUrl, zipPath)
  except Exception as e:
    updateStatus(fmt"Erreur de téléchargement : {e.msg}")
    markDone()
    return

  updateStatus("Vérification de l'intégrité...")
  try:
    let checksumsTxt = newHttpClient().getContent(fmt"{GITHUB_RELEASES_BASE}/checksums.txt")
    let expected = lookupChecksum(checksumsTxt, zipName)
    if expected.len == 0:
      updateStatus(fmt"Erreur : aucune entrée checksums.txt pour {zipName}")
      removeFile(zipPath)
      markDone()
      return
    let actual = sha256OfFile(zipPath)
    if actual != expected:
      updateStatus(fmt"Erreur : checksum invalide (attendu {expected}, obtenu {actual})")
      removeFile(zipPath)
      markDone()
      return
  except Exception as e:
    updateStatus(fmt"Erreur de vérification checksum : {e.msg}")
    removeFile(zipPath)
    markDone()
    return

  updateStatus("Téléchargement terminé. Extraction...")
  if findExe("unzip").len > 0:
    discard execCmd(fmt"unzip -o {quoteShell(zipPath)} -d {quoteShell(destDir)} > /dev/null 2>&1")
    removeFile(zipPath)
  else:
    updateStatus("Erreur : la commande 'unzip' est introuvable.")
    markDone()
    return

  if not fileExists(bootstrapperPath):
    updateStatus(fmt"Erreur : bootstrapper introuvable dans {destDir}")
    markDone()
    return

  setFilePermissions(bootstrapperPath, {fpUserRead, fpUserWrite, fpUserExec, fpGroupRead, fpGroupExec, fpOthersRead, fpOthersExec})
  updateStatus("Bootstrapper extrait.")

  updateStatus("Téléchargement des licences...")
  try:
    downloadFile(LICENSE_URL, licenseFile)
  except:
    updateStatus("Avertissement : licences non téléchargées.")

  updateStatus("Téléchargement de l'icône...")
  var iconOk = true
  try:
    downloadFile(ICON_URL, iconIco)
  except:
    iconOk = false
    updateStatus("Avertissement : icône non téléchargée.")

  if iconOk:
    updateStatus("Conversion de l'icône...")
    let magickPath = findExe("magick")
    let convertPath = findExe("convert")
    if magickPath.len > 0:
      discard execCmd(fmt"{quoteShell(magickPath)} {quoteShell(iconIco)} -resize 256x256 {quoteShell(iconFile)} 2>/dev/null")
    elif convertPath.len > 0:
      discard execCmd(fmt"{quoteShell(convertPath)} {quoteShell(iconIco)} -resize 256x256 {quoteShell(iconFile)} 2>/dev/null")
    else:
      updateStatus("Avertissement : pas de convertisseur d'image, icône ignorée.")

  updateStatus("Création du lien exécutable...")
  if symlinkExists(linkPath):
    removeFile(linkPath)
  createSymlink(bootstrapperPath, linkPath)

  updateStatus("Création de l'entrée .desktop...")
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

  updateStatus("Rafraîchissement des caches système...")
  let appsDir = homeDir / ".local/share/applications"
  let iconsDir = homeDir / ".local/share/icons/hicolor"
  if findExe("update-desktop-database").len > 0:
    discard execCmd(fmt"update-desktop-database {quoteShell(appsDir)} 2>/dev/null")
  if findExe("gtk-update-icon-cache").len > 0:
    discard execCmd(fmt"gtk-update-icon-cache {quoteShell(iconsDir)} 2>/dev/null")

  updateStatus("Lancement du bootstrapper...")
  discard execCmd(fmt"nohup {quoteShell(bootstrapperPath)} > /dev/null 2>&1 &")

  updateStatus("Installation terminée ! Fermeture de l'installeur...")
  markDone()

proc onInstallButtonClicked(btn: ptr GtkButton, userData: pointer) {.cdecl.} =
  gtk_widget_set_sensitive(cast[ptr GtkWidget](btn), 0)
  createThread(installThread, performInstallation)
  discard g_timeout_add(100, pollUiUpdates, nil)

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
  installBtnRef = installBtn

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

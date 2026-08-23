{.passC: gorge("pkg-config --cflags gtk4").}
{.passL: gorge("pkg-config --libs gtk4").}
{.passL: gorge("pkg-config --libs gmodule-2.0").}

import std/[httpclient, os, strutils, strformat, osproc, locks, streams]

const OS_SUFFIX =
  when defined(linux):     "linux"
  elif defined(freebsd):   "freebsd"
  elif defined(openbsd):   "openbsd"
  elif defined(netbsd):    "netbsd"
  elif defined(dragonfly): "dragonfly"
  else:                    "unknown"

const ARCH_SUFFIX =
  when defined(amd64):   "x86_64"
  elif defined(arm64):   "aarch64"
  elif defined(i386):    "i686"
  else:                  "unknown"

const IS_BSD = defined(freebsd) or defined(openbsd) or
               defined(netbsd)  or defined(dragonfly)

const GITHUB_RELEASES_BASE =
  "https://github.com/CeroWorks/Cero-Client/releases/latest/download"
const GITHUB_RAW_BASE =
  "https://raw.githubusercontent.com/CeroWorks/Cero-Client/main"

const LICENSE_URL = fmt"{GITHUB_RAW_BASE}/THIRD_PARTY_LICENSES.txt"
const ICON_URL    = fmt"{GITHUB_RAW_BASE}/assets/favicon.ico"

type
  GtkApplication {.importc, header: "<gtk/gtk.h>".} = object
  GtkWidget      {.importc, header: "<gtk/gtk.h>".} = object
  GApplication   {.importc, header: "<gtk/gtk.h>".} = object
  GtkBox         {.importc, header: "<gtk/gtk.h>".} = object
  GtkButton      {.importc, header: "<gtk/gtk.h>".} = object
  GtkLabel       {.importc, header: "<gtk/gtk.h>".} = object
  GtkProgressBar {.importc, header: "<gtk/gtk.h>".} = object
  GError         {.importc, header: "<glib.h>".}    = object

proc gtk_application_new(app_id: cstring, flags: cint): ptr GtkApplication
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_application_window_new(app: ptr GtkApplication): ptr GtkWidget
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_window_set_title(window: ptr GtkWidget, title: cstring)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_window_set_default_size(window: ptr GtkWidget, w, h: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_window_set_resizable(window: ptr GtkWidget, resizable: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_window_present(window: ptr GtkWidget)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_widget_add_css_class(widget: ptr GtkWidget, class_name: cstring)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_widget_set_sensitive(widget: ptr GtkWidget, sensitive: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_widget_set_visible(widget: ptr GtkWidget, visible: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_widget_set_size_request(widget: ptr GtkWidget, w, h: cint)
  {.importc, header: "<gtk/gtk.h>".}

proc gtk_widget_set_margin_top(widget: ptr GtkWidget, margin: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_widget_set_margin_bottom(widget: ptr GtkWidget, margin: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_widget_set_margin_start(widget: ptr GtkWidget, margin: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_widget_set_margin_end(widget: ptr GtkWidget, margin: cint)
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
proc gtk_label_set_wrap(label: ptr GtkLabel, wrap: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_label_set_max_width_chars(label: ptr GtkLabel, n: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_label_set_justify(label: ptr GtkLabel, j: cint)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_box_append(box: ptr GtkBox, child: ptr GtkWidget)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_window_set_child(window: ptr GtkWidget, child: ptr GtkWidget)
  {.importc, header: "<gtk/gtk.h>".}

proc gtk_progress_bar_new(): ptr GtkWidget
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_progress_bar_set_fraction(pb: ptr GtkProgressBar, f: cdouble)
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_progress_bar_pulse(pb: ptr GtkProgressBar)
  {.importc, header: "<gtk/gtk.h>".}

proc g_signal_connect_data(instance: pointer, detailed_signal: cstring,
                           c_handler: pointer, data: pointer,
                           destroy_data: pointer, flags: cint): culong
  {.importc, header: "<glib-object.h>".}
proc g_application_run(app: pointer, argc: cint, argv: ptr cstring): cint
  {.importc, header: "<gio/gio.h>".}
proc g_application_quit(app: pointer)
  {.importc, header: "<gio/gio.h>".}
proc g_timeout_add(interval: cuint,
                   function: proc (data: pointer): cint {.cdecl.},
                   data: pointer): cuint
  {.importc, header: "<glib.h>".}

proc gtk_css_provider_new(): pointer
  {.importc, header: "<gtk/gtk.h>".}
proc gtk_css_provider_load_from_string(provider: pointer, css: cstring)
  {.importc, header: "<gtk/gtk.h>".}
proc gdk_display_get_default(): pointer
  {.importc, header: "<gdk/gdk.h>".}
proc gtk_style_context_add_provider_for_display(display, provider: pointer,
                                                priority: cuint)
  {.importc, header: "<gtk/gtk.h>".}

proc gdk_pixbuf_new_from_file_at_scale(filename: cstring, w, h: cint,
                                       preserve: cint,
                                       error: ptr ptr GError): pointer
  {.importc, header: "<gdk-pixbuf/gdk-pixbuf.h>".}
proc gdk_pixbuf_savev(pixbuf: pointer, filename: cstring, `type`: cstring,
                      option_keys: ptr cstring, option_values: ptr cstring,
                      error: ptr ptr GError): cint
  {.importc, header: "<gdk-pixbuf/gdk-pixbuf.h>".}
proc g_object_unref(obj: pointer)
  {.importc, header: "<glib-object.h>".}
proc g_clear_error(err: ptr ptr GError)
  {.importc, header: "<glib.h>".}

const
  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION: cuint = 600
  GTK_ORIENTATION_VERTICAL: cint = 1
  GTK_ALIGN_CENTER: cint = 3
  GTK_JUSTIFY_CENTER: cint = 2

const CSS = """
window.cero-installer { background-color: alpha(@theme_bg_color, 0.92); }
button.cero-install-btn { padding: 12px 26px; font-weight: bold; border-radius: 8px; }
label.cero-title { font-size: 1.4em; font-weight: bold; }
label.cero-status { font-style: italic; opacity: 0.85; }
label.cero-error  { color: #e01b24; font-weight: bold; }
"""

type Step = tuple[msg: string, frac: float]

var
  statusLabel:    ptr GtkLabel
  progressBar:    ptr GtkProgressBar
  installBtnRef:  ptr GtkWidget
  globalApp:      pointer = nil
  statusLock:     Lock
  pendingStatus:  string = "Prêt à installer."
  pendingFrac:    float  = 0.0
  hasError:       bool   = false
  installDone:    bool   = false
  quitTicks:      int    = 0
  installThread:  Thread[void]

initLock(statusLock)

proc setStatus(msg: string, frac: float = -1.0, err = false) {.gcsafe.} =
  echo (if err: "[!] " else: "[*] ") & msg
  {.cast(gcsafe).}:
    withLock statusLock:
      pendingStatus = msg
      if frac >= 0.0: pendingFrac = frac
      if err: hasError = true

proc markDone() {.gcsafe.} =
  {.cast(gcsafe).}:
    withLock statusLock:
      installDone = true

proc which(prog: string): string =
  result = findExe(prog)

proc runSilent(exe: string, args: seq[string]): int =
  try:
    let p = startProcess(exe, args = args, options = {poStdErrToStdOut})
    defer: p.close()
    discard p.outputStream.readAll()
    result = p.waitForExit()
  except OSError:
    result = 127

proc runDetached(exe: string, args: seq[string]) =
  try:
    let p = startProcess(exe, args = args,
                         options = {poDaemon, poStdErrToStdOut})
    p.close()
  except OSError:
    discard

proc xdgDataHome(): string =
  let e = getEnv("XDG_DATA_HOME")
  if e.len > 0 and e.isAbsolute: e else: getHomeDir() / ".local" / "share"

proc xdgBinHome(): string =
  getHomeDir() / ".local" / "bin"

proc sha256OfFile(path: string): string =
  block gnu:
    let e = which("sha256sum")
    if e.len > 0:
      let (o, c) = execCmdEx(quoteShell(e) & " " & quoteShell(path))
      if c == 0 and o.splitWhitespace().len > 0:
        return o.splitWhitespace()[0].toLowerAscii()
  block bsd:
    let e = which("sha256")
    if e.len > 0:
      let (o, c) = execCmdEx(quoteShell(e) & " -q " & quoteShell(path))
      if c == 0: return o.strip().toLowerAscii()
  block obsd:
    let e = which("cksum")
    if e.len > 0:
      let (o, c) = execCmdEx(quoteShell(e) & " -a sha256 " & quoteShell(path))
      if c == 0:
        let parts = o.strip().split('=')
        if parts.len == 2: return parts[1].strip().toLowerAscii()
  block perl:
    let e = which("shasum")
    if e.len > 0:
      let (o, c) = execCmdEx(quoteShell(e) & " -a 256 " & quoteShell(path))
      if c == 0 and o.splitWhitespace().len > 0:
        return o.splitWhitespace()[0].toLowerAscii()
  return ""

proc lookupChecksum(checksumsTxt, filename: string): string =
  for line in checksumsTxt.splitLines():
    let parts = line.splitWhitespace()
    if parts.len >= 2 and parts[1].strip(chars = {'*'}) == filename:
      return parts[0].toLowerAscii()
  return ""

proc extractZip(zipPath, destDir: string): bool =
  let unzipExe = which("unzip")
  if unzipExe.len > 0:
    return runSilent(unzipExe, @["-o", "-q", zipPath, "-d", destDir]) == 0
  let bsdtar = which("bsdtar")
  if bsdtar.len > 0:
    return runSilent(bsdtar, @["-x", "-f", zipPath, "-C", destDir]) == 0
  when IS_BSD:
    let tarExe = which("tar")
    if tarExe.len > 0:
      if runSilent(tarExe, @["-x", "-f", zipPath, "-C", destDir]) == 0:
        return true
  for sevenz in ["7z", "7zz", "7za"]:
    let e = which(sevenz)
    if e.len > 0:
      return runSilent(e, @["x", "-y", "-o" & destDir, zipPath]) == 0
  let py = which("python3")
  if py.len > 0:
    return runSilent(py, @["-c",
      "import sys,zipfile;zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])",
      zipPath, destDir]) == 0
  return false

proc hasGraphicalSession(): bool =
  getEnv("DISPLAY").len > 0 or getEnv("WAYLAND_DISPLAY").len > 0

proc findAskpass(): string =
  let env = getEnv("SUDO_ASKPASS")
  if env.len > 0 and fileExists(env): return env

  let candidates = @[
    "ssh-askpass", "x11-ssh-askpass", "lxqt-openssh-askpass",
    "ksshaskpass", "seahorse-ssh-askpass",
    "gnome-ssh-askpass3", "gnome-ssh-askpass2", "gnome-ssh-askpass",
    "mate-ssh-askpass", "openssh-askpass"
  ]
  for c in candidates:
    let p = which(c)
    if p.len > 0: return p

  let paths = @[
    "/usr/libexec/openssh/gnome-ssh-askpass",
    "/usr/lib/openssh/gnome-ssh-askpass",
    "/usr/lib/ssh/x11-ssh-askpass",
    "/usr/local/libexec/ssh-askpass",
    "/usr/local/bin/ssh-askpass",
    "/usr/X11R6/bin/ssh-askpass",
    "/usr/pkg/bin/ssh-askpass"
  ]
  for p in paths:
    if fileExists(p): return p
  return ""

proc findTerminal(): (string, seq[string]) =
  let terms = @[
    ("gnome-terminal",  @["--wait", "--"]),
    ("kgx",             @["--"]),
    ("konsole",         @["-e"]),
    ("xfce4-terminal",  @["--disable-server", "-x"]),
    ("mate-terminal",   @["--"]),
    ("lxterminal",      @["-e"]),
    ("qterminal",       @["-e"]),
    ("tilix",           @["--"]),
    ("alacritty",       @["-e"]),
    ("kitty",           @["--"]),
    ("foot",            @["--"]),
    ("urxvt",           @["-e"]),
    ("xterm",           @["-e"])
  ]
  for (name, pre) in terms:
    let p = which(name)
    if p.len > 0: return (p, pre)
  return ("", @[])

proc runPrivileged(argv: seq[string], reason: string): bool =
  if argv.len == 0: return false
  let gui = hasGraphicalSession()

  if gui:
    let pkexec = which("pkexec")
    if pkexec.len > 0:
      setStatus(reason & " — authentification (PolicyKit)…")
      let rc = runSilent(pkexec, @["--disable-internal-agent"] & argv)
      if rc == 0: return true
      if rc == 126 or rc == 127:
        setStatus("Authentification annulée ou refusée.", err = true)
        return false
      return false

    for (name, pre) in [("lxqt-sudo", @[]), ("kdesu", @["-t", "--"]),
                        ("kdesudo", @[]),   ("gksu", @[]),
                        ("gksudo",   @[]),  ("beesu", @[])]:
      let e = which(name)
      if e.len > 0:
        setStatus(reason & fmt" — authentification ({name})…")
        return runSilent(e, pre & argv) == 0

    let askpass = findAskpass()
    if askpass.len > 0:
      let sudoExe = which("sudo")
      if sudoExe.len > 0:
        setStatus(reason & " — authentification (sudo/askpass)…")
        putEnv("SUDO_ASKPASS", askpass)
        if runSilent(sudoExe, @["-A", "--"] & argv) == 0: return true
      let doasExe = which("doas")
      if doasExe.len > 0:
        setStatus(reason & " — authentification (doas)…")
        if runSilent(doasExe, @["--"] & argv) == 0: return true

    let (term, pre) = findTerminal()
    if term.len > 0:
      let sudoExe = if which("sudo").len > 0: which("sudo")
                    elif which("doas").len > 0: which("doas")
                    else: ""
      if sudoExe.len > 0:
        setStatus(reason & " — saisie du mot de passe dans le terminal…")
        return runSilent(term, pre & @[sudoExe] & argv) == 0

  for name in ["sudo", "doas"]:
    let e = which(name)
    if e.len > 0:
      setStatus(reason & fmt" — mot de passe {name} requis (terminal)…")
      return runSilent(e, argv) == 0

  setStatus("Impossible d'obtenir les privilèges administrateur.", err = true)
  return false

proc convertIconWithPixbuf(src, dst: string): bool =
  var err: ptr GError = nil
  let pb = gdk_pixbuf_new_from_file_at_scale(src.cstring, 256, 256, 1, addr err)
  if pb == nil:
    g_clear_error(addr err)
    return false
  defer: g_object_unref(pb)
  var keys:   array[2, cstring] = ["compression".cstring, nil]
  var values: array[2, cstring] = ["9".cstring, nil]
  let ok = gdk_pixbuf_savev(pb, dst.cstring, "png".cstring,
                            addr keys[0], addr values[0], addr err)
  if ok == 0:
    g_clear_error(addr err)
    return false
  return true

proc installImageMagickCmd(): seq[string] =
  if which("apt-get").len > 0:
    return @[which("apt-get"), "install", "-y", "imagemagick"]
  if which("pacman").len > 0:
    return @[which("pacman"), "-S", "--noconfirm", "imagemagick"]
  if which("dnf").len > 0:
    return @[which("dnf"), "install", "-y", "ImageMagick"]
  if which("zypper").len > 0:
    return @[which("zypper"), "--non-interactive", "install", "ImageMagick"]
  if which("xbps-install").len > 0:
    return @[which("xbps-install"), "-Sy", "ImageMagick"]
  if which("emerge").len > 0:
    return @[which("emerge"), "-q", "media-gfx/imagemagick"]
  if which("apk").len > 0:
    return @[which("apk"), "add", "--no-progress", "imagemagick"]
  when defined(freebsd) or defined(dragonfly):
    if which("pkg").len > 0:
      return @[which("pkg"), "install", "-y", "ImageMagick7"]
  when defined(openbsd):
    if which("pkg_add").len > 0:
      return @[which("pkg_add"), "-I", "ImageMagick"]
  when defined(netbsd):
    if which("pkgin").len > 0:
      return @[which("pkgin"), "-y", "install", "ImageMagick"]
    if which("pkg_add").len > 0:
      return @[which("pkg_add"), "ImageMagick"]
  return @[]

proc prepareIcon(icoPath, pngPath: string): bool =
  if convertIconWithPixbuf(icoPath, pngPath):
    return true
  for tool in ["magick", "convert"]:
    let e = which(tool)
    if e.len > 0:
      var args = if tool == "magick": @[icoPath] else: @[icoPath]
      args.add(@["-resize", "256x256", pngPath])
      if runSilent(e, args) == 0 and fileExists(pngPath): return true
  let cmd = installImageMagickCmd()
  if cmd.len > 0:
    setStatus("ImageMagick requis pour l'icône — installation…")
    if runPrivileged(cmd, "Installation d'ImageMagick"):
      for tool in ["magick", "convert"]:
        let e = which(tool)
        if e.len > 0:
          var args = @[icoPath, "-resize", "256x256", pngPath]
          if runSilent(e, args) == 0 and fileExists(pngPath): return true
  return false

proc newClient(): HttpClient =
  result = newHttpClient(timeout = 30_000)
  result.headers = newHttpHeaders({
    "User-Agent": fmt"CeroClient-Installer/1.0 ({OS_SUFFIX}; {ARCH_SUFFIX})"
  })

proc downloadFile(url, dest: string) =
  let c = newClient()
  defer: c.close()
  c.downloadFile(url, dest)

proc performInstallation() {.thread.} =
  if OS_SUFFIX == "unknown" or ARCH_SUFFIX == "unknown":
    setStatus("Plateforme non supportée par cet installeur.", 1.0, err = true)
    markDone(); return

  let
    homeDir    = getHomeDir()
    destDir    = homeDir / ".ceroclient"
    bootstrap  = destDir / "ceroclient-bootstrapper"
    dataHome   = xdgDataHome()
    iconDir    = dataHome / "icons" / "hicolor" / "256x256" / "apps"
    iconFile   = iconDir / "ceroclient.png"
    iconIco    = destDir / "ceroclient.ico"
    licenseF   = destDir / "THIRD_PARTY_LICENSES.txt"
    appsDir    = dataHome / "applications"
    desktopF   = appsDir / "ceroclient.desktop"
    binDir     = xdgBinHome()
    linkPath   = binDir / "ceroclient"

  setStatus("Préparation des dossiers…", 0.05)
  try:
    createDir(destDir); createDir(iconDir); createDir(binDir); createDir(appsDir)
  except OSError as e:
    setStatus(fmt"Impossible de créer les dossiers : {e.msg}", 1.0, err = true)
    markDone(); return

  let zipName = fmt"CeroClient-bootstrapper-{OS_SUFFIX}-{ARCH_SUFFIX}.zip"
  let zipPath = destDir / zipName

  setStatus(fmt"Téléchargement de {zipName}…", 0.15)
  try:
    downloadFile(fmt"{GITHUB_RELEASES_BASE}/{zipName}", zipPath)
  except CatchableError as e:
    setStatus(fmt"Erreur de téléchargement : {e.msg}", 1.0, err = true)
    markDone(); return

  setStatus("Vérification de l'intégrité (SHA-256)…", 0.35)
  try:
    let c = newClient()
    defer: c.close()
    let expected = lookupChecksum(
      c.getContent(fmt"{GITHUB_RELEASES_BASE}/checksums.txt"), zipName)
    if expected.len == 0:
      setStatus(fmt"Aucune entrée checksums.txt pour {zipName}", 1.0, err = true)
      removeFile(zipPath); markDone(); return
    let actual = sha256OfFile(zipPath)
    if actual.len == 0:
      setStatus("Aucun outil SHA-256 disponible (sha256sum/sha256/cksum).",
                1.0, err = true)
      removeFile(zipPath); markDone(); return
    if actual != expected:
      setStatus(fmt"Checksum invalide — fichier corrompu ou altéré.",
                1.0, err = true)
      removeFile(zipPath); markDone(); return
  except CatchableError as e:
    setStatus(fmt"Erreur de vérification : {e.msg}", 1.0, err = true)
    removeFile(zipPath); markDone(); return

  setStatus("Extraction de l'archive…", 0.50)
  if not extractZip(zipPath, destDir):
    setStatus("Extraction impossible (unzip/bsdtar/7z/python3 introuvables).",
              1.0, err = true)
    markDone(); return
  removeFile(zipPath)

  if not fileExists(bootstrap):
    setStatus(fmt"Bootstrapper introuvable dans {destDir}", 1.0, err = true)
    markDone(); return

  setFilePermissions(bootstrap, {fpUserRead, fpUserWrite, fpUserExec,
                                 fpGroupRead, fpGroupExec,
                                 fpOthersRead, fpOthersExec})

  setStatus("Téléchargement des licences…", 0.62)
  try: downloadFile(LICENSE_URL, licenseF)
  except CatchableError: setStatus("Licences non téléchargées (ignoré).", 0.62)

  setStatus("Préparation de l'icône…", 0.72)
  var iconOk = false
  try:
    downloadFile(ICON_URL, iconIco)
    iconOk = prepareIcon(iconIco, iconFile)
  except CatchableError:
    iconOk = false
  if not iconOk:
    setStatus("Icône indisponible — poursuite de l'installation.", 0.78)

  setStatus("Création du lien exécutable…", 0.84)
  try:
    if symlinkExists(linkPath) or fileExists(linkPath): removeFile(linkPath)
    createSymlink(bootstrap, linkPath)
  except OSError:
    discard

  setStatus("Création de l'entrée de menu…", 0.90)
  let iconValue = if iconOk: iconFile else: "application-x-executable"
  let desktopContent = fmt"""[Desktop Entry]
Version=1.0
Type=Application
Name=Cero Client
GenericName=Game Launcher
Comment=Launch the Cero Client
Exec={bootstrap} %U
TryExec={bootstrap}
Icon={iconValue}
Path={destDir}
Terminal=false
Categories=Game;
StartupNotify=true
StartupWMClass=CeroClient
Keywords=Cero;Client;Minecraft;
"""
  try:
    writeFile(desktopF, desktopContent)
    setFilePermissions(desktopF, {fpUserRead, fpUserWrite, fpUserExec,
                                  fpGroupRead, fpGroupExec,
                                  fpOthersRead, fpOthersExec})
  except IOError:
    setStatus("Entrée de menu non créée (ignoré).", 0.90)

  setStatus("Rafraîchissement des caches du bureau…", 0.94)
  let udd = which("update-desktop-database")
  if udd.len > 0: discard runSilent(udd, @[appsDir])
  let guic = which("gtk-update-icon-cache")
  if guic.len > 0:
    discard runSilent(guic, @["-f", "-t", dataHome / "icons" / "hicolor"])
  let xdgm = which("xdg-desktop-menu")
  if xdgm.len > 0: discard runSilent(xdgm, @["forceupdate"])

  if binDir notin getEnv("PATH").split(PathSep):
    setStatus(fmt"Note : ajoutez {binDir} à votre PATH.", 0.96)

  setStatus("Lancement de CeroClient…", 0.98)
  runDetached(bootstrap, @[])

  setStatus("Installation terminée avec succès !", 1.0)
  markDone()

proc pollUiUpdates(data: pointer): cint {.cdecl.} =
  var msg = ""; var frac = 0.0; var done = false; var err = false
  withLock statusLock:
    msg = pendingStatus; frac = pendingFrac
    done = installDone;  err = hasError

  if statusLabel != nil and msg.len > 0:
    gtk_label_set_text(statusLabel, msg.cstring)
    if err:
      gtk_widget_add_css_class(cast[ptr GtkWidget](statusLabel), "cero-error")

  if progressBar != nil:
    gtk_progress_bar_set_fraction(progressBar, frac.cdouble)

  if done:
    if err:
      if installBtnRef != nil:
        gtk_widget_set_sensitive(installBtnRef, 1)
      return 0
    inc quitTicks
    if quitTicks > 15:
      if globalApp != nil: g_application_quit(globalApp)
      return 0
  return 1

proc onInstallButtonClicked(btn: ptr GtkButton, userData: pointer) {.cdecl.} =
  gtk_widget_set_sensitive(cast[ptr GtkWidget](btn), 0)
  withLock statusLock:
    pendingStatus = "Démarrage…"; pendingFrac = 0.0
    hasError = false; installDone = false
  quitTicks = 0
  if progressBar != nil:
    gtk_widget_set_visible(cast[ptr GtkWidget](progressBar), 1)
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
  gtk_window_set_default_size(window, 460, 260)
  gtk_window_set_resizable(window, 0)
  gtk_widget_add_css_class(window, "cero-installer")

  let mainBox = cast[ptr GtkBox](gtk_box_new(GTK_ORIENTATION_VERTICAL, 16))
  let mb = cast[ptr GtkWidget](mainBox)
  gtk_widget_set_halign(mb, GTK_ALIGN_CENTER)
  gtk_widget_set_valign(mb, GTK_ALIGN_CENTER)
  gtk_widget_set_margin_top(mb, 24);    gtk_widget_set_margin_bottom(mb, 24)
  gtk_widget_set_margin_start(mb, 28);  gtk_widget_set_margin_end(mb, 28)

  let title = gtk_label_new("Cero Client")
  gtk_widget_add_css_class(title, "cero-title")
  gtk_box_append(mainBox, title)

  let sub = gtk_label_new(fmt"Plateforme détectée : {OS_SUFFIX} · {ARCH_SUFFIX}")
  gtk_box_append(mainBox, sub)

  let installBtn = gtk_button_new_with_label("Installer CeroClient")
  gtk_widget_add_css_class(installBtn, "cero-install-btn")
  gtk_widget_set_halign(installBtn, GTK_ALIGN_CENTER)
  installBtnRef = installBtn
  if OS_SUFFIX == "unknown" or ARCH_SUFFIX == "unknown":
    gtk_widget_set_sensitive(installBtn, 0)
  gtk_box_append(mainBox, installBtn)

  let pbw = gtk_progress_bar_new()
  gtk_widget_set_size_request(pbw, 320, -1)
  gtk_widget_set_visible(pbw, 0)
  progressBar = cast[ptr GtkProgressBar](pbw)
  gtk_box_append(mainBox, pbw)

  let statusWidget = gtk_label_new("Prêt à installer.")
  gtk_widget_add_css_class(statusWidget, "cero-status")
  gtk_label_set_wrap(cast[ptr GtkLabel](statusWidget), 1)
  gtk_label_set_max_width_chars(cast[ptr GtkLabel](statusWidget), 46)
  gtk_label_set_justify(cast[ptr GtkLabel](statusWidget), GTK_JUSTIFY_CENTER)
  statusLabel = cast[ptr GtkLabel](statusWidget)
  gtk_box_append(mainBox, statusWidget)

  discard g_signal_connect_data(installBtn, "clicked",
                                onInstallButtonClicked, nil, nil, 0)
  gtk_window_set_child(window, mb)
  gtk_window_present(window)

proc main() =
  let app = gtk_application_new("fr.ceroworks.installer", 0)
  discard g_signal_connect_data(app, "activate", onActivate, nil, nil, 0)
  discard g_application_run(cast[pointer](app), 0, nil)

main()

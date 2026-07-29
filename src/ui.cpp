#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #define _WINSOCKAPI_ 
  #include <winsock2.h>
  #include <windows.h>
  #include <dwmapi.h>
  #include <wrl.h>
  #include <WebView2.h>
  using namespace Microsoft::WRL;
  #ifdef _MSC_VER
    #pragma comment(lib, "dwmapi.lib")
  #endif
  #ifndef DWMWA_WINDOW_CORNER_PREFERENCE
    #define DWMWA_WINDOW_CORNER_PREFERENCE 33
  #endif
  #ifndef DWMWCP_ROUND
    #define DWMWCP_ROUND 2
  #endif
#endif

#include "../include/logger.h"
#include "webview/webview.h"
#include <cstdlib>
#include <cstring>
#include <math.h>

#if defined(__linux__) || defined(__BSD__)
  #include <gtk/gtk.h>
  #include <gio/gio.h>
  #include <webkit2/webkit2.h>
#endif

#if defined(_WIN32)
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || \
      defined(__NetBSD__)   || defined(__DragonFly__)
  #define __BSD__ 1
#elif defined(__linux__)
#endif

#if defined(__linux__) || defined(__BSD__)
extern "C" {
    #include "../include/assets_loader.h"
    
extern int g_really_quit;

bool system_prefers_dark() {
    GSettings* settings = g_settings_new("org.gnome.desktop.interface");
    if (!settings) return false;
    
    gchar* scheme = g_settings_get_string(settings, "color-scheme");
    bool is_dark = scheme && strstr(scheme, "dark") != nullptr;
    
    g_free(scheme);
    g_object_unref(settings);
    return is_dark;
}

void ui_apply_system_theme() {
    bool dark = system_prefers_dark();
    g_object_set(gtk_settings_get_default(),
                 "gtk-application-prefer-dark-theme", dark, NULL);
}

static void on_size_allocate(GtkWidget* widget, GdkRectangle* alloc, gpointer) {
    const int radius = 12;
    int w = alloc->width;
    int h = alloc->height;

    cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_A1, w, h);
    cairo_t* cr = cairo_create(surf);

    double r = radius;
    cairo_new_path(cr);
    cairo_arc(cr, r,     r,     r, M_PI,     3*M_PI/2);
    cairo_arc(cr, w-r,   r,     r, 3*M_PI/2, 2*M_PI);
    cairo_arc(cr, w-r,   h-r,   r, 0,        M_PI/2);
    cairo_arc(cr, r,     h-r,   r, M_PI/2,   M_PI);
    cairo_close_path(cr);
    cairo_fill(cr);

    cairo_region_t* region = gdk_cairo_region_create_from_surface(surf);
    GdkWindow* gdkwin = gtk_widget_get_window(widget);
    if (gdkwin) gdk_window_shape_combine_region(gdkwin, region, 0, 0);

    cairo_region_destroy(region);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
}
static void on_theme_changed(GSettings*, gchar*, gpointer user_data) {
    ui_apply_system_theme();
    webview_t w = (webview_t)user_data;
    webview_eval(w, "window.dispatchEvent(new Event('themechange'));");
}

void ui_watch_system_theme(void* w) {
    GSettings* settings = g_settings_new("org.gnome.desktop.interface");
    g_signal_connect(settings, "changed::color-scheme",
                     G_CALLBACK(on_theme_changed), w);
    ui_apply_system_theme();
}

static gboolean on_window_delete(GtkWidget* widget, GdkEvent* event, gpointer data) {
    (void)event; (void)data;
    if (!g_really_quit) {
        log_msg("info", "[TRAY] Close requested, hiding to tray\n");
        gtk_widget_hide(widget);
        return TRUE;
    }
    return FALSE;
}

static void ceroclient_uri_scheme_cb(WebKitURISchemeRequest *request, gpointer user_data) {
    const gchar *path = webkit_uri_scheme_request_get_path(request);
    
    if (path && path[0] == '/') {
        path++;
    }

    const uint8_t *data = NULL;
    size_t size = 0;
    
    if (assets_get_file(path, &data, &size) && data && size > 0) {
        GBytes *bytes = g_bytes_new_with_free_func(data, size, (GDestroyNotify)assets_free_buffer, (void*)data);
        GInputStream *stream = g_memory_input_stream_new_from_bytes(bytes);
        g_bytes_unref(bytes);
        
        const char *mime = "text/html";
        const char *ext = strrchr(path, '.');
        if (ext) {
            if (strcmp(ext, ".css") == 0) mime = "text/css";
            else if (strcmp(ext, ".js") == 0) mime = "application/javascript";
            else if (strcmp(ext, ".png") == 0) mime = "image/png";
            else if (strcmp(ext, ".ico") == 0) mime = "image/x-icon";
            else if (strcmp(ext, ".svg") == 0) mime = "image/svg+xml";
            else if (strcmp(ext, ".json") == 0) mime = "application/json";
            else if (strcmp(ext, ".woff2") == 0) mime = "font/woff2";
            else if (strcmp(ext, ".woff") == 0) mime = "font/woff";
            else if (strcmp(ext, ".ttf") == 0) mime = "font/ttf";
        }
        
        webkit_uri_scheme_request_finish(request, stream, size, mime);
        g_object_unref(stream);
    } else {
        GError *err = g_error_new(G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "Asset not found in RAM: %s", path);
        webkit_uri_scheme_request_finish_error(request, err);
        g_error_free(err);
    }
}

}
#endif


struct EvalData { webview_t w; char* js; };

static void eval_dispatch_cb(webview_t, void* arg) {
    EvalData* d = (EvalData*)arg;
    webview_eval(d->w, d->js);
    free(d->js);
    free(d);
}

extern "C" void ui_eval(void* w, const char* js) {
    EvalData* d = (EvalData*)malloc(sizeof(EvalData));
    d->w = (webview_t)w;
#ifdef _WIN32
    d->js = _strdup(js);
#else
    d->js = strdup(js);
#endif
    webview_dispatch((webview_t)w, eval_dispatch_cb, d);
}

#ifdef _WIN32
static const GUID IID_ICoreWebView2Settings3_local =
    { 0xFDB5AB74, 0xAF33, 0x4854, { 0x84, 0xF0, 0x0A, 0x63, 0x1D, 0xEB, 0x5E, 0xBA } };

extern "C" void ui_lockdown(void* w) {
    auto controller = (ICoreWebView2Controller*)webview_get_native_handle(
        (webview_t)w, WEBVIEW_NATIVE_HANDLE_KIND_BROWSER_CONTROLLER);
    if (!controller) return;
    ComPtr<ICoreWebView2> core;
    controller->get_CoreWebView2(&core);
    if (!core) return;
    ComPtr<ICoreWebView2Settings> settings;
    core->get_Settings(&settings);
    if (!settings) return;
    settings->put_AreDefaultContextMenusEnabled(FALSE);
    settings->put_AreDevToolsEnabled(FALSE);
    settings->put_IsStatusBarEnabled(FALSE);
    ICoreWebView2Settings3* s3 = nullptr;
    if (SUCCEEDED(settings->QueryInterface(
            IID_ICoreWebView2Settings3_local, (void**)&s3)) && s3) {
        s3->put_AreBrowserAcceleratorKeysEnabled(FALSE);
        s3->Release();
    }
}
#else
extern "C" void ui_lockdown(void* w) { (void)w; }
#endif

extern "C" {

void ui_terminate(void* w) { webview_terminate((webview_t)w); }

void ui_set_icon(void* w, const char* icon_path) {
    (void)icon_path;

    const uint8_t* data = NULL;
    size_t size = 0;
    
    if (assets_get_file("app/favicon.ico", &data, &size) && data && size > 0) {
#ifdef _WIN32
        HICON hIcon = (HICON)CreateIconFromResource((PBYTE)data, (DWORD)size, TRUE, 0x00030000);
        if (hIcon) {
            HWND hwnd = (HWND)webview_get_window((webview_t)w);
            if (hwnd) {
                SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
                SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            }
        }
#elif defined(__linux__) || defined(__BSD__)
        GtkWindow* win = GTK_WINDOW(webview_get_window((webview_t)w));
        if (win) {
            GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
            if (gdk_pixbuf_loader_write(loader, data, size, NULL)) {
                GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
                if (pixbuf) {
                    gtk_window_set_icon(win, pixbuf);
                }
            }
            gdk_pixbuf_loader_close(loader, NULL);
            g_object_unref(loader);
        }
#endif
        assets_free_buffer(data);
    }
}

void* ui_get_window(void* w) { return webview_get_window((webview_t)w); }

void ui_show_window(void* w) {
#ifdef _WIN32
    HWND hwnd = (HWND)webview_get_window((webview_t)w);
    if (!hwnd) return;
    ShowWindow(hwnd, SW_RESTORE);
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
#elif defined(__linux__)
    GtkWindow* win = GTK_WINDOW(webview_get_window((webview_t)w));
    if (!win) return;
    gtk_widget_show(GTK_WIDGET(win));
    gtk_window_deiconify(win);
    gtk_window_present(win);
#endif
}

void ui_hide_window(void* w) {
#ifdef _WIN32
    HWND hwnd = (HWND)webview_get_window((webview_t)w);
    if (hwnd) ShowWindow(hwnd, SW_HIDE);
#elif defined(__linux__)
    GtkWindow* win = GTK_WINDOW(webview_get_window((webview_t)w));
    if (win) gtk_widget_hide(GTK_WIDGET(win));
#endif
}

void ui_minimize_window(void* w) {
#ifdef _WIN32
    HWND hwnd = (HWND)webview_get_window((webview_t)w);
    if (hwnd) ShowWindow(hwnd, SW_MINIMIZE);
#elif defined(__linux__)
    GtkWindow* win = GTK_WINDOW(webview_get_window((webview_t)w));
    if (win) gtk_window_iconify(win);
#endif
}

void ui_set_frameless(void* w) {
#ifdef _WIN32
    HWND hwnd = (HWND)webview_get_window((webview_t)w);
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    style |= WS_POPUP;
    SetWindowLong(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
    int pref = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));
#elif defined(__linux__)
    GtkWindow* win = GTK_WINDOW(webview_get_window((webview_t)w));
    gtk_window_set_decorated(win, FALSE);
    g_signal_connect(GTK_WIDGET(win), "size-allocate",
                     G_CALLBACK(on_size_allocate), NULL);
    g_signal_connect(GTK_WIDGET(win), "delete-event",
                     G_CALLBACK(on_window_delete), NULL);
#endif
}

void ui_drag_start(void* w) {
#ifdef _WIN32
    HWND hwnd = (HWND)webview_get_window((webview_t)w);
    ReleaseCapture();
    SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
#elif defined(__linux__)
    GtkWindow* win = GTK_WINDOW(webview_get_window((webview_t)w));
    GdkDisplay* display = gdk_display_get_default();
    GdkSeat* seat = gdk_display_get_default_seat(display);
    GdkDevice* pointer = gdk_seat_get_pointer(seat);
    gint x, y;
    GdkScreen* screen;
    gdk_device_get_position(pointer, &screen, &x, &y);
    gtk_window_begin_move_drag(win, 1, x, y, GDK_CURRENT_TIME);
#endif
}

void* ui_create(const char* title) {
    webview_t w = webview_create(0, nullptr);
    webview_set_title(w, title);
    webview_set_size(w, 1100, 600, WEBVIEW_HINT_NONE);

#if defined(__linux__) || defined(__BSD__)
    WebKitWebContext *ctx = webkit_web_context_get_default();
    webkit_web_context_register_uri_scheme(ctx, "cero", ceroclient_uri_scheme_cb, NULL, NULL);
#endif

    return w;
}

void ui_navigate(void* w, const char* url) { webview_navigate((webview_t)w, url); }

void ui_run(void* w) {
    webview_run((webview_t)w);
    webview_destroy((webview_t)w);
}

void ui_bind(void* w, const char* name,
             void (*fn)(const char* id, const char* req, void* arg),
             void* arg) {
    webview_bind((webview_t)w, name, fn, arg);
}

void ui_return(void* w, const char* id, int status, const char* result) {
    webview_return((webview_t)w, id, status, result);
}

static void js_log_handler(const char* id, const char* req, void* arg) {
    webview_t w = (webview_t)arg;
    log_msg("js", "%s\n", req);
    fflush(stdout);
    webview_return(w, id, 0, "null");
}

void ui_enable_js_console(void* w) {
    webview_bind((webview_t)w, "__log", js_log_handler, w);
    const char* init =
        "(function(){"
        "  const orig = { log: console.log, warn: console.warn, error: console.error, info: console.info };"
        "  function send(level, args){"
        "    try {"
        "      const msg = Array.from(args).map(a =>"
        "        (typeof a === 'object') ? JSON.stringify(a) : String(a)"
        "      ).join(' ');"
        "      window.__log(level, msg);"
        "    } catch(e){}"
        "  }"
        "  console.log   = function(){ send('LOG',   arguments); orig.log.apply(console, arguments); };"
        "  console.info  = function(){ send('INFO',  arguments); orig.info.apply(console, arguments); };"
        "  console.warn  = function(){ send('WARN',  arguments); orig.warn.apply(console, arguments); };"
        "  console.error = function(){ send('ERROR', arguments); orig.error.apply(console, arguments); };"
        "  window.addEventListener('error', e => send('UNCAUGHT', [e.message + ' @ ' + e.filename + ':' + e.lineno]));"
        "  window.addEventListener('unhandledrejection', e => send('PROMISE', [String(e.reason)]));"
        "})();";
    webview_init((webview_t)w, init);

    webview_init((webview_t)w,
        "(function(){"
        "  const _mq = window.matchMedia.bind(window);"
        "  window.matchMedia = function(q){"
        "    const r = _mq(q);"
        "    if(q === '(prefers-color-scheme: dark)') return Object.assign(Object.create(r), { matches: true });"
        "    if(q === '(prefers-color-scheme: light)') return Object.assign(Object.create(r), { matches: false });"
        "    return r;"
        "  };"
        "})();"
    );
}

} // extern "C"
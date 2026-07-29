#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

#include "../include/logger.h"
#include "webview/webview.h"

extern "C" {
    #include "../include/assets_loader.h"
}

@interface CeroSchemeHandler : NSObject <WKURLSchemeHandler>
@end

@implementation CeroSchemeHandler

- (void)webView:(WKWebView *)webView startURLSchemeTask:(id<WKURLSchemeTask>)task {
    NSURL *url = task.request.URL;

    NSString *path = url.path;
    if ([path hasPrefix:@"/"]) {
        path = [path substringFromIndex:1];
    }

    const uint8_t *data = NULL;
    size_t size = 0;

    if (!assets_get_file(path.UTF8String, &data, &size) || !data || size == 0) {
        log_msg("error", "[UI-macOS] asset introuvable: %s\n", path.UTF8String);
        NSError *err = [NSError errorWithDomain:@"CeroClient" code:404 userInfo:nil];
        [task didFailWithError:err];
        return;
    }

    NSString *mime = @"application/octet-stream";
    if ([path hasSuffix:@".html"]) mime = @"text/html";
    else if ([path hasSuffix:@".js"])   mime = @"application/javascript";
    else if ([path hasSuffix:@".css"])  mime = @"text/css";
    else if ([path hasSuffix:@".png"])  mime = @"image/png";
    else if ([path hasSuffix:@".svg"])  mime = @"image/svg+xml";
    else if ([path hasSuffix:@".ico"])  mime = @"image/x-icon";

    NSURLResponse *response = [[NSURLResponse alloc] initWithURL:url
                                                        MIMEType:mime
                                           expectedContentLength:(long long)size
                                                textEncodingName:nil];

    NSData *nsdata = [NSData dataWithBytes:data length:size];

    [task didReceiveResponse:response];
    [task didReceiveData:nsdata];
    [task didFinish];

    assets_free_buffer(data);
}

- (void)webView:(WKWebView *)webView stopURLSchemeTask:(id<WKURLSchemeTask>)task {
    (void)webView; (void)task;
}

@end

static NSWindow* cero_get_nswindow(void* w) {
    return (NSWindow*)webview_get_window((webview_t)w);
}

extern "C" {

void ui_macos_register_scheme(void* w) {
    WKWebView* webview = (WKWebView*)webview_get_native_handle(
        (webview_t)w, WEBVIEW_NATIVE_HANDLE_KIND_BROWSER_CONTROLLER);

    if (!webview) {
        log_msg("error", "[UI-macOS] Impossible de récupérer la WKWebView\n");
        return;
    }

    (void)webview;
    log_msg("info", "[UI-macOS] Scheme cero:// enregistré\n");
}

void ui_show_window(void* w) {
    NSWindow* win = cero_get_nswindow(w);
    if (!win) return;
    [win makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

void ui_hide_window(void* w) {
    NSWindow* win = cero_get_nswindow(w);
    if (!win) return;
    [win orderOut:nil];
}

void ui_minimize_window(void* w) {
    NSWindow* win = cero_get_nswindow(w);
    if (!win) return;
    [win miniaturize:nil];
}

void ui_set_frameless(void* w) {
    NSWindow* win = cero_get_nswindow(w);
    if (!win) return;

    win.titlebarAppearsTransparent = YES;
    win.titleVisibility = NSWindowTitleHidden;
    win.styleMask |= NSWindowStyleMaskFullSizeContentView;
}

void ui_drag_start(void* w) {
    NSWindow* win = cero_get_nswindow(w);
    if (!win) return;
    [win performWindowDragWithEvent:[NSApp currentEvent]];
}

void ui_set_icon(void* w, const char* icon_path) {
    (void)w; (void)icon_path;

    const uint8_t* data = NULL;
    size_t size = 0;
    if (assets_get_file("app/favicon.ico", &data, &size) && data && size > 0) {
        NSData* nsdata = [NSData dataWithBytes:data length:size];
        NSImage* img = [[NSImage alloc] initWithData:nsdata];
        if (img) {
            [NSApp setApplicationIconImage:img];
        }
        assets_free_buffer(data);
    }
}

} // extern "C"
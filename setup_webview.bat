@echo off
powershell -NoProfile -Command "Invoke-WebRequest -Uri 'https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2' -OutFile webview2.zip"
powershell -NoProfile -Command "Expand-Archive -Path webview2.zip -DestinationPath webview2_sdk -Force"
echo WebView2 SDK ready.
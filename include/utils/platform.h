#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
    #define NATIVES_KEY "natives-windows"
#elif __APPLE__
    #define NATIVES_KEY "natives-osx"
#else
    #define NATIVES_KEY "natives-linux"
#endif

#endif

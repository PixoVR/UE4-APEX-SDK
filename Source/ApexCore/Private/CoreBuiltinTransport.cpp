// Copyright 2022-2026 PixoVR Corp. All Rights Reserved.

// Compiles the core's HTTP backend for the target platform; ApexCore.Build.cs defines which.
#if defined(APEX_TRANSPORT_WINHTTP)
#include "src/transport/WinHttpBackend.cpp"
#elif defined(APEX_TRANSPORT_ANDROID)
#include "src/transport/AndroidBackend.cpp"
#else
#include "src/transport/NoBuiltinTransport.cpp"
#endif

#include "pch.h"
#include <Windows.h>
#include <wininet.h>

#pragma comment(lib, "WinINet.lib")

__declspec(dllexport) LPVOID _hVirtualAlloc(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD flAllocationType,
    DWORD flProtect
) { 
    MessageBoxA(NULL, "VirtualAlloc intercepted ! ", "VirtualAlloc intercepted ! ", MB_ICONINFORMATION);

    return VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
}

__declspec(dllexport) HANDLE _hGetCurrentProcess() {
    MessageBoxA(NULL, "GetCurrentProcess intercepted ! ", "GetCurrentProcess intercepted ! ", MB_ICONINFORMATION);

    return GetCurrentProcess();
}

__declspec(dllexport) BOOL _hReadProcessMemory(
    HANDLE hProcess,
    LPCVOID lpBaseAddress,
    LPVOID lpBuffer,
    SIZE_T nSize,
    SIZE_T* lpNumberOfBytesRead
) {
    MessageBoxA(NULL, "ReadProcessMemory intercepted ! ", "ReadProcessMemory intercepted ! ", MB_ICONINFORMATION);

    return ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
}

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

# Title add/reply
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

std::wstring windowName;
	std::getline(std::wcin, windowName);

	HWND windowHandle = FindWindowW(NULL, windowName.c_str());
	DWORD* processID = new DWORD;
	GetWindowThreadProcessId(windowHandle, processID);

	std::wcout << L"Process ID of " << windowName.c_str() << L" is: " << *processID << std::endl;

	system("PAUSE");

bool llama_adapter_cvec::apply(
        const llama_model & model,
        const float * data,
        size_t len,
        int32_t n_embd,
        int32_t il_start,
        int32_t il_end) {
    const auto & hparams = model.hparams;

    if (data == nullptr) {
        // disable the current control vector (but leave allocated for later)
        layer_start = -1;
        layer_end   = -1;
        return true;
    }

    if (n_embd != (int) hparams.n_embd) {
        LLAMA_LOG_ERROR("%s: control vector n_embd does not match model\n", __func__);
        return false;
    }

    if (tensors.empty()) {
        if (!init(model)) {
            return false;
        }
    }

    layer_start = il_start;
    layer_end   = il_end;

    for (size_t il = 1; il < hparams.n_layer(); il++) {
        assert(tensors[il] != nullptr);

        const size_t off = n_embd * (il - 1); // buffer doesn't have data for layer 0, since it's never present
        if (off + n_embd <= len) {
            ggml_backend_tensor_set(tensors[il], data + off, 0, n_embd * ggml_element_size(tensors[il]));
        }
    }

    return true;
}

#pragma once
#include <windows.h>
#include <dwmapi.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <d3d11.h>
#include <cstdio>
#include <Auth/skStr.h>
#include <Infrastructure/Logger.h>
#include <Psapi.h>
#include <Infrastructure/EatParser.h>

namespace Api {

    struct DynamicApi {
        HMODULE Module;
        FARPROC Address;
        const char* Name;
        bool Resolved;

        DynamicApi(HMODULE mod, const char* name) : Module(mod), Address(nullptr), Name(name), Resolved(false) {}

        template <typename T>
        T Get() {
            if (!Resolved) {
                Address = reinterpret_cast<FARPROC>(EatParser::GetProcAddressEAT(Module, Name));
                if (!Address) {
                    Address = ::GetProcAddress(Module, Name);
                }
                Resolved = true;
                if (!Address) {
                    char buf[256];
                    snprintf(buf, sizeof(buf), skCrypt("[Api] FAILED: %s in module 0x%llX"), Name, (unsigned long long)Module);
                    Logger::Log(buf);
                    Logger::Flush();
                }
            }
            return reinterpret_cast<T>(Address);
        }
    };

    inline HMODULE GetKernel32() {
        static HMODULE mod = reinterpret_cast<HMODULE>(EatParser::FindModuleInPEB(skCrypt(L"kernel32.dll")));
        return mod;
    }

    inline HMODULE GetNtdll() {
        static HMODULE mod = reinterpret_cast<HMODULE>(EatParser::FindModuleInPEB(skCrypt(L"ntdll.dll")));
        return mod;
    }

    inline HMODULE GetUser32() {
        static HMODULE mod = reinterpret_cast<HMODULE>(EatParser::FindModuleInPEB(skCrypt(L"user32.dll")));
        if (!mod) {
            ::LoadLibraryA(skCrypt("user32.dll"));
            mod = reinterpret_cast<HMODULE>(EatParser::FindModuleInPEB(skCrypt(L"user32.dll")));
        }
        return mod;
    }

    inline HMODULE GetShcore() {
        static HMODULE mod = reinterpret_cast<HMODULE>(EatParser::FindModuleInPEB(skCrypt(L"shcore.dll")));
        if (!mod) {
            ::LoadLibraryA(skCrypt("shcore.dll"));
            mod = reinterpret_cast<HMODULE>(EatParser::FindModuleInPEB(skCrypt(L"shcore.dll")));
        }
        return mod;
    }

    inline HMODULE GetDwmapi() {
        static HMODULE mod = reinterpret_cast<HMODULE>(EatParser::FindModuleInPEB(skCrypt(L"dwmapi.dll")));
        if (!mod) {
            ::LoadLibraryA(skCrypt("dwmapi.dll"));
            mod = reinterpret_cast<HMODULE>(EatParser::FindModuleInPEB(skCrypt(L"dwmapi.dll")));
        }
        return mod;
    }

    inline HMODULE GetWinmm() {
        static HMODULE mod = reinterpret_cast<HMODULE>(EatParser::FindModuleInPEB(skCrypt(L"winmm.dll")));
        if (!mod) {
            ::LoadLibraryA(skCrypt("winmm.dll"));
            mod = reinterpret_cast<HMODULE>(EatParser::FindModuleInPEB(skCrypt(L"winmm.dll")));
        }
        return mod;
    }

    inline HMODULE GetD3D11() {
        static HMODULE mod = reinterpret_cast<HMODULE>(EatParser::FindModuleInPEB(skCrypt(L"d3d11.dll")));
        if (!mod) {
            ::LoadLibraryA(skCrypt("d3d11.dll"));
            mod = reinterpret_cast<HMODULE>(EatParser::FindModuleInPEB(skCrypt(L"d3d11.dll")));
        }
        return mod;
    }

#define DEFINE_DYNAMIC_API_MOD0(mod, ret, name) \
    inline ret name() { \
        static auto fn = DynamicApi(mod, skCrypt(#name)).Get<ret(WINAPI*)()>(); \
        return fn(); \
    }

#define DEFINE_DYNAMIC_API_MOD1(mod, ret, name, t1, n1) \
    inline ret name(t1 n1) { \
        static auto fn = DynamicApi(mod, skCrypt(#name)).Get<ret(WINAPI*)(t1)>(); \
        return fn(n1); \
    }

#define DEFINE_DYNAMIC_API_MOD2(mod, ret, name, t1, n1, t2, n2) \
    inline ret name(t1 n1, t2 n2) { \
        static auto fn = DynamicApi(mod, skCrypt(#name)).Get<ret(WINAPI*)(t1, t2)>(); \
        return fn(n1, n2); \
    }

#define DEFINE_DYNAMIC_API_MOD3(mod, ret, name, t1, n1, t2, n2, t3, n3) \
    inline ret name(t1 n1, t2 n2, t3 n3) { \
        static auto fn = DynamicApi(mod, skCrypt(#name)).Get<ret(WINAPI*)(t1, t2, t3)>(); \
        return fn(n1, n2, n3); \
    }

#define DEFINE_DYNAMIC_API_MOD4(mod, ret, name, t1, n1, t2, n2, t3, n3, t4, n4) \
    inline ret name(t1 n1, t2 n2, t3 n3, t4 n4) { \
        static auto fn = DynamicApi(mod, skCrypt(#name)).Get<ret(WINAPI*)(t1, t2, t3, t4)>(); \
        return fn(n1, n2, n3, n4); \
    }

#define DEFINE_DYNAMIC_API_MOD5(mod, ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5) \
    inline ret name(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5) { \
        static auto fn = DynamicApi(mod, skCrypt(#name)).Get<ret(WINAPI*)(t1, t2, t3, t4, t5)>(); \
        return fn(n1, n2, n3, n4, n5); \
    }

#define DEFINE_DYNAMIC_API_MOD6(mod, ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6) \
    inline ret name(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6) { \
        static auto fn = DynamicApi(mod, skCrypt(#name)).Get<ret(WINAPI*)(t1, t2, t3, t4, t5, t6)>(); \
        return fn(n1, n2, n3, n4, n5, n6); \
    }

#define DEFINE_DYNAMIC_API_MOD7(mod, ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7) \
    inline ret name(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7) { \
        static auto fn = DynamicApi(mod, skCrypt(#name)).Get<ret(WINAPI*)(t1, t2, t3, t4, t5, t6, t7)>(); \
        return fn(n1, n2, n3, n4, n5, n6, n7); \
    }

#define DEFINE_DYNAMIC_API_MOD8(mod, ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8) \
    inline ret name(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8) { \
        static auto fn = DynamicApi(mod, skCrypt(#name)).Get<ret(WINAPI*)(t1, t2, t3, t4, t5, t6, t7, t8)>(); \
        return fn(n1, n2, n3, n4, n5, n6, n7, n8); \
    }

#define DEFINE_DYNAMIC_API_MOD9(mod, ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9) \
    inline ret name(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9) { \
        static auto fn = DynamicApi(mod, skCrypt(#name)).Get<ret(WINAPI*)(t1, t2, t3, t4, t5, t6, t7, t8, t9)>(); \
        return fn(n1, n2, n3, n4, n5, n6, n7, n8, n9); \
    }

#define DEFINE_DYNAMIC_API_MOD10(mod, ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10) \
    inline ret name(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10) { \
        static auto fn = DynamicApi(mod, skCrypt(#name)).Get<ret(WINAPI*)(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10)>(); \
        return fn(n1, n2, n3, n4, n5, n6, n7, n8, n9, n10); \
    }

#define DEFINE_DYNAMIC_API_MOD11(mod, ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11) \
    inline ret name(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10, t11 n11) { \
        static auto fn = DynamicApi(mod, skCrypt(#name)).Get<ret(WINAPI*)(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11)>(); \
        return fn(n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11); \
    }

#define DEFINE_DYNAMIC_API_MOD12(mod, ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12) \
    inline ret name(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7, t8 n8, t9 n9, t10 n10, t11 n11, t12 n12) { \
        static auto fn = DynamicApi(mod, skCrypt(#name)).Get<ret(WINAPI*)(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12)>(); \
        return fn(n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12); \
    }

// Kernel32 helpers
#define DEFINE_DYNAMIC_API0(ret, name) DEFINE_DYNAMIC_API_MOD0(Api::GetKernel32(), ret, name)
#define DEFINE_DYNAMIC_API1(ret, name, t1, n1) DEFINE_DYNAMIC_API_MOD1(Api::GetKernel32(), ret, name, t1, n1)
#define DEFINE_DYNAMIC_API2(ret, name, t1, n1, t2, n2) DEFINE_DYNAMIC_API_MOD2(Api::GetKernel32(), ret, name, t1, n1, t2, n2)
#define DEFINE_DYNAMIC_API3(ret, name, t1, n1, t2, n2, t3, n3) DEFINE_DYNAMIC_API_MOD3(Api::GetKernel32(), ret, name, t1, n1, t2, n2, t3, n3)
#define DEFINE_DYNAMIC_API4(ret, name, t1, n1, t2, n2, t3, n3, t4, n4) DEFINE_DYNAMIC_API_MOD4(Api::GetKernel32(), ret, name, t1, n1, t2, n2, t3, n3, t4, n4)
#define DEFINE_DYNAMIC_API5(ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5) DEFINE_DYNAMIC_API_MOD5(Api::GetKernel32(), ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5)
#define DEFINE_DYNAMIC_API6(ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6) DEFINE_DYNAMIC_API_MOD6(Api::GetKernel32(), ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6)
#define DEFINE_DYNAMIC_API7(ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7) DEFINE_DYNAMIC_API_MOD7(Api::GetKernel32(), ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7)
#define DEFINE_DYNAMIC_API8(ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8) DEFINE_DYNAMIC_API_MOD8(Api::GetKernel32(), ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8)
#define DEFINE_DYNAMIC_API9(ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9) DEFINE_DYNAMIC_API_MOD9(Api::GetKernel32(), ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9)
#define DEFINE_DYNAMIC_API10(ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10) DEFINE_DYNAMIC_API_MOD10(Api::GetKernel32(), ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10)
#define DEFINE_DYNAMIC_API11(ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11) DEFINE_DYNAMIC_API_MOD11(Api::GetKernel32(), ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11)
#define DEFINE_DYNAMIC_API12(ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12) DEFINE_DYNAMIC_API_MOD12(Api::GetKernel32(), ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12)

// NT variants
#define DEFINE_DYNAMIC_API_NT0(ret, name) \
    inline ret name() { \
        static auto fn = DynamicApi(Api::GetNtdll(), skCrypt(#name)).Get<ret(NTAPI*)()>(); \
        return fn(); \
    }

#define DEFINE_DYNAMIC_API_NT1(ret, name, t1, n1) \
    inline ret name(t1 n1) { \
        static auto fn = DynamicApi(Api::GetNtdll(), skCrypt(#name)).Get<ret(NTAPI*)(t1)>(); \
        return fn(n1); \
    }

#define DEFINE_DYNAMIC_API_NT2(ret, name, t1, n1, t2, n2) \
    inline ret name(t1 n1, t2 n2) { \
        static auto fn = DynamicApi(Api::GetNtdll(), skCrypt(#name)).Get<ret(NTAPI*)(t1, t2)>(); \
        return fn(n1, n2); \
    }

#define DEFINE_DYNAMIC_API_NT3(ret, name, t1, n1, t2, n2, t3, n3) \
    inline ret name(t1 n1, t2 n2, t3 n3) { \
        static auto fn = DynamicApi(Api::GetNtdll(), skCrypt(#name)).Get<ret(NTAPI*)(t1, t2, t3)>(); \
        return fn(n1, n2, n3); \
    }

#define DEFINE_DYNAMIC_API_NT4(ret, name, t1, n1, t2, n2, t3, n3, t4, n4) \
    inline ret name(t1 n1, t2 n2, t3 n3, t4 n4) { \
        static auto fn = DynamicApi(Api::GetNtdll(), skCrypt(#name)).Get<ret(NTAPI*)(t1, t2, t3, t4)>(); \
        return fn(n1, n2, n3, n4); \
    }

#define DEFINE_DYNAMIC_API_NT5(ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5) \
    inline ret name(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5) { \
        static auto fn = DynamicApi(Api::GetNtdll(), skCrypt(#name)).Get<ret(NTAPI*)(t1, t2, t3, t4, t5)>(); \
        return fn(n1, n2, n3, n4, n5); \
    }

#define DEFINE_DYNAMIC_API_NT6(ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6) \
    inline ret name(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6) { \
        static auto fn = DynamicApi(Api::GetNtdll(), skCrypt(#name)).Get<ret(NTAPI*)(t1, t2, t3, t4, t5, t6)>(); \
        return fn(n1, n2, n3, n4, n5, n6); \
    }

// User32 variants
#define DEFINE_DYNAMIC_API_USER0(ret, name) DEFINE_DYNAMIC_API_MOD0(Api::GetUser32(), ret, name)
#define DEFINE_DYNAMIC_API_USER1(ret, name, t1, n1) DEFINE_DYNAMIC_API_MOD1(Api::GetUser32(), ret, name, t1, n1)
#define DEFINE_DYNAMIC_API_USER2(ret, name, t1, n1, t2, n2) DEFINE_DYNAMIC_API_MOD2(Api::GetUser32(), ret, name, t1, n1, t2, n2)
#define DEFINE_DYNAMIC_API_USER3(ret, name, t1, n1, t2, n2, t3, n3) DEFINE_DYNAMIC_API_MOD3(Api::GetUser32(), ret, name, t1, n1, t2, n2, t3, n3)
#define DEFINE_DYNAMIC_API_USER4(ret, name, t1, n1, t2, n2, t3, n3, t4, n4) DEFINE_DYNAMIC_API_MOD4(Api::GetUser32(), ret, name, t1, n1, t2, n2, t3, n3, t4, n4)
#define DEFINE_DYNAMIC_API_USER5(ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5) DEFINE_DYNAMIC_API_MOD5(Api::GetUser32(), ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5)
#define DEFINE_DYNAMIC_API_USER7(ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7) DEFINE_DYNAMIC_API_MOD7(Api::GetUser32(), ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7)
#define DEFINE_DYNAMIC_API_USER12(ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12) DEFINE_DYNAMIC_API_MOD12(Api::GetUser32(), ret, name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12)

    // Kernel32 wrappers

    DEFINE_DYNAMIC_API2(HANDLE, CreateToolhelp32Snapshot, DWORD, dwFlags, DWORD, dwPID)
    DEFINE_DYNAMIC_API2(BOOL, Process32First, HANDLE, hSnapshot, LPPROCESSENTRY32, lppe)
    DEFINE_DYNAMIC_API2(BOOL, Process32Next, HANDLE, hSnapshot, LPPROCESSENTRY32, lppe)
    DEFINE_DYNAMIC_API2(BOOL, Module32First, HANDLE, hSnapshot, LPMODULEENTRY32, lpme)
    DEFINE_DYNAMIC_API2(BOOL, Module32Next, HANDLE, hSnapshot, LPMODULEENTRY32, lpme)
    DEFINE_DYNAMIC_API1(BOOL, CloseHandle, HANDLE, hObject)
    DEFINE_DYNAMIC_API3(HANDLE, OpenProcess, DWORD, dwDesiredAccess, BOOL, bInheritHandle, DWORD, dwProcessId)
    DEFINE_DYNAMIC_API1(DWORD, GetProcessId, HANDLE, hProcess)
    DEFINE_DYNAMIC_API5(LPVOID, VirtualAllocEx, HANDLE, hProcess, LPVOID, lpAddress, SIZE_T, dwSize, DWORD, flAllocationType, DWORD, flProtect)
    DEFINE_DYNAMIC_API5(BOOL, VirtualProtectEx, HANDLE, hProcess, LPVOID, lpAddress, SIZE_T, dwSize, DWORD, flNewProtect, PDWORD, lpflOldProtect)
    DEFINE_DYNAMIC_API5(BOOL, ReadProcessMemory, HANDLE, hProcess, LPCVOID, lpBaseAddress, LPVOID, lpBuffer, SIZE_T, nSize, SIZE_T*, lpNumberOfBytesRead)
    DEFINE_DYNAMIC_API5(BOOL, WriteProcessMemory, HANDLE, hProcess, LPVOID, lpBaseAddress, LPCVOID, lpBuffer, SIZE_T, nSize, SIZE_T*, lpNumberOfBytesWritten)
    
    // User32 wrappers
    DEFINE_DYNAMIC_API_USER1(BOOL, GetCursorPos, LPPOINT, lpPoint)
    DEFINE_DYNAMIC_API_USER2(BOOL, ScreenToClient, HWND, hWnd, LPPOINT, lpPoint)
    DEFINE_DYNAMIC_API_USER2(BOOL, ClientToScreen, HWND, hWnd, LPPOINT, lpPoint)
    DEFINE_DYNAMIC_API_USER2(BOOL, SetWindowDisplayAffinity, HWND, hWnd, DWORD, dwAffinity)
    DEFINE_DYNAMIC_API_USER2(HWND, FindWindowA, LPCSTR, lpClassName, LPCSTR, lpWindowName)
    DEFINE_DYNAMIC_API_USER2(BOOL, GetClientRect, HWND, hWnd, LPRECT, lpRect)
    DEFINE_DYNAMIC_API_USER2(BOOL, GetWindowRect, HWND, hWnd, LPRECT, lpRect)
    DEFINE_DYNAMIC_API_USER7(BOOL, SetWindowPos, HWND, hWnd, HWND, hWndInsertAfter, int, X, int, Y, int, cx, int, cy, UINT, uFlags)
    DEFINE_DYNAMIC_API_USER1(HDC, GetDC, HWND, hWnd)
    DEFINE_DYNAMIC_API_USER2(int, ReleaseDC, HWND, hWnd, HDC, hDC)
    DEFINE_DYNAMIC_API_USER1(SHORT, GetAsyncKeyState, int, vKey)
    DEFINE_DYNAMIC_API_USER3(UINT, SendInput, UINT, cInputs, LPINPUT, pInputs, int, cbSize)

    // Kernel32 wrappers contt
    DEFINE_DYNAMIC_API0(DWORD, GetTickCount)
    DEFINE_DYNAMIC_API4(BOOL, EnumProcessModules, HANDLE, hProcess, HMODULE*, lphModule, DWORD, cb, LPDWORD, lpcbNeeded)
    DEFINE_DYNAMIC_API4(DWORD, GetModuleFileNameExW, HANDLE, hProcess, HMODULE, hModule, LPWSTR, lpFilename, DWORD, nSize)
    DEFINE_DYNAMIC_API0(BOOL, IsDebuggerPresent)
    DEFINE_DYNAMIC_API2(BOOL, CheckRemoteDebuggerPresent, HANDLE, hProcess, PBOOL, pbDebuggerPresent)
    DEFINE_DYNAMIC_API1(void, ExitProcess, UINT, uExitCode)
    
    // User32 wrappers cont.
    DEFINE_DYNAMIC_API_USER1(int, GetSystemMetrics, int, nIndex)
    
    // Dwmapi wrappers 
    DEFINE_DYNAMIC_API_MOD2(Api::GetDwmapi(), BOOL, DwmExtendFrameIntoClientArea, HWND, hWnd, const MARGINS*, pMargins)
    
    // Kernel32 wrappers cont. 
    DEFINE_DYNAMIC_API1(HANDLE, GetModuleHandleA, LPCSTR, lpModuleName)
    DEFINE_DYNAMIC_API1(BOOL, FreeLibrary, HMODULE, hLibModule)
    DEFINE_DYNAMIC_API4(BOOL, VirtualProtect, LPVOID, lpAddress, SIZE_T, dwSize, DWORD, flNewProtect, PDWORD, lpflOldProtect)
    DEFINE_DYNAMIC_API6(HANDLE, CreateThread, LPSECURITY_ATTRIBUTES, lpThreadAttributes, SIZE_T, dwStackSize, LPTHREAD_START_ROUTINE, lpStartAddress, LPVOID, lpParameter, DWORD, dwCreationFlags, LPDWORD, lpThreadId)
    DEFINE_DYNAMIC_API2(DWORD, WaitForSingleObject, HANDLE, hHandle, DWORD, dwMilliseconds)
    DEFINE_DYNAMIC_API1(HMODULE, LoadLibraryA, LPCSTR, lpLibFileName)
    DEFINE_DYNAMIC_API2(FARPROC, GetProcAddress, HMODULE, hModule, LPCSTR, lpProcName)
    DEFINE_DYNAMIC_API2(BOOL, TerminateProcess, HANDLE, hProcess, UINT, uExitCode)
    DEFINE_DYNAMIC_API3(HANDLE, OpenThread, DWORD, dwDesiredAccess, BOOL, bInheritHandle, DWORD, dwThreadId)
    DEFINE_DYNAMIC_API2(BOOL, TerminateThread, HANDLE, hThread, DWORD, dwExitCode)
    DEFINE_DYNAMIC_API1(DWORD, SuspendThread, HANDLE, hThread)
    DEFINE_DYNAMIC_API1(DWORD, ResumeThread, HANDLE, hThread)
    
    // User32 wrappers cont.
    DEFINE_DYNAMIC_API_USER4(int, MessageBoxA, HWND, hWnd, LPCSTR, lpText, LPCSTR, lpCaption, UINT, uType)
    DEFINE_DYNAMIC_API_USER1(void, PostQuitMessage, int, nExitCode)
    DEFINE_DYNAMIC_API_USER3(LONG, SetWindowLongA, HWND, hWnd, int, nIndex, LONG, dwNewLong)
    DEFINE_DYNAMIC_API_USER3(LONG_PTR, SetWindowLongPtrA, HWND, hWnd, int, nIndex, LONG_PTR, dwNewLong)
    DEFINE_DYNAMIC_API_USER2(LONG, GetWindowLongA, HWND, hWnd, int, nIndex)
    DEFINE_DYNAMIC_API_USER12(HWND, CreateWindowExA, DWORD, dwExStyle, LPCSTR, lpClassName, LPCSTR, lpWindowName, DWORD, dwStyle, int, X, int, Y, int, nWidth, int, nHeight, HWND, hWndParent, HMENU, hMenu, HINSTANCE, hInstance, LPVOID, lpParam)
    DEFINE_DYNAMIC_API_USER1(BOOL, DestroyWindow, HWND, hWnd)
    DEFINE_DYNAMIC_API_USER1(ATOM, RegisterClassExA, const WNDCLASSEXA*, lpWndClass)
    DEFINE_DYNAMIC_API_USER2(BOOL, UnregisterClassA, LPCSTR, lpClassName, HINSTANCE, hInstance)
    DEFINE_DYNAMIC_API_USER2(BOOL, ShowWindow, HWND, hWnd, int, nCmdShow)
    DEFINE_DYNAMIC_API_USER1(BOOL, UpdateWindow, HWND, hWnd)
    DEFINE_DYNAMIC_API_USER4(BOOL, SetLayeredWindowAttributes, HWND, hWnd, COLORREF, crKey, BYTE, bAlpha, DWORD, dwFlags)
    DEFINE_DYNAMIC_API_USER1(BOOL, IsWindow, HWND, hWnd)
    DEFINE_DYNAMIC_API_USER4(HHOOK, SetWindowsHookExA, int, idHook, HOOKPROC, lpfn, HINSTANCE, hmod, DWORD, dwThreadId)
    DEFINE_DYNAMIC_API_USER4(LRESULT, CallNextHookEx, HHOOK, hhk, int, nCode, WPARAM, wParam, LPARAM, lParam)
    DEFINE_DYNAMIC_API_USER1(BOOL, UnhookWindowsHookEx, HHOOK, hhk)
    
    // Winmm wrappers 
    DEFINE_DYNAMIC_API_MOD1(Api::GetWinmm(), MMRESULT, timeBeginPeriod, UINT, uPeriod)
    DEFINE_DYNAMIC_API_MOD1(Api::GetWinmm(), MMRESULT, timeEndPeriod, UINT, uPeriod)

    // D3D11 wrappers
    DEFINE_DYNAMIC_API_MOD12(Api::GetD3D11(), HRESULT, D3D11CreateDeviceAndSwapChain,
        IDXGIAdapter*, pAdapter, D3D_DRIVER_TYPE, DriverType, HMODULE, Software, UINT, Flags,
        const D3D_FEATURE_LEVEL*, pFeatureLevels, UINT, FeatureLevels, UINT, SDKVersion,
        const DXGI_SWAP_CHAIN_DESC*, pSwapChainDesc, IDXGISwapChain**, ppSwapChain,
        ID3D11Device**, ppDevice, D3D_FEATURE_LEVEL*, pFeatureLevel, ID3D11DeviceContext**, ppImmediateContext)

    // Kernel32 wrappers cont. 
    DEFINE_DYNAMIC_API0(HANDLE, GetCurrentProcess)
    DEFINE_DYNAMIC_API0(DWORD, GetCurrentProcessId)
    DEFINE_DYNAMIC_API0(DWORD, GetCurrentThreadId)
    DEFINE_DYNAMIC_API0(HANDLE, GetCurrentThread)
    DEFINE_DYNAMIC_API7(BOOL, DuplicateHandle, HANDLE, hSourceProcessHandle, HANDLE, hSourceHandle, HANDLE, hTargetProcessHandle, LPHANDLE, lpTargetHandle, DWORD, dwDesiredAccess, BOOL, bInheritHandle, DWORD, dwOptions)
    DEFINE_DYNAMIC_API0(DWORD, GetLastError)
    DEFINE_DYNAMIC_API1(void, Sleep, DWORD, dwMilliseconds)
    
    // Kernel32 wrappers cont. for AntiInjection
    DEFINE_DYNAMIC_API1(void, GetSystemInfo, LPSYSTEM_INFO, lpSystemInfo)
    DEFINE_DYNAMIC_API3(SIZE_T, VirtualQuery, LPCVOID, lpAddress, PMEMORY_BASIC_INFORMATION, lpBuffer, SIZE_T, dwLength)
    DEFINE_DYNAMIC_API2(BOOL, Thread32First, HANDLE, hSnapshot, LPTHREADENTRY32, lpte)
    DEFINE_DYNAMIC_API2(BOOL, Thread32Next, HANDLE, hSnapshot, LPTHREADENTRY32, lpte)
    DEFINE_DYNAMIC_API4(BOOL, GetModuleInformation, HANDLE, hProcess, HMODULE, hModule, LPMODULEINFO, lpmodinfo, DWORD, cb)
    DEFINE_DYNAMIC_API1(HMODULE, GetModuleHandleW, LPCWSTR, lpModuleName)
    DEFINE_DYNAMIC_API7(HANDLE, CreateFileW, LPCWSTR, lpFileName, DWORD, dwDesiredAccess, DWORD, dwShareMode, LPSECURITY_ATTRIBUTES, lpSecurityAttributes, DWORD, dwCreationDisposition, DWORD, dwFlagsAndAttributes, HANDLE, hTemplateFile)
    DEFINE_DYNAMIC_API6(HANDLE, CreateFileMappingW, HANDLE, hFile, LPSECURITY_ATTRIBUTES, lpFileMappingAttributes, DWORD, flProtect, DWORD, dwMaximumSizeHigh, DWORD, dwMaximumSizeLow, LPCWSTR, lpName)
    DEFINE_DYNAMIC_API5(LPVOID, MapViewOfFile, HANDLE, hFileMappingObject, DWORD, dwDesiredAccess, DWORD, dwFileOffsetHigh, DWORD, dwFileOffsetLow, SIZE_T, dwNumberOfBytesToMap)
    DEFINE_DYNAMIC_API1(BOOL, UnmapViewOfFile, LPCVOID, lpBaseAddress)

    // NTDLL wrappers
    DEFINE_DYNAMIC_API_NT5(NTSTATUS, NtQueryInformationProcess, HANDLE, ProcessHandle, PROCESSINFOCLASS, ProcessInformationClass, PVOID, ProcessInformation, ULONG, ProcessInformationLength, PULONG, ReturnLength)
    DEFINE_DYNAMIC_API_NT4(NTSTATUS, NtSetInformationThread, HANDLE, ThreadHandle, THREADINFOCLASS, ThreadInformationClass, PVOID, ThreadInformation, ULONG, ThreadInformationLength)
    DEFINE_DYNAMIC_API_NT5(NTSTATUS, NtQueryInformationThread, HANDLE, ThreadHandle, THREADINFOCLASS, ThreadInformationClass, PVOID, ThreadInformation, ULONG, ThreadInformationLength, PULONG, ReturnLength)
    // User32 wrappers 
    DEFINE_DYNAMIC_API_USER5(LRESULT, CallWindowProcA, WNDPROC, lpPrevWndFunc, HWND, hWnd, UINT, Msg, WPARAM, wParam, LPARAM, lParam)
    DEFINE_DYNAMIC_API_USER4(BOOL, GetMessageA, LPMSG, lpMsg, HWND, hWnd, UINT, wMsgFilterMin, UINT, wMsgFilterMax)

    inline BOOL PeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
        return ::PeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
    }

    inline LRESULT DispatchMessage(const MSG* lpMsg) {
        return ::DispatchMessageA(lpMsg);
    }

    inline BOOL TranslateMessage(const MSG* lpMsg) {
        return ::TranslateMessage(lpMsg);
    }
}

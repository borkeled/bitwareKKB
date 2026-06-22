#include "BackendSelector.h"
#include <Windows.h>
#include <cstdio>

static BackendSelector::Mode g_SelectedMode = BackendSelector::Usermode;

static LRESULT CALLBACK BackendWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_COMMAND)
    {
        if (LOWORD(wParam) == 2)
            g_SelectedMode = BackendSelector::KernelMode;
        else
            g_SelectedMode = BackendSelector::Usermode;
        DestroyWindow(hwnd);
        return 0;
    }

    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    if (msg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        HBRUSH bgBrush = CreateSolidBrush(RGB(25, 25, 30));
        FillRect(hdc, &rc, bgBrush);
        DeleteObject(bgBrush);

        SetBkMode(hdc, TRANSPARENT);

        RECT titleRect = rc;
        titleRect.top += 16;
        titleRect.bottom = titleRect.top + 30;
        HFONT titleFont = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        SelectObject(hdc, titleFont);
        SetTextColor(hdc, RGB(220, 220, 220));
        DrawTextW(hdc, L"Select Memory Backend", -1, &titleRect, DT_CENTER | DT_SINGLELINE);

        RECT subRect = rc;
        subRect.top += 48;
        subRect.bottom = subRect.top + 24;
        SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
        SetTextColor(hdc, RGB(150, 150, 150));
        DrawTextW(hdc, L"Choose the memory access method for this session", -1, &subRect, DT_CENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

BackendSelector::Mode BackendSelector::Show()
{
    g_SelectedMode = Usermode;

    HINSTANCE hInstance = GetModuleHandleW(NULL);

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = BackendWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(25, 25, 30));
    static wchar_t szClass[64];
    swprintf_s(szClass, L"WndClass_%04X", GetTickCount() & 0xFFFF);
    wc.lpszClassName = szClass;
    RegisterClassExW(&wc);

    int winW = 410, winH = 185;
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    int x = (scrW - winW) / 2;
    int y = (scrH - winH) / 2;

    HWND hwnd = CreateWindowExW(0, szClass, L"Configuration",
        WS_CAPTION | WS_SYSMENU,
        x, y, winW, winH, NULL, NULL, hInstance, NULL);

    if (!hwnd)
    {
        UnregisterClassW(szClass, hInstance);
        return Usermode;
    }

    CreateWindowW(L"BUTTON", L"Usermode",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        30, 90, 160, 40, hwnd, (HMENU)1, hInstance, NULL);

    CreateWindowW(L"BUTTON", L"Kernel Mode",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        220, 90, 160, 40, hwnd, (HMENU)2, hInstance, NULL);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DeleteObject(wc.hbrBackground);
    UnregisterClassW(szClass, hInstance);
    return g_SelectedMode;
}

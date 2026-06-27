#include "SplashWindow.h"
#include "LoaderConsole.h"
#include <Version.h>
#include <Auth/skStr.h>
#include <windows.h>
#include <dwmapi.h>
#include <cstdio>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cmath>

#pragma comment(lib, "dwmapi.lib")

namespace {

    struct SplashData {
        std::atomic<int> progress{ 0 };
        std::atomic<int64_t> dlNow{ 0 };
        std::atomic<int64_t> dlTotal{ 0 };
        std::atomic<bool> dlActive{ false };
        std::atomic<bool> closeRequested{ false };
        std::atomic<bool> windowReady{ false };
        std::atomic<int> alpha{ 0 };

        // Realtime animation and shimmer parameters
        float currentProgress{ 0.0f };
        float shimmerOffset{ -80.0f };

        wchar_t stage[64]{};
        wchar_t message[256]{};
        CRITICAL_SECTION cs;

        SplashData() { InitializeCriticalSection(&cs); }
        ~SplashData() { DeleteCriticalSection(&cs); }

        void SetText(const char* s, wchar_t* buf, size_t cap) {
            int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
            if (len > 0 && (size_t)len <= cap) {
                MultiByteToWideChar(CP_UTF8, 0, s, -1, buf, (int)cap);
            }
        }
    };

    SplashData* g_data = nullptr;
    std::thread* g_thread = nullptr;
    HWND g_hwnd = nullptr;
    const int SPLASH_WIDTH = 460;
    const int SPLASH_HEIGHT = 280;

    COLORREF BgColor() { return RGB(8, 12, 10); } // Base dark color
    COLORREF AccentColor() { return RGB(52, 211, 153); } // High-end Mint Green
    COLORREF TextPrimary() { return RGB(241, 245, 249); }
    COLORREF TextSecondary() { return RGB(148, 163, 184); }
    COLORREF TextMuted() { return RGB(71, 85, 105); }
    COLORREF BorderColor() { return RGB(24, 45, 34); } // Deep slate-green border
    COLORREF ProgressTrack() { return RGB(18, 30, 24); }

    HFONT CreateTitleFont() {
        return CreateFontW(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    }

    HFONT CreateSubFont() {
        return CreateFontW(15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    }

    HFONT CreateSmallFont() {
        return CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    }

    void CenterWindow(HWND hwnd) {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);
        SetWindowPos(hwnd, 0, (sw - w) / 2, (sh - h) / 2, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    void DrawRoundRect(HDC hdc, int x, int y, int w, int h, int r, COLORREF color) {
        HPEN pen = CreatePen(PS_NULL, 0, color);
        HBRUSH brush = CreateSolidBrush(color);
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        RoundRect(hdc, x, y, x + w, y + h, r * 2, r * 2);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(pen);
        DeleteObject(brush);
    }

    void DrawTextCentered(HDC hdc, int x, int y, int w, int h, const wchar_t* text, COLORREF color, HFONT font) {
        SetTextColor(hdc, color);
        SetBkMode(hdc, TRANSPARENT);
        HGDIOBJ oldFont = SelectObject(hdc, font);
        RECT rc = { x, y, x + w, y + h };
        DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
    }

    void DrawTextLeft(HDC hdc, int x, int y, int w, int h, const wchar_t* text, COLORREF color, HFONT font) {
        SetTextColor(hdc, color);
        SetBkMode(hdc, TRANSPARENT);
        HGDIOBJ oldFont = SelectObject(hdc, font);
        RECT rc = { x, y, x + w, y + h };
        DrawTextW(hdc, text, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
    }

    void RenderSplash(HDC hdc, HWND hwnd) {
        RECT client;
        GetClientRect(hwnd, &client);
        int w = client.right;
        int h = client.bottom;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, w, h);
        HGDIOBJ oldBM = SelectObject(memDC, memBM);

        // 1. Draw smooth vertical background gradient (dark tech forest palette)
        COLORREF colorTop = RGB(15, 27, 21);
        COLORREF colorBottom = RGB(8, 12, 10);
        for (int y = 0; y < h; ++y) {
            float ratio = (float)y / (float)h;
            BYTE r = (BYTE)(GetRValue(colorTop) * (1.0f - ratio) + GetRValue(colorBottom) * ratio);
            BYTE g = (BYTE)(GetGValue(colorTop) * (1.0f - ratio) + GetGValue(colorBottom) * ratio);
            BYTE b = (BYTE)(GetBValue(colorTop) * (1.0f - ratio) + GetBValue(colorBottom) * ratio);
            
            HPEN p = CreatePen(PS_SOLID, 1, RGB(r, g, b));
            HGDIOBJ oldP = SelectObject(memDC, p);
            MoveToEx(memDC, 0, y, nullptr);
            LineTo(memDC, w, y);
            SelectObject(memDC, oldP);
            DeleteObject(p);
        }

        // 2. Overlay extremely faint high-tech diagonal stripes for added depth
        HPEN linePen = CreatePen(PS_SOLID, 1, RGB(18, 32, 25));
        HGDIOBJ oldLinePen = SelectObject(memDC, linePen);
        for (int i = -h; i < w; i += 20) {
            MoveToEx(memDC, i, 0, nullptr);
            LineTo(memDC, i + h, h);
        }
        SelectObject(memDC, oldLinePen);
        DeleteObject(linePen);

        // 3. Top Accent Bar (3px thick) as a visual anchor
        RECT accentRect = { 0, 0, w, 3 };
        HBRUSH accentBrush = CreateSolidBrush(AccentColor());
        FillRect(memDC, &accentRect, accentBrush);
        DeleteObject(accentBrush);

        // 4. Clean outer border matching the theme
        HPEN borderPen = CreatePen(PS_SOLID, 1, BorderColor());
        HGDIOBJ oldPen = SelectObject(memDC, borderPen);
        HGDIOBJ oldBrush = SelectObject(memDC, GetStockObject(NULL_BRUSH));
        Rectangle(memDC, 0, 0, w, h);
        SelectObject(memDC, oldPen);
        SelectObject(memDC, oldBrush);
        DeleteObject(borderPen);

        HFONT titleFont = CreateTitleFont();
        HFONT subFont = CreateSubFont();
        HFONT smallFont = CreateSmallFont();

        // Title with elegant wider letter tracking
        SetTextCharacterExtra(memDC, 4);
        DrawTextCentered(memDC, 0, 42, w, 40, L"BITWARE", TextPrimary(), titleFont);
        SetTextCharacterExtra(memDC, 0); // Reset

        // Version subtitle
        wchar_t versionStr[32];
        swprintf_s(versionStr, L"v%s", BITWARE_VERSION_WSTRING);
        DrawTextCentered(memDC, 0, 84, w, 20, versionStr, TextSecondary(), smallFont);

        EnterCriticalSection(&g_data->cs);
        wchar_t stageText[64], messageText[256];
        wcscpy_s(stageText, g_data->stage);
        wcscpy_s(messageText, g_data->message);
        LeaveCriticalSection(&g_data->cs);

        // Layout boundaries
        int barX = 50;
        int barW = w - 100;
        int barY = 175;
        int barH = 4;

        // Stage label (formatted in UPPERCASE)
        wchar_t stageUpper[64];
        wcscpy_s(stageUpper, stageText);
        _wcsupr_s(stageUpper);

        // Draw Stage Category
        SetTextCharacterExtra(memDC, 1);
        DrawTextLeft(memDC, barX, barY - 38, barW - 60, 15, stageUpper, AccentColor(), smallFont);
        SetTextCharacterExtra(memDC, 0);

        // Draw Stage Message
        DrawTextLeft(memDC, barX, barY - 22, barW - 60, 18, messageText, TextPrimary(), subFont);

        // Read smooth animated progress value
        float pctF = g_data->currentProgress;
        int pctDisplay = (int)std::round(pctF);

        // Draw percentage text matching layout
        wchar_t pctStr[16];
        swprintf_s(pctStr, L"%d%%", pctDisplay);
        DrawTextCentered(memDC, barX + barW - 50, barY - 22, 50, 18, pctStr, AccentColor(), subFont);

        // Draw thin sleek progress bar track
        RECT trackRect = { barX, barY, barX + barW, barY + barH };
        HBRUSH trackBrush = CreateSolidBrush(ProgressTrack());
        FillRect(memDC, &trackRect, trackBrush);
        DeleteObject(trackBrush);

        // Draw smooth animated progress fill
        if (pctF > 0.0f) {
            int fillW = (int)(barW * pctF / 100.0f);
            if (fillW > 0) {
                RECT fillRect = { barX, barY, barX + fillW, barY + barH };
                HBRUSH fillBrush = CreateSolidBrush(AccentColor());
                FillRect(memDC, &fillRect, fillBrush);
                DeleteObject(fillBrush);

                // Sweep a beautiful light mint-white shimmer / glint highlight across the active progress area
                int shimX = barX + (int)g_data->shimmerOffset;
                int shimW = 35; // shimmer wave width
                int startX = (std::max)(shimX, barX);
                int endX = (std::min)(shimX + shimW, barX + fillW);
                if (startX < endX) {
                    RECT shimRect = { startX, barY, endX, barY + barH };
                    HBRUSH shimBrush = CreateSolidBrush(RGB(167, 243, 208)); // Bright highlight mint green
                    FillRect(memDC, &shimRect, shimBrush);
                    DeleteObject(shimBrush);
                }
            }
        }

        // Footer with clean high-end spaced tracking
        wchar_t footerStr[128];
        swprintf_s(footerStr, L"B I T W A R E");
        SetTextCharacterExtra(memDC, 6);
        DrawTextCentered(memDC, 0, h - 35, w, 20, footerStr, TextMuted(), smallFont);
        SetTextCharacterExtra(memDC, 0);

        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBM);
        DeleteObject(memBM);
        DeleteDC(memDC);
        DeleteObject(titleFont);
        DeleteObject(subFont);
        DeleteObject(smallFont);
    }

    LRESULT CALLBACK SplashWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RenderSplash(hdc, hwnd);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_TIMER: {
            if (g_data) {
                // Smoothly slide progress variable towards target atomically set progress
                float target = (float)g_data->progress.load();
                float delta = target - g_data->currentProgress;
                if (std::abs(delta) > 0.05f) {
                    g_data->currentProgress += delta * 0.12f; // Smooth linear-interpolation step
                } else {
                    g_data->currentProgress = target;
                }

                // Increment progress glint shimmering position
                g_data->shimmerOffset += 3.2f; // Smooth horizontal shimmer movement speed
                if (g_data->shimmerOffset > (SPLASH_WIDTH - 100)) {
                    g_data->shimmerOffset = -80.0f; // Wrap to start from left side
                }

                if (g_data->closeRequested.load()) {
                    int a = g_data->alpha.load();
                    a -= 20;
                    if (a <= 0) {
                        a = 0;
                        KillTimer(hwnd, 1);
                        SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
                        PostMessageW(hwnd, WM_CLOSE, 0, 0);
                    }
                    g_data->alpha.store(a);
                    SetLayeredWindowAttributes(hwnd, 0, (BYTE)a, LWA_ALPHA);
                }
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_CREATE: {
            SetTimer(hwnd, 1, 33, nullptr);
            return 0;
        }
        case WM_DESTROY: {
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    DWORD WINAPI SplashThreadProc(LPVOID) {
        const wchar_t* className = L"BitwareSplash";

        HINSTANCE hInst = GetModuleHandleW(nullptr);

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
        wc.lpfnWndProc = SplashWndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = className;
        RegisterClassExW(&wc);

        HWND hwnd = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            className, L"Loader",
            WS_POPUP,
            0, 0, SPLASH_WIDTH, SPLASH_HEIGHT,
            nullptr, nullptr, hInst, nullptr
        );

        if (!hwnd) {
            g_data->windowReady.store(true);
            return 1;
        }

        // Apply native Windows 11 rounded corners preference safely without header macro requirements
        int preference = 2; // DWMWCP_ROUND = 2
        DwmSetWindowAttribute(hwnd, 33, &preference, sizeof(preference)); // DWMWA_WINDOW_CORNER_PREFERENCE = 33

        g_hwnd = hwnd;
        SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
        CenterWindow(hwnd);
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        for (int a = 0; a <= 255; a += 25) {
            SetLayeredWindowAttributes(hwnd, 0, (BYTE)a, LWA_ALPHA);
            if (g_data) g_data->alpha.store(a);
            Sleep(8);
        }
        if (g_data) g_data->alpha.store(255);
        SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

        g_data->windowReady.store(true);

        MSG msg;
        BOOL ret;
        while ((ret = GetMessageW(&msg, nullptr, 0, 0)) != 0) {
            if (ret == -1) break;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (g_hwnd) {
            DestroyWindow(g_hwnd);
            g_hwnd = nullptr;
        }
        UnregisterClassW(className, hInst);

        return 0;
    }

}

namespace SplashWindow {

    bool Create() {
        if (g_data) return false;

        g_data = new SplashData();
        g_data->SetText("Initializing", g_data->stage, 64);
        g_data->SetText("Starting...", g_data->message, 256);

        SetStatus(Status::Idle, "Init", "Starting...");

        g_thread = new std::thread(SplashThreadProc, nullptr);

        for (int i = 0; i < 200; i++) {
            if (g_data->windowReady.load())
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        return g_data->windowReady.load();
    }

    void SetStatus(Status, const char* stage, const char* message) {
        if (!g_data) return;
        EnterCriticalSection(&g_data->cs);
        g_data->SetText(stage, g_data->stage, 64);
        g_data->SetText(message, g_data->message, 256);
        LeaveCriticalSection(&g_data->cs);
    }

    void SetProgress(int percent) {
        if (!g_data) return;
        if (percent < 0) percent = 0;
        if (percent > 100) percent = 100;
        g_data->progress.store(percent);
    }

    void SetDownloadProgress(int64_t downloaded, int64_t total) {
        if (!g_data || total <= 0) return;
        g_data->dlNow.store(downloaded);
        g_data->dlTotal.store(total);
        int pct = (int)(downloaded * 100 / total);
        SetProgress(pct);

        wchar_t msg[256];
        if (total > 1024 * 1024) {
            swprintf_s(msg, L"Downloading... (%lld / %lld MB)",
                downloaded / (1024 * 1024), total / (1024 * 1024));
        } else {
            swprintf_s(msg, L"Downloading... (%lld / %lld KB)",
                downloaded / 1024, total / 1024);
        }
        EnterCriticalSection(&g_data->cs);
        wcscpy_s(g_data->message, msg);
        LeaveCriticalSection(&g_data->cs);
    }

    void Close() {
        if (!g_data) return;
        g_data->closeRequested.store(true);

        if (g_thread && g_thread->joinable()) {
            g_thread->join();
        }

        delete g_thread;
        g_thread = nullptr;
        delete g_data;
        g_data = nullptr;
        g_hwnd = nullptr;
    }

    bool IsRunning() {
        return g_data && !g_data->closeRequested.load();
    }

}

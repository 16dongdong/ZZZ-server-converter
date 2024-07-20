#include "转服小工具.h"
#include <shlobj.h>
#include <fstream>
#include <commctrl.h>
#include <filesystem>
#include <iostream>

#pragma comment(lib, "comctl32.lib")

namespace fs = std::filesystem;

转服小工具::转服小工具(HINSTANCE hInst) : hInstance(hInst) {}

void 转服小工具::run() {
    const wchar_t CLASS_NAME[] = L"ZhuanFuXiaoGongJuWindowClass";

    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    config = L"config.ini";

    if (!loadConfig()) {
        MessageBox(NULL, L"请选择本地游戏目录", L"提示", MB_OK);
        destination_zzz = browseFolder(L"请选择本地游戏目录");

        MessageBox(NULL, L"请选择资源目录", L"提示", MB_OK);
        std::wstring data_folder = browseFolder(L"请选择资源目录");

        source_global = data_folder + L"\\global";
        source_cn = data_folder + L"\\cn";

        saveConfig();
    }

    hwndMain = CreateWindowEx(
        0,
        CLASS_NAME,
        L"转服小工具",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL,
        NULL,
        hInstance,
        this
    );

    if (hwndMain == NULL) {
        return;
    }

    ShowWindow(hwndMain, SW_SHOW);

    CreateWindow(
        L"STATIC", L"当前为：",
        WS_VISIBLE | WS_CHILD,
        50, 30, 100, 20,
        hwndMain, NULL, hInstance, NULL);

    CreateWindow(
        L"STATIC", L"",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        150, 30, 200, 20,
        hwndMain, (HMENU)2, hInstance, NULL);

    CreateWindow(
        L"BUTTON", L"",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        100, 80, 200, 30,
        hwndMain, (HMENU)1, hInstance, NULL);

    detectCurrentServer(hwndMain);
    updateButtonLabel(hwndMain);
    updateServerStatus(hwndMain); // 更新服务器状态

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK 转服小工具::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    转服小工具* app;
    if (uMsg == WM_NCCREATE) {
        app = static_cast<转服小工具*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else {
        app = reinterpret_cast<转服小工具*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (app) {
        switch (uMsg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                app->showProgressDialog(hwnd, L"正在复制文件...");
                if (app->currentServer == L"国服") {
                    app->copyDirectoryRecursively(app->source_global, app->destination_zzz);
                    app->setConfigValue(app->config, app->key, L"hoyoverse");
                    app->currentServer = L"国际服";
                }
                else {
                    app->copyDirectoryRecursively(app->source_cn, app->destination_zzz);
                    app->setConfigValue(app->config, app->key, L"update_pc");
                    app->currentServer = L"国服";
                }
                app->updateButtonLabel(hwnd);
                app->updateServerStatus(hwnd);
            }
            break;
        case WM_CREATE:
            app->detectCurrentServer(hwnd);
            app->updateServerStatus(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void 转服小工具::copyFiles(const std::wstring& source, const std::wstring& target) {
    SHFILEOPSTRUCTW fileOp = { 0 };
    fileOp.wFunc = FO_COPY;
    std::wstring from = source + L"\\*.*" + L'\0';  // 拷贝source目录下的所有文件
    std::wstring to = target + L'\0';
    fileOp.pFrom = from.c_str();
    fileOp.pTo = to.c_str();
    fileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOCOPYSECURITYATTRIBS | FOF_NOERRORUI;

    SHFileOperationW(&fileOp);
}

std::wstring 转服小工具::getConfigValue(const std::wstring& config, const std::wstring& key) {
    std::wifstream configFile(config);
    std::wstring line;
    while (std::getline(configFile, line)) {
        if (line.find(key + L"=") == 0) {
            return line.substr(key.length() + 1);
        }
    }
    return L"";
}

void 转服小工具::setConfigValue(const std::wstring& config, const std::wstring& key, const std::wstring& value) {
    std::wifstream configFile(config);
    std::wstring content;
    std::wstring line;
    bool found = false;

    while (std::getline(configFile, line)) {
        if (line.find(key + L"=") == 0) {
            content += key + L"=" + value + L"\n";
            found = true;
        }
        else {
            content += line + L"\n";
        }
    }

    if (!found) {
        content += key + L"=" + value + L"\n";
    }

    configFile.close();

    std::wofstream outFile(config);
    outFile << content;
    outFile.close();
}

void 转服小工具::detectCurrentServer(HWND hwnd) {
    std::wstring value = getConfigValue(config, key);
    if (value == L"update_pc") {
        currentServer = L"国服";
    }
    else if (value == L"hoyoverse") {
        currentServer = L"国际服";
    }
    else {
        currentServer = L"未知的";
    }
    SetWindowText(GetDlgItem(hwnd, 2), currentServer.c_str());
}

void 转服小工具::updateButtonLabel(HWND hwnd) {
    if (currentServer == L"国服") {
        SetWindowText(GetDlgItem(hwnd, 1), L"转为国际服");
    }
    else if (currentServer == L"国际服") {
        SetWindowText(GetDlgItem(hwnd, 1), L"转为国服");
    }
}

void 转服小工具::updateServerStatus(HWND hwnd) {
    SetWindowText(GetDlgItem(hwnd, 2), currentServer.c_str());
}

std::wstring 转服小工具::browseFolder(const wchar_t* title) {
    BROWSEINFOW bi = { 0 };
    bi.lpszTitle = title;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != 0) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) {
            std::wstring result(path);
            CoTaskMemFree(pidl);
            return result;
        }
        CoTaskMemFree(pidl);
    }
    return L"";
}

void 转服小工具::saveConfig() {
    std::wofstream configFile(config);
    configFile << L"source_global=" << source_global << std::endl;
    configFile << L"source_cn=" << source_cn << std::endl;
    configFile << L"destination_zzz=" << destination_zzz << std::endl;
}

bool 转服小工具::loadConfig() {
    std::wifstream configFile(config);
    if (!configFile.is_open()) {
        return false;
    }

    std::wstring line;
    while (std::getline(configFile, line)) {
        if (line.find(L"source_global=") == 0) {
            source_global = line.substr(14);
        }
        else if (line.find(L"source_cn=") == 0) {
            source_cn = line.substr(10);
        }
        else if (line.find(L"destination_zzz=") == 0) {
            destination_zzz = line.substr(16);
        }
    }

    return !(source_global.empty() || source_cn.empty() || destination_zzz.empty());
}

void 转服小工具::showProgressDialog(HWND hwnd, const std::wstring& message) {
    HWND progressDialog = CreateWindowEx(
        0, PROGRESS_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        50, 120, 300, 30,
        hwnd, NULL, hInstance, NULL);

    SendMessage(progressDialog, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(progressDialog, PBM_SETSTEP, (WPARAM)1, 0);

    for (int i = 0; i <= 100; ++i) {
        SendMessage(progressDialog, PBM_STEPIT, 0, 0);
        Sleep(10);
    }

    DestroyWindow(progressDialog);
}

void 转服小工具::copyDirectoryRecursively(const std::wstring& source, const std::wstring& target) {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(source)) {
            const auto& path = entry.path();
            auto relativePathStr = path.lexically_relative(source).wstring();
            fs::copy(path, target + L"\\" + relativePathStr, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
        }
    }
    catch (const fs::filesystem_error& e) {
        std::wstring error_message = L"文件系统错误: " + std::wstring(e.what(), e.what() + strlen(e.what()));
        MessageBox(NULL, error_message.c_str(), L"错误", MB_OK | MB_ICONERROR);
    }
}

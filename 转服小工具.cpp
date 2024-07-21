#include "转服小工具.h"
#include <shlobj.h>
#include <fstream>
#include <commctrl.h>
#include <filesystem>
#include <iostream>
#pragma comment(lib, "comctl32.lib")

namespace fs = std::filesystem;

转服小工具::转服小工具(HINSTANCE hInst) : hInstance(hInst), hwndProgress(NULL), hwndMessage(NULL) {}

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
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
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

    CreateWindow(
        L"BUTTON", L"检测当前服务器",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        100, 130, 200, 30,
        hwndMain, (HMENU)2, hInstance, NULL);

    CreateWindow(
        L"BUTTON", L"更新数据（切换服务器后）",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        100, 180, 200, 30,
        hwndMain, (HMENU)3, hInstance, NULL);

    CreateWindow(
        L"BUTTON", L"保留已更新数据",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        100, 230, 200, 30,
        hwndMain, (HMENU)4, hInstance, NULL);

    CreateWindow(
        L"BUTTON", L"重选文件目录",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        100, 280, 200, 30,
        hwndMain, (HMENU)5, hInstance, NULL);

    detectCurrentServer(hwndMain);
    updateButtonLabel(hwndMain);
    updateServerStatus(hwndMain);

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
                app->showProgressDialog(hwnd, L"正在复制文件...", 100);
                try {
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
                }
                catch (const std::exception& e) {
                    std::wstring error_message = L"文件系统错误: " + 转服小工具::s2ws(e.what());
                    app->copyToClipboard(error_message);
                    MessageBox(NULL, error_message.c_str(), L"错误", MB_OK | MB_ICONERROR);
                }
                app->updateButtonLabel(hwnd);
                app->updateServerStatus(hwnd);
            }
            else if (LOWORD(wParam) == 2) {
                app->detectCurrentServer(hwnd);
            }
            else if (LOWORD(wParam) == 3) {
                app->updateReplacementFiles(hwnd);
            }
            else if (LOWORD(wParam) == 4) {
                std::wstring value = app->getConfigValue(app->config, app->key);
                if (value == L"update_pc") {
                    app->backupResources(app->destination_zzz, app->source_cn);
                }
                else {
                    app->backupResources(app->destination_zzz, app->source_global);
                }
            }
            else if (LOWORD(wParam) == 5) {
                app->reselectDirectories(hwnd);
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

void 转服小工具::showProgressDialog(HWND hwnd, const std::wstring& message, int totalSteps) {
    hwndProgress = CreateWindowEx(0, PROGRESS_CLASS, NULL,
        WS_CHILD | WS_VISIBLE,
        50, 10, 300, 10,  // 设置进度条的位置和大小
        hwnd, NULL, hInstance, NULL);

    SendMessage(hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, totalSteps));
    SendMessage(hwndProgress, PBM_SETSTEP, (WPARAM)1, 0);

    hwndMessage = CreateWindowEx(0, L"STATIC", message.c_str(),
        WS_CHILD | WS_VISIBLE,
        50, 70, 300, 20,  // 设置消息框的位置和大小
        hwnd, NULL, hInstance, NULL);
}

void 转服小工具::updateProgressDialog(int step) {
    SendMessage(hwndProgress, PBM_STEPIT, 0, 0);
    if (step >= 100) {
        Sleep(500);  // 给用户一点时间看到进度完成
        DestroyWindow(hwndProgress);
        DestroyWindow(hwndMessage);
    }
}


void 转服小工具::copyDirectoryRecursively(const std::wstring& source, const std::wstring& target) {
    try {
        if (!fs::exists(source)) {
            throw std::runtime_error("源目录不存在: " + ws2s(source));
        }

        if (!fs::exists(target)) {
            fs::create_directories(target);
        }

        executeRobocopy(source, target, hwndProgress);
    }
    catch (const fs::filesystem_error& e) {
        std::wstring error_message = L"文件系统错误: " + s2ws(e.what());
        MessageBox(NULL, error_message.c_str(), L"错误", MB_OK | MB_ICONERROR);
    }
    catch (const std::exception& e) {
        std::wstring error_message = L"错误: " + s2ws(e.what());
        MessageBox(NULL, error_message.c_str(), L"错误", MB_OK | MB_ICONERROR);
    }
}

void 转服小工具::updateReplacementFiles(HWND hwnd) {
    showProgressDialog(hwnd, L"正在更新替换文件...", 100);
    try {
        if (currentServer == L"国服") {
            copyDirectoryRecursively(source_cn, destination_zzz);
        }
        else if (currentServer == L"国际服") {
            copyDirectoryRecursively(source_global, destination_zzz);
        }
    }
    catch (const std::exception& e) {
        std::wstring error_message = L"文件系统错误: " + 转服小工具::s2ws(e.what());
        copyToClipboard(error_message);
        MessageBox(NULL, error_message.c_str(), L"错误", MB_OK | MB_ICONERROR);
    }
}

void 转服小工具::backupFiles(const std::wstring& destination, const std::wstring& target) {
    try {
        // 创建进度条和消息框
        showProgressDialog(hwndMain, L"正在备份文件...", 100);

        executeRobocopy(destination, target, hwndProgress);

        DestroyWindow(hwndProgress);
        DestroyWindow(hwndMessage);
    }
    catch (const std::exception& e) {
        std::wstring error_message = L"错误: " + s2ws(e.what());
        MessageBox(NULL, error_message.c_str(), L"错误", MB_OK | MB_ICONERROR);
    }
}

void 转服小工具::backupResources(const std::wstring& destination, const std::wstring& target) {
    try {
        // 创建进度条和消息框
        showProgressDialog(hwndMain, L"正在备份资源文件...", 100);

        executeRobocopy(destination + L"\\ZenlessZoneZero_Data\\Persistent", target + L"\\ZenlessZoneZero_Data\\Persistent", hwndProgress);

        DestroyWindow(hwndProgress);
        DestroyWindow(hwndMessage);
    }
    catch (const std::exception& e) {
        std::wstring error_message = L"错误: " + s2ws(e.what());
        MessageBox(NULL, error_message.c_str(), L"错误", MB_OK | MB_ICONERROR);
    }
}

void 转服小工具::reselectDirectories(HWND hwnd) {
    MessageBox(hwnd, L"请选择本地游戏目录", L"提示", MB_OK);
    destination_zzz = browseFolder(L"请选择本地游戏目录");

    MessageBox(hwnd, L"请选择资源目录", L"提示", MB_OK);
    std::wstring data_folder = browseFolder(L"请选择资源目录");

    source_global = data_folder + L"\\global";
    source_cn = data_folder + L"\\cn";

    saveConfig();
    detectCurrentServer(hwnd);
    updateButtonLabel(hwnd);
    updateServerStatus(hwnd);
}

void 转服小工具::copyToClipboard(const std::wstring& text) {
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, (text.size() + 1) * sizeof(wchar_t));
        wchar_t* pchData = (wchar_t*)GlobalLock(hClipboardData);
        wcscpy_s(pchData, text.size() + 1, text.c_str());
        GlobalUnlock(hClipboardData);
        SetClipboardData(CF_UNICODETEXT, hClipboardData);
        CloseClipboard();
    }
}

std::wstring 转服小工具::s2ws(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

std::string 转服小工具::ws2s(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}
//进度条
void 转服小工具::executeRobocopy(const std::wstring& source, const std::wstring& target, HWND hwndProgress) {
    std::wstring command = L"robocopy \"" + source + L"\" \"" + target + L"\" /E /COPYALL /R:3 /W:5 /NFL /NDL /NJH /NJS /NC /NS";
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (CreateProcess(NULL, &command[0], NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        std::wcerr << L"Failed to execute robocopy command." << std::endl;
    }

    for (int i = 0; i <= 100; ++i) {
        SendMessage(hwndProgress, PBM_STEPIT, 0, 0);
        Sleep(50);
    }

    updateProgressDialog(100);

}

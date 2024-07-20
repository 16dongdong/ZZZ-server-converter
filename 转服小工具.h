#ifndef ZHUANFUXIAOGONGJU_H
#define ZHUANFUXIAOGONGJU_H

#include <windows.h>
#include <string>
#include <vector>

class ת��С���� {
public:
    ת��С����(HINSTANCE hInst);
    void run();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void copyFiles(const std::wstring& source, const std::wstring& target);
    std::wstring getConfigValue(const std::wstring& config, const std::wstring& key);
    void setConfigValue(const std::wstring& config, const std::wstring& key, const std::wstring& value);
    void detectCurrentServer(HWND hwnd);
    void updateButtonLabel(HWND hwnd);
    void updateServerStatus(HWND hwnd);
    std::wstring browseFolder(const wchar_t* title);
    void saveConfig();
    bool loadConfig();
    void showProgressDialog(HWND hwnd, const std::wstring& message);
    void copyDirectoryRecursively(const std::wstring& source, const std::wstring& target);

    std::wstring source_global;
    std::wstring source_cn;
    std::wstring destination_zzz;
    std::wstring config;
    std::wstring key = L"cps";
    std::wstring currentServer;
    HINSTANCE hInstance;
    HWND hwndMain;
};

#endif // ZHUANFUXIAOGONGJU_H

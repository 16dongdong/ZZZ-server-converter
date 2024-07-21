#include "转服小工具.h"
#include <windows.h>
#include <shellapi.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // 检查当前是否具有管理员权限
    BOOL isRunAsAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isRunAsAdmin);
        FreeSid(adminGroup);
    }

    // 如果不是以管理员权限运行，则请求管理员权限
    if (!isRunAsAdmin) {
        wchar_t szPath[MAX_PATH];
        if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath))) {
            SHELLEXECUTEINFO sei = { sizeof(sei) };
            sei.lpVerb = L"runas";
            sei.lpFile = szPath;
            sei.hwnd = NULL;
            sei.nShow = SW_NORMAL;

            if (!ShellExecuteEx(&sei)) {
                MessageBox(NULL, L"无法以管理员权限启动程序，请手动以管理员权限运行。", L"错误", MB_OK | MB_ICONERROR);
                return 1;
            }
            return 0; // 请求权限后返回，新的进程将以管理员权限运行
        }
    }

    // 继续运行程序
    转服小工具 app(hInstance);
    app.run();
    return 0;
}

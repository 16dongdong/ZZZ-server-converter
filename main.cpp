#include "ת��С����.h"
#include <windows.h>
#include <shellapi.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // ��鵱ǰ�Ƿ���й���ԱȨ��
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

    // ��������Թ���ԱȨ�����У����������ԱȨ��
    if (!isRunAsAdmin) {
        wchar_t szPath[MAX_PATH];
        if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath))) {
            SHELLEXECUTEINFO sei = { sizeof(sei) };
            sei.lpVerb = L"runas";
            sei.lpFile = szPath;
            sei.hwnd = NULL;
            sei.nShow = SW_NORMAL;

            if (!ShellExecuteEx(&sei)) {
                MessageBox(NULL, L"�޷��Թ���ԱȨ�������������ֶ��Թ���ԱȨ�����С�", L"����", MB_OK | MB_ICONERROR);
                return 1;
            }
            return 0; // ����Ȩ�޺󷵻أ��µĽ��̽��Թ���ԱȨ������
        }
    }

    // �������г���
    ת��С���� app(hInstance);
    app.run();
    return 0;
}

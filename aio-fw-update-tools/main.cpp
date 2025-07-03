#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <shellapi.h>   // 包含IsUserAnAdmin()的声明

//#define USING_ADMINISTRATOR_MODE

#ifdef USING_ADMINISTRATOR_MODE
#ifdef Q_OS_WIN
    #include <windows.h>
    #include <comdef.h>

    bool isUserAdmin() {
        BOOL isAdmin = FALSE;
        SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
        PSID AdministratorsGroup;

        if (AllocateAndInitializeSid(&NtAuthority, 2,
             SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
             0, 0, 0, 0, 0, 0, &AdministratorsGroup)) {

            if (CheckTokenMembership(NULL, AdministratorsGroup, &isAdmin)) {
                FreeSid(AdministratorsGroup);
                return (isAdmin != FALSE);
            }
            FreeSid(AdministratorsGroup);
        }
        return false;
    }
#endif
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

// 请求管理员权限（Windows）
#ifdef USING_ADMINISTRATOR_MODE
#ifdef Q_OS_WIN
    if (isUserAdmin() == FALSE) {
        // 重新以管理员权限启动
        wchar_t programPath[MAX_PATH];
        GetModuleFileName(NULL, programPath, MAX_PATH);

        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = programPath;
        sei.hwnd = NULL;
        sei.nShow = SW_NORMAL;

        if (!ShellExecuteEx(&sei)) {
            QMessageBox::warning(NULL, "Permission Required",
                "This application requires administrator privileges to scan WiFi networks.");
        }
        return 0;
    }
#endif
#endif

    MainWindow w;
    w.show();
    return a.exec();
}

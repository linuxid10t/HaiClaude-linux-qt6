/*  main.cpp – HaiClaude (Linux/Qt6)
 *
 *  Native Qt6 launcher for Claude Code.
 *
 *  Build:
 *      cmake -B build && cmake --build build
 *      ./build/haiclaude
 *
 *  Author: David Masson
 */

#include "LauncherWindow.h"

#include <QApplication>
#include <QIcon>

#include <string>
#include <unistd.h>
#include <cstdio>

std::string gPendingExec;

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("DavidMasson");
    app.setApplicationName("HaiClaude");
    app.setWindowIcon(QIcon(":/haiclaude-icon.png"));

    LauncherWindow window;
    window.show();

    int result = app.exec();

    if (!gPendingExec.empty()) {
        execl("/bin/sh", "sh", "-c", gPendingExec.c_str(), (char*)nullptr);
        // execl only returns on error
        perror("execl");
        return 1;
    }

    return result;
}

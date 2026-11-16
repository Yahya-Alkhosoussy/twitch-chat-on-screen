#include <pybind11/embed.h>
#include "../include/mainwindow.h"
#include "../include/updateChecker.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <QProcess>

namespace py = pybind11;

#ifdef Q_OS_WIN
    #ifdef _DEBUG
        QString findPythonHome() {
            QProcess process;
            process.start("python", QStringList() << "-c" << "import sys; print(sys.base_prefix)");
            process.waitForFinished();
            QString result = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            std::cout << result.toStdString() << std::endl;
            return result;
        }
    #endif
#endif

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    QApplication::setOrganizationName("spiderbyte");
    QApplication::setApplicationName("twitch-chat");

    QString appDir = QCoreApplication::applicationDirPath();


    bool update = checkForNewVersion();
    auto version = get_update();
    auto current = parseTags(APP_VERSION);
    if (version && update && current) {
        auto reply = QMessageBox::question(nullptr, "Update available!",
            QString("Version %1 is available (You have %2). Download and install now?")
                .arg(
                    QString::fromStdString(std::to_string(version->version[0]) + "." + std::to_string(version->version[1]) + "." + std::to_string(version->version[2])),
                    QString::fromStdString(std::to_string(current.value()[0]) + "." + std::to_string(current.value()[1]) + "." + std::to_string(current.value()[2]))
                )
        );

        if (reply == QMessageBox::Yes) {
            QString installer = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                .filePath("twitch-chat.exe");
            if (updateApp(version->downloadUrl, installer)) {
                QProcess::startDetached(installer, {});
                return 0; // so the installer can overwrite the exe.
            }
            QMessageBox::warning(nullptr, "Update failed", "Could not download the update.");
        }
    }

#ifdef Q_OS_MAC
QString resourcesDir = QDir(appDir).filePath("../Resources");
QString pythonRoot = QDir(resourcesDir).filePath("python");
#elif Q_OS_LINUX
QString resourcesDir = QDir(appDir).filePath("../Resources");
QString pythonRoot = QDir(resourcesDir).filePath("python");
#elif defined(Q_OS_WIN)
#ifndef _DEBUG 
QString pythonRoot = QDir(appDir).filePath("../python"); // sibling of bin/, matches your install layout
#else
QString pythonRoot = QDir(appDir).filePath("../venv");
#endif
#endif

    QString sitePackages = QDir(pythonRoot).filePath("site-packages");
    QString scriptsDir = QDir(pythonRoot).filePath("scripts");

#ifdef Q_OS_WIN
#ifndef _DEBUG
    qputenv("PYTHONHOME", pythonRoot.toUtf8().constData());
#else
    sitePackages = QDir(appDir).filePath("../../../.venv/Lib/site-packages");
    scriptsDir = QDir(appDir).filePath("../../../python");
    qputenv("PYTHONHOME", findPythonHome().toUtf8().constData());
    qDebug() << "DEBUG MODE: PATHS OVERWRITTEN";
    qDebug() << "found site packages: " << sitePackages.toStdString();
    qDebug() << "found scripts: " << scriptsDir.toStdString();
#endif
#endif
    
    QByteArray pathEnv = (sitePackages + QDir::listSeparator() + scriptsDir).toUtf8();
    qputenv("PYTHONPATH", pathEnv);
    
    // python stuff
    py::scoped_interpreter guard{};

    try {
        py::module_ sys = py::module_::import("sys");
        qDebug() << "Embedded Python version:" << QString::fromStdString(sys.attr("version").cast<std::string>());
        qDebug() << "Embedded Python executable:" << QString::fromStdString(sys.attr("executable").cast<std::string>());

        py::module_ twitch_api = py::module_::import("twitch_api");
    }
    catch (py::error_already_set& e) {
        qDebug() << "Got an error from python! Error: " << e.what();
        return 1;
    }

    MainWindow w;
    w.show();
    return QCoreApplication::exec();
}

#include "ui/MainWindow.hpp"
#include "core/config/AppConfig.hpp"
#include "core/database/DatabaseManager.hpp"
#include "core/ui/ThemeManager.hpp"

#include <QApplication>
#include <QTranslator>
#include <QSqlDatabase>
#include <QMessageBox>
#include <QStyleFactory>
#include <QDebug>

// ── Translation loader ────────────────────────────────────────────────────────

static void loadTranslation(QApplication& app, const QString& lang)
{
    if (lang == "en") return;
    const QString base = QCoreApplication::applicationDirPath();
    auto* t = new QTranslator(&app);
    if (t->load(QString("edv_%1").arg(lang), base)) {
        app.installTranslator(t);
    } else {
        delete t;
        qWarning() << "[i18n] Translation not found for language:" << lang;
    }
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("EDV Enterprise System");
    app.setApplicationVersion("2.0.0");
    app.setOrganizationName("EDV-System");
    app.setOrganizationDomain("edv-system.de");

    // Fusion as base style — fully QSS-overridable, consistent across platforms
    app.setStyle(QStyleFactory::create("Fusion"));

    // ── Language ──────────────────────────────────────────────────────────────
    const QString lang = AppConfig::instance().language();
    loadTranslation(app, lang);
    if (lang == "ar") app.setLayoutDirection(Qt::RightToLeft);

    // ── Theme ─────────────────────────────────────────────────────────────────
    ThemeManager::instance().applyFromConfig();

    // ── PostgreSQL driver check ───────────────────────────────────────────────
    if (!QSqlDatabase::drivers().contains("QPSQL")) {
        QMessageBox::critical(nullptr,
            QObject::tr("Driver Missing"),
            QObject::tr("PostgreSQL driver (QPSQL) not found.\n\n"
                        "Ensure libpq.dll and its dependencies are in the application directory."));
        return 1;
    }

    // ── Best-effort DB connect (if already configured from a previous run) ────
    //    Failures are handled gracefully inside MainWindow via the DB Settings
    //    button on the login page — no blocking loop needed before the window opens.
    auto& db = DatabaseManager::instance();
    if (AppConfig::instance().dbConfigured()) {
        if (!db.connectFromConfig()) {
            qWarning() << "[Main] Saved DB config failed:" << db.lastError()
                       << "— user can reconfigure via DB Settings on the login page.";
        } else {
            qDebug() << "[Main] Database connected.";
        }
    }

    // ── Single main window (login page is page 0 of the internal stack) ───────
    MainWindow mainWin;
    mainWin.show();

    const int exitCode = app.exec();
    DatabaseManager::instance().shutdown();
    return exitCode;
}

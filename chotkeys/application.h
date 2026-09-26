#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include "QHotkey/qhotkey.h"
#include "hotkeys.h"
#include "metatap.h"
#include <QSettings>
#include <QStandardPaths>
#include <QProcess>
#include <QList>
#include <QFile>

class Application : public QObject
{
    Q_OBJECT
public:
    explicit Application(QObject *parent = nullptr);

private slots:
    void initSetting();

private:
    void cleanSetting();
    void importForeignShortcuts(QSettings &setting);
    QStringList all;
    QStringList allexec;
    QList<QHotkey*> allkey;
    MetaTap *m_metaTap = nullptr;
    QString m_metaExec;   // command bound to Meta on its own, if any
    QSettings setting;
};

#endif // APPLICATION_H

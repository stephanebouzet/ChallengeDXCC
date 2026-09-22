#pragma once
// Portage C++ de l'interface Win32 avec Logger32 (services/v10_backend.py,
// classe Logger32ChallengeV9 : create_receiver_window/_wndproc/connect/
// process_logger_text/commit_qso).
//
// En natif C++, plus besoin de pomper manuellement la file de messages :
// QEventDispatcherWin32 (utilisé par Qt sous Windows) relaie déjà tous les
// messages du thread, y compris ceux de notre fenêtre invisible.
//
// Sur les plateformes non-Windows, cette classe reste compilable mais
// n'offre aucune fonctionnalité (Logger32 est un logiciel Windows).

#include "core/adif.h"

#include <QObject>
#include <QString>

class Logger32Bridge : public QObject
{
    Q_OBJECT
public:
    explicit Logger32Bridge(QObject *parent = nullptr);
    ~Logger32Bridge() override;

    static bool platformSupported();

    // Crée la fenêtre invisible et tente les canaux 1 à 5. Retourne true
    // si Logger32 a répondu.
    bool connectToLogger32();
    void disconnectAll();

    bool isConnected() const { return m_connected; }
    QString loggerTitle() const { return m_loggerTitle; }

    // Appelés depuis le trampoline WNDPROC (Windows uniquement) : publics
    // par nécessité technique (fonction C libre), mais non destinés à être
    // utilisés ailleurs dans l'application.
    //
    // IMPORTANT : ces déclarations doivent rester dans la section public.
    // Les placer sous 'signals:' fait interpréter 'struct Impl' comme une
    // déclaration de signal par moc, ce qui provoque :
    //   "Not a signal or slot declaration".
    struct Impl;
    Impl *propertyBag() { return m_impl; }
    void onLogger32Message(int command, long long value, void *replyHwnd);
    void onWmSetText(const QString &text);

signals:
    void logLine(const QString &line);
    void stateChanged(bool connected, const QString &detail);
    void qsoCommitted(const AdifFields &fields);
    void deletedDumpPathReceived(const QString &path);

private:
    void processLoggerText(const QString &text);
    void commitQso();

    AdifFields m_currentQso;
    bool m_connected = false;
    QString m_loggerTitle;
    long long m_frequencyHz = 0;

    // Détails d'implémentation Win32, opaques ici pour rester compilables
    // sans <windows.h> hors Windows (voir logger32bridge.cpp).
    Impl *m_impl = nullptr;
};

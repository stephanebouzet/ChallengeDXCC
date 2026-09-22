#pragma once
// Portage C++ de DeletedQSOWatcher (services/v10_backend.py).
// Surveille le fichier "ClubLog Dump File.txt" de Logger32 pour détecter
// les suppressions de QSO.

#include "db/challengedb.h"

#include <QString>

class DeletedQSOWatcher
{
public:
    explicit DeletedQSOWatcher(ChallengeDB &db);

    void setPath(const QString &path);
    void poll();

    bool changed = false;

private:
    void processLine(const QString &line);

    ChallengeDB &m_db;
    QString m_path;
    qint64 m_lastPosition = 0;
};

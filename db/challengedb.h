#pragma once
// Portage C++ de la classe ChallengeDB (services/v10_backend.py), via QtSql/SQLite.

#include "core/adif.h"

#include <QString>
#include <QSqlDatabase>
#include <QMap>
#include <QSet>
#include <QDate>

class ChallengeDB
{
public:
    explicit ChallengeDB(const QString &path, const QString &connectionName = "challenge_db");
    ~ChallengeDB();

    bool isOpen() const;

    static QString makeQsoKey(const AdifFields &fields);

    // Insère/actualise un QSO. Retourne true si l'opération a réussi.
    bool upsertQso(const AdifFields &fields, bool deleted, const QString &source);

    // Retourne true si cette suppression est nouvelle pour la base locale.
    bool markDeleted(const AdifFields &fields);

    bool probableDuplicate(const AdifFields &fields, int seconds = 30);

    int countActiveQso();
    int countChallengeQso(const QDate &start, const QDate &end);

    // bande (minuscule) -> {dxcc -> nb qso}
    QMap<QString, QMap<int, int>> workedDxccByBand(const QDate &start, const QDate &end);

    void close();

private:
    void createSchema();

    QString m_path;
    QString m_connectionName;
};

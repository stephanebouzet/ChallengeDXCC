#pragma once
// Portage C++ de ADIFImporter (services/v10_backend.py).

#include "db/challengedb.h"
#include "backend/localdxccresolver.h"

#include <QString>
#include <QVector>

struct AdifImportStats {
    QString file;
    int read = 0;
    int accepted = 0;
    int challenge = 0;
    int resolved = 0;
    int unresolved = 0;
};

class ADIFImporter
{
public:
    ADIFImporter(ChallengeDB &db, const LocalDXCCResolver &resolver);

    AdifImportStats importFile(const QString &path);
    QVector<AdifImportStats> importDirectory(const QString &directory);

private:
    ChallengeDB &m_db;
    const LocalDXCCResolver &m_resolver;
};

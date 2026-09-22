#include "backend/adifimporter.h"
#include "backend/backendcommon.h"
#include "core/adif.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <algorithm>

ADIFImporter::ADIFImporter(ChallengeDB &db, const LocalDXCCResolver &resolver)
    : m_db(db)
    , m_resolver(resolver)
{
}

AdifImportStats ADIFImporter::importFile(const QString &path)
{
    AdifImportStats stats;
    stats.file = QFileInfo(path).fileName();

    const QString text = adifReadTextFlexible(path);
    if (text.isEmpty() && !QFileInfo(path).exists()) {
        qWarning() << "[WARN] Import ADIF impossible" << stats.file;
        return stats;
    }

    const auto period = backendChallengePeriod();

    for (const AdifFields &constFields : adifIterRecords(text)) {
        AdifFields fields = constFields;
        stats.read++;

        const QString call = bcClean(fields.value("CALL"));
        if (!backendPlausibleCallsign(call))
            continue;

        const QString resolution = m_resolver.enrichFields(fields);
        if (resolution == "local" || resolution == "adif")
            stats.resolved++;
        else
            stats.unresolved++;

        const QString isoDate = backendAdifDateToIso(fields.value("QSO_DATE"));
        if (!isoDate.isEmpty() &&
            isoDate >= period.first.toString(Qt::ISODate) &&
            isoDate <= period.second.toString(Qt::ISODate)) {
            stats.challenge++;
        }

        if (m_db.upsertQso(fields, false, "adif:" + stats.file))
            stats.accepted++;
    }

    return stats;
}

QVector<AdifImportStats> ADIFImporter::importDirectory(const QString &directory)
{
    QDir dir(directory);
    dir.mkpath(".");

    QStringList filters = {"*.adi", "*.adif"};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    QVector<AdifImportStats> allStats;
    if (files.isEmpty()) {
        qInfo() << "Import ADIF   : aucun fichier .adi/.adif dans" << directory;
        return allStats;
    }

    qInfo() << "Import ADIF   :" << files.size() << "fichier(s) trouvé(s)";
    for (const QFileInfo &fi : files) {
        const AdifImportStats stats = importFile(fi.absoluteFilePath());
        allStats.append(stats);
        qInfo().noquote() << QString("  %1: %2 QSO lus, %3 dans la période, %4 DXCC résolus")
                                  .arg(fi.fileName())
                                  .arg(stats.read)
                                  .arg(stats.challenge)
                                  .arg(stats.resolved);
    }
    return allStats;
}

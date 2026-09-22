#include "backend/deletedqsowatcher.h"
#include "backend/backendcommon.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDebug>

DeletedQSOWatcher::DeletedQSOWatcher(ChallengeDB &db)
    : m_db(db)
{
}

void DeletedQSOWatcher::setPath(const QString &path)
{
    if (m_path == path)
        return;
    m_path = path;
    // Pour une première lecture, on relit le fichier complet.
    // L'UPSERT empêche les doublons.
    m_lastPosition = 0;
    qInfo() << "Dump suppressions :" << m_path;
}

void DeletedQSOWatcher::poll()
{
    if (m_path.isEmpty())
        return;

    QFileInfo fi(m_path);
    if (!fi.exists())
        return;

    const qint64 size = fi.size();
    if (size < m_lastPosition)
        m_lastPosition = 0;
    if (size == m_lastPosition)
        return;

    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[WARN] Lecture dump impossible :" << f.errorString();
        return;
    }

    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    f.seek(m_lastPosition);

    while (!in.atEnd())
        processLine(in.readLine());

    m_lastPosition = f.pos();
}

void DeletedQSOWatcher::processLine(const QString &lineIn)
{
    const QString line = lineIn.trimmed();
    if (line.isEmpty())
        return;

    const AdifFields fields = backendParseAdif(line);
    if (fields.isEmpty())
        return;

    // Dans les exemples Logger32 le champ est : <APP_QSO_DELETED:0>
    if (!fields.contains("APP_QSO_DELETED"))
        return;

    if (m_db.markDeleted(fields)) {
        changed = true;
        const QString call = bcClean(fields.value("CALL"));
        const QString dxcc = bcClean(fields.value("DXCC")).isEmpty() ? "?" : bcClean(fields.value("DXCC"));
        const QString band = bcClean(fields.value("BAND")).isEmpty() ? "?" : bcClean(fields.value("BAND"));
        const QString date = bcClean(fields.value("QSO_DATE")).isEmpty() ? "?" : bcClean(fields.value("QSO_DATE"));
        const QString time = bcClean(fields.value("TIME_ON")).isEmpty() ? "?" : bcClean(fields.value("TIME_ON"));
        qInfo().noquote() << QString("[DELETE] %1 DXCC=%2 %3 %4 %5")
                                  .arg(call, -12).arg(dxcc, 3).arg(band, 4).arg(date, time);
    }
}

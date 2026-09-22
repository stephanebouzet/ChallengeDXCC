#include "core/challengeengine.h"
#include "core/adif.h"

#include <QFileInfo>
#include <algorithm>

ChallengeEngine::ChallengeEngine(const QDir &resourcesDir)
    : m_resolver(resourcesDir)
{
}

void ChallengeEngine::loadProfile(const ChallengeProfile &profile)
{
    m_profile = profile;
    reload();
}

void ChallengeEngine::reload()
{
    m_worked.clear();
    for (const QString &b : m_profile.bands)
        m_worked[b.toLower()] = QSet<int>();

    m_qsoCount = 0;
    m_unresolved = 0;

    if (!m_profile.hasAdif())
        return;
    QFileInfo fi(m_profile.adifPath);
    if (!fi.exists())
        return;

    QSet<QString> modes;
    for (const QString &m : m_profile.modes)
        modes.insert(m.toUpper());

    const QString text = adifReadTextFlexible(m_profile.adifPath);
    const auto records = adifIterRecords(text);

    for (const AdifFields &f : records) {
        const QString d = adifNormalizeDate(f.value("QSO_DATE"));
        if (d.isEmpty() || !(m_profile.startDate <= d && d <= m_profile.endDate))
            continue;

        const QString band = f.value("BAND").trimmed().toLower();
        if (!m_worked.contains(band))
            continue;

        const QString mode = f.value("MODE").trimmed().toUpper();
        if (!modes.isEmpty() && !modes.contains(mode))
            continue;

        m_qsoCount++;

        const QString dxccText = f.value("DXCC").trimmed();
        bool ok = false;
        int code = dxccText.toInt(&ok);
        if (!ok || code <= 0) {
            const auto resolved = m_resolver.resolveCall(f.value("CALL"));
            code = resolved.dxcc;
        }

        if (code >= 0 && m_resolver.entities().contains(code))
            m_worked[band].insert(code);
        else
            m_unresolved++;
    }
}

bool ChallengeEngine::markWorkedLive(int dxcc, const QString &band)
{
    const QString key = band.trimmed().toLower();
    if (!m_worked.contains(key) || !m_resolver.entities().contains(dxcc))
        return false;
    if (m_worked[key].contains(dxcc))
        return false;
    m_worked[key].insert(dxcc);
    m_qsoCount++;
    return true;
}

QVector<BandState> ChallengeEngine::bandStates() const
{
    const int total = m_resolver.entities().size();
    QVector<BandState> result;
    result.reserve(m_profile.bands.size());
    for (const QString &b : m_profile.bands) {
        const int worked = m_worked.value(b.toLower()).size();
        result.append(BandState{b, worked, total - worked, total});
    }
    return result;
}

QVector<MissingEntity> ChallengeEngine::missingEntities(const QString &band) const
{
    const QSet<int> worked = m_worked.value(band.toLower());
    QVector<MissingEntity> result;
    const auto &entities = m_resolver.entities();
    for (auto it = entities.constBegin(); it != entities.constEnd(); ++it) {
        if (!worked.contains(it.key()))
            result.append(MissingEntity{it.key(), m_resolver.prefixForDxcc(it.key()), it.value()});
    }
    std::sort(result.begin(), result.end(), [](const MissingEntity &a, const MissingEntity &b) {
        return a.dxcc < b.dxcc;
    });
    return result;
}

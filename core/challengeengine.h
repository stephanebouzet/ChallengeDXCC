#pragma once
// Portage C++ de core/challenge_engine.py

#include "core/dxcc.h"
#include "core/models.h"

#include <QSet>
#include <QMap>
#include <QVector>
#include <QDir>

class ChallengeEngine
{
public:
    explicit ChallengeEngine(const QDir &resourcesDir);

    const DXCCResolver &resolver() const { return m_resolver; }

    void loadProfile(const ChallengeProfile &profile);
    void reload();

    // Marque immédiatement un couple DXCC/bande comme travaillé suite à un
    // QSO live (Logger32/JTDX). Retourne true si le slot était manquant.
    bool markWorkedLive(int dxcc, const QString &band);

    QVector<BandState> bandStates() const;
    QVector<MissingEntity> missingEntities(const QString &band) const;

    int qsoCount() const { return m_qsoCount; }
    int unresolved() const { return m_unresolved; }
    const ChallengeProfile &profile() const { return m_profile; }

private:
    DXCCResolver m_resolver;
    ChallengeProfile m_profile;
    QMap<QString, QSet<int>> m_worked; // bande (minuscule) -> codes DXCC travaillés
    int m_qsoCount = 0;
    int m_unresolved = 0;
};

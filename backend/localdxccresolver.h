#pragma once
// Portage C++ de LocalDXCCResolver (services/v10_backend.py).
// CALL -> préfixe -> pays -> numéro DXCC, entièrement local.

#include "core/adif.h"

#include <QString>
#include <QMap>
#include <QVector>
#include <QPair>

class LocalDXCCResolver
{
public:
    explicit LocalDXCCResolver(const QString &prefixPath, const QString &entitiesPath,
                                const QString &aliasesPath);

    bool loaded() const { return m_loaded; }
    QString lastError() const { return m_lastError; }
    int prefixCount() const { return m_prefixes.size(); }

    const QMap<int, QString> &entitiesByCode() const { return m_entitiesByCode; }

    QPair<int, QString> countryToDxcc(const QString &country) const;

    struct Resolution {
        bool found = false;
        QString call, candidate, prefix, country, officialCountry;
        int dxcc = -1;
    };
    Resolution resolveCall(const QString &call) const;

    // Complète en place DXCC/COUNTRY. Retourne :
    // "adif" | "local" | "country-only" | "not-found"
    QString enrichFields(AdifFields &fields) const;

private:
    void loadEntities(const QString &entitiesPath);
    void loadAliases(const QString &aliasesPath);
    void loadPrefixes(const QString &prefixPath);
    static QStringList prefixVariants(const QString &raw);
    static QStringList callCandidates(const QString &call);

    QVector<QPair<QString, QString>> m_prefixes; // (préfixe, pays), trié par longueur décroissante
    QMap<QString, QPair<int, QString>> m_entitiesByName; // clé normalisée -> (code, nom)
    QMap<int, QString> m_entitiesByCode;
    QMap<QString, QString> m_aliasesByNorm; // clé normalisée source -> clé normalisée cible
    bool m_loaded = false;
    QString m_lastError;
};

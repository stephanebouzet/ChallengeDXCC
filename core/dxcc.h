#pragma once
// Portage C++ de core/dxcc.py
//
// Les données (entités DXCC / alias) sont chargées depuis
// resources/dxcc_entities.txt et resources/dxcc_aliases.txt plutôt que
// compilées en dur, pour rester éditables sans recompilation.

#include <QString>
#include <QMap>
#include <QVector>
#include <QPair>
#include <QDir>

// Normalisation générique (accents, ponctuation) utilisée pour comparer
// des chaînes de façon robuste.
QString dxccNorm(const QString &s);

// Normalisation dédiée au raccord fichier préfixes <-> noms ADIF.
QString dxccEntityKey(const QString &s);

class DXCCResolver
{
public:
    // resourcesDir doit contenir dxcc_entities.txt, dxcc_aliases.txt et
    // dxcc_prefixes.txt.
    explicit DXCCResolver(const QDir &resourcesDir);

    // code DXCC -> nom officiel
    const QMap<int, QString> &entities() const { return m_entities; }

    QString prefixForDxcc(int dxcc) const;

    // Renvoie (code, nom officiel) ou (-1, QString()) si non trouvé.
    QPair<int, QString> resolveCountry(const QString &country) const;

    // Renvoie (code DXCC ou -1, pays, préfixe utilisé).
    struct CallResolution { int dxcc = -1; QString country; QString prefix; };
    CallResolution resolveCall(const QString &call) const;

private:
    void loadEntities(const QDir &resourcesDir);
    void loadAliases(const QDir &resourcesDir);
    void loadPrefixes(const QDir &resourcesDir);
    void buildPrimaryPrefixes();

    QMap<int, QString> m_entities;              // code -> nom
    QMap<QString, QPair<int, QString>> m_byName; // clé normalisée -> (code, nom)
    QMap<QString, QString> m_aliases;            // source (brut) -> cible (brute)
    QVector<QPair<QString, QString>> m_prefixes; // (préfixe, pays) trié par longueur décroissante
    QMap<int, QString> m_primaryPrefix;          // code -> préfixe représentatif
};

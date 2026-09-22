#include "backend/localdxccresolver.h"
#include "backend/backendcommon.h"
#include "core/dxcc.h" // dxccNorm()

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <algorithm>

LocalDXCCResolver::LocalDXCCResolver(const QString &prefixPath, const QString &entitiesPath,
                                      const QString &aliasesPath)
{
    loadEntities(entitiesPath);
    loadAliases(aliasesPath);
    loadPrefixes(prefixPath);
}

void LocalDXCCResolver::loadEntities(const QString &entitiesPath)
{
    QFile f(entitiesPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        const int sep = line.indexOf('|');
        if (sep < 0)
            continue;
        bool ok = false;
        const int code = line.left(sep).trimmed().toInt(&ok);
        if (!ok)
            continue;
        const QString name = line.mid(sep + 1).trimmed();
        m_entitiesByCode[code] = name;
        m_entitiesByName[dxccNorm(name)] = qMakePair(code, name);
    }
}

void LocalDXCCResolver::loadAliases(const QString &aliasesPath)
{
    QFile f(aliasesPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        const int sep = line.indexOf('|');
        if (sep < 0)
            continue;
        const QString source = line.left(sep);
        const QString target = line.mid(sep + 1).trimmed();
        m_aliasesByNorm[dxccNorm(source)] = dxccNorm(target);
    }
}

QStringList LocalDXCCResolver::prefixVariants(const QString &rawPrefix)
{
    const QString p = rawPrefix.trimmed().toUpper();
    QStringList variants;
    QString noSlash = p;
    noSlash.remove('\\');
    if (!noSlash.isEmpty())
        variants << noSlash;
    if (p.contains('\\')) {
        const QString head = p.section('\\', 0, 0);
        if (!head.isEmpty() && !variants.contains(head))
            variants << head;
    }
    return variants;
}

void LocalDXCCResolver::loadPrefixes(const QString &prefixPath)
{
    m_prefixes.clear();
    m_loaded = false;

    QFile f(prefixPath);
    if (!f.exists()) {
        m_lastError = "fichier absent : " + prefixPath;
        return;
    }
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = f.errorString();
        return;
    }

    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#') || !line.contains('='))
            continue;
        if (line.toLower().contains("(deleted"))
            continue;
        const int eq = line.indexOf('=');
        const QString rawPrefix = line.left(eq);
        const QString country = line.mid(eq + 1).trimmed();
        for (const QString &prefix : prefixVariants(rawPrefix))
            m_prefixes.append(qMakePair(prefix, country));
    }

    std::sort(m_prefixes.begin(), m_prefixes.end(),
              [](const QPair<QString, QString> &a, const QPair<QString, QString> &b) {
                  return a.first.length() > b.first.length();
              });
    m_loaded = !m_prefixes.isEmpty();
    if (!m_loaded)
        m_lastError = "aucun préfixe exploitable";
}

QStringList LocalDXCCResolver::callCandidates(const QString &callIn)
{
    QString call = bcClean(callIn).toUpper();
    if (call.isEmpty())
        return {};

    QStringList candidates;
    candidates << call;
    if (call.contains('/')) {
        for (const QString &p : call.split('/', Qt::SkipEmptyParts))
            candidates << p;
    }

    static const QRegularExpression nonAlnum("[^A-Z0-9]");
    QStringList out;
    for (QString item : candidates) {
        item.remove(nonAlnum);
        if (!item.isEmpty() && !out.contains(item))
            out << item;
    }
    return out;
}

QPair<int, QString> LocalDXCCResolver::countryToDxcc(const QString &country) const
{
    QString n = dxccNorm(country);
    auto aliasIt = m_aliasesByNorm.constFind(n);
    if (aliasIt != m_aliasesByNorm.constEnd())
        n = aliasIt.value();

    auto it = m_entitiesByName.constFind(n);
    if (it != m_entitiesByName.constEnd())
        return it.value();
    return qMakePair(-1, QString());
}

LocalDXCCResolver::Resolution LocalDXCCResolver::resolveCall(const QString &call) const
{
    Resolution best;
    if (!m_loaded)
        return best;

    for (const QString &candidate : callCandidates(call)) {
        for (const auto &item : m_prefixes) {
            if (candidate.startsWith(item.first)) {
                const auto dxccName = countryToDxcc(item.second);
                if (!best.found || item.first.length() > best.prefix.length()) {
                    best.found = true;
                    best.call = bcClean(call).toUpper();
                    best.candidate = candidate;
                    best.prefix = item.first;
                    best.country = item.second;
                    best.dxcc = dxccName.first;
                    best.officialCountry = dxccName.second;
                }
                break; // liste triée par longueur : premier match = meilleur pour ce candidat
            }
        }
    }
    return best;
}

QString LocalDXCCResolver::enrichFields(AdifFields &fields) const
{
    const QString dxccText = bcClean(fields.value("DXCC"));
    bool ok = false;
    const int dxcc = dxccText.toInt(&ok);
    if (ok && dxcc > 0)
        return "adif";

    const Resolution result = resolveCall(fields.value("CALL"));
    if (!result.found)
        return "not-found";

    if (!result.country.isEmpty())
        fields["COUNTRY"] = result.country;

    if (result.dxcc >= 0) {
        fields["DXCC"] = QString::number(result.dxcc);
        fields["_DXCC_PREFIX"] = result.prefix;
        return "local";
    }
    return "country-only";
}

#include "core/dxcc.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

static QString stripCombining(const QString &s)
{
    const QString decomposed = s.normalized(QString::NormalizationForm_D);
    QString out;
    out.reserve(decomposed.size());
    for (const QChar &ch : decomposed) {
        if (ch.category() != QChar::Mark_NonSpacing)
            out.append(ch);
    }
    return out;
}

QString dxccNorm(const QString &s)
{
    QString upper = s.toUpper();
    upper = stripCombining(upper);
    upper.replace('&', " AND ");
    upper.remove('\'');
    static const QRegularExpression nonAlnum("[^A-Z0-9]+");
    upper.replace(nonAlnum, " ");
    return upper.simplified();
}

QString dxccEntityKey(const QString &s)
{
    const QStringList tokens = dxccNorm(s).split(' ', Qt::SkipEmptyParts);
    QStringList result;
    result.reserve(tokens.size());
    for (QString token : tokens) {
        if (token == "ISLAND" || token == "ISLANDS" || token == "I")
            token = "IS";
        else if (token == "REPUBLIC")
            token = "REP";
        result.append(token);
    }
    return result.join(' ');
}

DXCCResolver::DXCCResolver(const QDir &resourcesDir)
{
    loadEntities(resourcesDir);
    loadAliases(resourcesDir);
    loadPrefixes(resourcesDir);
    buildPrimaryPrefixes();
}

void DXCCResolver::loadEntities(const QDir &resourcesDir)
{
    QFile f(resourcesDir.filePath("dxcc_entities.txt"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        const int sep = line.indexOf('|');
        if (sep < 0)
            continue;
        bool ok = false;
        const int code = line.left(sep).toInt(&ok);
        if (!ok)
            continue;
        const QString name = line.mid(sep + 1).trimmed();
        m_entities[code] = name;
        m_byName[dxccEntityKey(name)] = qMakePair(code, name);
    }
}

void DXCCResolver::loadAliases(const QDir &resourcesDir)
{
    QFile f(resourcesDir.filePath("dxcc_aliases.txt"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        const int sep = line.indexOf('|');
        if (sep < 0)
            continue;
        m_aliases[line.left(sep)] = line.mid(sep + 1).trimmed();
    }
}

void DXCCResolver::loadPrefixes(const QDir &resourcesDir)
{
    QFile f(resourcesDir.filePath("dxcc_prefixes.txt"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#') || !line.contains('='))
            continue;
        if (line.toLower().contains("(deleted"))
            continue;
        const int eq = line.indexOf('=');
        QString prefix = line.left(eq).trimmed();
        // Retirer un éventuel BOM résiduel puis les antislashs.
        while (!prefix.isEmpty() && prefix.at(0) == QChar(0xFEFF))
            prefix.remove(0, 1);
        prefix = prefix.toUpper();
        prefix.remove('\\');
        const QString country = line.mid(eq + 1).trimmed();
        if (!prefix.isEmpty())
            m_prefixes.append(qMakePair(prefix, country));
    }
    std::sort(m_prefixes.begin(), m_prefixes.end(),
              [](const QPair<QString, QString> &a, const QPair<QString, QString> &b) {
                  return a.first.length() > b.first.length();
              });
}

void DXCCResolver::buildPrimaryPrefixes()
{
    QMap<int, QSet<QString>> candidates;
    for (const auto &item : m_prefixes) {
        const auto resolved = resolveCountry(item.second);
        if (resolved.first >= 0)
            candidates[resolved.first].insert(item.first);
    }
    for (auto it = candidates.constBegin(); it != candidates.constEnd(); ++it) {
        QStringList clean;
        for (const QString &v : it.value()) {
            if (!v.isEmpty() && v.at(0).isLetterOrNumber())
                clean.append(v);
        }
        if (clean.isEmpty())
            continue;
        std::sort(clean.begin(), clean.end(), [](const QString &a, const QString &b) {
            if (a.length() != b.length())
                return a.length() < b.length();
            return a < b;
        });
        m_primaryPrefix[it.key()] = clean.first();
    }
}

QString DXCCResolver::prefixForDxcc(int dxcc) const
{
    return m_primaryPrefix.value(dxcc);
}

QPair<int, QString> DXCCResolver::resolveCountry(const QString &country) const
{
    const QString key = dxccEntityKey(country);
    auto it = m_byName.constFind(key);
    if (it != m_byName.constEnd())
        return it.value();

    for (auto ait = m_aliases.constBegin(); ait != m_aliases.constEnd(); ++ait) {
        if (dxccEntityKey(ait.key()) == key) {
            auto target = m_byName.constFind(dxccEntityKey(ait.value()));
            if (target != m_byName.constEnd())
                return target.value();
            break;
        }
    }
    return qMakePair(-1, QString());
}

DXCCResolver::CallResolution DXCCResolver::resolveCall(const QString &callIn) const
{
    static const QRegularExpression stripRe("[^A-Z0-9/]");
    QString call = callIn.toUpper();
    call.replace(stripRe, "");

    QStringList candidates;
    candidates << call;
    for (const QString &part : call.split('/', Qt::SkipEmptyParts))
        candidates << part;

    for (const QString &candidate : candidates) {
        for (const auto &item : m_prefixes) {
            if (candidate.startsWith(item.first)) {
                const auto resolved = resolveCountry(item.second);
                CallResolution result;
                result.dxcc = resolved.first;
                result.country = item.second;
                result.prefix = item.first;
                return result;
            }
        }
    }
    return CallResolution{};
}

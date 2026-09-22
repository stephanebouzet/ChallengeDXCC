#include "core/adif.h"

#include <QFile>
#include <QStringDecoder>
#include <QStringConverter>

QString adifReadTextFlexible(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    const QByteArray raw = file.readAll();

    // utf-8-sig : QStringDecoder gère nativement le BOM UTF-8.
    {
        QStringDecoder decoder(QStringConverter::Utf8);
        QString text = decoder.decode(raw);
        if (!decoder.hasError())
            return text;
    }

    // Repli Windows-1252, puis latin-1 (toujours valide).
    {
        QStringDecoder decoder("Windows-1252");
        QString text = decoder.decode(raw);
        if (!decoder.hasError())
            return text;
    }

    QStringDecoder latin1(QStringConverter::Latin1);
    return latin1.decode(raw);
}

QVector<AdifFields> adifIterRecords(const QString &text)
{
    QVector<AdifFields> records;
    AdifFields fields;

    int pos = 0;
    const int len = text.length();

    while (pos < len) {
        int lt = text.indexOf('<', pos);
        if (lt < 0)
            break;
        int gt = text.indexOf('>', lt + 1);
        if (gt < 0)
            break;

        const QString tag = text.mid(lt + 1, gt - lt - 1).trimmed();
        const QString upper = tag.toUpper();

        if (upper == QLatin1String("EOH")) {
            fields.clear();
            pos = gt + 1;
            continue;
        }
        if (upper == QLatin1String("EOR")) {
            if (!fields.isEmpty())
                records.append(fields);
            fields.clear();
            pos = gt + 1;
            continue;
        }

        const QStringList parts = tag.split(':');
        if (parts.size() >= 2) {
            bool ok = false;
            const int length = parts.at(1).toInt(&ok);
            if (!ok) {
                pos = gt + 1;
                continue;
            }
            const int start = gt + 1;
            const int end = start + length;
            if (end > len)
                break;
            fields[parts.at(0).toUpper()] = text.mid(start, length);
            pos = end;
        } else {
            pos = gt + 1;
        }
    }

    return records;
}

QString adifNormalizeDate(const QString &value)
{
    const QString v = value.trimmed();
    if (v.length() >= 8) {
        bool allDigits = true;
        for (int i = 0; i < 8; ++i) {
            if (!v.at(i).isDigit()) { allDigits = false; break; }
        }
        if (allDigits)
            return v.left(4) + "-" + v.mid(4, 2) + "-" + v.mid(6, 2);
    }
    return QString();
}

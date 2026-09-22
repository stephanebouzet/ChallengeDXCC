#include "backend/backendcommon.h"

#include <QProcessEnvironment>
#include <QRegularExpression>

const QStringList CHALLENGE_BANDS = {
    "160m", "80m", "40m", "30m", "20m", "17m", "15m", "12m", "10m"
};

AdifFields backendParseAdif(const QString &text)
{
    static const QRegularExpression tagRe(
        R"(<([^:>\s]+):(\d+)(?::[^>]*)?>)",
        QRegularExpression::CaseInsensitiveOption);

    AdifFields fields;
    int pos = 0;
    while (true) {
        auto match = tagRe.match(text, pos);
        if (!match.hasMatch())
            break;
        const QString name = match.captured(1).toUpper();
        const int length = match.captured(2).toInt();
        const int start = match.capturedEnd(0);
        const int end = start + length;
        if (end > text.length())
            break;
        fields[name] = text.mid(start, length);
        pos = end;
    }
    return fields;
}

bool backendPlausibleCallsign(const QString &callIn)
{
    const QString call = bcClean(callIn).toUpper();
    if (call.length() < 3)
        return false;
    bool hasAlpha = false, hasDigit = false;
    for (const QChar &ch : call) {
        if (ch.isLetter()) hasAlpha = true;
        if (ch.isDigit()) hasDigit = true;
    }
    return hasAlpha && hasDigit;
}

QPair<QDate, QDate> backendChallengePeriod()
{
    const auto env = QProcessEnvironment::systemEnvironment();
    const QString envStart = env.value("CHALLENGE_START_DATE").trimmed();
    const QString envEnd = env.value("CHALLENGE_END_DATE").trimmed();

    if (!envStart.isEmpty() && !envEnd.isEmpty()) {
        const QDate start = QDate::fromString(envStart, "yyyy-MM-dd");
        const QDate end = QDate::fromString(envEnd, "yyyy-MM-dd");
        if (start.isValid() && end.isValid())
            return qMakePair(start, end);
    }

    const QDate today = QDate::currentDate();
    if (today.month() >= 9)
        return qMakePair(QDate(today.year(), 9, 1), QDate(today.year() + 1, 8, 31));
    return qMakePair(QDate(today.year() - 1, 9, 1), QDate(today.year(), 8, 31));
}

QString backendAdifDateToIso(const QString &valueIn)
{
    const QString value = bcClean(valueIn);
    if (value.length() < 8)
        return QString();
    for (int i = 0; i < 8; ++i) {
        if (!value.at(i).isDigit())
            return QString();
    }
    return value.left(4) + "-" + value.mid(4, 2) + "-" + value.mid(6, 2);
}

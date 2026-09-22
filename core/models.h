#pragma once
// Portage C++ de core/models.py

#include <QString>
#include <QStringList>

inline const QStringList &defaultBands()
{
    static const QStringList bands = {
        "160m", "80m", "40m", "30m", "20m", "17m", "15m", "12m", "10m"
    };
    return bands;
}

struct ChallengeProfile
{
    QString name = QStringLiteral("Nouveau Challenge");
    QString adifPath;
    QString startDate = QStringLiteral("2025-09-01");
    QString endDate = QStringLiteral("2026-08-31");
    QStringList bands = defaultBands();
    QStringList modes;

    bool hasAdif() const { return !adifPath.isEmpty(); }
};

struct BandState
{
    QString band;
    int worked = 0;
    int missing = 0;
    int total = 0;

    double percent() const
    {
        return total == 0 ? 0.0 : 100.0 * worked / total;
    }
};

struct MissingEntity
{
    int dxcc = 0;
    QString prefix;
    QString name;
};

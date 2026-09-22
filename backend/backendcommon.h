#pragma once
// Fonctions utilitaires communes, portées de services/v10_backend.py.

#include "core/adif.h"

#include <QString>
#include <QDate>
#include <QPair>

// Équivalent de clean(value) : jamais nul, toujours trimé.
inline QString bcClean(const QString &value)
{
    return value.trimmed();
}

// Table d'entités DXCC actives (code -> nom officiel).
extern const QStringList CHALLENGE_BANDS;

// parse_adif(text) : identique à adifIterRecords mais pour un texte
// contenant un seul enregistrement (ex : ligne WM_SETTEXT de Logger32).
AdifFields backendParseAdif(const QString &text);

// plausible_callsign(call)
bool backendPlausibleCallsign(const QString &call);

// challenge_period() : lit CHALLENGE_START_DATE / CHALLENGE_END_DATE dans
// l'environnement, sinon applique la règle 1er septembre -> 31 août.
QPair<QDate, QDate> backendChallengePeriod();

// adif_date_to_iso(value)
QString backendAdifDateToIso(const QString &value);

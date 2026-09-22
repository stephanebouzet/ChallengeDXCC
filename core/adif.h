#pragma once
// Portage C++ de core/adif.py

#include <QString>
#include <QMap>
#include <QVector>

using AdifFields = QMap<QString, QString>;

// Lecture tolérante d'un export ADIF (utf-8-sig / cp1252 / latin-1).
QString adifReadTextFlexible(const QString &path);

// Découpe un texte ADIF complet en enregistrements (un par QSO).
QVector<AdifFields> adifIterRecords(const QString &text);

// Convertit une date ADIF (YYYYMMDD) en ISO (YYYY-MM-DD).
QString adifNormalizeDate(const QString &value);

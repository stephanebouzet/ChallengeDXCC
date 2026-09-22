/****************************************************************************
** Meta object code from reading C++ file 'challengebackend.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../backend/challengebackend.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'challengebackend.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN16ChallengeBackendE_t {};
} // unnamed namespace

template <> constexpr inline auto ChallengeBackend::qt_create_metaobjectdata<qt_meta_tag_ZN16ChallengeBackendE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ChallengeBackend",
        "logLine",
        "",
        "line",
        "serviceStateChanged",
        "serviceId",
        "active",
        "detail",
        "qsoAdded",
        "QJsonObject",
        "fields",
        "challengePeriod",
        "dxSpotLine",
        "formattedLine",
        "dxSpotAccepted",
        "alert",
        "reportsUpdated",
        "tryConnectLogger32",
        "pollDeletedWatcher"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'logLine'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'serviceStateChanged'
        QtMocHelpers::SignalData<void(const QString &, bool, const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 5 }, { QMetaType::Bool, 6 }, { QMetaType::QString, 7 },
        }}),
        // Signal 'qsoAdded'
        QtMocHelpers::SignalData<void(const QJsonObject &, bool)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 10 }, { QMetaType::Bool, 11 },
        }}),
        // Signal 'dxSpotLine'
        QtMocHelpers::SignalData<void(const QString &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 13 },
        }}),
        // Signal 'dxSpotAccepted'
        QtMocHelpers::SignalData<void(const QJsonObject &, const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 15 }, { QMetaType::QString, 13 },
        }}),
        // Signal 'reportsUpdated'
        QtMocHelpers::SignalData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'tryConnectLogger32'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'pollDeletedWatcher'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ChallengeBackend, qt_meta_tag_ZN16ChallengeBackendE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ChallengeBackend::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16ChallengeBackendE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16ChallengeBackendE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN16ChallengeBackendE_t>.metaTypes,
    nullptr
} };

void ChallengeBackend::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ChallengeBackend *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->logLine((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->serviceStateChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 2: _t->qsoAdded((*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 3: _t->dxSpotLine((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->dxSpotAccepted((*reinterpret_cast<std::add_pointer_t<QJsonObject>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 5: _t->reportsUpdated(); break;
        case 6: _t->tryConnectLogger32(); break;
        case 7: _t->pollDeletedWatcher(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ChallengeBackend::*)(const QString & )>(_a, &ChallengeBackend::logLine, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChallengeBackend::*)(const QString & , bool , const QString & )>(_a, &ChallengeBackend::serviceStateChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChallengeBackend::*)(const QJsonObject & , bool )>(_a, &ChallengeBackend::qsoAdded, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChallengeBackend::*)(const QString & )>(_a, &ChallengeBackend::dxSpotLine, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChallengeBackend::*)(const QJsonObject & , const QString & )>(_a, &ChallengeBackend::dxSpotAccepted, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (ChallengeBackend::*)()>(_a, &ChallengeBackend::reportsUpdated, 5))
            return;
    }
}

const QMetaObject *ChallengeBackend::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChallengeBackend::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN16ChallengeBackendE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ChallengeBackend::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void ChallengeBackend::logLine(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void ChallengeBackend::serviceStateChanged(const QString & _t1, bool _t2, const QString & _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2, _t3);
}

// SIGNAL 2
void ChallengeBackend::qsoAdded(const QJsonObject & _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void ChallengeBackend::dxSpotLine(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void ChallengeBackend::dxSpotAccepted(const QJsonObject & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void ChallengeBackend::reportsUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP

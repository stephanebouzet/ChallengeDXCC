#include "backend/logger32bridge.h"
#include "backend/backendcommon.h"

#include <QSet>
#include <QDateTime>

#ifdef CHALLENGEDXCC_WINDOWS
#include <windows.h>
#include <QElapsedTimer>
#include <QCoreApplication>

namespace {
const wchar_t *kClassName = L"ChallengeDXCCReceiverWindow";
}

struct Logger32Bridge::Impl
{
    HWND hwnd = nullptr;
    HWND loggerHwnd = nullptr;
    UINT l32Msg = 0;
    bool classRegistered = false;
};

static LRESULT CALLBACK bridgeWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    auto *self = reinterpret_cast<Logger32Bridge *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (self && self->propertyBag()->l32Msg != 0 && msg == self->propertyBag()->l32Msg) {
        const int command = static_cast<int>(wparam);
        const long long value = static_cast<long long>(lparam);
        self->onLogger32Message(command, value, hwnd);
        return 0;
    }

    if (self && msg == WM_SETTEXT) {
        QString text;
        if (lparam)
            text = QString::fromWCharArray(reinterpret_cast<const wchar_t *>(lparam));
        self->onWmSetText(text);
        return 1;
    }

    if (msg == WM_DESTROY)
        return 0;

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static QString getWindowTitle(HWND hwnd)
{
    const int length = GetWindowTextLengthW(hwnd);
    if (length <= 0)
        return QString();
    QVector<wchar_t> buf(length + 1);
    GetWindowTextW(hwnd, buf.data(), buf.size());
    return QString::fromWCharArray(buf.data());
}

static BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lparam)
{
    auto *out = reinterpret_cast<QPair<HWND, QString> *>(lparam);
    if (!IsWindowVisible(hwnd))
        return TRUE;
    const QString title = getWindowTitle(hwnd);
    if (title.trimmed().compare("logger32", Qt::CaseInsensitive) == 0) {
        *out = qMakePair(hwnd, title);
        return FALSE; // correspondance exacte : on arrête la recherche
    }
    if (out->first == nullptr && title.contains("logger32", Qt::CaseInsensitive))
        *out = qMakePair(hwnd, title); // correspondance partielle, on continue au cas où une exacte existe
    return TRUE;
}

#endif // CHALLENGEDXCC_WINDOWS

Logger32Bridge::Logger32Bridge(QObject *parent)
    : QObject(parent)
{
#ifdef CHALLENGEDXCC_WINDOWS
    m_impl = new Impl();
#endif
}

Logger32Bridge::~Logger32Bridge()
{
    disconnectAll();
#ifdef CHALLENGEDXCC_WINDOWS
    delete m_impl;
#endif
}

bool Logger32Bridge::platformSupported()
{
#ifdef CHALLENGEDXCC_WINDOWS
    return true;
#else
    return false;
#endif
}

#ifdef CHALLENGEDXCC_WINDOWS

bool Logger32Bridge::connectToLogger32()
{
    HINSTANCE hinstance = GetModuleHandleW(nullptr);

    if (!m_impl->classRegistered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = bridgeWndProc;
        wc.hInstance = hinstance;
        wc.lpszClassName = kClassName;

        const ATOM atom = RegisterClassW(&wc);
        if (!atom) {
            const DWORD error = GetLastError();
            if (error != ERROR_CLASS_ALREADY_EXISTS) {
                emit logLine(QString("[WARN] Échec RegisterClassW (code %1)").arg(error));
                return false;
            }
        } else {
            m_impl->classRegistered = true;
        }
    }

    m_impl->hwnd = CreateWindowExW(0, kClassName, L"Logger32 Challenge DXCC Cpp", 0,
                                    0, 0, 0, 0, nullptr, nullptr, hinstance, nullptr);
    if (!m_impl->hwnd) {
        emit logLine(QString("[WARN] Échec CreateWindowExW (code %1)").arg(GetLastError()));
        return false;
    }
    SetWindowLongPtrW(m_impl->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    HWND loggerHwnd = nullptr;
    QString loggerTitle;
    QPair<HWND, QString> found(nullptr, QString());
    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&found));
    loggerHwnd = found.first;
    loggerTitle = found.second;

    if (!loggerHwnd) {
        emit logLine("Logger32 détecté : NON");
        return false;
    }

    m_impl->loggerHwnd = loggerHwnd;
    m_loggerTitle = loggerTitle;
    emit logLine("Logger32 détecté : OUI (" + loggerTitle + ")");

    for (int channel = 1; channel <= 5; ++channel) {
        m_connected = false;
        const QString name = QString("Logger32 %1").arg(channel);
        const UINT msgId = RegisterWindowMessageW(reinterpret_cast<const wchar_t *>(name.utf16()));
        if (!msgId)
            continue;
        m_impl->l32Msg = msgId;

        emit logLine(QString("Essai canal %1...").arg(channel));
        const BOOL ok = PostMessageW(m_impl->loggerHwnd, msgId, 1, reinterpret_cast<LPARAM>(m_impl->hwnd));
        if (!ok)
            continue;

        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 1000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
            if (m_connected) {
                emit logLine(QString("Canal %1 : OK").arg(channel));
                emit stateChanged(true, loggerTitle);
                return true;
            }
        }
    }

    emit logLine("Logger32 : aucun canal n'a répondu.");
    return false;
}

void Logger32Bridge::disconnectAll()
{
    if (!m_impl)
        return;
    if (m_impl->hwnd) {
        DestroyWindow(m_impl->hwnd);
        m_impl->hwnd = nullptr;
    }
    if (m_impl->classRegistered) {
        UnregisterClassW(kClassName, GetModuleHandleW(nullptr));
        m_impl->classRegistered = false;
    }
    m_connected = false;
}

void Logger32Bridge::onLogger32Message(int command, long long value, void *replyHwnd)
{
    if (command == 0) {
        if (value == 1) {
            m_connected = true;
            PostMessageW(m_impl->loggerHwnd, m_impl->l32Msg, 2, reinterpret_cast<LPARAM>(m_impl->hwnd));
        }
    } else if (command == 100) {
        m_frequencyHz = value;
    }
    Q_UNUSED(replyHwnd);
}

void Logger32Bridge::onWmSetText(const QString &text)
{
    processLoggerText(text);
}

#else // !CHALLENGEDXCC_WINDOWS

bool Logger32Bridge::connectToLogger32()
{
    emit logLine("Logger32      : interface Win32 indisponible sur cette plateforme.");
    return false;
}

void Logger32Bridge::disconnectAll()
{
}

void Logger32Bridge::onLogger32Message(int, long long, void *)
{
}

void Logger32Bridge::onWmSetText(const QString &)
{
}

#endif

void Logger32Bridge::processLoggerText(const QString &textIn)
{
    const QString text = textIn.trimmed();
    if (text.isEmpty())
        return;

    const QString upper = text.toUpper();
    if (upper == "<EOR>") {
        commitQso();
        return;
    }
    if (upper == "<EOF>")
        return;

    const AdifFields fields = backendParseAdif(text);
    if (fields.isEmpty())
        return;

    if (fields.contains("APP_CLUBLOG_DUMP_FILENAME")) {
        const QString path = bcClean(fields.value("APP_CLUBLOG_DUMP_FILENAME"));
        if (!path.isEmpty())
            emit deletedDumpPathReceived(path);
        return;
    }

    static const QSet<QString> qsoRelated = {
        "CALL", "RST_SENT", "RST_RCVD", "NAME", "MODE", "SUBMODE", "BAND", "FREQ",
        "GRIDSQUARE", "IOTA", "APP_COUNTRY", "COUNTRY", "DXCC", "CONT", "CQZ", "ITUZ",
        "QSO_DATE", "TIME_ON", "APP_LOGGER32_QSO_NUMBER",
    };

    bool any = false;
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
        if (qsoRelated.contains(it.key())) {
            m_currentQso[it.key()] = bcClean(it.value());
            any = true;
        }
    }
    Q_UNUSED(any);
}

void Logger32Bridge::commitQso()
{
    const QString call = bcClean(m_currentQso.value("CALL")).toUpper();
    if (!backendPlausibleCallsign(call)) {
        m_currentQso.clear();
        return;
    }

    if (bcClean(m_currentQso.value("QSO_DATE")).isEmpty())
        m_currentQso["QSO_DATE"] = QDateTime::currentDateTime().toString("yyyyMMdd");
    if (bcClean(m_currentQso.value("TIME_ON")).isEmpty())
        m_currentQso["TIME_ON"] = QDateTime::currentDateTime().toString("HHmmss");

    if (bcClean(m_currentQso.value("FREQ")).isEmpty() && m_frequencyHz > 0) {
        m_currentQso["FREQ"] = QString::number(m_frequencyHz / 1000000.0, 'f', 6);
    }

    emit qsoCommitted(m_currentQso);
    m_currentQso.clear();
}

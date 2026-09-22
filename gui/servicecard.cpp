#include "gui/servicecard.h"

#include <QGridLayout>
#include <QSizePolicy>

ServiceCard::ServiceCard(const QString &name, const QString &detail, QWidget *parent)
    : QFrame(parent)
    , m_serviceName(name)
{
    setObjectName("serviceCard");
    setFrameShape(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_dot = new QLabel("●", this);
    m_dot->setFixedWidth(16);
    m_title = new QLabel(name, this);
    m_title->setStyleSheet("font-weight:600; color:#1f2937;");
    m_badge = new QLabel(this);
    m_badge->setAlignment(Qt::AlignCenter);
    m_badge->setMinimumWidth(78);
    m_detail = new QLabel(detail, this);
    m_detail->setStyleSheet("color:#6b7280; font-size:9pt;");

    auto *layout = new QGridLayout(this);
    layout->setContentsMargins(5, 3, 5, 3);
    layout->setHorizontalSpacing(5);
    layout->setVerticalSpacing(2);
    layout->addWidget(m_dot, 0, 0, 2, 1);
    layout->addWidget(m_title, 0, 1);
    layout->addWidget(m_badge, 0, 2, 1, 1, Qt::AlignRight);
    layout->addWidget(m_detail, 1, 1, 1, 2);

    setStyleSheet(
        "QFrame#serviceCard {"
        "background:#ffffff; border:1px solid #d9dee5;"
        "border-radius:7px;}");

    setConnected(false, detail);
}

QString ServiceCard::statusText(bool ok) const
{
    if (!ok)
        return "ARRÊTÉ";
    if (m_serviceName == "JTDX UDP")
        return "PRÊT";
    if (m_serviceName == "Cluster Local")
        return "ACTIF";
    return "CONNECTÉ";
}

void ServiceCard::setConnected(bool ok, const QString &detail)
{
    const QString color = ok ? "#16a34a" : "#c62828";
    const QString bg = ok ? "#eaf7ef" : "#fdecec";
    m_dot->setStyleSheet(QString("font-size:15px; color:%1;").arg(color));
    m_badge->setText(statusText(ok));
    m_badge->setStyleSheet(QString("color:%1; background:%2; font-weight:700;"
                                    "font-size:8pt; border-radius:8px; padding:2px 7px;")
                                .arg(color, bg));
    if (!detail.isNull())
        m_detail->setText(detail);
}

bool NumericTableWidgetItem::operator<(const QTableWidgetItem &other) const
{
    const QVariant left = data(Qt::UserRole);
    const QVariant right = other.data(Qt::UserRole);
    if (left.isValid() && right.isValid()) {
        bool leftOk = false, rightOk = false;
        const qlonglong l = left.toLongLong(&leftOk);
        const qlonglong r = right.toLongLong(&rightOk);
        if (leftOk && rightOk)
            return l < r;
    }
    return QTableWidgetItem::operator<(other);
}

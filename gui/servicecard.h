#pragma once
// Portage C++ de ServiceCard (gui/main_window.py).

#include <QFrame>
#include <QLabel>

class ServiceCard : public QFrame
{
    Q_OBJECT
public:
    explicit ServiceCard(const QString &name, const QString &detail, QWidget *parent = nullptr);

    void setConnected(bool ok, const QString &detail = QString());

    QString serviceName() const { return m_serviceName; }

private:
    QString statusText(bool ok) const;

    QString m_serviceName;
    QLabel *m_dot;
    QLabel *m_title;
    QLabel *m_badge;
    QLabel *m_detail;
};

// QTableWidgetItem trié sur une valeur numérique stockée en Qt::UserRole.
#include <QTableWidgetItem>
class NumericTableWidgetItem : public QTableWidgetItem
{
public:
    using QTableWidgetItem::QTableWidgetItem;
    bool operator<(const QTableWidgetItem &other) const override;
};

#pragma once

#include "core/security/Session.hpp"
#include <QWidget>
#include <QAbstractTableModel>
#include <QVector>
#include <QDate>

QT_BEGIN_NAMESPACE
namespace Ui { class SdWidget; }
QT_END_NAMESPACE

struct SalesOrderRecord {
    int     id;
    QString orderNumber;
    QString customerName;
    QDate   orderDate;
    QString status;
    double  totalGross{0.0};
    QString currency;
};

class SalesOrderModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit SalesOrderModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& = {}) const override { return m_data.size(); }
    int columnCount(const QModelIndex& = {}) const override { return 6; }
    QVariant data(const QModelIndex& idx, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void load(const QString& statusFilter = {}, const QString& search = {});
    [[nodiscard]] int orderIdAt(int row) const;
    [[nodiscard]] QString orderNumberAt(int row) const;
    [[nodiscard]] QString orderStatusAt(int row) const;
private:
    QVector<SalesOrderRecord> m_data;
};

class SdWidget : public QWidget {
    Q_OBJECT
public:
    explicit SdWidget(const Session& session, QWidget *parent = nullptr);
    ~SdWidget() override;

private slots:
    void onNewOrder();
    void onConfirmOrder();
    void onPostInvoice();
    void onRefresh();
    void onOrderSelected(const QModelIndex& idx);
    void onFilterChanged();

private:
    void loadOrders();
    void loadLineItems(int orderId);
    void postInvoiceJournal(int orderId, const QString& orderNumber, double netAmount);

    Ui::SdWidget     *ui;
    SalesOrderModel  *m_model;
    Session           m_session;
};

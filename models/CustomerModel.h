#ifndef CUSTOMERMODEL_H
#define CUSTOMERMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QVariant>
#include <QSqlQuery>

struct CustomerRecord {
    int     id;
    QString customerNumber;
    QString name;
    QString taxId;
    QString email;
    QString phone;
    QString billingAddress;
    double  creditLimit;
    QString currency;
    bool    isActive;
};

class CustomerModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column {
        ColNumber = 0,
        ColName,
        ColTaxId,
        ColEmail,
        ColPhone,
        ColCreditLimit,
        ColStatus,
        ColCount
    };

    explicit CustomerModel(QObject *parent = nullptr);

    // QAbstractTableModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Data management
    void load(const QString &filter = QString());
    const CustomerRecord &record(int row) const;
    bool addCustomer(const CustomerRecord &rec);
    bool updateCustomer(const CustomerRecord &rec);
    bool deactivateCustomer(int id);

private:
    QList<CustomerRecord> m_data;
    QString nextCustomerNumber();
};

#endif // CUSTOMERMODEL_H

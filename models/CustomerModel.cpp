#include "CustomerModel.h"
#include "core/DatabaseManager.h"

#include <QColor>
#include <QFont>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>

CustomerModel::CustomerModel(QObject *parent) : QAbstractTableModel(parent) {}

int CustomerModel::rowCount(const QModelIndex &) const
{
    return m_data.size();
}

int CustomerModel::columnCount(const QModelIndex &) const
{
    return ColCount;
}

QVariant CustomerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case ColNumber:      return tr("Number");
    case ColName:        return tr("Customer Name");
    case ColTaxId:       return tr("Tax ID");
    case ColEmail:       return tr("Email");
    case ColPhone:       return tr("Phone");
    case ColCreditLimit: return tr("Credit Limit");
    case ColStatus:      return tr("Status");
    default:             return {};
    }
}

QVariant CustomerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size())
        return {};

    const CustomerRecord &r = m_data.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColNumber:      return r.customerNumber;
        case ColName:        return r.name;
        case ColTaxId:       return r.taxId;
        case ColEmail:       return r.email;
        case ColPhone:       return r.phone;
        case ColCreditLimit: return QString("%1 %2").arg(r.creditLimit, 0, 'f', 2).arg(r.currency);
        case ColStatus:      return r.isActive ? tr("Active") : tr("Inactive");
        default:             return {};
        }
    }

    if (role == Qt::ForegroundRole && index.column() == ColStatus) {
        return r.isActive ? QColor("#2EA043") : QColor("#F85149");
    }

    if (role == Qt::FontRole && index.column() == ColStatus) {
        QFont f;
        f.setBold(true);
        return f;
    }

    if (role == Qt::TextAlignmentRole && index.column() == ColCreditLimit)
        return int(Qt::AlignRight | Qt::AlignVCenter);

    if (role == Qt::UserRole)
        return r.id;

    return {};
}

void CustomerModel::load(const QString &filter)
{
    beginResetModel();
    m_data.clear();

    QString sql =
        "SELECT id, customer_number, name, tax_id, email, phone, "
        "       billing_address, credit_limit, currency, is_active "
        "FROM customers WHERE is_active = true ";

    if (!filter.isEmpty())
        sql += " AND (LOWER(name) LIKE :f OR customer_number LIKE :f OR email LIKE :f)";

    sql += " ORDER BY name";

    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare(sql);
    if (!filter.isEmpty())
        q.bindValue(":f", "%" + filter.toLower() + "%");

    if (q.exec()) {
        while (q.next()) {
            CustomerRecord rec;
            rec.id             = q.value(0).toInt();
            rec.customerNumber = q.value(1).toString();
            rec.name           = q.value(2).toString();
            rec.taxId          = q.value(3).toString();
            rec.email          = q.value(4).toString();
            rec.phone          = q.value(5).toString();
            rec.billingAddress = q.value(6).toString();
            rec.creditLimit    = q.value(7).toDouble();
            rec.currency       = q.value(8).toString();
            rec.isActive       = q.value(9).toBool();
            m_data.append(rec);
        }
    } else {
        qWarning() << "CustomerModel::load error:" << q.lastError().text();
    }

    endResetModel();
}

const CustomerRecord &CustomerModel::record(int row) const
{
    return m_data.at(row);
}

bool CustomerModel::addCustomer(const CustomerRecord &rec)
{
    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare(
        "INSERT INTO customers "
        "(customer_number, name, tax_id, email, phone, billing_address, "
        " shipping_address, credit_limit, currency, is_active) "
        "VALUES (:num, :name, :tax, :email, :phone, :addr, :addr, :credit, :cur, true)"
    );
    q.bindValue(":num",   nextCustomerNumber());
    q.bindValue(":name",  rec.name);
    q.bindValue(":tax",   rec.taxId);
    q.bindValue(":email", rec.email);
    q.bindValue(":phone", rec.phone);
    q.bindValue(":addr",  rec.billingAddress);
    q.bindValue(":credit",rec.creditLimit);
    q.bindValue(":cur",   rec.currency);

    if (q.exec()) {
        load();
        return true;
    }
    qWarning() << "CustomerModel::addCustomer error:" << q.lastError().text();
    return false;
}

bool CustomerModel::updateCustomer(const CustomerRecord &rec)
{
    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare(
        "UPDATE customers SET name=:name, tax_id=:tax, email=:email, phone=:phone, "
        "billing_address=:addr, credit_limit=:credit, currency=:cur, updated_at=NOW() "
        "WHERE id=:id"
    );
    q.bindValue(":name",  rec.name);
    q.bindValue(":tax",   rec.taxId);
    q.bindValue(":email", rec.email);
    q.bindValue(":phone", rec.phone);
    q.bindValue(":addr",  rec.billingAddress);
    q.bindValue(":credit",rec.creditLimit);
    q.bindValue(":cur",   rec.currency);
    q.bindValue(":id",    rec.id);

    if (q.exec()) {
        load();
        return true;
    }
    qWarning() << "CustomerModel::updateCustomer error:" << q.lastError().text();
    return false;
}

bool CustomerModel::deactivateCustomer(int id)
{
    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare("UPDATE customers SET is_active=false, updated_at=NOW() WHERE id=:id");
    q.bindValue(":id", id);
    if (q.exec()) {
        load();
        return true;
    }
    return false;
}

QString CustomerModel::nextCustomerNumber()
{
    QSqlQuery q(DatabaseManager::instance().database());
    q.exec("SELECT NEXTVAL('seq_customer')");
    if (q.next())
        return QString("C%1").arg(q.value(0).toInt(), 6, 10, QChar('0'));
    return QString("C%1").arg(QDateTime::currentMSecsSinceEpoch() % 1000000, 6, 10, QChar('0'));
}

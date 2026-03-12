#ifndef CRMWIDGET_H
#define CRMWIDGET_H

#include <QWidget>

class QTableView;
class QLineEdit;
class QPushButton;
class QLabel;
class CustomerModel;

class CrmWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CrmWidget(QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onSearch(const QString &text);
    void onSelectionChanged();

private:
    void setupUi();
    void showCustomerDialog(int customerId = -1);

    QTableView    *m_table;
    CustomerModel *m_model;
    QLineEdit     *m_searchEdit;
    QPushButton   *m_editBtn;
    QPushButton   *m_deleteBtn;
};

#endif // CRMWIDGET_H

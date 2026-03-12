#ifndef INVENTORYWIDGET_H
#define INVENTORYWIDGET_H

#include <QWidget>

class QTableWidget;
class QLineEdit;
class QPushButton;

class InventoryWidget : public QWidget
{
    Q_OBJECT
public:
    explicit InventoryWidget(QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onAddMaterial();
    void onSearch(const QString &text);

private:
    void setupUi();
    void loadData(const QString &filter = QString());

    QTableWidget *m_table;
    QLineEdit    *m_searchEdit;
};

#endif // INVENTORYWIDGET_H

#ifndef HRWIDGET_H
#define HRWIDGET_H

#include <QWidget>

class QTableWidget;
class QLineEdit;

class HrWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HrWidget(QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onAddEmployee();
    void onSearch(const QString &text);

private:
    void setupUi();
    void loadData(const QString &filter = QString());

    QTableWidget *m_table;
    QLineEdit    *m_searchEdit;
};

#endif // HRWIDGET_H

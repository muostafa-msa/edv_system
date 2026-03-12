#ifndef FINANCEWIDGET_H
#define FINANCEWIDGET_H

#include <QWidget>

class QTableWidget;
class QDateEdit;
class QComboBox;

class FinanceWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FinanceWidget(QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onAddJournalEntry();
    void onFilterChanged();

private:
    void setupUi();
    void loadJournal();

    QTableWidget *m_table;
    QDateEdit    *m_fromDate;
    QDateEdit    *m_toDate;
};

#endif // FINANCEWIDGET_H

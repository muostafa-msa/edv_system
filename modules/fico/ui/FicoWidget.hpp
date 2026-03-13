#pragma once

#include "core/security/Session.hpp"
#include <QWidget>
#include <QAbstractTableModel>
#include <QVector>
#include <QDate>

QT_BEGIN_NAMESPACE
namespace Ui { class FicoWidget; }
QT_END_NAMESPACE

// ── JournalHeaderModel ────────────────────────────────────────────────────────

struct JournalHeaderRecord {
    QString documentNumber;
    QString documentType;
    QDate   postingDate;
    QString reference;
    QString headerText;
    double  totalDebit{0.0};
    bool    isReversed{false};
};

class JournalHeaderModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit JournalHeaderModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& = {}) const override { return m_data.size(); }
    int columnCount(const QModelIndex& = {}) const override { return 7; }
    QVariant data(const QModelIndex& idx, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void load(const QString& docTypeFilter = {}, const QString& search = {});
    [[nodiscard]] QString documentNumberAt(int row) const;

private:
    QVector<JournalHeaderRecord> m_data;
};

// ── FicoWidget ────────────────────────────────────────────────────────────────

class FicoWidget : public QWidget {
    Q_OBJECT
public:
    explicit FicoWidget(const Session& session, QWidget *parent = nullptr);
    ~FicoWidget() override;

private slots:
    void onNewEntry();
    void onReverse();
    void onRefresh();
    void onRunTrialBalance();
    void onFilterChanged();

private:
    void loadJournalTable();
    void loadTrialBalance(const QDate& from, const QDate& to);

    Ui::FicoWidget      *ui;
    JournalHeaderModel  *m_model;
    Session              m_session;
};

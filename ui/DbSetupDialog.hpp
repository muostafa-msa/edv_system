#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class DbSetupDialog; }
QT_END_NAMESPACE

class DbSetupDialog : public QDialog {
    Q_OBJECT
public:
    explicit DbSetupDialog(QWidget *parent = nullptr);
    ~DbSetupDialog() override;

private slots:
    void onTestConnection();
    void onConnect();

private:
    Ui::DbSetupDialog *ui;
};

#pragma once

#include "core/security/Session.hpp"
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class PpWidget; }
QT_END_NAMESPACE

class PpWidget : public QWidget {
    Q_OBJECT
public:
    explicit PpWidget(const Session& session, QWidget *parent = nullptr);
    ~PpWidget() override;

private slots:
    void loadOrders();
    void onOrderSelected(int row);
    void onNewOrder();
    void onRelease();
    void onRefresh();

private:
    void loadBom(int materialId);

    Ui::PpWidget *ui;
    Session       m_session;
};

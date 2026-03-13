#pragma once

#include "core/security/Session.hpp"
#include "modules/mm/WmsScanner.hpp"
#include "modules/mm/PredictiveReorder.hpp"
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class MmWidget; }
QT_END_NAMESPACE

class MmWidget : public QWidget {
    Q_OBJECT
public:
    explicit MmWidget(const Session& session, QWidget *parent = nullptr);
    ~MmWidget() override;

private slots:
    void loadStockLevels();
    void onGoodsReceipt();
    void onGoodsIssue();
    void onConnectPort();
    void onDisconnectPort();
    void onTestScan();
    void onBarcodeScanned(const QString& barcode);
    void onRunReorderAnalysis();

private:
    void postStockMovement(int materialId, int warehouseId,
                           double quantity, const QString& movementType,
                           const QString& reference);

    Ui::MmWidget   *ui;
    WmsScanner      m_scanner;
    Session         m_session;
};

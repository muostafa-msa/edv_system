#pragma once

#include <QObject>
#include <QString>
#include <QSerialPort>
#include <QTimer>

/**
 * @brief Sub-millisecond warehouse barcode scanner over QSerialPort.
 *
 * Connects to a serial barcode scanner device (e.g. Zebra RS232/USB-CDC).
 * Emits barcodeScanned() whenever a complete barcode line is received.
 * Also supports test mode for emitting simulated barcodes.
 *
 * Usage:
 *   WmsScanner scanner;
 *   scanner.openPort("COM3", QSerialPort::Baud9600);
 *   connect(&scanner, &WmsScanner::barcodeScanned, this, &MyWidget::handleBarcode);
 */
class WmsScanner : public QObject {
    Q_OBJECT
public:
    explicit WmsScanner(QObject *parent = nullptr);
    ~WmsScanner() override;

    /**
     * @brief Open the specified serial port.
     * @param portName  e.g. "COM3" on Windows, "/dev/ttyUSB0" on Linux
     * @param baudRate  Default: 9600
     * @return true if the port was opened successfully.
     */
    [[nodiscard]] bool openPort(const QString& portName,
                                QSerialPort::BaudRate baudRate = QSerialPort::Baud9600);

    void closePort();
    [[nodiscard]] bool isOpen() const;

    /// Enumerate available serial ports.
    [[nodiscard]] static QStringList availablePorts();

    /// Emit a test barcode without hardware (development/demo).
    void emitTestBarcode(const QString& barcode);

    [[nodiscard]] QString lastError() const { return m_lastError; }

signals:
    /// Emitted when a complete barcode line has been received.
    void barcodeScanned(const QString& barcode);

    /// Emitted on port error.
    void portError(const QString& error);

private slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);

private:
    QSerialPort  m_port;
    QByteArray   m_buffer;
    QString      m_lastError;
};

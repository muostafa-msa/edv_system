#include "WmsScanner.hpp"

#include <QSerialPortInfo>
#include <QDebug>

WmsScanner::WmsScanner(QObject *parent) : QObject(parent)
{
    connect(&m_port, &QSerialPort::readyRead,
            this,    &WmsScanner::onReadyRead);
    connect(&m_port, &QSerialPort::errorOccurred,
            this,    &WmsScanner::onSerialError);
}

WmsScanner::~WmsScanner()
{
    closePort();
}

bool WmsScanner::openPort(const QString& portName, QSerialPort::BaudRate baudRate)
{
    if (m_port.isOpen()) m_port.close();

    m_port.setPortName(portName);
    m_port.setBaudRate(baudRate);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port.open(QIODevice::ReadOnly)) {
        m_lastError = m_port.errorString();
        qWarning() << "[WmsScanner] Failed to open" << portName << ":" << m_lastError;
        return false;
    }

    qDebug() << "[WmsScanner] Opened port" << portName << "at" << baudRate;
    m_buffer.clear();
    return true;
}

void WmsScanner::closePort()
{
    if (m_port.isOpen()) {
        m_port.close();
        qDebug() << "[WmsScanner] Port closed.";
    }
}

bool WmsScanner::isOpen() const { return m_port.isOpen(); }

QStringList WmsScanner::availablePorts()
{
    QStringList ports;
    for (const auto& info : QSerialPortInfo::availablePorts())
        ports << info.portName();
    return ports;
}

void WmsScanner::emitTestBarcode(const QString& barcode)
{
    qDebug() << "[WmsScanner] Test barcode:" << barcode;
    emit barcodeScanned(barcode.trimmed());
}

void WmsScanner::onReadyRead()
{
    m_buffer.append(m_port.readAll());

    // Scanners typically terminate barcodes with CR, LF, or CR+LF
    while (true) {
        int crPos = m_buffer.indexOf('\r');
        int lfPos = m_buffer.indexOf('\n');
        int termPos = -1;
        int skipLen = 1;

        if (crPos >= 0 && lfPos == crPos + 1) {
            termPos = crPos; skipLen = 2;          // CR+LF
        } else if (crPos >= 0 && (lfPos < 0 || crPos < lfPos)) {
            termPos = crPos;                        // CR only
        } else if (lfPos >= 0) {
            termPos = lfPos;                        // LF only
        }

        if (termPos < 0) break;

        const QString barcode = QString::fromLatin1(m_buffer.left(termPos)).trimmed();
        m_buffer.remove(0, termPos + skipLen);

        if (!barcode.isEmpty()) {
            qDebug() << "[WmsScanner] Scanned:" << barcode;
            emit barcodeScanned(barcode);
        }
    }
}

void WmsScanner::onSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;
    m_lastError = m_port.errorString();
    qWarning() << "[WmsScanner] Error:" << m_lastError;
    emit portError(m_lastError);
}

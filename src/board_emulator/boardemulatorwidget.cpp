#include "boardemulatorwidget.h"
#include "ui_boardemulatorwidget.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QByteArray>

BoardEmulatorWidget::BoardEmulatorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::BoardEmulatorWidget)
{
    ui->setupUi(this);

    QObject::connect(this->ui->pushButton, QPushButton::clicked, this, &BoardEmulatorWidget::writeData);

    QObject::connect(&(this->m_serial), &QSerialPort::errorOccurred, this, &BoardEmulatorWidget::handleError);
    QObject::connect(&(this->m_serial), &QSerialPort::readyRead, this, &BoardEmulatorWidget::readData);

    QObject::connect(this->ui->comboBox, &QComboBox::currentTextChanged, this, &BoardEmulatorWidget::connectToPort);

    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        ui->comboBox->addItem(info.portName());
    }

    auto serialName = ui->comboBox->currentText();
    connectToPort(serialName);
}

BoardEmulatorWidget::~BoardEmulatorWidget() {
    delete ui;
    if (m_serial.isOpen())
        m_serial.close();
}

void BoardEmulatorWidget::writeData() {
    QByteArray data = ui->inputValueTextEdit->toPlainText().toUtf8();
    m_serial.write(data);
}

void BoardEmulatorWidget::readData() {
    const QByteArray data = m_serial.readAll();
    ui->outputValueTextEdit->insertPlainText(data);
}

void BoardEmulatorWidget::handleError(QSerialPort::SerialPortError error) {
    // TODO: add possibility to reopen port
    if (error == QSerialPort::ResourceError) {
        qInfo() << "Board emulator port" << m_serial.errorString() <<  "error:" << m_serial.portName();
        if (m_serial.isOpen())
            m_serial.close();
    }
}

void BoardEmulatorWidget::connectToPort(const QString &text) {
    if(m_serial.isOpen()) {
        m_serial.close();
    }

    m_serial.setPortName(text);
    m_serial.setBaudRate(QSerialPort::Baud9600);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial.open(QIODevice::ReadWrite)) {
        qInfo() << "Emulator board" << "connected to" << text << "with settings:"
                << "(baud rate:" << m_serial.baudRate() << ")"
                << "(data bits:" << m_serial.dataBits() << ")"
                << "(parity:" << m_serial.parity() << ")"
                << "(stop bits:" << m_serial.stopBits() << ")"
                << "(flow control:" << m_serial.flowControl() << ")";

        qInfo() << "serial baud" << m_serial.baudRate();
    } else {
        qInfo() << "Cannot open port" << text;
    }
}

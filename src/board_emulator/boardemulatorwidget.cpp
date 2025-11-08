#include "boardemulatorwidget.h"
#include "ui_boardemulatorwidget.h"

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QByteArray>

#include "communication/command.h"
#include "communication/BRDAPProtocol.h"
#include "communication/codec.h"

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
    // connectToPort(serialName);
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
    BRDAPCodec codec;
    QPointer<BRDAPV1::CommandV1> v1Command = qobject_cast<BRDAPV1::CommandV1*>(codec.decode(data));
    if(v1Command != nullptr) {
        switch (v1Command->type()) {
        case BRDAPV1::CommandType::HandShakeVersionCommandType:
            evaluateCommand((BRDAPV1::HandshakeVersionCommand &) *v1Command);
            break;

        default:
            qInfo() << "Cannot perform command";
        }
    }
    ui->outputValueTextEdit->setPlainText(data);
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

    } else {
        qInfo() << "Cannot open port" << text;
    }
}

void BoardEmulatorWidget::evaluateCommand(BRDAPV1::HandshakeVersionCommand &command) {
    using namespace BRDAPV1;

    switch (command.m_handshakeNumber) {
    case HandshakeCommandConstants::HandshakeCommandNumber::HandshakeStart:
        {
            QPointer<BRDAPV1::HandshakeVersionCommand> response = new BRDAPV1::HandshakeVersionCommand();
            response->m_handshakeNumber = HandshakeCommandConstants::HandshakeCommandNumber::HandshakeResponse;
            response->m_mode = HandshakeCommandConstants::HandshakeCommandMode::Single;
            response->m_versions.push_back(1);
            BRDAPCodec codec;
            QPointer<Command> com = qobject_cast<Command*>(response);
            m_serial.write(codec.encode(com));
        }
        break;

        case HandshakeCommandConstants::HandshakeCommandNumber::HandshakeResponse:
        {
            if(command.m_mode == HandshakeCommandConstants::HandshakeCommandMode::Single) {
                QPointer<BRDAPV1::HandshakeVersionCommand> response = new BRDAPV1::HandshakeVersionCommand();
                response->m_handshakeNumber = HandshakeCommandConstants::HandshakeCommandNumber::HandshakeAccept;
                response->m_mode = HandshakeCommandConstants::HandshakeCommandMode::Single;
                response->m_versions.push_back([&](){ return command.m_versions.contains(1) ? 1 : 0; } ());
                BRDAPCodec codec;
                QPointer<Command> com = qobject_cast<Command*>(response);
                m_serial.write(codec.encode(com));
                m_BRDAPVersion = command.m_versions.contains(1) ? 1 : 0;
            }
            else {
                // probably response may be not in a single mode, but this question is about protocol design
                qInfo() << "The accepted by board emulator HandshakeVersionCommand response is not in single mode";
            }
            break;
        }

        case HandshakeCommandConstants::HandshakeCommandNumber::HandshakeAccept:
            if(command.m_mode != HandshakeCommandConstants::HandshakeCommandMode::Single) {
                qInfo() << "Handshake accept command not in a single mode, picked first version from version array";
            }
            m_BRDAPVersion = command.m_versions[0];
            qInfo() << "Handshake on board emulator proceeded successfully, version installed to " << m_BRDAPVersion;
            break;

        default:
            qInfo() << "Not specified handshake number";
            break;
    }
}

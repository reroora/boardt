#include "core/mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QComboBox>
#include <QPushButton>
#include <QSerialPort>
#include <QSerialPortInfo>

#include "board_widget/boardwidget.h"
#include "config_reader/configreader.h"
#include "logger/logger.h"
#include "board_emulator/boardemulatorwidget.h"
#include "communication/command.h"
#include "communication/codec.h"
#include "communication/BRDAPProtocol.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // modifies message handler (messages from qDebug, qInfo and etc) for custom logging
    m_originalMessageHandler = qInstallMessageHandler(mainWindowMessageHandler);
    auto& logger = Logger::getInstance();
    logger.setTextEdit(this->ui->logTextEdit);

    QObject::connect(this->ui->refreshSetupPushButton, &QPushButton::pressed, this, &MainWindow::updateSetup);
    QObject::connect(this->ui->connectBoardPushButton, &QPushButton::pressed, this, &MainWindow::connectBoard);

    QObject::connect(this->ui->tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);

    QObject::connect(this->ui->connectionTypeComboBox, &QComboBox::currentTextChanged, this, [this] (const QString &text) {
        if (text == "COM") {
            this->ui->setupStackedWidget->setCurrentWidget(this->ui->comPage);
        }
        else {
            this->ui->setupStackedWidget->setCurrentWidget(this->ui->blankPage);
        }
    });

    ui->connectionTypeComboBox->addItem("COM");

    // disable the closing of standard tabs (setup, log)
    QTabBar *tabBar = ui->tabWidget->tabBar();
    QStringList tabList = {"Setup", "Log"};
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        for (int j = 0; j < tabList.size(); ++j) {
            if (ui->tabWidget->tabText(i) == tabList[j]) {
                tabBar->setTabButton(i, QTabBar::RightSide, nullptr);
            }
        }
    }

    BoardEmulatorWidget* boardEmulatorPtr = new BoardEmulatorWidget();
    auto boardEmulatorIndex = this->ui->tabWidget->addTab(boardEmulatorPtr, "Board emulator");
    tabBar->setTabButton(boardEmulatorIndex, QTabBar::RightSide, nullptr);

    // if I run tool from Qt Creator, current dir is ..../boardt/build/mingw_qt_5_12_8-Debug
    m_configPath = ".\\debug\\boards.json";
    // m_configPath = "boards.json";

    this->supportedBRDAPVersions.push_back(1);

    updateSetup();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mainWindowMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString txt;
    switch (type) {
    case QtDebugMsg:
        txt = QString("Debug: %1").arg(msg);
        break;

    case QtInfoMsg:
        txt = QString("Info: %1").arg(msg);
        break;

    case QtWarningMsg:
        txt = QString("Warning: %1").arg(msg);;
        break;

    case QtCriticalMsg:
        txt = QString("Critical: %1").arg(msg);
        break;

    case QtFatalMsg:
        txt = QString("Fatal: %1").arg(msg);
        abort();
    }

    if (m_originalMessageHandler) {
        m_originalMessageHandler(type, context, txt);
    }

    auto& logger = Logger::getInstance();
    logger.log(txt);
}

void MainWindow::updateBoardList(const QString configPath)
{
    ConfigReaderJson reader;
    QList<QString> names = reader.getBoardNames(configPath);
    this->ui->boardSelectComboBox->addItems(names);
}

void MainWindow::updateSetup()
{
    updateBoardList(this->m_configPath);
    setupComSettings();
}

void MainWindow::connectBoard()
{
    QString boardName = ui->boardSelectComboBox->currentText();

    // if tab exist, maybe try to reconnect and etc
    if(m_Boards.contains(boardName)) {
        return;
    }

    if(!connectToCommunicator(boardName)) {
        return;
    }

    BoardWidget* ptr = new BoardWidget();

    auto index = this->ui->tabWidget->addTab(ptr, boardName);
    ptr->setTabName(boardName);

    ConfigReaderJson reader;
    Board::RegisterMap registerMap = reader.getBoardRegisterMapbyName(m_configPath, boardName);

    QPointer<QComboBox> comboBoxPtr = ptr->getRegisterComboBox();
    comboBoxPtr->addItems(registerMap.keys());

    QObject::connect(ptr, &BoardWidget::sendDataSignal, this, &MainWindow::sendData);
    QObject::connect(ptr, &BoardWidget::refreshDataSignal, this, &MainWindow::refreshData);

    Board::BoardPointer boardPtr = new Board();
    boardPtr->setRegisterMap(registerMap);
    m_Boards[boardName] = boardPtr;

    qInfo() << "Connected new board:" << boardName;
}

void MainWindow::refreshData(QString boardName) {
    QPointer<BoardWidget> widget;
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i) == boardName) {
            widget = qobject_cast<BoardWidget*>(ui->tabWidget->widget(i));
            break;
        }
    }

    QPointer<BRDAPV1::GetRegisterCommand> response = new BRDAPV1::GetRegisterCommand();
    response->m_mode = BRDAPV1::GetRegisterCommandConstants::GetRegisterCommandMode::Single;
    response->m_offsetSizes.push_back(3);
    auto registerMap = m_Boards[boardName]->getRegisterMap();
    auto registerName = widget->getRegisterComboBox()->currentText();
    unsigned long long offset = 0;
    if(registerMap.contains(registerName)) {
        offset = registerMap[registerName].toULongLong(0, 16);
    }
    response->m_registerOffsets.push_back(offset);
    BRDAPCodec codec;
    QPointer<Command> com = qobject_cast<Command*>(response);
    this->m_communicators[boardName]->write(codec.encode(com));
}

void MainWindow::sendData(QString boardName) {
    QPointer<BoardWidget> widget;
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i) == boardName) {
            widget = qobject_cast<BoardWidget*>(ui->tabWidget->widget(i));
            break;
        }
    }

    // sends single mode command (since other modes are not supported)
    QPointer<BRDAPV1::SetRegisterCommand> response = new BRDAPV1::SetRegisterCommand();
    response->m_mode = BRDAPV1::SetRegisterCommandConstants::SetRegisterCommandMode::Single;
    response->m_registerSizes.push_back(4);
    response->m_offsetSizes.push_back(3);
    auto registerMap = m_Boards[boardName]->getRegisterMap();
    auto registerName = widget->getRegisterComboBox()->currentText();
    unsigned long long offset = registerMap[registerName].toULongLong();
    response->m_registerOffsets.push_back(offset);
    unsigned long long value = widget->getregisterDataTextEdit()->toPlainText().toULongLong();
    response->m_registerValues.push_back(value);
    BRDAPCodec codec;
    QPointer<Command> com = qobject_cast<Command*>(response);
    this->m_communicators[boardName]->write(codec.encode(com));

    qInfo() << "Register with offset" << offset << "is set to" << value;

}

void MainWindow::readData() {
    for(auto c : m_communicators) {
        const QByteArray data = c->readAll();
        if (data.size()) {
            if(QPointer<QSerialPort> port = qobject_cast<QSerialPort*>(c.data()); port != nullptr) {
                auto boardName = m_communicators.key(c);
                QPointer<BoardWidget> widget;
                for (int i = 0; i < ui->tabWidget->count(); ++i) {
                    if (ui->tabWidget->tabText(i) == boardName) {
                        widget = qobject_cast<BoardWidget*>(ui->tabWidget->widget(i));
                        break;
                    }
                }

                switch(m_communicator_protocol_versions[boardName]) {
                    case 1:
                        {
                            BRDAPCodec codec;
                            QPointer<BRDAPV1::CommandV1> v1Command = qobject_cast<BRDAPV1::CommandV1*>(codec.decode(data));
                            if(v1Command != nullptr) {
                                switch (v1Command->type()) {
                                case BRDAPV1::CommandType::HandShakeVersionCommandType:
                                    evaluateCommand((BRDAPV1::HandshakeVersionCommand &) *v1Command, boardName);
                                    break;

                                case BRDAPV1::CommandType::SetRegisterCommandType:
                                    evaluateCommand((BRDAPV1::SetRegisterCommand &) *v1Command, boardName);
                                    break;

                                case BRDAPV1::CommandType::GetRegisterCommandType:
                                    qInfo() << "Cannot perform GetRegisterCommand" << 1;
                                    break;

                                default:
                                    qInfo() << "Cannot perform command of version" << 1;
                                }
                            }
                        }
                        break;

                    default:
                        qInfo() << "Unspecified protocol version, trying to use the supported version";
                        {
                            BRDAPCodec codec;
                            QPointer<BRDAPV1::CommandV1> v1Command = qobject_cast<BRDAPV1::CommandV1*>(codec.decode(data));
                            if(v1Command != nullptr) {
                                switch (v1Command->type()) {
                                case BRDAPV1::CommandType::HandShakeVersionCommandType:
                                    evaluateCommand((BRDAPV1::HandshakeVersionCommand &) *v1Command, boardName);
                                    break;

                                default:
                                    qInfo() << "Cannot perform command of version" << 1;
                                }
                            }
                        }

                        break;
                }

            }
            else {
                qInfo() << "Cannot read data";
            }
        }
    }
}

void MainWindow::handlePortError(QSerialPort::SerialPortError error) {

}

bool MainWindow::connectToCommunicator(QString boardName) {
    auto connectionType = ui->connectionTypeComboBox->currentText();
    if(connectionType == "COM") {
        QSerialPort* serial = new QSerialPort();

        auto serialName = ui->comPortNameComboBox->currentText();
        serial->setPortName(serialName);
        serial->setBaudRate(ui->comPortBaudRateComboBox->currentData().value<QSerialPort::BaudRate>());
        serial->setDataBits(ui->comPortDataBitsComboBox->currentData().value<QSerialPort::DataBits>());
        serial->setParity(ui->comPortParityComboBox->currentData().value<QSerialPort::Parity>());
        serial->setStopBits(ui->comPortStopBitsComboBox->currentData().value<QSerialPort::StopBits>());
        serial->setFlowControl(ui->comPortFlowControlComboBox->currentData().value<QSerialPort::FlowControl>());

        if (serial->open(QIODevice::ReadWrite)) {
            qInfo() << "Board" << "connected to" << serialName << "with settings:"
                    << "(baud rate:" << serial->baudRate() << ")"
                    << "(data bits:" << serial->dataBits() << ")"
                    << "(parity:" << serial->parity() << ")"
                    << "(stop bits:" << serial->stopBits() << ")"
                    << "(flow control:" << serial->flowControl() << ")";

            serial->readAll();

            this->m_communicators[boardName] = QPointer(serial);
            this->m_communicator_protocol_versions[boardName] = 0;
            QObject::connect(serial, &QSerialPort::errorOccurred, this, &MainWindow::handlePortError);
            QObject::connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);

            QPointer<BRDAPV1::HandshakeVersionCommand> request = new BRDAPV1::HandshakeVersionCommand();
            request->m_handshakeNumber = BRDAPV1::HandshakeCommandConstants::HandshakeCommandNumber::HandshakeStart;
            request->m_mode = BRDAPV1::HandshakeCommandConstants::HandshakeCommandMode::Single;
            request->m_versions.push_back(1);
            BRDAPCodec codec;
            QPointer<Command> com = qobject_cast<Command*>(request);
            serial->write(codec.encode(com));

            return true;
        } else {
            qInfo() << "Cannot open port" << serialName;
            return false;
        }
    }
    return false;
}

void MainWindow::setupComSettings() {
    updateComNames();

    ui->comPortBaudRateComboBox->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    ui->comPortBaudRateComboBox->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    ui->comPortBaudRateComboBox->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    ui->comPortBaudRateComboBox->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);

    ui->comPortDataBitsComboBox->addItem(QStringLiteral("5"), QSerialPort::Data5);
    ui->comPortDataBitsComboBox->addItem(QStringLiteral("6"), QSerialPort::Data6);
    ui->comPortDataBitsComboBox->addItem(QStringLiteral("7"), QSerialPort::Data7);
    ui->comPortDataBitsComboBox->addItem(QStringLiteral("8"), QSerialPort::Data8);
    ui->comPortDataBitsComboBox->setCurrentIndex(3);


    ui->comPortParityComboBox->addItem(tr("None"), QSerialPort::NoParity);
    ui->comPortParityComboBox->addItem(tr("Even"), QSerialPort::EvenParity);
    ui->comPortParityComboBox->addItem(tr("Odd"), QSerialPort::OddParity);
    ui->comPortParityComboBox->addItem(tr("Mark"), QSerialPort::MarkParity);
    ui->comPortParityComboBox->addItem(tr("Space"), QSerialPort::SpaceParity);

    ui->comPortStopBitsComboBox->addItem(QStringLiteral("1"), QSerialPort::OneStop);
    ui->comPortStopBitsComboBox->addItem(QStringLiteral("2"), QSerialPort::TwoStop);

    ui->comPortFlowControlComboBox->addItem(tr("None"), QSerialPort::NoFlowControl);
    ui->comPortFlowControlComboBox->addItem(tr("RTS/CTS"), QSerialPort::HardwareControl);
    ui->comPortFlowControlComboBox->addItem(tr("XON/XOFF"), QSerialPort::SoftwareControl);
}

void MainWindow::updateComNames() {
    const auto infos = QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &info : infos) {
        ui->comPortNameComboBox->addItem(info.portName());
    }
}

void MainWindow::evaluateCommand(BRDAPV1::HandshakeVersionCommand &command, QString boardName) {
    using namespace BRDAPV1;

    auto findSuitableVersion = [=, this, &command] () -> unsigned int {
        if (command.m_mode == HandshakeCommandConstants::HandshakeCommandMode::Single) {
            for (int i = 0; i < supportedBRDAPVersions.size(); ++i) {
                if(supportedBRDAPVersions[i] == command.m_versions[0])
                    return supportedBRDAPVersions[i];
                    break;
            }
        }
        else {
            qInfo() << "Other mods are not supported yet, version 1 has been returned as a stub.";
            return 1;
        }
    };

    switch (command.m_handshakeNumber) {
        case HandshakeCommandConstants::HandshakeCommandNumber::HandshakeStart:
        {
            QPointer<BRDAPV1::HandshakeVersionCommand> response  = new BRDAPV1::HandshakeVersionCommand();
            response->m_handshakeNumber = HandshakeCommandConstants::HandshakeCommandNumber::HandshakeResponse;
            response->m_mode = HandshakeCommandConstants::HandshakeCommandMode::Single;
            response->m_versions.push_back(findSuitableVersion());
            BRDAPCodec codec;
            QPointer<Command> com = qobject_cast<Command*>(response);
            this->m_communicators[boardName]->write(codec.encode(com));
        }
        break;

        case HandshakeCommandConstants::HandshakeCommandNumber::HandshakeResponse:
        {
            if(command.m_mode == HandshakeCommandConstants::HandshakeCommandMode::Single) {
                QPointer<BRDAPV1::HandshakeVersionCommand> response = new BRDAPV1::HandshakeVersionCommand();
                response->m_handshakeNumber = HandshakeCommandConstants::HandshakeCommandNumber::HandshakeAccept;
                response->m_mode = HandshakeCommandConstants::HandshakeCommandMode::Single;
                unsigned int version = findSuitableVersion();
                response->m_versions.push_back(version);
                BRDAPCodec codec;
                QPointer<Command> com = qobject_cast<Command*>(response);
                this->m_communicators[boardName]->write(codec.encode(com));
                this->m_communicator_protocol_versions[boardName] = version;
                qInfo() << "Handshake accept sended with version installed to " << this->m_communicator_protocol_versions[boardName];
            }
            else {
                // probably response may be not in a single mode, but this question is about protocol design
                qInfo() << "The accepted HandshakeVersionCommand response is not in single mode";
            }
            break;
        }

        case HandshakeCommandConstants::HandshakeCommandNumber::HandshakeAccept:
            if(command.m_mode != HandshakeCommandConstants::HandshakeCommandMode::Single) {
                qInfo() << "Handshake accept command not in a single mode, picked first version from version array";
            }
            this->m_communicator_protocol_versions[boardName] = command.m_versions[0];
            qInfo() << "Handshake proceed successfully, version installed to " << this->m_communicator_protocol_versions[boardName];
            break;

        default:
            qInfo() << "Not specified handshake number";
            break;
    }
}

void MainWindow::evaluateCommand(BRDAPV1::SetRegisterCommand &command, QString boardName) {
    QPointer<BoardWidget> widget;
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i) == boardName) {
            widget = qobject_cast<BoardWidget*>(ui->tabWidget->widget(i));
            break;
        }
    }

    // logic only for single mode
    auto currentRegisterName =  widget->getRegisterComboBox()->currentText();
    QString receivedRegisterName;
    auto registerMap = m_Boards[boardName]->getRegisterMap();
    auto offset = QString::number(command.m_registerOffsets[0], 16);
    if(registerMap.contains(registerMap.key(offset))) {
        receivedRegisterName = registerMap.key(offset);
    }
    else {
        qInfo() << "No register name for received offset from SetRegisterCommand";
    }

    if(receivedRegisterName == currentRegisterName) {
        widget->getregisterDataTextEdit()->setPlainText(QString::number(command.m_registerValues[0]));
    }
}

void MainWindow::evaluateCommand(BRDAPV1::GetRegisterCommand &command, QString boardName) {

}

void MainWindow::closeTab(int index) {
    auto boardName = ui->tabWidget->tabText(index);
    qInfo() << "Disconnect from" << boardName;

    if (m_communicators[boardName]->isOpen()) {
        m_communicators[boardName]->close();
        m_communicators.remove(boardName);
        m_communicator_protocol_versions.remove(boardName);
    }

    m_Boards.remove(boardName);
    ui->tabWidget->removeTab(index);
}

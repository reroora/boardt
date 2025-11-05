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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    auto boardEmulatorIndex = this->ui->tabWidget->addTab(new BoardEmulatorWidget(), "Board emulator");
    tabBar->setTabButton(boardEmulatorIndex, QTabBar::RightSide, nullptr);

    // if I run tool from Qt Creator, current dir is ..../boardt/build/mingw_qt_5_12_8-Debug
    m_configPath = ".\\debug\\boards.json";
    // m_configPath = "boards.json";
\
    // modifies message handler (messages from qDebug, qInfo and etc) for custom logging
    m_originalMessageHandler = qInstallMessageHandler(mainWindowMessageHandler);
    auto& logger = Logger::getInstance();
    logger.setTextEdit(this->ui->logTextEdit);

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

    // if exist tab maybe try to reconnect and etc
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

    // BoardWidget* ptr = qobject_cast<BoardWidget*>(ui->tabWidget->widget(index));
    QPointer<QComboBox> comboBoxPtr = ptr->getRegisterComboBox();
    comboBoxPtr->addItems(registerMap.keys());

    QObject::connect(ptr, &BoardWidget::sendDataSignal, this, &MainWindow::sendData);

    Board::BoardPointer boardPtr = new Board();
    boardPtr->setRegisterMap(registerMap);
    m_Boards[boardName] = boardPtr;
    // auto name = ptr->objectName();



    qInfo() << "Connected new board:" << boardName;
}

void MainWindow::sendData(QString tabName) {
    qInfo() << "sendDataSignal from " << tabName;
    // auto boardWidget = ui->tabWidget->tab

    QPointer<BoardWidget> widget;
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i) == tabName) {
            widget = qobject_cast<BoardWidget*>(ui->tabWidget->widget(i));
            break;
        }
    }

    QString text = widget->getregisterDataTextEdit()->toPlainText();

    auto communicator = m_communicators[tabName];

    communicator->write(text.toUtf8());
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
                widget->getregisterDataTextEdit()->setPlainText(data);
            }
            else {
                qInfo() << "Cannot read data";
            }
        }
    }
}

void MainWindow::handlePortError(QSerialPort::SerialPortError error) {

}

// void BoardEmulatorWidget::readData() {
//     const QByteArray data = m_serial->readAll();
//     ui->outputValueTextEdit->insertPlainText(data);
// }

// void BoardEmulatorWidget::handleError() {
//     // TODO: add possibility to reopen port
//     if (error == QSerialPort::ResourceError) {
//         qInfo() << "Board emulator port" << m_serial->errorString() <<  "error:" << m_serial.portName();
//         if (m_serial->isOpen())
//             m_serial->close();
//     }
// }

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

            this->m_communicators[boardName] = QPointer(serial);
            QObject::connect(serial, &QSerialPort::errorOccurred, this, &MainWindow::handlePortError);
            QObject::connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);
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

void MainWindow::closeTab(int index) {
    auto tabName = ui->tabWidget->tabText(index);
    qInfo() << "Disconnect from" << tabName;

    if (m_communicators[tabName]->isOpen()) {
        m_communicators[tabName]->close();
        m_communicators.remove(tabName);
    }

    m_Boards.remove(tabName);
    ui->tabWidget->removeTab(index);

}

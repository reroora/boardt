#include "core/mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QComboBox>
#include <QPushButton>

#include "board_widget/boardwidget.h"
#include "config_reader/configreader.h"
#include "logger/logger.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(this->ui->refreshSetupPushButton, &QPushButton::pressed, this, &MainWindow::updateSetup);
    QObject::connect(this->ui->connectBoardPushButton, &QPushButton::pressed, this, &MainWindow::connectBoard);

    QObject::connect(this->ui->tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);

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
}

void MainWindow::connectBoard()
{
    QString boardName = ui->boardSelectComboBox->currentText();

    // if exist tab maybe try to reconnect and etc
    if(m_Boards.contains(boardName)) {
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

    QObject::connect(ptr, &BoardWidget::sendDataSignal, this, &MainWindow::sendDataSlot);

    Board::BoardPointer boardPtr = new Board();
    boardPtr->setRegisterMap(registerMap);
    m_Boards[boardName] = boardPtr;
    auto name = ptr->objectName();

    qDebug() << "Connected new board:" << boardName;
}

void MainWindow::sendDataSlot(QString tabName) {
    qDebug() << "sendDataSignal from " << tabName;
}

void MainWindow::closeTab(int index) {
    m_Boards.remove(ui->tabWidget->tabText(index));
    ui->tabWidget->removeTab(index);

    qDebug() << m_Boards;
}

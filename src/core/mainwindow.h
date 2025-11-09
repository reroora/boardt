#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <QHash>
#include <QString>
#include <QSerialPort>
#include <QVarLengthArray>

#include "communication/command.h"
#include "board_widget/boardwidget.h"
#include "board/board.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    MainWindow(QtMessageHandler originalMessageHandler, QWidget *parent = nullptr);
    ~MainWindow();

    static void mainWindowMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);


public slots:
    void updateBoardList(const QString configPath);
    void updateSetup();

private slots:
    void closeTab(int index);

    void connectBoard();

    void refreshData(QString boardName);
    void sendData(QString boardName);
    void readData();
    void handlePortError(QSerialPort::SerialPortError error);

private:
    bool connectToCommunicator(QString boardName);
    void setupComSettings();
    void updateComNames();

    void evaluateCommand(BRDAPV1::HandshakeVersionCommand& command, QString boardName);
    void evaluateCommand(BRDAPV1::SetRegisterCommand& command, QString boardName);
    void evaluateCommand(BRDAPV1::GetRegisterCommand& command, QString boardName);

    Ui::MainWindow *ui;

    QString m_configPath; //TODO: maybe add getting path to config from argv

    QHash<QString, Board::BoardPointer> m_Boards;

    //TODO: add abstraction for communicator to provide incapsulation of codec and necessary objects (for example protocol version)
    QHash<QString, QPointer<QIODevice>> m_communicators;
    // version that sets in beginning of communication
    // if I had abstracted the communicator, this thing would have been hidden
    QHash<QString, unsigned int> m_communicator_protocol_versions;

    QVarLengthArray<int, 4> supportedBRDAPVersions;

    inline static QtMessageHandler m_originalMessageHandler = nullptr;
};

#endif // MAINWINDOW_H

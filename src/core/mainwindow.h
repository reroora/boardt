#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "board/board.h"
#include <QHash>
#include <QString>
#include "board_widget/boardwidget.h"
#include <QSerialPort>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static void mainWindowMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);


public slots:
    void updateBoardList(const QString configPath);
    void updateSetup();

private slots:
    void closeTab(int index);

    void connectBoard();

    void sendData(QString tabName);
    void readData();
    void handlePortError(QSerialPort::SerialPortError error);

private:
    bool connectToCommunicator(QString boardName);
    void setupComSettings();
    void updateComNames();

    Ui::MainWindow *ui;

    QString m_configPath; //TODO: maybe add getting path to config from argv

    QHash<QString, Board::BoardPointer> m_Boards;

    QHash<QString, QPointer<QIODevice>> m_communicators;

    inline static QtMessageHandler m_originalMessageHandler = nullptr;
};

#endif // MAINWINDOW_H

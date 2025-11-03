#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "board/board.h"
#include <QHash>
#include <QString>
#include "board_widget/boardwidget.h"

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

    void connectBoard();

private slots:
    void closeTab(int index);

private:
    Ui::MainWindow *ui;

    QString m_configPath; //TODO: maybe add getting path to config from argv

    QHash<QString, Board::BoardPointer> m_Boards;

    inline static QtMessageHandler m_originalMessageHandler = nullptr;
};

#endif // MAINWINDOW_H

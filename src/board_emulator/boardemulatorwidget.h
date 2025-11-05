#ifndef BOARDEMULATORWIDGET_H
#define BOARDEMULATORWIDGET_H

#include <QWidget>
#include <QSerialPort>

namespace Ui {
class BoardEmulatorWidget;
}

class BoardEmulatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BoardEmulatorWidget(QWidget *parent = nullptr);
    ~BoardEmulatorWidget();

public slots:
    void writeData();
    void readData();
    void handleError(QSerialPort::SerialPortError error);
    void connectToPort(const QString &text);


private:
    Ui::BoardEmulatorWidget *ui;

    QSerialPort m_serial;
};

#endif // BOARDEMULATORWIDGET_H

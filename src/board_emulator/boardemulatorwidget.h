#ifndef BOARDEMULATORWIDGET_H
#define BOARDEMULATORWIDGET_H

#include <QWidget>
#include <QSerialPort>

#include "communication/command.h"

namespace Ui {
class BoardEmulatorWidget;
}

class BoardEmulatorWidget : public QWidget
{
    Q_OBJECT

public:
    friend class MainWindow;

    explicit BoardEmulatorWidget(QWidget *parent = nullptr);
    ~BoardEmulatorWidget();

public slots:
    void writeData();
    void readData();
    void handleError(QSerialPort::SerialPortError error);
    void connectToPort(const QString &text);


private:
    void evaluateCommand(BRDAPV1::HandshakeVersionCommand& command);
    void evaluateCommand(BRDAPV1::SetRegisterCommand& command);
    void evaluateCommand(BRDAPV1::GetRegisterCommand& command);

    Ui::BoardEmulatorWidget *ui;

    QSerialPort m_serial;

    unsigned int m_BRDAPVersion = 0;
    QVarLengthArray<int, 1> m_SupportedBRDAPVersions = {1};

    QHash<unsigned long long, unsigned long long> m_registers;
};

#endif // BOARDEMULATORWIDGET_H

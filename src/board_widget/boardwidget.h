#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QPointer>
#include <QPushButton>
#include <QTextEdit>

namespace Ui {
class BoardWidget;
}

class BoardWidget : public QWidget {
    Q_OBJECT

public:
    friend class MainWindow;

    explicit BoardWidget(QWidget *parent = nullptr);
    BoardWidget(QString tabName, QWidget *parent = nullptr);
    ~BoardWidget();

    QPointer<QComboBox> getRegisterComboBox();
    QPointer<QTextEdit> getregisterDataTextEdit();

    int getIndex() const;
    void setIndex(int newIndex);

    QString tabName() const;
    void setTabName(const QString &newTabName);

signals:
    void sendDataSignal(QString boardName);
    void refreshDataSignal(QString boardName);

private:
    Ui::BoardWidget *ui;

    QString m_tabName = "";
};

#endif // BOARDWIDGET_H

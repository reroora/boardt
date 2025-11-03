#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QPointer>

namespace Ui {
class BoardWidget;
}

class BoardWidget : public QWidget {
    Q_OBJECT

public:
    friend class MainWindow;

    explicit BoardWidget(QWidget *parent = nullptr);
    ~BoardWidget();

    QPointer<QComboBox> getComboBox();

private:
    Ui::BoardWidget *ui;
};

#endif // BOARDWIDGET_H

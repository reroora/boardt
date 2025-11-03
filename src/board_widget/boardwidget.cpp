#include "boardwidget.h"
#include "ui_boardwidget.h"

BoardWidget::BoardWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::BoardWidget)
{
    ui->setupUi(this);


}

BoardWidget::~BoardWidget() {
    delete ui;
}

QPointer<QComboBox> BoardWidget::getComboBox() {
    return QPointer(ui->comboBox);
}

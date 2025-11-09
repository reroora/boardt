#include "boardwidget.h"
#include "ui_boardwidget.h"

BoardWidget::BoardWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::BoardWidget)
{
    ui->setupUi(this);
    QObject::connect(ui->sendPushButton, &QPushButton::clicked, this, [this] { emit this->sendDataSignal(this->m_tabName);} );
    QObject::connect(ui->refreshPushButton, &QPushButton::clicked, this, [this] { emit this->refreshDataSignal(this->m_tabName);} );
    QObject::connect(ui->registerComboBox, &QComboBox::currentTextChanged, this, [this] (const QString &text) { emit this->refreshDataSignal(this->m_tabName);} );
}

BoardWidget::BoardWidget(QString tabName, QWidget *parent)
    : BoardWidget(parent)
{
    m_tabName = tabName;
}

BoardWidget::~BoardWidget() {
    delete ui;
}

QPointer<QComboBox> BoardWidget::getRegisterComboBox() {
    return QPointer(ui->registerComboBox);
}

QPointer<QTextEdit> BoardWidget::getregisterDataTextEdit() {
    return QPointer(ui->registerDataTextEdit);
}

QString BoardWidget::tabName() const
{
    return m_tabName;
}

void BoardWidget::setTabName(const QString &tabName)
{
    m_tabName = tabName;
}

#ifndef BOARD_H
#define BOARD_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QPointer>

class Board : public QObject {

    Q_OBJECT

public:
    using BoardPointer = QPointer<Board>;
    using RegisterMap = QHash<QString, QString>;

    Board(QObject* parent = nullptr);
    ~Board();

    void setRegisterMap(RegisterMap map);
    RegisterMap getRegisterMap() const;

private:
    QString m_boardName;

    RegisterMap  m_registerMap;
};

#endif // BOARD_H

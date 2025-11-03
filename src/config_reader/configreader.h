#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <QList>
#include <QPointer>
#include <QString>

#include "board/board.h"

class ConfigReader {

public:
    virtual QList<QString> getBoardNames(QString configPath) = 0;
    virtual Board::RegisterMap getBoardRegisterMapbyName(QString configPath, QString boardName) = 0;
};

class ConfigReaderJson : public ConfigReader {

public:
    virtual QList<QString> getBoardNames(QString configPath) override;
    virtual Board::RegisterMap getBoardRegisterMapbyName(QString configPath, QString boardName) override;

private:
    QJsonDocument openJsonDocument(QString configPath);
};

#endif // CONFIGREADER_H

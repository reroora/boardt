#include "configreader.h"

#include <QFileInfo>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QStringList>

QList<QString> ConfigReaderJson::getBoardNames(QString configPath)
{
    QList<QString> names;

    auto document = openJsonDocument(configPath);
    if(document.isNull() ) {
        return names;
    }

    auto boardsObject = document.object();

    if(boardsObject.contains("Boards") && boardsObject["Boards"].isArray()) {
        auto boardsArray = boardsObject["Boards"].toArray();

        names.reserve(boardsArray.size());

        for (int i = 0; i < boardsArray.size(); ++i) {
            QJsonObject obj = boardsArray[i].toObject();
            if (obj.contains("BoardName") && obj["BoardName"].isString()) {
                names.append(obj["BoardName"].toString());
            }
        }
    }

    qInfo() << "Loaded names:" << names;
    return names;
}

Board::RegisterMap ConfigReaderJson::getBoardRegisterMapbyName(QString configPath, QString boardName) {
    Board::RegisterMap registerMap;

    QJsonDocument document = openJsonDocument(configPath);

    auto boardsObject = document.object();

    if(boardsObject.contains("Boards") && boardsObject["Boards"].isArray()) {
        auto boardsArray = boardsObject["Boards"].toArray();

        QJsonObject obj;
        for (int i = 0; i < boardsArray.size(); ++i) {
            obj = boardsArray[i].toObject();
            if (obj.contains("BoardName") && (obj["BoardName"].toString() == boardName)) {
                break;
            }
        }

        if (obj.contains("RegisterNameAddressPairs") && obj["RegisterNameAddressPairs"].isArray()) {
            auto mapArray = obj["RegisterNameAddressPairs"].toArray();
            for (int i = 0; i < mapArray.size(); ++i) {
                auto mapObj = mapArray[i].toObject();
                QStringList  keys = mapObj.keys();
                QString key = keys[0];
                QString value = mapObj[key].toString();
                registerMap[key] = value;
            }
        }

    }

    qDebug() << "Loaded registers:" << registerMap;
    return registerMap;
}

QJsonDocument ConfigReaderJson::openJsonDocument(QString configPath)
{
    QJsonDocument document;
    {
        QFileInfo fileInfo(configPath);

        if(fileInfo.suffix() != "json") {
            qDebug() << "File" << configPath << "is not json.";
            return document;
        }
    }

    QFile file(configPath);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't open config" << file.fileName();
        return document;
    }

    QString data = file.readAll();
    file.close();

    return document = QJsonDocument(QJsonDocument::fromJson(data.toUtf8()));
}

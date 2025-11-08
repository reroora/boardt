#ifndef CODEC_H
#define CODEC_H

#include <QObject>
#include "command.h"
#include <QPointer>

class Codec : public QObject
{
    Q_OBJECT
public:
    explicit Codec(QObject *parent = nullptr);

    virtual QByteArray encode(QPointer<Command> command) = 0;
    virtual QPointer<Command> decode(QByteArray data) = 0;
};

class BRDAPCodec : public Codec {
    Q_OBJECT
public:
    explicit BRDAPCodec(QObject *parent = nullptr);

    virtual QByteArray encode(QPointer<Command> command) override;
    virtual QPointer<Command> decode(QByteArray data) override;

private:
    QByteArray encode(BRDAPV1::HandshakeVersionCommand& command);

    void decode(BRDAPV1::HandshakeVersionCommand& command, QByteArray data);

};

#endif // CODEC_H

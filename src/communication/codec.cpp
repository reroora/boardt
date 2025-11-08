#include "codec.h"

#include "BRDAPProtocol.h"

#include <QBitArray>
#include <QByteArray>

Codec::Codec(QObject *parent)
    : QObject{parent}
{}

BRDAPCodec::BRDAPCodec(QObject *parent) : Codec(parent) {}

QByteArray BRDAPCodec::encode(QPointer<Command> command) {
    auto brdapCommand = qobject_cast<BRDAPV1::CommandV1*>(command);

    switch (brdapCommand->type()) {
    case BRDAPV1::CommandType::HandShakeVersionCommandType:
        return encode((BRDAPV1::HandshakeVersionCommand &) *command);

    default:
        return QByteArray();
    }
}

QPointer<Command> BRDAPCodec::decode(QByteArray data) {
    switch(data[0]) {
    case BRDAPV1::CommandCode::HandshakeCommandCode:
    {   QPointer<Command> command = new BRDAPV1::HandshakeVersionCommand();
        decode((BRDAPV1::HandshakeVersionCommand &) *command, data);
        return command; }
    default:
        return nullptr;
    }
}

QByteArray BRDAPCodec::encode(BRDAPV1::HandshakeVersionCommand &command) {
    QByteArray bytes(2, 0);

    bytes[0] = (char) BRDAPV1::CommandCode::HandshakeCommandCode;

    switch (command.m_mode) {
    case BRDAPV1::HandshakeCommandConstants::HandshakeCommandMode::Single:
        bytes[1] = (char)command.m_versions.at(0);
        break;

    case BRDAPV1::HandshakeCommandConstants::HandshakeCommandMode::Vector:
    case BRDAPV1::HandshakeCommandConstants::HandshakeCommandMode::Range:
        bytes[1] = (char)command.m_versions.size();
        break;

    default:
        break;
    }

    char byte1 = bytes[1];
    byte1 |= (char) (command.m_handshakeNumber << BRDAPV1::HandshakeCommandConstants::HandshakeCommandShift::handshakeNumber);
    byte1 |= (char) (command.m_mode << BRDAPV1::HandshakeCommandConstants::HandshakeCommandShift::mode);
    bytes[1] = byte1;

    for (int i = 0; i < command.m_versions.size() / 2; ++i) {
        char byte = (char) command.m_versions[i * 2 + 1];
        byte |= (char) command.m_versions[i * 2] << BRDAPV1::HandshakeCommandConstants::HandshakeCommandShift::version;
        bytes.push_back(byte);
    }
    if (command.m_versions.size() % 2) {
        char byte = (char) command.m_versions[command.m_versions.size() - 1];
        bytes.push_back(byte);
    }

    return bytes;
}

void BRDAPCodec::decode(BRDAPV1::HandshakeVersionCommand &command, QByteArray data) {
    unsigned char byte = data[1];

    auto versionCount = byte & ~(0xf << BRDAPV1::HandshakeCommandConstants::HandshakeCommandFieldSize::versionCount);
    command.m_handshakeNumber = (byte >> BRDAPV1::HandshakeCommandConstants::HandshakeCommandShift::handshakeNumber)
                                & ~(0x3f << BRDAPV1::HandshakeCommandConstants::HandshakeCommandFieldSize::handshakeNumber);
    command.m_mode = (byte >> BRDAPV1::HandshakeCommandConstants::HandshakeCommandShift::mode)
                     & ~(0x3f << BRDAPV1::HandshakeCommandConstants::HandshakeCommandFieldSize::mode);

    if(command.m_mode == BRDAPV1::HandshakeCommandConstants::HandshakeCommandMode::Single) {
        command.m_versions.push_back(versionCount);
        return;
    }
    else {
        command.m_versions.reserve(versionCount);
        for(int i = 0; i < versionCount / 2; i++) {
            command.m_versions[i * 2] = (data[i + 2] >> 4) & ~(0xf << 4);
            command.m_versions[i * 2 + 1] = data[i + 2] & ~(0xf << 4);
        }
        if (versionCount % 2) {
            command.m_versions[versionCount - 1] = data[2 + data.size() - 1];
        }
    }

}




















#include "codec.h"

#include "BRDAPProtocol.h"

#include <QBitArray>
#include <QByteArray>
#include <QDebug>

Codec::Codec(QObject *parent)
    : QObject{parent}
{}

BRDAPCodec::BRDAPCodec(QObject *parent) : Codec(parent) {}

QByteArray BRDAPCodec::encode(QPointer<Command> command) {
    auto brdapCommand = qobject_cast<BRDAPV1::CommandV1*>(command);

    switch (brdapCommand->type()) {
    case BRDAPV1::CommandType::HandShakeVersionCommandType:
        return encode((BRDAPV1::HandshakeVersionCommand &) *command);

    case BRDAPV1::CommandType::SetRegisterCommandType:
        return encode((BRDAPV1::SetRegisterCommand &) *command);

    case BRDAPV1::CommandType::GetRegisterCommandType:
        return encode((BRDAPV1::GetRegisterCommand &) *command);

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

    case BRDAPV1::CommandCode::SetRegisterCommandCode:
    {   QPointer<Command> command = new BRDAPV1::SetRegisterCommand();
        if (decode((BRDAPV1::SetRegisterCommand &) *command, data))
            return command;
        else
            return nullptr; }

    case BRDAPV1::CommandCode::GetRegisterCommandCode:
    {   QPointer<Command> command = new BRDAPV1::GetRegisterCommand();
        if (decode((BRDAPV1::GetRegisterCommand &) *command, data))
            return command;
        else
            return nullptr; }

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

QByteArray BRDAPCodec::encode(BRDAPV1::SetRegisterCommand &command) {
    QByteArray bytes(2, 0);

    bytes[0] = (char) BRDAPV1::CommandCode::SetRegisterCommandCode;

    bytes[1] = (char) command.m_mode;

    // provide only single mode yet
    if (command.m_mode == BRDAPV1::SetRegisterCommandConstants::SetRegisterCommandMode::Single) {
        char sizeByte = (char) command.m_offsetSizes[0];
        sizeByte |= (char) (command.m_registerSizes[0] << BRDAPV1::SetRegisterCommandConstants::SetRegisterCommandShift::registerSize);
        bytes.push_back(sizeByte);

        QByteArray regOffsetBytes;
        regOffsetBytes.resize(sizeof(command.m_registerOffsets[0]));
        memcpy(regOffsetBytes.data(), &(command.m_registerOffsets[0]), sizeof(command.m_registerOffsets[0]));
        bytes.push_back(regOffsetBytes);

        QByteArray regValueBytes;
        regValueBytes.resize(sizeof(command.m_registerValues[0]));
        memcpy(regValueBytes.data(), &(command.m_registerValues[0]), sizeof(command.m_registerValues[0]));
        bytes.push_back(regValueBytes);
    }
    else {
        qInfo() << "BDRAP codec support only single mode in SetRegisterCommand yet";
        return QByteArray();
    }

    return bytes;
}

QByteArray BRDAPCodec::encode(BRDAPV1::GetRegisterCommand &command) {
    QByteArray bytes(2, 0);

    bytes[0] = (char) BRDAPV1::CommandCode::GetRegisterCommandCode;

    bytes[1] = (char) command.m_mode;

    // provide only single mode yet
    if (command.m_mode == BRDAPV1::GetRegisterCommandConstants::GetRegisterCommandMode::Single) {
        bytes.push_back((char) command.m_offsetSizes[0]);

        QByteArray regOffsetBytes;
        regOffsetBytes.resize(sizeof(command.m_registerOffsets[0]));
        memcpy(regOffsetBytes.data(), &(command.m_registerOffsets[0]), sizeof(command.m_registerOffsets[0]));
        bytes.push_back(regOffsetBytes);
    }
    else {
        qInfo() << "BDRAP codec support only single mode in GetRegisterCommand yet";
        return QByteArray();
    }

    return bytes;
}

bool BRDAPCodec::decode(BRDAPV1::HandshakeVersionCommand &command, QByteArray data) {
    unsigned char byte = data[1];

    auto versionCount = byte & ~(0xf << BRDAPV1::HandshakeCommandConstants::HandshakeCommandFieldSize::versionCount);
    command.m_handshakeNumber = (byte >> BRDAPV1::HandshakeCommandConstants::HandshakeCommandShift::handshakeNumber)
                                & ~(0x3f << BRDAPV1::HandshakeCommandConstants::HandshakeCommandFieldSize::handshakeNumber);
    command.m_mode = (byte >> BRDAPV1::HandshakeCommandConstants::HandshakeCommandShift::mode)
                     & ~(0x3f << BRDAPV1::HandshakeCommandConstants::HandshakeCommandFieldSize::mode);

    if(command.m_mode == BRDAPV1::HandshakeCommandConstants::HandshakeCommandMode::Single) {
        command.m_versions.push_back(versionCount);
        return true;
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
    return true;
}

bool BRDAPCodec::decode(BRDAPV1::SetRegisterCommand &command, QByteArray data) {
    command.m_mode = data[1];

    if(command.m_mode == BRDAPV1::SetRegisterCommandConstants::SetRegisterCommandMode::Single) {
        unsigned char byte = data[2];

        command.m_offsetSizes.push_back(byte & ~(0x3f << BRDAPV1::SetRegisterCommandConstants::SetRegisterCommandFieldSize::registerOffset) );
        command.m_registerSizes.push_back( (byte >> BRDAPV1::SetRegisterCommandConstants::SetRegisterCommandShift::registerSize)
                                    & ~(0x1f << BRDAPV1::SetRegisterCommandConstants::SetRegisterCommandFieldSize::registerSize) );

        int offset = 1 << command.m_offsetSizes[0];

        command.m_registerOffsets.push_back(0);
        QByteArray registerOffset = data.mid(3, offset);
        memcpy(&(command.m_registerOffsets[0]), registerOffset.data(), sizeof(command.m_registerOffsets[0]));

        command.m_registerValues.push_back(0);
        QByteArray registerValue = data.mid(3 + offset, (1 << (command.m_registerSizes[0] - 1)) + !(bool)command.m_registerSizes[0]);
        memcpy(&(command.m_registerValues[0]), registerValue.data(), sizeof(command.m_registerValues[0]));
    }
    else {
        qInfo() << "BDRAP codec support only single mode in SetRegisterCommand yet";
        return false;
    }
    return true;
}

bool BRDAPCodec::decode(BRDAPV1::GetRegisterCommand &command, QByteArray data) {
    command.m_mode = data[1];

    if(command.m_mode == BRDAPV1::GetRegisterCommandConstants::GetRegisterCommandMode::Single) {
        command.m_offsetSizes.push_back(data[2]);

        int offset = 1 << command.m_offsetSizes[0];

        command.m_registerOffsets.push_back(0);
        QByteArray registerOffset = data.mid(3, offset);
        memcpy(&(command.m_registerOffsets[0]), registerOffset.data(), sizeof(command.m_registerOffsets[0]));
    }
    else {
        qInfo() << "BDRAP codec support only single mode in SetRegisterCommand yet";
        return false;
    }
    return true;
}




















#include "command.h"

Command::Command(QObject *parent)
    : QObject{parent}
{}

namespace BRDAPV1 {
    CommandV1::CommandV1(CommandType type, QObject *parent) : Command(parent), m_type(type) {}

    CommandType BRDAPV1::CommandV1::type() const
    {
        return m_type;
    }

    void CommandV1::setType(CommandType newType)
    {
        m_type = newType;
    }

    HandshakeVersionCommand::HandshakeVersionCommand(QObject *parent) : CommandV1(CommandType::HandShakeVersionCommandType, parent) {}

    GetRegisterCommand::GetRegisterCommand(QObject *parent) : CommandV1(CommandType::GetRegisterCommandType, parent) {}

    SetRegisterCommand::SetRegisterCommand(QObject *parent) : CommandV1(CommandType::SetRegisterCommandType, parent) {}

}



#ifndef PACKET_H
#define PACKET_H

#include <QObject>
#include <QByteArray>
#include <QList>

class Command : public QObject
{
    Q_OBJECT
public:

    explicit Command(QObject *parent = nullptr);
};

namespace BRDAPV1 {
    enum CommandType {
        HandShakeVersionCommandType,
        SetRegisterCommandType,
        GetRegisterCommandType,
        ErrorCommandType
    };

    class CommandV1 : public Command
    {
        Q_OBJECT
        public:
            explicit CommandV1(CommandType type, QObject *parent = nullptr);

            CommandType type() const;
            void setType(CommandType newType);

        private:
            CommandType m_type;
    };

    class HandshakeVersionCommand : public CommandV1 {
        Q_OBJECT
    public:
        explicit HandshakeVersionCommand(QObject *parent = nullptr);

        QList<unsigned char> m_versions;
        unsigned char m_handshakeNumber = 0;
        unsigned char m_mode = 0;
    };

    class SetRegisterCommand : public CommandV1 {
        Q_OBJECT
    public:
        explicit SetRegisterCommand(QObject *parent = nullptr);

        QList<unsigned int> m_registerSizes;
        QList<unsigned int> m_offsetSizes;

        // TODO: provide more flexible types for values and offsets larger than 8 bytes (at the moment, the class only supports sizes less than or equal to 8 bytes)
        // maybe use templates or qbytearrays
        QList<unsigned long long> m_registerOffsets;
        QList<unsigned long long> m_registerValues;

        unsigned int m_mode = 0;
    };

    class GetRegisterCommand : public CommandV1 {
        Q_OBJECT
    public:
        explicit GetRegisterCommand(QObject *parent = nullptr);

        QList<unsigned int> m_offsetSizes;

        // TODO: same as in SetRegisterCommand, provide more flexible types for offsets larger than 8 bytes (at the moment, the class only supports sizes less than or equal to 8 bytes)
        // maybe use templates, qbytearrays or QVariant's
        QList<unsigned long long> m_registerOffsets;

        unsigned int m_mode = 0;
    };

}





#endif // PACKET_H

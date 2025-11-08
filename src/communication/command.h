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

        QList<unsigned int> m_versions;
        unsigned int m_handshakeNumber = 0;
        unsigned int m_mode = 0;
    };






}





#endif // PACKET_H

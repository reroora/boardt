#ifndef BRDAPPROTOCOL_H
#define BRDAPPROTOCOL_H

namespace BRDAPV1 {

    enum CommandCode {
        HandshakeCommandCode = 0x01
    };

struct HandshakeCommandConstants {
    struct HandshakeCommandFieldOffset {
        static constexpr int handshakeNumber = 7;
        static constexpr int mode = 5;
        static constexpr int versionCount = 3;

        static constexpr int version = 7;
    };

    struct HandshakeCommandFieldSize {
        static constexpr int handshakeNumber = 2;
        static constexpr int mode = 2;
        static constexpr int versionCount = 4;

        static constexpr int version = 4;
    };

    struct HandshakeCommandShift {
        static constexpr int handshakeNumber = HandshakeCommandFieldOffset::handshakeNumber - HandshakeCommandFieldSize::handshakeNumber + 1;
        static constexpr int mode = HandshakeCommandFieldOffset::mode - HandshakeCommandFieldSize::mode + 1;
        static constexpr int versionCount = HandshakeCommandFieldOffset::versionCount - HandshakeCommandFieldSize::versionCount + 1;

        static constexpr int version = HandshakeCommandFieldOffset::version - HandshakeCommandFieldSize::version + 1;
    };

    enum HandshakeCommandNumber {
        HandshakeStart = 0,
        HandshakeResponse,
        HandshakeAccept
    };

    enum HandshakeCommandMode {
        Single = 0,
        Vector,
        Range
    };
};



}

#endif // BRDAPPROTOCOL_H

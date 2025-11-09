# boardt

Boardt (board register debugging tool) is a software for checking and changing 
states of board registers.

## Functionality

This tool provides the following functionality:

- Adding various boards using the configuration file.
- Communication between board and tool via COM port.
- Emulation of board.
- Codec for own communication protocol.

**All features mentioned above are currently under development.**

## Build

Build using mingw-w64 14.2.0-3 and Qt 5.12.8 with qmake in Qt Creator.

## Protocol

This is a prototype of protocol for communication between the board and the debugging tool.

Communication starts with installation of the protocol version via handshake with the 
suggestion of versions from both sides. 

After version installing, both sides can send commands and be sure that commands will be interpreted correctly.

At this moment protocol provide some set of commands.

Each command starts from command code. The command codes are shown in the table below.

| Command            | Code | 
| ------------------ | ---- |
| HandshakeCommand   | 0x01 |
| SetRegisterCommand | 0x02 |
| GetRegisterCommand | 0x03 |

And after command code each command stores some usable information.

#### HandshakeCommand 

This command used for version installation at the beginning of communication.

| Handshake number | Mode  | size/version | version1/version2 | version3/version4 | ... |
| ---------------- | ----- | ------------ | ----------------- | ----------------- | --- |
| 2 bit            | 2 bit |     4 bit    | 4 bit / 4 bit     |  4 bit / 4 bit    |     |

**Handshake number** - sequence number of handhsake (maybe analogue of syn/ack) 
(0b00 - HandshakeStart, 0b01 - HandshakeResponse, 0b10 - HandshakeAccept)

**Mode** - this field indicates how the following data should be interpreted.
Value **Single** (0b0) idicates that sended only one version (version must be set to field size/version),
value **Vector** (0b1) indicates that next data must be interpred like array, set 
array size in field size/version and set array like in table above. Value **Range** (0b10) indicate 
that next data is array of ranges with begin-end pairs, this pairs stores in one byte (begin/end = version1/version2 from table above).

**Size/version** - depending on the mode, it may be the size or version.

**Version1/version2**, **Version1/version2** ... - like a field **size/version** depends of **mode**.

#### SetRegisterCommand

Command used for set registers values by offset.

| Flags  | Sizes  | Register Offset | Register value  | 
| ------ | ------ | --------------- |---------------- |
| 1 byte | 1 byte |  byte - 8 bytes | byte - 64 bytes |

**Flags** - flags for command. At this moment provide only one field: **mode** - 
analog field with same name in **HandshakeCommand**. Only **single** mode supported yet.

**Sizes** - sizes for next fields: 

- **offset size** - size of **register offset** field (2 bit) coding sizes 8, 16, 32, 64; in one word 2^(3 + value) bits.
- **register size** - size of **register value** field (3 bit) coding sizes 4, 8, 16, 32, 64, 128, 256, 512; in one word 2^(2 + value) bits.

**Register Offset** - offset of a register. Field lenghth depends of **offset size** field. 

**Register value** - value of a register. Field lenghth depends of **register size** field. 

#### GetRegisterCommand

Command used for requests of registers values.

| Flags  | Sizes  | Register Offset |
| ------ | ------ | --------------- |
| 1 byte | 1 byte |  byte - 8 bytes | 

**Flags** - same as in **SetRegisterCommand**.

**Sizes** - same as in **SetRegisterCommand** but without **register size**.

**Register Offset** - same as in **SetRegisterCommand**.

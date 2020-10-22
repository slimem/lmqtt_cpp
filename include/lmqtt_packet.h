#pragma once

namespace lmqtt {

    // bitwise or with upcoming byte to identify packet type
    enum class packet_type : uint8_t {
        //           VALUE                Direction            Description 
        RESERVED    = 0x0,        //   ***Forbidden***     ***Reserved***
        CONNECT     = 0x1,        //   Client --> Server   Connection request
        CONNACK     = 0x2,        //   Client <-- Server   Connect acknowledgment
        PUBLISH     = 0x3,        //   Client <-> Server   Publish message
        PUBACK      = 0x4,        //   Client <-> Server   Publish achnowledgment (QoS1)
        PUBREC      = 0x5,        //   Client <-> Server   Publish received (QoS 2 delivery part 1)
        PUBREL      = 0x6,        //   Client <-> Server   Publish release (QoS 2 delivery part 2)
        PUBCOMP     = 0x7,        //   Client <-> Server   Publish complete (QoS 2 delivery part 3)
        SUBSCRIBE   = 0x8,        //   Client --> Server   Subscribe request
        SUBACK      = 0x9,        //   Client <-> Server   Subscribe acknowledgment
        UNSUBSCRIBE = 0xA,        //   Client --> Server   Unsubscribe request
        UNSUBACK    = 0xB,        //   Client <-- Server   Unsubscribe acknowledgment
        PINGREQ     = 0xC,        //   Client --> Server   PING request
        PINGRESP    = 0xD,        //   Client <-- Server   PING response
        DISCONNECT  = 0xE,        //   Client <-> Server   Disconnect notification
        AUTH        = 0xF,        //   Client <-> Server   Authentication exchange
    };

    enum class packet_flag : uint8_t {
        //           VALUE                 FLAG          
        RESERVED    = 0x0,        //   ***Reserved***
        CONNECT     = 0x0,        //   ***Reserved***
        CONNACK     = 0xF,        //   | DUP | QoS | QoS | RETAIN
        PUBLISH     = 0x0,        //   ***Reserved***
        PUBACK      = 0x0,        //   ***Reserved***
        PUBREC      = 0x0,        //   ***Reserved***
        PUBREL      = 0x2,        //   ***Reserved***
        PUBCOMP     = 0x0,        //   ***Reserved***
        SUBSCRIBE   = 0x2,        //   ***Reserved***
        SUBACK      = 0x0,        //   ***Reserved***
        UNSUBSCRIBE = 0x2,        //   ***Reserved***
        UNSUBACK    = 0x0,        //   ***Reserved***
        PINGREQ     = 0x0,        //   ***Reserved***
        PINGRESP    = 0x0,        //   ***Reserved***
        DISCONNECT  = 0x0,        //   ***Reserved***
        AUTH        = 0x0,        //   ***Reserved***
    };

    class fixed_header { 
        uint8_t _controlHeader;
        uint32_t _packetL;
    };

} // namespace lmqtt

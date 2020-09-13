#ifndef MESSAGE_WEBGL
#define MESSAGE_WEBGL

#include <memory>
#include <libwebsockets.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <argos3/core/utility/datatypes/datatypes.h>

namespace argos {
    struct SMessage {
        CByteArray* Data;
        lws_write_protocol Type;

        // Will never be called because it is allocated by libwebsocket
        // which is a c library
        SMessage() = delete;
        SMessage(const SMessage&) = delete;
        SMessage(lws_write_protocol e_type)
            : Data(new CByteArray), Type(e_type) {}
        SMessage(CByteArray* data, lws_write_protocol e_type)
            : Data(data), Type(e_type){}

        ~SMessage() {
            delete Data;
        }
    };

    struct SCPPPerSessionData {
        std::shared_ptr<SMessage> CurrentSendMessage;
        std::vector<UInt32> UpdateVersions;
    };

    struct SPerSessionData {
        struct lws *m_psWSI;
        UInt32 m_uLastSpawnedNetId; // TODO: network id
        // Recieved by client
        SMessage m_psCurrentRecvMessage;
        size_t m_uSent;
        // ESendMessageStep m_eSendType;
        UInt32 m_uLuaVersion;
        bool m_bIsPlaying; // PAUSE...
        UInt32 m_uNextUpdateId;

        // part that should not be allocated by libwebsocket
        // which is a c lib and does not know about constructors
        SCPPPerSessionData* m_sCPP;
    };

    bool WriteMessage(SPerSessionData* ps_session);

    void EscapeChar(std::ostringstream& str_stream, char c);
}

#endif
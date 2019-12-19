#ifndef MESSAGE_WEBGL
#define MESSAGE_WEBGL

#include <memory>
#include <libwebsockets.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <argos3/core/utility/datatypes/datatypes.h>

namespace argos {
    struct SMessage {
        std::unique_ptr<CByteArray> data;
        lws_write_protocol type;
    };


    struct SPerSessionData {
        struct lws *m_psWSI;
        UInt32 m_uLastSpawnedNetId; // TODO: network id
        // Recieved by client
        SMessage m_psCurrentRecvMessage;
        size_t m_uSent;
        std::shared_ptr<SMessage> m_psCurrentSendMessage;
        // ESendMessageStep m_eSendType;
        UInt32 m_uLuaVersion;
        bool m_bIsPlaying; // PAUSE...
        std::vector<UInt32> m_vecUpdateVersions;
        UInt32 m_uNextUpdateId;
    };

    bool WriteMessage(SPerSessionData* ps_session);

    void EscapeChar(std::ostringstream& cStrStream, char c);
}

#endif
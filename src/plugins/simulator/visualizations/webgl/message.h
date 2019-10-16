#ifndef MESSAGE_WEBGL
#define MESSAGE_WEBGL

#include <libwebsockets.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <argos3/core/utility/datatypes/datatypes.h>

namespace argos {
    struct SMessage {
        CByteArray* data;
        lws_write_protocol type;
    };


    struct SPerSessionData {
        struct lws *m_psWSI;
        UInt32 m_uLastSpawnedNetId; // TODO: network id
        UInt32 m_uRingTail;
        bool m_bKicked;
        bool m_bInRing;
        // Recieved by client
        SMessage m_psCurrentRecvMessage;

        size_t m_uSent;
        /*  when m_uSent == 0 this should not be accessed
            except if it has been assigned just now */
        SMessage m_psCurrentSendMessage;
    };

    bool WriteMessage(SPerSessionData* ps_session, const SMessage* ps_message);

    void DestroyMessage(void *ps_message);
}

#endif
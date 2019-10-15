#include "message.h"

namespace argos {
    /**
     * Returns true if it finished to send the whole message
    */
    bool WriteMessage(SPerSessionData* ps_session, const SMessage* ps_message) {
        size_t uToSend = ps_message->data->Size() - ps_session->m_uSent;
        // the LWS_PRE first bytes are for libwebsocket
        UInt8 buffer[uToSend + LWS_PRE];
        UInt8* uSrc = ps_message->data->ToCArray() + ps_session->m_uSent;
        memcpy(buffer + LWS_PRE, uSrc, uToSend);

        size_t uSent = static_cast<size_t>(lws_write(ps_session->m_psWSI, buffer + LWS_PRE, uToSend, ps_message->type));//ps_message->type);
        if (uSent == uToSend) {
            ps_session->m_uSent = 0;
            return true;
        }
        ps_session->m_uSent += uSent;
        return false;
    }

    void DestroyMessage(void *ps_message){
        SMessage* psMessage = reinterpret_cast<SMessage*>(ps_message);
        delete psMessage->data;
    }
}
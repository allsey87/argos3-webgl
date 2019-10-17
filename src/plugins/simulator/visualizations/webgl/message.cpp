#include "message.h"
#include <sstream>
#include <iomanip>

namespace argos {
    /**
     * Returns true if it finished to send the whole message
    */
    bool WriteMessage(SPerSessionData* ps_session) {
        SMessage sMessage = ps_session->m_psCurrentSendMessage;
        size_t uToSend = sMessage.data->Size() - ps_session->m_uSent;
        // the LWS_PRE first bytes are for libwebsocket
        UInt8 buffer[uToSend + LWS_PRE];
        UInt8* uSrc = sMessage.data->ToCArray() + ps_session->m_uSent;
        memcpy(buffer + LWS_PRE, uSrc, uToSend);

        size_t uSent = static_cast<size_t>(lws_write(ps_session->m_psWSI, buffer + LWS_PRE, uToSend, sMessage.type));
        if (uSent == uToSend) {
            ps_session->m_uSent = 0;
            return true;
        }
        ps_session->m_uSent += uSent;
        return false;
    }

    void EscapeChar(std::ostringstream& cStrStream, char c) {
        switch (c) {
            case '"':
                cStrStream << "\\\"";
                break;
            case '\\':
                cStrStream << "\\\\";
                break;
            case '\b':
                cStrStream << "\\b";
                break;
            case '\f':
                cStrStream << "\\f";
                break;
            case '\n':
                cStrStream << "\\n";
                break;
            case '\r':
                cStrStream << "\\r";
                break;
            case '\t':
                cStrStream << "\\t";
                break;
            default:
                if (('\x00' <= c && c <= '\x1f')) {
                    cStrStream << std::hex << std::setw(4)
                                << std::setfill('0') << (int)c;
                } else {
                    cStrStream << c;
                }
            }
        }

    void DestroyMessage(void *ps_message){
        SMessage* psMessage = reinterpret_cast<SMessage*>(ps_message);
        delete psMessage->data;
    }
}
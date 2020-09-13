#include "message.h"
#include <sstream>
#include <iomanip>

namespace argos {
    /**
     * Returns true if it finished to send the whole message
    */
    bool WriteMessage(SPerSessionData* ps_session) {
        SMessage* psMessage = ps_session->m_sCPP->CurrentSendMessage.get();
        size_t uToSend = psMessage->Data->Size() - ps_session->m_uSent;
        // the LWS_PRE first bytes are for libwebsocket
        UInt8 buffer[uToSend + LWS_PRE];
        UInt8* uSrc = psMessage->Data->ToCArray() + ps_session->m_uSent;
        memcpy(buffer + LWS_PRE, uSrc, uToSend);

        size_t uSent = static_cast<size_t>(lws_write(ps_session->m_psWSI, buffer + LWS_PRE, uToSend, psMessage->Type));
        if (uSent == uToSend) {
            ps_session->m_sCPP->CurrentSendMessage.reset();
            ps_session->m_uSent = 0;
            return true;
        }
        ps_session->m_uSent += uSent;
        return false;
    }

    void EscapeChar(std::ostringstream& str_stream, char c) {
        switch (c) {
            case '"':
                str_stream << "\\\"";
                break;
            case '\\':
                str_stream << "\\\\";
                break;
            case '\b':
                str_stream << "\\b";
                break;
            case '\f':
                str_stream << "\\f";
                break;
            case '\n':
                str_stream << "\\n";
                break;
            case '\r':
                str_stream << "\\r";
                break;
            case '\t':
                str_stream << "\\t";
                break;
            default:
                if (('\x00' <= c && c <= '\x1f')) {
                    str_stream << std::hex << std::setw(4)
                                << std::setfill('0') << (int)c;
                } else {
                    str_stream << c;
                }
            }
        }
}
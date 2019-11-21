#ifndef RING_BUFFER_WEBGL
#define RING_BUFFER_WEBGL

#include <libwebsockets.h>
#include "message.h"
#include <argos3/core/utility/datatypes/datatypes.h>
#include <argos3/core/utility/logging/argos_log.h>
#include <argos3/core/utility/datatypes/byte_array.h>
#include <algorithm>


namespace argos {
    class CRingBuffer {
        typedef std::vector<SPerSessionData*> TClients;
        typedef TClients::iterator TIterCleints;
    public:
        CRingBuffer() = delete;
        CRingBuffer(size_t sSize)
            :m_psRingBuffer(lws_ring_create(sizeof(SMessage), sSize, DestroyMessage)) {}
        ~CRingBuffer() {
            lws_ring_destroy(m_psRingBuffer);
        }

        void UpdatedLua(SPerSessionData* ps_except) {
            for (TIterCleints tIter = m_vecClients.begin(); tIter != m_vecClients.end(); ++tIter) {
                SPerSessionData* psSession = *tIter;
                if (psSession != ps_except) {
                    psSession->m_bHasLua = false;
                }
            }
        }

        void EnsureRingSpace() {
            if (lws_ring_get_count_free_elements(m_psRingBuffer) == 0) {
                // Kick lagging clients
                UInt32 uOldestTail = lws_ring_get_oldest_tail(m_psRingBuffer);
                UInt32 uMaxWaitingTail = 0;
                UInt64 uMaxWaiting = 0;
                SPerSessionData* psOldestSession = nullptr;

                for (TIterCleints tIter = m_vecClients.begin(); tIter != m_vecClients.end();) {
                    SPerSessionData* psSession = *tIter;

                    if (psSession->m_uRingTail == uOldestTail) {
                        psOldestSession = psSession;
                        LOGERR << "Kicking lagging client" << std::endl;
                        psSession->m_bKicked = true;
                        tIter = m_vecClients.erase(tIter);
                        continue;
                        // lws_set_timeout(psSession->m_psWSI, PENDING_TIMEOUT_LAGGING, LWS_TO_KILL_SYNC);
                    } else {
                        UInt64 uWaiting = lws_ring_get_count_waiting_elements(m_psRingBuffer, &(psSession->m_uRingTail));
                        uMaxWaitingTail = psSession->m_uRingTail;
                        if (uWaiting > uMaxWaiting) uMaxWaiting = uWaiting;
                    }
                    tIter++;
                }
                lws_ring_update_oldest_tail(m_psRingBuffer, uMaxWaitingTail);
                /*
                lws_ring_consume(m_psRingBuffer, &psOldestSession->m_uRingTail, NULL, 
                    lws_ring_get_count_waiting_elements(m_psRingBuffer, &uOldestTail));
                lws_ring_update_oldest_tail(m_psRingBuffer, psOldestSession->m_uRingTail);
                */
            }
        }

        void AddBinaryMessage(CByteArray* c_data) {
            EnsureRingSpace();
            SMessage psMsg{c_data, LWS_WRITE_BINARY};
            lws_ring_insert(m_psRingBuffer, &psMsg, 1);
        }

        void AddTextMessage(const std::string& str_send) {
            EnsureRingSpace();
            CByteArray* pcData = new CByteArray();
            (*pcData) << str_send;
            pcData->Resize(pcData->Size() - 1);
            SMessage psMsg{pcData, LWS_WRITE_TEXT};
            lws_ring_insert(m_psRingBuffer, &psMsg, 1);
        }

        void AddClient(SPerSessionData* ps_session) {
            m_vecClients.push_back(ps_session);
            ps_session->m_uRingTail = lws_ring_get_oldest_tail(m_psRingBuffer);
        }

        const SMessage* GetMessage(SPerSessionData* ps_session) {
            const SMessage* sMessage = reinterpret_cast<const SMessage*>(
                lws_ring_get_element(m_psRingBuffer, &ps_session->m_uRingTail));
        }

        void CallbackWriteMessage(SPerSessionData* ps_session) {
            const SMessage* sMessage = reinterpret_cast<const SMessage*>(
                lws_ring_get_element(m_psRingBuffer, &ps_session->m_uRingTail));
            ps_session->m_psCurrentSendMessage = *sMessage;
            if (!sMessage) return;
            if (WriteMessage(ps_session)) {
                /* Advance in the session tail in the buffer */
                lws_ring_consume(m_psRingBuffer, &ps_session->m_uRingTail, NULL, 1);
            } else {
                lws_callback_on_writable(ps_session->m_psWSI);
                return;
            }
            /*
            size_t uOldestRemain = lws_ring_get_count_waiting_elements(m_psRingBuffer, &m_vecClients[0]->m_uRingTail);
            UInt32 tOldestTail = m_vecClients[0]->m_uRingTail;

            for (TIterCleints tIter = m_vecClients.begin() + 1; tIter != m_vecClients.end(); ++tIter) {
                size_t uCurrentRemain = lws_ring_get_count_waiting_elements(m_psRingBuffer, &((*tIter)->m_uRingTail));
                if (uCurrentRemain > uOldestRemain) {
                    uOldestRemain = uCurrentRemain;
                    tOldestTail = (*tIter)->m_uRingTail;
                }
            }
            lws_ring_update_oldest_tail(m_psRingBuffer, tOldestTail);

            // */
            if (lws_ring_get_element(m_psRingBuffer, &ps_session->m_uRingTail))
                lws_callback_on_writable(ps_session->m_psWSI);
        }

        void CallbackCancel() {
            for (TIterCleints tIter = m_vecClients.begin(); tIter != m_vecClients.end(); ++tIter) {
            if (lws_ring_get_count_waiting_elements(m_psRingBuffer, &((*tIter)->m_uRingTail)))
                lws_callback_on_writable((*tIter)->m_psWSI);
            }
        }

        void DisconectedClient(SPerSessionData* ps_session) {
            m_vecClients.erase(std::find(m_vecClients.begin(), m_vecClients.end(), ps_session));
        }

    private:
        lws_ring* m_psRingBuffer;
        TClients m_vecClients;
    };
}

#endif
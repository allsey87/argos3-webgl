#include "websocket_server.h"
#include <algorithm>

#define MY_PROTOCOL                                                            \
    { "spawn-objects", my_callback, sizeof(SPerSessionData), 1024, 0, NULL, 0 }

namespace argos {

static const struct lws_protocol_vhost_options GLB_MIMETYPE = {
        NULL, NULL, ".glb", "application/x-binary"};

static struct lws_http_mount MOUNT_SETTINGS = {
        /* .mount_next*/           NULL,        /* linked-list "next" */
        /* .mountpoint */        "/",        /* mountpoint URL */
        /* .origin */        nullptr,    /* serve from dir */
        /* .def */            "index.html",    /* default filename */
        /* .protocol */            NULL,
        /* .cgienv */            NULL,
        /* .extra_mimetypes */    &GLB_MIMETYPE,
        /* .interpret */        NULL,
        /* .cgi_timeout */        0,
        /* .cache_max_age */        0,
        /* .auth_mask */        0,
        /* .cache_reusable */        0,
        /* .cache_revalidate */        0,
        /* .cache_intermediaries */    0,
        /* .origin_protocol */        LWSMPRO_FILE,    /* files in a dir */
        /* .mountpoint_len */        1,        /* char count */
        /* .basic_auth_login_file */    NULL,
};

static struct lws_protocols PROTOCOLS[] = {
    {"http", lws_callback_http_dummy, 0, 0}, MY_PROTOCOL, {NULL, NULL, 0, 0}};

struct lws_protocol_vhost_options ACCESS_CONTROL_ALLOW_ORIGIN {
    nullptr, nullptr, "access-control-allow-origin:", "*"
};

struct lws_protocol_vhost_options ACCESS_CONTROL_REQUEST_HEADERS = {
    /*next*/ &ACCESS_CONTROL_ALLOW_ORIGIN,
    /*options*/ nullptr, "access-control-request-headers:", "*"};

static void __destroy_message(void *ps_message) {
    SMessage* psMessage = reinterpret_cast<SMessage*>(ps_message);
    delete psMessage->data;
}

CWebsocketServer::CWebsocketServer(std::string str_HostName, UInt16 un_Port,
                                   std::string str_Static, CPlayState* pc_paly_state,
                                   CWebGLRender* pc_visualization)
    : m_strHostName(str_HostName), m_strStatic(str_Static), m_unPort(un_Port),
    m_pcPalyState(pc_paly_state), m_pcVisualization(pc_visualization),
    m_psRingBuffer(lws_ring_create(sizeof(SMessage), 80, __destroy_message)) {
    struct lws_context_creation_info sInfo;
    memset(&sInfo, 0, sizeof sInfo);
    MOUNT_SETTINGS.origin = m_strStatic.c_str();
    sInfo.mounts = &MOUNT_SETTINGS;
    sInfo.vhost_name = m_strHostName.c_str();
    sInfo.port = un_Port;
    sInfo.protocols = PROTOCOLS;
    sInfo.user = reinterpret_cast<void *>(this);
    sInfo.headers = &ACCESS_CONTROL_REQUEST_HEADERS;
    lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);
    m_psContext = lws_create_context(&sInfo);
    if (!m_psContext) {
        LOGERR << "Context creation failed" << std::endl;
    }
}

void CWebsocketServer::Run() {
    while (!m_bStop) {
        Step();
    }
}

void CWebsocketServer::Step() { lws_service(m_psContext, 10); }

void CWebsocketServer::SendBinary(CByteArray* c_data) {
    if (m_vecClients.empty()) {
        return;
    }
    EnsureRingSpace();
    SMessage psMsg{c_data, LWS_WRITE_BINARY};
    lws_ring_insert(m_psRingBuffer, &psMsg, 1);
    lws_callback_on_writable_all_protocol(m_psContext, PROTOCOLS + 1);
}

void CWebsocketServer::EnsureRingSpace() {
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

void CWebsocketServer::SendText(const std::string& std_send) {
    EnsureRingSpace();
    CByteArray* pcData = new CByteArray();
    (*pcData) << std_send;
    pcData->Resize(pcData->Size() - 1);

    SMessage psMsg{pcData, LWS_WRITE_TEXT};
    lws_ring_insert(m_psRingBuffer, &psMsg, 1);
    lws_cancel_service(m_psContext);
}


int CWebsocketServer::Callback(SPerSessionData *ps_session, lws_callback_reasons e_reason,
            UInt8* un_bytes, size_t u_len) {
    
    switch (e_reason) {
    case LWS_CALLBACK_ESTABLISHED:
        m_vecClients.push_back(ps_session);
        ps_session->m_uLastSpawnedNetId = 0; // maybe the already 0 at allocation
        ps_session->m_bKicked = false;
        ps_session->m_uSent = 0;
        ps_session->m_psCurrentMessage.data = new CByteArray();
        lws_callback_on_writable(ps_session->m_psWSI);
        break;
    case LWS_CALLBACK_PROTOCOL_DESTROY:
        m_bStop = true;
        break;
    case LWS_CALLBACK_CLOSED:
        delete ps_session->m_psCurrentMessage.data;
        if (ps_session->m_bKicked) break;
        m_vecClients.erase(std::find(m_vecClients.begin(), m_vecClients.end(), ps_session));
        break;
    case LWS_CALLBACK_SERVER_WRITEABLE:
        if (ps_session->m_bKicked) return -1;

        if (ps_session->m_uLastSpawnedNetId < m_pcVisualization->GetRootEntitiesCount()) {

            CByteArray* pcData = m_pcVisualization->GetSpawnMsg(ps_session->m_uLastSpawnedNetId);
            SMessage sMessage = {pcData, LWS_WRITE_TEXT};

            if (WriteMessage(ps_session, &sMessage)) {
                ps_session->m_uLastSpawnedNetId += 1;
                // the client finished spawning all entities
                if (ps_session->m_uLastSpawnedNetId == m_pcVisualization->GetRootEntitiesCount()) {
                    ps_session->m_uRingTail = lws_ring_get_oldest_tail(m_psRingBuffer);
                }
            }
            lws_callback_on_writable(ps_session->m_psWSI);
        } else {

            const SMessage* sMessage = reinterpret_cast<const SMessage*>(
                lws_ring_get_element(m_psRingBuffer, &ps_session->m_uRingTail));
            lws_ring_get_count_waiting_elements(m_psRingBuffer, &ps_session->m_uRingTail);
            if (!sMessage) break;

            if (WriteMessage(ps_session, sMessage)) {
                lws_ring_consume(m_psRingBuffer, &ps_session->m_uRingTail, NULL, 1);
            } else {
                lws_callback_on_writable(ps_session->m_psWSI);
                break;
            }

            size_t uOldestRemain = lws_ring_get_count_waiting_elements(m_psRingBuffer, &m_vecClients[0]->m_uRingTail);
            UInt32 tOldestTail = m_vecClients[0]->m_uRingTail;

            for (TIterCleints tIter = m_vecClients.begin() + 1; tIter != m_vecClients.end(); ++tIter) {
                // ignore users who are still receiving spawn messages
                if ((*tIter)->m_uLastSpawnedNetId < ps_session->m_uLastSpawnedNetId) continue;
                size_t uCurrentRemain = lws_ring_get_count_waiting_elements(m_psRingBuffer, &((*tIter)->m_uRingTail));
                if (uCurrentRemain > uOldestRemain) {
                    uOldestRemain = uCurrentRemain;
                    tOldestTail = (*tIter)->m_uRingTail;
                }
            }
            lws_ring_update_oldest_tail(m_psRingBuffer, tOldestTail);

            if (lws_ring_get_element(m_psRingBuffer, &ps_session->m_uRingTail))
                lws_callback_on_writable(ps_session->m_psWSI);
        }
        break;
    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        for (TIterCleints tIter = m_vecClients.begin(); tIter != m_vecClients.end(); ++tIter) {
            if (lws_ring_get_count_waiting_elements(m_psRingBuffer, &((*tIter)->m_uRingTail)))
                lws_callback_on_writable((*tIter)->m_psWSI);
        }
        break;
    case LWS_CALLBACK_RECEIVE:
        ps_session->m_psCurrentMessage.data->AddBuffer(un_bytes, u_len);

        if (lws_is_final_fragment(ps_session->m_psWSI)) {
            ps_session->m_psCurrentMessage.type = lws_frame_is_binary(ps_session->m_psWSI)
                ? LWS_WRITE_BINARY
                : LWS_WRITE_TEXT;
            ReceivedMessage(&ps_session->m_psCurrentMessage);
            ps_session->m_psCurrentMessage.data->Clear();
        }
        break;
    default:
        LOGERR << "[LWS] Unhandled case reason=" << e_reason << std::endl;
        break;
    }
    return 0;
}


bool CWebsocketServer::WriteMessage(SPerSessionData* ps_session, const SMessage* ps_message) {
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

void CWebsocketServer::ReceivedMessage(SMessage *ps_msg) {
        lwsl_user("Received size: %lu\n", ps_msg->data->Size());
        UInt8 unMessageType;
        *(ps_msg->data) >> unMessageType;
        switch(unMessageType) {
            case EClientMessageType::PAUSE:
                m_pcPalyState->Pause();
            break;
            case EClientMessageType::AUTO:
                m_pcPalyState->Automatic();
            break;
            case EClientMessageType::STEP:
                m_pcPalyState->Frame();
            break;
            case EClientMessageType::MOVE:
                m_pcVisualization->RecievedMove(*(ps_msg->data));
                break;
        default:
            lwsl_err("Unknown message type\n");
            break;
        }
    }

CWebsocketServer::~CWebsocketServer() {
    m_bStop = false;
    if (m_psContext)
        lws_context_destroy(m_psContext);
}

int my_callback(lws *ps_wsi, enum lws_callback_reasons e_reason, void *user,
                void *in, size_t un_len) {
    if (e_reason == LWS_CALLBACK_PROTOCOL_INIT) {
        SDataPerVhost *ps_vhd =
            reinterpret_cast<SDataPerVhost *>(lws_protocol_vh_priv_zalloc(
                lws_get_vhost(ps_wsi), lws_get_protocol(ps_wsi),
                sizeof(SDataPerVhost)));
        ps_vhd->context = lws_get_context(ps_wsi);
        ps_vhd->protocol = lws_get_protocol(ps_wsi);
        ps_vhd->vhost = lws_get_vhost(ps_wsi);
        ps_vhd->server = reinterpret_cast<CWebsocketServer *>(
            lws_context_user(ps_vhd->context));
        LOG << ps_vhd << std::endl;
    } else {
        SDataPerVhost *ps_vhd =
            reinterpret_cast<SDataPerVhost *>(lws_protocol_vh_priv_get(
                lws_get_vhost(ps_wsi), lws_get_protocol(ps_wsi)));
        SPerSessionData *ps_SessionData =
            reinterpret_cast<SPerSessionData *>(user);
        if (e_reason == LWS_CALLBACK_ESTABLISHED) {
            ps_SessionData->m_psWSI = ps_wsi;
        }
        return ps_vhd->server->Callback(ps_SessionData, e_reason, (UInt8*) in, un_len);
    }
    return 0;
}

} // namespace argos

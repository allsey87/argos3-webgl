#include "websocket_server.h"

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

CWebsocketServer::CWebsocketServer(std::string str_HostName, UInt16 un_Port,
                                   std::string str_Static, CPlayState* pc_paly_state)
    : m_strHostName(str_HostName), m_strStatic(str_Static), m_unPort(un_Port),
    m_sClient(nullptr), m_pcPalyState(pc_paly_state) {
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

void CWebsocketServer::waitForConnection() {
    while (m_sClient == nullptr) {
        Step();
    }
}

void CWebsocketServer::Step() { lws_service(m_psContext, 10); }

void CWebsocketServer::SendBinary(CByteArray c_data) {
    m_MessageQueue.push_back({c_data, LWS_WRITE_BINARY});
    lws_cancel_service(m_psContext);
}

void CWebsocketServer::SendText(std::string send) {
    CByteArray cData;
    cData << send;
    cData.Resize(cData.Size() - 1); // avoid having a \0
    m_MessageQueue.push_back({cData, LWS_WRITE_TEXT});
    lws_cancel_service(m_psContext);
}

int CWebsocketServer::Callback(SPerSessionData *ps_session_data, lws_callback_reasons e_reason,
            UInt8* un_bytes, size_t len) {
    switch (e_reason) {
    case LWS_CALLBACK_ESTABLISHED:
        if (m_sClient != nullptr) {
            lwsl_user("Already has a connection\n");
        } else {
            m_sClient = ps_session_data;
        }
        lws_callback_on_writable(ps_session_data->wsi);
        break;
    case LWS_CALLBACK_PROTOCOL_DESTROY:
        m_bStop = true;
        break;
    case LWS_CALLBACK_CLOSED:
        if (ps_session_data == m_sClient) {
            lwsl_user("Browser disconnected\n");
            m_sClient = nullptr;
        }
        break;
    case LWS_CALLBACK_SERVER_WRITEABLE:
        if (ps_session_data != m_sClient) {
            lwsl_user("Kicking. On writable\n");
            return -1;
        }
        if (!m_MessageQueue.empty()) {
            auto a = m_MessageQueue.front();
            size_t size = a.data.Size();
            UInt8 buffer[size + LWS_PRE];
            a.data.FetchBuffer(buffer + LWS_PRE, size);
            lws_write(ps_session_data->wsi, buffer + LWS_PRE, size, a.type);
            m_MessageQueue.pop_front();
            if (!m_MessageQueue.empty()) {
                lws_callback_on_writable(ps_session_data->wsi);
            }
        }
        break;
    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        if (m_sClient) lws_callback_on_writable(m_sClient->wsi);
        break;
    case LWS_CALLBACK_RECEIVE:
        m_cCurrentMessage.AddBuffer(un_bytes, len);
        if (lws_is_final_fragment(ps_session_data->wsi)) {
            RecievedMessage();
            m_cCurrentMessage.Clear();
        }
        break;
    default:
        LOGERR << "[LWS] Unhandled case " << std::endl;
        break;
    }
    return 0;
}

void CWebsocketServer::RecievedMessage() {
        lwsl_user("Recieved size: %d\n", m_cCurrentMessage.Size());
        UInt8 unMessageType;
        m_cCurrentMessage >> unMessageType;
        switch(unMessageType) {
            case EClientMessageType::PAUSE:
                lwsl_user("PAUSE\n");
                m_pcPalyState->Pause();
            break;

            case EClientMessageType::AUTO:
                lwsl_user("AUTO\n");
                m_pcPalyState->Automatic();
            break;
            case EClientMessageType::STEP:
                lwsl_user("STEP\n");
                m_pcPalyState->Frame();
            break;
        default:
            lwsl_err("Unhandled case\n");
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
    } else {
        SDataPerVhost *ps_vhd =
            reinterpret_cast<SDataPerVhost *>(lws_protocol_vh_priv_get(
                lws_get_vhost(ps_wsi), lws_get_protocol(ps_wsi)));
        SPerSessionData *ps_SessionData =
            reinterpret_cast<SPerSessionData *>(user);

        if (!ps_vhd) { // Not initialized
            return 0;
        } else if (e_reason == LWS_CALLBACK_ESTABLISHED) {
            ps_SessionData->wsi = ps_wsi;
        }
        return ps_vhd->server->Callback(ps_SessionData, e_reason, (UInt8*) in, un_len);
    }
    return 0;
}

} // namespace argos

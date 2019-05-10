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
                                   std::string str_Static)
    : m_strHostName(str_HostName), m_strStatic(str_Static), m_unPort(un_Port),
    m_sClient(nullptr) {
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

int CWebsocketServer::Callback(lws *ps_WSI, lws_callback_reasons e_Reason,
                               SPerSessionData *ps_SessionData) {
    switch (e_Reason) {
    case LWS_CALLBACK_ESTABLISHED:
        if (m_sClient != nullptr) {
            lwsl_user("Already has a connection\n");
        } else {
            ps_SessionData->wsi = ps_WSI;
            m_sClient = ps_SessionData;
        }
        lws_callback_on_writable(ps_WSI);
        break;
    case LWS_CALLBACK_PROTOCOL_DESTROY:
        m_bStop = true;
        break;
    case LWS_CALLBACK_CLOSED:
        if (ps_SessionData == m_sClient) {
            lwsl_user("Browser disconnected\n");
            m_sClient = nullptr;
        }
        break;
    case LWS_CALLBACK_SERVER_WRITEABLE:
        if (ps_SessionData != m_sClient) {
            lwsl_user("Kicking. On writable\n");
            return -1;
        }
        if (!m_MessageQueue.empty()) {
            auto a = m_MessageQueue.front();
            size_t size = a.data.Size();
            UInt8 buffer[size + LWS_PRE];
            a.data.FetchBuffer(buffer + LWS_PRE, size);
            lws_write(ps_WSI, buffer + LWS_PRE, size, a.type);
            m_MessageQueue.pop_front();
            if (!m_MessageQueue.empty()) {
                lws_callback_on_writable(ps_WSI);
            }
        }
        break;
    case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        if (m_sClient) lws_callback_on_writable(m_sClient->wsi);
        break;
    default:
        LOGERR << "[LWS] Unhandled case " << std::endl;
        break;
    }
    return 0;
}

CWebsocketServer::~CWebsocketServer() {
    m_bStop = false;
    if (m_psContext)
        lws_context_destroy(m_psContext);
}

int my_callback(lws *s_wsi, enum lws_callback_reasons e_reason, void *user,
                void *in, size_t len) {
    if (e_reason == LWS_CALLBACK_PROTOCOL_INIT) {
        SDataPerVhost *ps_vhd =
            reinterpret_cast<SDataPerVhost *>(lws_protocol_vh_priv_zalloc(
                lws_get_vhost(s_wsi), lws_get_protocol(s_wsi),
                sizeof(SDataPerVhost)));
        ps_vhd->context = lws_get_context(s_wsi);
        ps_vhd->protocol = lws_get_protocol(s_wsi);
        ps_vhd->vhost = lws_get_vhost(s_wsi);
        ps_vhd->server = reinterpret_cast<CWebsocketServer *>(
            lws_context_user(ps_vhd->context));
    } else {
        SDataPerVhost *ps_vhd =
            reinterpret_cast<SDataPerVhost *>(lws_protocol_vh_priv_get(
                lws_get_vhost(s_wsi), lws_get_protocol(s_wsi)));
        if (!ps_vhd) { // Not initialized
            return 0;
        }
        SPerSessionData *ps_SessionData =
            reinterpret_cast<SPerSessionData *>(user);
        return ps_vhd->server->Callback(s_wsi, e_reason, ps_SessionData);
    }
    return 0;
}

} // namespace argos

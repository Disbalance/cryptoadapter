/***************************************************
 * websocket.cpp
 * Created on Fri, 28 Sep 2018 09:37:36 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <map>
#include <iostream>
#include <exception>
#include <platform/log.h>
#include "websocket.h"

namespace platform {

std::set<WebSocketClient*> WebSocketClient::s_instances;
size_t WebSocketClient::s_numInstance = 0;

WebSocketConnection::WebSocketConnection(WebSocketClient *client)
  : m_closing(false)
  , m_webSocket(NULL)
  , m_client(client)
  , m_wantWrite(false)
{
  m_writeBuffer.resize(LWS_PRE);
  m_writtenBuffer.resize(LWS_PRE);
}

WebSocketConnection::~WebSocketConnection()
{
  m_closing = true;
  if(m_client) {
    m_client->detach(this);
  }
  if(m_webSocket) {
    WebSocketConnection **wsc = (WebSocketConnection**)lws_wsi_user(m_webSocket);
    *wsc = NULL;
  }
  if(m_connectInfo.userdata) {
    delete (void**)m_connectInfo.userdata;
  }
}

void WebSocketConnection::setConnectionHandler(WebSocketConnectionHandler *handler)
{
  m_handler = handler;
}

void WebSocketConnection::connect(std::string url)
{
  const char *tmpProtocol;
  const char *tmpAddress;
  int  port;
  const char *tmpPath;

  if(lws_parse_uri(&url[0], &tmpProtocol, &tmpAddress, &port, &tmpPath)) {
    throw std::invalid_argument("invalid url");
  }

  auto protocol = std::string(tmpProtocol);
  auto address = std::string(tmpAddress);
  auto path = std::string("/").append(tmpPath);

  bool ssl = false;
  if(protocol == std::string("wss")) {
      ssl = true;
  }
  else if(protocol == std::string("ws")) {
    ssl = false;
  }  
  else {
    throw std::invalid_argument("invalid protocol: '" + protocol + "'. Must be 'ws' or 'wss'.");
  }

  connect(std::move(address), port, std::move(path), ssl);
}

void WebSocketConnection::connect(std::string address, int port, std::string path, bool ssl)
{
  static const char *origin = "fin";
  if(m_webSocket) {
    return;
  }

  memset(&m_connectInfo, 0, sizeof(m_connectInfo));

  m_address.swap(address);
  m_port = port;
  m_path.swap(path);
  m_ssl = ssl;
  m_connectInfo.address = m_address.c_str();
  m_connectInfo.host    = m_address.c_str();
  m_connectInfo.port = m_port;
  m_connectInfo.path = m_path.c_str();
  m_connectInfo.ssl_connection = ssl;
  m_connectInfo.ietf_version_or_minus_one = -1;
  m_connectInfo.origin = origin;

  WebSocketConnection **thisInfo = new (WebSocketConnection*);
  *thisInfo = this;

  m_connectInfo.userdata = (void*)thisInfo;
  if(m_client) {
    m_client->add(this);
  }
}

void WebSocketConnection::disconnect()
{
  if(m_webSocket) {
    WebSocketConnection **wsc = (WebSocketConnection**)lws_wsi_user(m_webSocket);
    if(wsc)
    {
      *wsc = NULL;
    }
    memset(&m_connectInfo, 0, sizeof(m_connectInfo));
    m_webSocket = NULL;
  }
}

void WebSocketConnection::reconnect()
{
  connect(m_address, m_port, m_path, m_ssl);
}

bool WebSocketConnection::isConnected()
{
  return m_webSocket != NULL;
}

int WebSocketConnection::write(const char *buffer, size_t size)
{
  if(m_webSocket) {
    {
    std::lock_guard<std::mutex> lock(m_writeSync);
    m_writeBuffer.insert(m_writeBuffer.end(), buffer, buffer + size);
    m_writeBuffer.resize(m_writeBuffer.size() + LWS_PRE);
    m_writeMessageSizes.emplace_back(size);
    m_wantWrite = true;
    }
    lws_cancel_service_pt(m_webSocket);
    return size;
  }

  return -1;
}

int WebSocketConnection::onDataReady(char *data, size_t length) {
  if(m_closing) {
    return -1;
  }

  if(data) {
    m_readBuffer.insert(m_readBuffer.end(), data, data + length);
    const size_t remaining = lws_remaining_packet_payload(m_webSocket);

    if(remaining || !lws_is_final_fragment(m_webSocket)) {
      return 0;
    }
  }

  if(m_handler) {
    WSMessage msg;
    msg.type = data ? (lws_frame_is_binary(m_webSocket) ? BINARY : TEXT) : TEXT;
    if(m_readBuffer.size() == m_readBuffer.capacity()) {
      m_readBuffer.emplace_back(0);
      m_readBuffer.pop_back();
    }
    msg.data = m_readBuffer.data();
    msg.size = m_readBuffer.size();
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    msg.timestamp = now.tv_sec * 1000000000 + now.tv_nsec;
    m_handler->onDataReady(this, msg);
  }

  m_readBuffer.resize(0);
  return 0;
}

void WebSocketConnection::checkTimers() {
  if(m_handler) {
    m_handler->checkTimers();
  }
}

int WebSocketConnection::onConnectFailed() {
  if(m_handler) {
    return m_handler->onConnectFailed(this);
  }
  return 0;
}

int WebSocketConnection::onConnected() {
  if(m_handler) {
    m_handler->onConnected(this);
  }
  return 0;
}

int WebSocketConnection::onWrite()
{
  if(m_closing) {
    return -1;
  }

  if(!m_wantWrite) {
    return 0;
  }

  {
  std::lock_guard<std::mutex> lock(m_writeSync);
  m_writeBuffer.swap(m_writtenBuffer);
  m_writeMessageSizes.swap(m_writtenMessageSizes);
  m_wantWrite = false;
  }

  if(m_writtenBuffer.size() <= LWS_PRE) {
    return 0;
  }

  size_t ptr = LWS_PRE, i = 0;
  WSMessage msg;
  msg.type = TEXT;

  while(ptr < m_writtenBuffer.size()) {
    if(m_readBuffer.size() == m_readBuffer.capacity()) {
      m_readBuffer.emplace_back(0);
      m_readBuffer.pop_back();
    }
    msg.data = (char*)&m_writtenBuffer[ptr];
    msg.size = m_writtenMessageSizes[i];
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    msg.timestamp = now.tv_sec * 1000000000 + now.tv_nsec;

    if(m_handler) {
      m_handler->onMessageSent(this, msg);
    }

    lws_write(m_webSocket, (unsigned char*)msg.data, msg.size, LWS_WRITE_TEXT);

    ptr += m_writtenMessageSizes[i] + LWS_PRE;
    i ++;
  }
  m_writtenBuffer.resize(LWS_PRE);
  m_writtenMessageSizes.resize(0);
  return 0;
}

int WebSocketConnection::onClose()
{
  if(m_client) {
    m_client->remove(this);
  }
  m_webSocket = NULL;
  if(m_handler) {
    m_handler->onClose(this);
  }
  return 0;
}

int WebSocketConnection::wsCallback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
  WebSocketConnection *wsc = NULL;
  if(user) {
    wsc = *(WebSocketConnection**)user;
  }

  switch(reason) {
  case LWS_CALLBACK_CLIENT_ESTABLISHED:
    if(wsc) {
      return wsc->onConnected();
    }
    return -1;
  case LWS_CALLBACK_CLIENT_RECEIVE:
    if(wsc) {
      return wsc->onDataReady((char*)in, len);
    }
    return -1;
  case LWS_CALLBACK_CLIENT_WRITEABLE:
    if(wsc) {
      wsc->checkTimers();
      if(wsc->m_wantWrite) {
        return wsc->onWrite();
      }
      return 0;
    }
    return -1;
  case LWS_CALLBACK_CLOSED:
    {
    WebSocketClient *client = NULL;
    int result = 0;
    if(wsc) {
      client = wsc->m_client;
      result = wsc->onClose();
    } else if(wsi) {
      client = (WebSocketClient*)lws_context_user(lws_get_context(wsi));
    }
    if(client) {
      client->removeHandle(wsi);
    }
    return result;
    }
  case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    if(wsc) {
      wsc->onConnectFailed();
    }
    return -1;
  case LWS_CALLBACK_WSI_DESTROY:
    if(user) {
      delete (void**)user;
    }
  default:
    //if(wsc) {
      //wsc->onDataReady(NULL, 0);
    //}
    return 0;
  }
  return 0;
}



WebSocketClient::WebSocketClient(const std::string &outboundAddr, bool enableLWSLogging)
  : m_changed(false)
  , m_protocols({
                  {
                    "default-protocol",
                    &WebSocketConnection::wsCallback,
                    sizeof(void*),
                    4096,
                    0,
                    this
                  },
                  { NULL, NULL, 0, 0 } /* terminator */
                })
  , m_outgoingInterface(outboundAddr)
  , m_numConnections(0)
{
  if(!enableLWSLogging)
  {
    lws_set_log_level(LLL_DEBUG, &wsLog);
    lws_set_log_level(LLL_INFO, &wsLog);
    lws_set_log_level(LLL_NOTICE, &wsLog);
    lws_set_log_level(LLL_WARN, &wsLog);
    lws_set_log_level(LLL_ERR, &wsLog);
  }

  lws_context_creation_info info;
  memset(&info, 0, sizeof(info));

  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = m_protocols.data();
  info.gid = -1;
  info.uid = -1;

  if(!m_outgoingInterface.empty()) {
    info.iface = m_outgoingInterface.c_str();
  }

  #if LWS_LIBRARY_VERSION_MAJOR >= 2
  info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
  #endif

  m_context = lws_create_context(&info);

  m_running = true;

  m_thread = std::thread([this]() { run(); });
  s_instances.insert(this);
}

void WebSocketClient::wsLog(int level, const char *line) {
  static std::map<int, Logger::Severity> logLevelMapping = {
    {LLL_ERR, Logger::Critical}, 
    {LLL_WARN, Logger::Error}, 
    {LLL_NOTICE, Logger::Warning}, 
    {LLL_INFO, Logger::Info}, 
    {LLL_DEBUG, Logger::Debug}
  };

  auto i = logLevelMapping.find(level);
  if(i == logLevelMapping.end()) {
    return;
  }

  long len = strlen(line);
  if(line[len-1] == '\n') {
    len --;
  }
  LoggerMessage(i->second) << std::string(line, len);
}

WebSocketClient::~WebSocketClient()
{
  s_instances.erase(this);
  m_running = false;
  lws_cancel_service(m_context);
  m_thread.join();

  for(auto conn : m_boundConnections) {
    conn->m_webSocket = NULL;
    conn->m_client = NULL;
  }

  lws_context_destroy(m_context);
}

WebSocketConnection *WebSocketClient::createConnection()
{
  WebSocketConnection *conn = new WebSocketConnection(this);
  m_boundConnections.insert(conn);
  return conn;
}

WebSocketClient &WebSocketClient::instance()
{
  if(!s_instances.size()) {
    throw std::runtime_error("WebSocketClient instance does not exist!");
  }

  size_t instance = s_numInstance ++;
  s_numInstance %= s_instances.size();

  auto currentInstance = s_instances.begin();
  while(instance -- && currentInstance != s_instances.end()) {
    currentInstance ++;
  }

  return **currentInstance;
}

void WebSocketClient::detach(WebSocketConnection *conn)
{
  m_boundConnections.erase(conn);
  remove(conn);
}

void WebSocketClient::add(WebSocketConnection *conn)
{
  std::lock_guard<std::mutex> lock(m_connectionsSync);
  m_addConnections.insert(conn);
  m_changed = true;
  lws_cancel_service(m_context);
}

void WebSocketClient::remove(WebSocketConnection *conn)
{
  std::lock_guard<std::mutex> lock(m_connectionsSync);
  if(m_addConnections.find(conn) != m_addConnections.end()) {
    m_addConnections.erase(conn);
  }
  if(!conn->m_webSocket) {
    return;
  }
  m_removeConnections.insert(conn->m_webSocket);
  m_changed = true;
  lws_cancel_service(m_context);
}

void WebSocketClient::removeHandle(lws *handle)
{
  m_connections.erase(handle);
  lws_cancel_service(m_context);
}

void WebSocketClient::run()
{
  while(m_running) {
    if(m_changed) {
      std::lock_guard<std::mutex> lock(m_connectionsSync);
      for(auto &conn : m_addConnections) {
        if(conn->m_webSocket == NULL) {
          conn->m_connectInfo.context  = m_context;
          conn->m_connectInfo.protocol = m_protocols[0].name;
          conn->m_webSocket = lws_client_connect_via_info(&conn->m_connectInfo);
          conn->m_connectInfo.userdata = NULL;
          if(conn->m_webSocket) {
            m_connections.insert(conn->m_webSocket);
            m_numConnections ++;
          } else {
            conn->onConnectFailed();
          }
        }
      }
      m_addConnections.clear();
      for(auto &conn : m_removeConnections) {
        m_connections.erase(conn);
        m_numConnections --;
      }
      m_removeConnections.clear();
      m_changed = false;
    }

    if(!m_numConnections) {
      usleep(250000);
      continue;
    }

    int result = lws_service(m_context, 10);

    if(result) {
      std::cout << "LWS service return value: " << result << std::endl;
    }

    for(auto &conn : m_connections) {
      if(lws_callback_on_writable(conn) < 0) {
        std::lock_guard<std::mutex> lock(m_connectionsSync);
        m_removeConnections.insert(conn);
        m_changed = true;
      }
    }
  }
}

}

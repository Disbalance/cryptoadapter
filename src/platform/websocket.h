/***************************************************
 * websocket.h
 * Created on Fri, 28 Sep 2018 09:27:34 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <atomic>
#include <set>
#include <thread>
#include <mutex>
#include <functional>
#include <vector>
#include <libwebsockets.h>

namespace platform {

enum WSFrameType {
  BINARY = 0,
  TEXT
};

struct WSMessage {
  WSFrameType type;
  const char *data;
  size_t size;
  unsigned long timestamp;
};

class WebSocketConnection;
typedef std::function<int (WebSocketConnection *)> WSCallback;
typedef std::function<int (WebSocketConnection *, WSMessage msg)> WSReadCallback;
class WebSocketClient;

class WebSocketConnectionHandler {
public:
  virtual int onDataReady(WebSocketConnection *, WSMessage msg) { return 0; }
  virtual int onConnected(WebSocketConnection *) { return 0; }
  virtual int onConnectFailed(WebSocketConnection *) { return 0; }
  virtual int onClose(WebSocketConnection *) { return 0; }
  virtual int onMessageSent(WebSocketConnection *, WSMessage msg) { return 0; }
  virtual void checkTimers() { }
};

class WebSocketConnection {
public:
  ~WebSocketConnection();
  void connect(std::string url);
  void connect(std::string address, int port, std::string path, bool ssl);
  void reconnect();
  void disconnect();
  bool isConnected();

  int write(const char *, size_t);

  int onDataReady(char *, size_t);
  int onConnected();
  int onConnectFailed();
  int onWrite();
  int onClose();
  void checkTimers();

  void setConnectionHandler(WebSocketConnectionHandler *handler);

private:
  static int wsCallback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);
  WebSocketConnection(WebSocketClient *);

  WebSocketConnection(const WebSocketConnection &) = delete;
  void operator =(const WebSocketConnection &) = delete;

  WebSocketConnectionHandler *m_handler;
  bool m_closing;
  lws_client_connect_info m_connectInfo;
  lws *m_webSocket;
  std::string m_address;
  std::string m_path;
  std::mutex  m_writeSync;
  std::vector<char> m_readBuffer;
  std::vector<char> m_writeBuffer;
  std::vector<size_t> m_writeMessageSizes;
  std::vector<char> m_writtenBuffer;
  std::vector<size_t> m_writtenMessageSizes;
  int m_port;
  bool m_ssl;
  WebSocketClient *m_client;
  bool m_wantWrite;

  friend class WebSocketClient;
};

class WebSocketClient {
public:
  WebSocketClient(const std::string &outboundAddr = "", bool enableLWSLogging = false);
  ~WebSocketClient();

  WebSocketConnection *createConnection();

  static WebSocketClient &instance();

private:
  static void wsLog(int level, const char *line);
  void add(WebSocketConnection *conn);
  void remove(WebSocketConnection *conn);
  void detach(WebSocketConnection *conn);
  void removeHandle(lws*);
  void run();

  WebSocketClient(const WebSocketClient &) = delete;
  void operator =(const WebSocketClient &) = delete;

  bool m_running;
  std::atomic<bool> m_changed;
  std::thread m_thread;
  std::mutex  m_connectionsSync;
  std::set<WebSocketConnection*> m_boundConnections;
  std::set<WebSocketConnection*> m_addConnections;
  std::set<lws*> m_removeConnections;

  std::vector<lws_protocols> m_protocols;
  lws_context *m_context;
  std::set<lws*> m_connections;
  static std::set<WebSocketClient*> s_instances;
  static size_t s_numInstance;
  std::string m_outgoingInterface;
  int m_numConnections;

  friend class WebSocketConnection;
};

}

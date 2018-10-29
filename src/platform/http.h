/***************************************************
 * http.h
 * Created on Wed, 26 Sep 2018 16:05:09 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/
#pragma once

#include <atomic>
#include <set>
#include <map>
#include <string>
#include <list>
#include <thread>
#include <mutex>
#include <sstream>
#include <functional>
#include <curl/curl.h>

namespace platform {

enum class HttpMethod {
  GET = 0,
  POST,
  DELETE,
  PUT
};

class HttpClient;
class HttpResponse;
class HttpRequest;

typedef std::multimap<std::string, std::string> HttpHeaderList;

class HttpResponse {
public:
  HttpResponse(const HttpResponse &);
  long getStatus() const;
  bool isTimedOut() const;
  const std::string &getBody() const;
  const HttpHeaderList &getHeaders() const;
  const HttpRequest &getRequest() const;

private:
  HttpResponse(CURL *handle);

  long m_status;
  HttpHeaderList m_headers;
  std::string m_body;
  bool m_timedOut;
  HttpRequest *m_request;

  friend class HttpClient;
  friend class HttpRequest;
};

class HttpReadyStateHandler {
public:
  virtual void onReadyState(const HttpResponse *) = 0;
};

class HttpRequest {
public:
  ~HttpRequest();
  void setBody(std::string body);
  void setMethod(HttpMethod method);
  void addHeader(std::string name, std::string value);
  void setConnectionTimeout(long ms);
  void setResponseTimeout(long ms);
  void setOutgoingInterface(const std::string &ip);
  void send();

  void setReadyStateHandler(HttpReadyStateHandler *);

  const std::string &getUrl() const {
    return m_url;
  }
  const std::string getBody() const {
    return m_body.str();
  }
  const HttpMethod getMethod() const {
    return m_method;
  }
  const HttpHeaderList &getHeaders() const {
    return m_headers;
  }

private:
  HttpRequest(std::string url, HttpClient *);
  HttpRequest(HttpMethod method, std::string url, HttpClient *);

  HttpRequest(const HttpRequest &) = delete;
  void operator =(const HttpRequest &) = delete;

  static size_t httpRead(void *ptr, size_t size, size_t nmemb, void *userp);
  static size_t httpWrite(char *ptr, size_t size, size_t nmemb, void *userp);
  static size_t httpWriteHeaders(char *ptr, size_t size, size_t nmemb, void *userp);
  void onReadyState(const HttpResponse &resp);
  void setHandle(CURL *m_handle);

  CURL *m_handle;
  std::string m_url;
  HttpMethod m_method;
  std::stringstream m_body;
  HttpHeaderList m_headers;
  curl_slist *m_curlHeaders;
  std::stringstream m_responseBody;
  std::stringstream m_responseHeaders;
  HttpReadyStateHandler *m_onReadyStateHandler;
  HttpClient *m_client;
  long m_connectionTimeout;
  long m_responseTimeout;
  std::string m_outgoingInterface;

  friend class HttpResponse;
  friend class HttpClient;
};

class HttpClient {
public:
  HttpClient();
  ~HttpClient();

  HttpRequest *createRequest(std::string url);
  HttpRequest *createRequest(HttpMethod method, std::string url);

  static HttpClient &instance();

private:
  void add(HttpRequest *request);
  void remove(HttpRequest *request);
  void detach(HttpRequest *request);
  void run();

  HttpClient(const HttpClient &) = delete;
  void operator =(const HttpClient &) = delete;

  CURLM *m_multiHandle;
  bool m_running;
  std::atomic<bool> m_changed;
  std::thread m_thread;
  std::mutex  m_handlesSync;
  std::list<CURL*> m_addHandles;
  std::list<CURL*> m_removeHandles;
  std::set<HttpRequest*> m_boundRequests;

  static std::set<HttpClient*> s_instances;
  static size_t s_numInstance;
  friend class HttpRequest;
};

}

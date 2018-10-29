/***************************************************
 * http.cpp
 * Created on Wed, 28 Sep 2018 16:10:37 +0000 by vladimir
 *
 * $Author$
 * $Rev$
 * $Date$
 ***************************************************/

#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include "http.h"

namespace platform {

std::set<HttpClient*> HttpClient::s_instances;
size_t HttpClient::s_numInstance = 0;

HttpResponse::HttpResponse(const HttpResponse &r)
  : m_status(r.m_status)
  , m_headers(r.m_headers)
  , m_body(r.m_body)
  , m_timedOut(r.m_timedOut)
  , m_request(NULL)
{ }

long HttpResponse::getStatus() const {
  return m_status;
}

bool HttpResponse::isTimedOut() const {
  return m_timedOut;
}

const std::string &HttpResponse::getBody() const {
  return m_body;
}

const HttpHeaderList &HttpResponse::getHeaders() const {
  return m_headers;
}

HttpResponse::HttpResponse(CURL *handle) {
  HttpRequest *req;
  curl_easy_getinfo(handle, CURLINFO_PRIVATE, &req);
  if(!req) {
    throw std::runtime_error("No HttpRequest object found");
  }

  m_body.assign(req->m_responseBody.str());
  curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &m_status);
  // Don't parse headers for now
}

const HttpRequest &HttpResponse::getRequest() const {
  if(!m_request) {
    throw std::runtime_error("Response is not bound to request");
  }
  return *m_request;
}

HttpRequest::HttpRequest(HttpMethod method, std::string url, HttpClient *client)
  : m_handle(NULL)
  , m_url(url)
  , m_method(method)
  , m_curlHeaders(NULL)
  , m_client(client)
  , m_connectionTimeout(1000)
  , m_responseTimeout(2000)
{ }

HttpRequest::HttpRequest(std::string url, HttpClient *client)
  : m_handle(NULL)
  , m_url(url)
  , m_curlHeaders(NULL)
  , m_client(client)
  , m_connectionTimeout(1000)
  , m_responseTimeout(2000)
{ }

HttpRequest::~HttpRequest() {
  if(m_handle) {
    curl_easy_setopt(m_handle, CURLOPT_PRIVATE, NULL);
    if(m_client) {
      m_client->remove(this);
    }
  }
}

void HttpRequest::setBody(std::string body) {
  m_body.str(std::move(body));
  m_body.seekg(0, m_body.beg);
}

void HttpRequest::setMethod(HttpMethod method) {
  m_method = method;
}

void HttpRequest::setResponseTimeout(long ms) {
  m_responseTimeout = ms;
}

void HttpRequest::setConnectionTimeout(long ms) {
  m_connectionTimeout = ms;
}

void HttpRequest::setOutgoingInterface(const std::string &interface) {
  m_outgoingInterface = interface;
}

void HttpRequest::addHeader(std::string name, std::string value) {
  m_headers.insert(m_headers.end(), std::make_pair(name, value));
  m_curlHeaders = curl_slist_append(m_curlHeaders, (name + ": " + value).c_str());
}

void HttpRequest::send() {
  // Add to HttpClient
  if(m_client) {
    m_client->add(this);
  }
}

size_t HttpRequest::httpRead(void *ptr, size_t size, size_t nmemb, void *userp) {
  return reinterpret_cast<HttpRequest*>(userp)->m_body.read((char*)ptr, size * nmemb).gcount();
}

size_t HttpRequest::httpWrite(char *ptr, size_t size, size_t nmemb, void *userp) {
  reinterpret_cast<HttpRequest*>(userp)->m_responseBody.write((char*)ptr, size * nmemb);
  return size * nmemb;
}

size_t HttpRequest::httpWriteHeaders(char *ptr, size_t size, size_t nmemb, void *userp) {
  reinterpret_cast<HttpRequest*>(userp)->m_responseHeaders.write((char*)ptr, size * nmemb);
  return size * nmemb;
}

void HttpRequest::setReadyStateHandler(HttpReadyStateHandler *handler) {
  m_onReadyStateHandler = handler;
}

void HttpRequest::onReadyState(const HttpResponse &resp) {
  const_cast<HttpResponse&>(resp).m_request = this;
  if(m_onReadyStateHandler) {
    m_onReadyStateHandler->onReadyState(&resp);
  }
}

void HttpRequest::setHandle(CURL *handle) {
  m_handle = handle;
  curl_easy_setopt(m_handle, CURLOPT_PRIVATE, this);
  switch(m_method) {
  case HttpMethod::POST:
    curl_easy_setopt(m_handle, CURLOPT_POST, 1);
    curl_easy_setopt(m_handle, CURLOPT_READFUNCTION, &HttpRequest::httpRead);
    curl_easy_setopt(m_handle, CURLOPT_READDATA, this);
    curl_easy_setopt(m_handle, CURLOPT_POSTFIELDSIZE, (long)m_body.str().size());
    break;
  case HttpMethod::DELETE:
    curl_easy_setopt(m_handle, CURLOPT_CUSTOMREQUEST, "DELETE");
    break;
  case HttpMethod::PUT:
    curl_easy_setopt(m_handle, CURLOPT_CUSTOMREQUEST, "PUT");
    break;
  default:
    break;
  }

  curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, &HttpRequest::httpWrite);
  curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, this);
  curl_easy_setopt(m_handle, CURLOPT_SSL_VERIFYPEER, 0);
  curl_easy_setopt(m_handle, CURLOPT_TIMEOUT_MS, m_responseTimeout);
  curl_easy_setopt(m_handle, CURLOPT_CONNECTTIMEOUT_MS, m_connectionTimeout);
  curl_easy_setopt(m_handle, CURLOPT_HTTPHEADER, m_curlHeaders);
  curl_easy_setopt(m_handle, CURLOPT_URL, m_url.c_str());
  curl_easy_setopt(m_handle, CURLOPT_FOLLOWLOCATION, 1);
  if(!m_outgoingInterface.empty()) {
    curl_easy_setopt(m_handle, CURLOPT_INTERFACE, m_outgoingInterface.c_str());
  }
}


HttpClient::HttpClient() {
  m_running = true;
  m_changed = false;
  m_thread = std::thread([this]() { run(); });
  s_instances.insert(this);
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

HttpClient::~HttpClient() {
  for(auto request : m_boundRequests) {
    request->m_client = NULL;
  }
  s_instances.erase(this);
  m_running = false;
  m_thread.join();
  curl_global_cleanup();
}

HttpClient &HttpClient::instance() {
  if(!s_instances.size()) {
    throw std::runtime_error("HttpClient instance does not exist!");
  }

  size_t instance = s_numInstance ++;
  s_numInstance %= s_instances.size();

  auto currentInstance = s_instances.begin();
  while(instance -- && currentInstance != s_instances.end()) {
    currentInstance ++;
  }

  return **currentInstance;
}

HttpRequest *HttpClient::createRequest(std::string url) {
  HttpRequest *r = new HttpRequest(url, this);
  m_boundRequests.insert(r);
  return r;
}

HttpRequest *HttpClient::createRequest(HttpMethod method, std::string url) {
  HttpRequest *r = new HttpRequest(method, url, this);
  m_boundRequests.insert(r);
  return r;
}

void HttpClient::add(HttpRequest *request) {
  if(request->m_handle) {
    return; // already added
  }

  CURL *handle = curl_easy_init();
  request->setHandle(handle);

  std::lock_guard<std::mutex> lock(m_handlesSync);
  m_addHandles.push_back(request->m_handle);
  m_changed = true;
}

void HttpClient::remove(HttpRequest *request) {
  std::lock_guard<std::mutex> lock(m_handlesSync);
  m_removeHandles.push_back(request->m_handle);
  m_changed = true;
}

void HttpClient::detach(HttpRequest *request) {
  m_boundRequests.erase(request);
}

void HttpClient::run() {
  m_multiHandle = curl_multi_init();

  while (m_running) {
    int runningHandles = 0;

    if(m_changed) {
      std::lock_guard<std::mutex> lock(m_handlesSync);

      while(!m_addHandles.empty()) {
        curl_multi_add_handle(m_multiHandle, m_addHandles.front());
        m_addHandles.pop_front();
      }

      while(!m_removeHandles.empty()) {
        curl_multi_remove_handle(m_multiHandle, m_removeHandles.front());

        HttpRequest *req;
        curl_easy_getinfo(m_removeHandles.front(), CURLINFO_PRIVATE, &req);
        if(req) {
          req->m_handle = NULL;
          delete req;
        }

        curl_easy_cleanup(m_removeHandles.front());
        m_removeHandles.pop_front();
      }
      m_changed = false;
    }

    CURLMcode rc;

    do {
      int max;
      long timeout;
      struct timeval tv;
      fd_set rd, wr, ex;
      FD_ZERO(&rd); FD_ZERO(&wr); FD_ZERO(&ex);
      curl_multi_fdset(m_multiHandle, &rd, &wr, &ex, &max);
      curl_multi_timeout(m_multiHandle, &timeout);

      if(timeout >= 0) {
        tv.tv_sec = timeout / 1000;
        if(tv.tv_sec > 1) {
          tv.tv_sec = 1;
        } else {
          tv.tv_usec = (timeout % 1000) * 1000;
        }
      }

      if(max > 0) {
        select(max + 1, &rd, &wr, &ex, &tv);
      } else {
        usleep(100000);
      }

      rc = curl_multi_perform(m_multiHandle, &runningHandles);
    } while (rc == CURLM_CALL_MULTI_PERFORM);

    CURLMsg *msg;

    do {
      int msgq = 0;
      msg = curl_multi_info_read(m_multiHandle, &msgq);

      if (msg && msg->msg == CURLMSG_DONE) {
        HttpRequest *req;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &req);
        if(req) {
          try {
            HttpResponse response(msg->easy_handle);
            if (msg->data.result == CURLE_OPERATION_TIMEDOUT) {
              response.m_timedOut = true;
            }
            req->onReadyState(response);
            remove(req);
          } catch(...) { }
        }
      }
    } while(msg);

    if(!runningHandles) {
      usleep(1000);
    }
  }

  curl_multi_cleanup(m_multiHandle);
}

}

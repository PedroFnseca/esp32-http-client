#ifndef GRAPHQL_BATCH_REQUEST_H
#define GRAPHQL_BATCH_REQUEST_H

#include <Arduino.h>
#include <vector>

#include "GraphQLRequest.h"
#include "GraphQLTypes.h"
#include "RestTypes.h"

class ESP32HTTPClient;

class GraphQLBatchRequest {
  friend class ESP32HTTPClient;

 public:
  GraphQLBatchRequest(ESP32HTTPClient* client, const char* path = "/graphql");
  ~GraphQLBatchRequest();

  GraphQLBatchRequest(const GraphQLBatchRequest&) = delete;
  GraphQLBatchRequest& operator=(const GraphQLBatchRequest&) = delete;
  GraphQLBatchRequest(GraphQLBatchRequest&& other);

  GraphQLRequest& add();
  GraphQLRequest& addQuery(const char* queryDocument);
  GraphQLRequest& addMutation(const char* mutationDocument);

  size_t size() const;
  GraphQLRequest& operation(size_t index);

  GraphQLBatchRequest& header(const char* name, const char* value);
  GraphQLBatchRequest& accept(const char* acceptHeader);
  GraphQLBatchRequest& timeout(uint16_t timeoutMs);
  GraphQLBatchRequest& maxRetry(int maxRetry);
  GraphQLBatchRequest& retry(int maxRetry);

  GraphQLBatchRequest& getRawResponse(String* target);

  GraphQLBatchRequest& onSuccess(HttpResponseCallback cb);
  GraphQLBatchRequest& onError(HttpErrorCallback cb);
  GraphQLBatchRequest& onError(HttpResponseCallback cb);
  GraphQLBatchRequest& onResponse(HttpResponseCallback cb);
  GraphQLBatchRequest& onGraphQLError(GraphQLMultiErrorCallback cb);

  void execute();
  String buildRequestBody() const;

 private:
  ESP32HTTPClient* _client;
  const char* _path;
  bool _executed;
  uint16_t _timeout;
  int _maxRetry;
  String _acceptHeader;

  std::vector<HttpHeader> _customHeaders;
  std::vector<GraphQLRequest> _operations;
  String* _rawResponseTarget;

  HttpResponseCallback _onSuccessCb;
  HttpErrorCallback _onErrorCb;
  HttpResponseCallback _onResponseCb;
  GraphQLMultiErrorCallback _onMultiErrorCb;

  void parseBatchResponse(BufferedStreamReader& r);
};

#endif

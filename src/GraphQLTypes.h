#ifndef GRAPHQL_TYPES_H
#define GRAPHQL_TYPES_H

#include <Arduino.h>
#include <functional>
#include <vector>

#include "RestTypes.h"

#define GRAPHQL_CONTENT_TYPE_JSON "application/json; charset=utf-8"
#define GRAPHQL_ACCEPT_HEADER "application/graphql-response+json;charset=utf-8, application/json;charset=utf-8, multipart/mixed"

struct GraphQLLocation {
  int line = 0;
  int column = 0;
};

struct GraphQLError {
  String message;
  std::vector<GraphQLLocation> locations;
  std::vector<String> path;
  String extensions;
};

struct GraphQLIncrementalPayload {
  String path;
  String data;
  bool hasNext = false;
  std::vector<GraphQLError> errors;
};

typedef std::function<void(const GraphQLError&)> GraphQLErrorCallback;
typedef std::function<void(const std::vector<GraphQLError>&)> GraphQLMultiErrorCallback;
typedef std::function<void(const GraphQLIncrementalPayload&)> GraphQLIncrementalCallback;

#endif

---
title: GraphQLBatchRequest Class Reference - Batch GraphQL Operations for ESP32
description: API reference for GraphQLBatchRequest: batch multiple GraphQL operations into a single HTTP request, parse array responses, and inspect individual errors.
keywords: GraphQLBatchRequest ESP32, GraphQL batching, batch query ESP32, low memory GraphQL batch
tags:
  - api
  - graphql
  - batch
  - class
---
# GraphQLBatchRequest

The fluent request builder returned by `.graphqlBatch(path)` on [`ESP32HTTPClient`](esp32httpclient.md). It allows bundling multiple independent GraphQL operations into a single HTTP POST request.

**The batch request is executed automatically** when the `GraphQLBatchRequest` object goes out of scope or when `.execute()` is called explicitly.

---

## Example Usage

```cpp
auto batch = client.graphqlBatch("/graphql");

String userName;
int unreadCount = 0;

auto& op1 = batch.addQuery("query { user { name } }");
op1.getData("user.name", &userName);

auto& op2 = batch.addQuery("query { unreadNotifications }");
op2.getData("unreadNotifications", &unreadCount);

// Executes automatically at end of scope or explicitly:
batch.execute();
```

---

## Methods

### `add()`
Appends a new empty `GraphQLRequest` operation to the batch and returns a reference to configure it.
```cpp
GraphQLRequest& add();
```

### `addQuery(queryDocument)`
Appends a query operation and returns a reference to configure variables and bindings.
```cpp
GraphQLRequest& addQuery(const char* queryDocument);
```

### `addMutation(mutationDocument)`
Appends a mutation operation.
```cpp
GraphQLRequest& addMutation(const char* mutationDocument);
```

### `size()`
Returns the number of operations added to the batch.
```cpp
size_t size() const;
```

### `operation(index)`
Returns a reference to the operation at the specified index.
```cpp
GraphQLRequest& operation(size_t index);
```

### `execute()`
Executes the batch request immediately by sending a single HTTP POST with an array of operations.
```cpp
void execute();
```

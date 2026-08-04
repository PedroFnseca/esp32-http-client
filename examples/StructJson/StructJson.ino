#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

struct Todo {
  int userId = 0;
  int id = 0;
  char title[64] = {0};
  bool completed = false;

  REST_JSON_MAP(
    REST_FIELD(userId),
    REST_FIELD(id),
    REST_FIELD(title),
    REST_FIELD(completed)
  )
};

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void loop() {
  Todo todo;

  Serial.println("Fetching Todo as struct...");
  client.get("/todos/1").getBody(&todo);

  if (client.isSuccess()) {
    Serial.printf("User ID: %d\n", todo.userId);
    Serial.printf("ID: %d\n", todo.id);
    Serial.printf("Title: %s\n", todo.title);
    Serial.printf("Completed: %s\n", todo.completed ? "true" : "false");
  } else {
    Serial.printf("Error: %d (%s)\n", client.getStatusCode(), client.getErrorMessage().c_str());
  }

  Todo newTodo;
  newTodo.userId = 1;
  newTodo.id = 101;
  strncpy(newTodo.title, "Buy ESP32", sizeof(newTodo.title));
  newTodo.completed = false;

  Serial.println("Posting Todo from struct...");
  client.post("/todos").body(newTodo).getBody(&todo);

  if (client.isSuccess()) {
    Serial.printf("Created Todo ID: %d\n", todo.id);
  }

  delay(10000);
}

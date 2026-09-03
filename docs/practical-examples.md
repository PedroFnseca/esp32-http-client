---
title: Practical Examples - ESP32 HTTP Client Examples
description: Practical IoT and embedded project examples using ESP32HTTPClient, including weather stations, GitHub monitors, IoT sensors, and REST APIs.
keywords: ESP32 practical examples, ESP32 API projects, ESP32 HTTP client use cases, Arduino REST examples, IoT weather station ESP32
tags:
  - examples
  - practical
  - iot
  - rest
  - projects
---

# Practical Examples

Explore practical examples demonstrating how to integrate `ESP32HTTPClient` into embedded projects and IoT devices.

---

## 1. Weather Station

Fetches real-time weather metrics such as temperature, relative humidity, and wind speed for a specific geographic location.

**API:** [Open-Meteo](https://open-meteo.com/) (`https://api.open-meteo.com/v1/forecast`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";

    // --- Signatures for specific / hardware logic ---
    void displayWeather(float temp, int humidity, float windSpeed); // specific implementation

    ESP32HTTPClient weatherClient("https://api.open-meteo.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        float temperature = 0.0f;
        int humidity = 0;
        float windSpeed = 0.0f;

        weatherClient.get("/v1/forecast")
            .query("latitude", "40.7128")
            .query("longitude", "-74.0060")
            .query("current", "temperature_2m,relative_humidity_2m,wind_speed_10m")
            .getBody("current.temperature_2m", &temperature)
            .getBody("current.relative_humidity_2m", &humidity)
            .getBody("current.wind_speed_10m", &windSpeed);

        if (weatherClient.isSuccess()) {
            displayWeather(temperature, humidity, windSpeed);
        } else {
            Serial.printf("Error fetching weather: %s (HTTP %d)\n",
                          weatherClient.getLastError(), weatherClient.getStatusCode());
        }

        delay(600000); // Poll every 10 minutes
    }
    ```

---

## 2. Pokémon Terminal

Searches for a Pokémon by name or ID to retrieve attributes such as base experience, height, weight, and primary species name using fluent path parameters.

**API:** [PokéAPI](https://pokeapi.co/) (`https://pokeapi.co/api/v2/pokemon/{name}`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";

    // --- Signatures for specific / hardware logic ---
    void renderPokemonCard(const char* name, int height, int weight, int baseExp); // specific implementation
    const char* getSelectedPokemonName();                                           // specific implementation

    ESP32HTTPClient pokeClient("https://pokeapi.co");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        const char* targetPokemon = getSelectedPokemonName();

        char name[32] = {0};
        int height = 0;
        int weight = 0;
        int baseExp = 0;

        pokeClient.get("/api/v2/pokemon/{name}")
            .path("name", targetPokemon)
            .getBody("name", name, sizeof(name))
            .getBody("height", &height)
            .getBody("weight", &weight)
            .getBody("base_experience", &baseExp);

        if (pokeClient.isSuccess()) {
            renderPokemonCard(name, height, weight, baseExp);
        }

        delay(30000);
    }
    ```

---

## 3. GitHub Repository Monitor

Monitors public statistics of a GitHub repository, tracking stars, forks count, and open issues.

**API:** [GitHub REST API](https://docs.github.com/en/rest/repos/repos#get-a-repository) (`https://api.github.com/repos/{owner}/{repo}`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";

    // --- Signatures for specific / hardware logic ---
    void updateRepoStatsDisplay(int stars, int forks, int openIssues); // specific implementation

    ESP32HTTPClient githubClient("https://api.github.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }

        // GitHub API requires a User-Agent header
        githubClient.header("User-Agent", "ESP32HTTPClient-Monitor");
    }

    void loop() {
        int stars = 0;
        int forks = 0;
        int openIssues = 0;

        githubClient.get("/repos/{owner}/{repo}")
            .path("owner", "PedroFnseca")
            .path("repo", "esp32-http-client")
            .getBody("stargazers_count", &stars)
            .getBody("forks_count", &forks)
            .getBody("open_issues_count", &openIssues);

        if (githubClient.isSuccess()) {
            updateRepoStatsDisplay(stars, forks, openIssues);
        }

        delay(300000); // Check every 5 minutes
    }
    ```

---

## 4. GitHub Release Checker

Checks a GitHub repository for the latest release tag to detect when a firmware update or new version is published.

**API:** [GitHub REST API](https://docs.github.com/en/rest/releases/releases#get-the-latest-release) (`https://api.github.com/repos/{owner}/{repo}/releases/latest`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid           = "YOUR_WIFI_SSID";
    const char* password       = "YOUR_WIFI_PASSWORD";
    const char* currentVersion = "v1.0.0";

    // --- Signatures for specific / hardware logic ---
    void triggerOtaUpdateNotice(const char* latestVersion, const char* downloadUrl); // specific implementation

    ESP32HTTPClient githubClient("https://api.github.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }

        githubClient.header("User-Agent", "ESP32-OTA-Checker");
    }

    void loop() {
        char latestTag[32] = {0};
        char releaseName[64] = {0};

        githubClient.get("/repos/{owner}/{repo}/releases/latest")
            .path("owner", "PedroFnseca")
            .path("repo", "esp32-http-client")
            .getBody("tag_name", latestTag, sizeof(latestTag))
            .getBody("name", releaseName, sizeof(releaseName));

        if (githubClient.isSuccess()) {
            if (strcmp(latestTag, currentVersion) != 0) {
                Serial.printf("New release detected: %s (current: %s)\n", latestTag, currentVersion);
                triggerOtaUpdateNotice(latestTag, releaseName);
            }
        }

        delay(3600000); // Check once per hour
    }
    ```

---

## 5. ISS Tracker

Tracks the real-time position (latitude and longitude coordinates) of the International Space Station (ISS).

**API:** [Open Notify](http://open-notify.org/) (`http://api.open-notify.org/iss-now.json`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";

    // --- Signatures for specific / hardware logic ---
    void plotIssOnMap(float latitude, float longitude); // specific implementation

    ESP32HTTPClient issClient("http://api.open-notify.org");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        float latitude = 0.0f;
        float longitude = 0.0f;

        issClient.get("/iss-now.json")
            .getBody("iss_position.latitude", &latitude)
            .getBody("iss_position.longitude", &longitude);

        if (issClient.isSuccess()) {
            plotIssOnMap(latitude, longitude);
        }

        delay(10000); // Update every 10 seconds
    }
    ```

---

## 6. Currency Converter

Fetches updated foreign exchange rates and calculates currency conversions against base currencies (e.g., EUR, USD, BRL).

**API:** [Frankfurter API](https://www.frankfurter.app/) (`https://api.frankfurter.app/latest`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";

    // --- Signatures for specific / hardware logic ---
    void displayExchangeRate(const char* base, const char* target, float rate); // specific implementation

    ESP32HTTPClient fxClient("https://api.frankfurter.app");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        float rateUSD = 0.0f;
        float rateBRL = 0.0f;

        fxClient.get("/latest")
            .query("from", "EUR")
            .query("to", "USD,BRL")
            .getBody("rates.USD", &rateUSD)
            .getBody("rates.BRL", &rateBRL);

        if (fxClient.isSuccess()) {
            displayExchangeRate("EUR", "USD", rateUSD);
            displayExchangeRate("EUR", "BRL", rateBRL);
        }

        delay(1800000); // Refresh every 30 minutes
    }
    ```

---

## 7. Random Facts

Queries and displays random trivia and facts for desktop gadgets, tickers, or smart displays.

**API:** [Useless Facts API](https://uselessfacts.jsph.pl/) (`https://uselessfacts.jsph.pl/api/v2/facts/random`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";

    // --- Signatures for specific / hardware logic ---
    void showTriviaFact(const char* factText); // specific implementation

    ESP32HTTPClient factsClient("https://uselessfacts.jsph.pl");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        char fact[256] = {0};

        factsClient.get("/api/v2/facts/random")
            .query("language", "en")
            .getBody("text", fact, sizeof(fact));

        if (factsClient.isSuccess()) {
            showTriviaFact(fact);
        }

        delay(60000); // New fact every minute
    }
    ```

---

## 8. Random Dog Viewer

Fetches random dog image URLs from an open API to render or download onto an e-paper or TFT display.

**API:** [Dog CEO Dog API](https://dog.ceo/dog-api/) (`https://dog.ceo/api/breeds/image/random`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";

    // --- Signatures for specific / hardware logic ---
    void downloadAndRenderImage(const char* imageUrl); // specific implementation

    ESP32HTTPClient dogClient("https://dog.ceo");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        char imageUrl[128] = {0};
        char status[16] = {0};

        dogClient.get("/api/breeds/image/random")
            .getBody("message", imageUrl, sizeof(imageUrl))
            .getBody("status", status, sizeof(status));

        if (dogClient.isSuccess() && strcmp(status, "success") == 0) {
            downloadAndRenderImage(imageUrl);
        }

        delay(30000);
    }
    ```

---

## 9. NASA Astronomy Picture (APOD)

Fetches the Astronomy Picture of the Day (APOD) metadata from NASA, including title, date, copyright, and image URL.

**API:** [NASA Open APIs](https://api.nasa.gov/) (`https://api.nasa.gov/planetary/apod`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";
    const char* nasaKey  = "DEMO_KEY"; // Replace with your NASA API key

    // --- Signatures for specific / hardware logic ---
    void displayApod(const char* title, const char* date, const char* url); // specific implementation

    ESP32HTTPClient nasaClient("https://api.nasa.gov");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        char title[128] = {0};
        char date[16]   = {0};
        char url[256]   = {0};

        nasaClient.get("/planetary/apod")
            .query("api_key", nasaKey)
            .getBody("title", title, sizeof(title))
            .getBody("date", date, sizeof(date))
            .getBody("url", url, sizeof(url));

        if (nasaClient.isSuccess()) {
            displayApod(title, date, url);
        }

        delay(86400000); // Check once daily
    }
    ```

---

## 10. Book Search

Queries the Open Library catalog to retrieve information about a book by ISBN or title (title, number of pages, publication year).

**API:** [Open Library](https://openlibrary.org/developers/api) (`https://openlibrary.org/api/books`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";

    // --- Signatures for specific / hardware logic ---
    void showBookDetails(const char* title, int pages, const char* publishDate); // specific implementation

    ESP32HTTPClient libraryClient("https://openlibrary.org");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        char title[64] = {0};
        char pubDate[32] = {0};
        int pages = 0;

        // Query ISBN: 0451526538 (e.g., 1984 by George Orwell)
        libraryClient.get("/api/books")
            .query("bibkeys", "ISBN:0451526538")
            .query("format", "json")
            .query("jscmd", "data")
            .getBody("ISBN:0451526538.title", title, sizeof(title))
            .getBody("ISBN:0451526538.publish_date", pubDate, sizeof(pubDate))
            .getBody("ISBN:0451526538.number_of_pages", &pages);

        if (libraryClient.isSuccess()) {
            showBookDetails(title, pages, pubDate);
        }

        delay(60000);
    }
    ```

---

## 11. Smart Plant Monitor

Collects sensor readings (soil moisture, temperature, ambient light) and sends telemetry data to an IoT cloud platform via a JSON POST request.

**API:** Custom IoT Telemetry API (`https://api.example-iot.com/v1/telemetry`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";

    // --- Signatures for specific / hardware logic ---
    float readSoilMoistureSensor(); // specific implementation
    float readAmbientTemperature(); // specific implementation
    int   readLightLevel();         // specific implementation

    ESP32HTTPClient iotClient("https://api.example-iot.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }

        iotClient.bearerAuth("DEVICE_SECRET_TOKEN");
    }

    void loop() {
        float moisture = readSoilMoistureSensor();
        float temp     = readAmbientTemperature();
        int light      = readLightLevel();

        bool acknowledged = false;

        iotClient.post("/v1/telemetry")
            .body("deviceId", "plant-node-01")
            .body("soilMoisture", moisture)
            .body("temperature", temp)
            .body("lightLevel", light)
            .getBody("success", &acknowledged);

        if (iotClient.isSuccess() && acknowledged) {
            Serial.println("Telemetry successfully transmitted.");
        }

        delay(60000); // Publish every 60 seconds
    }
    ```

---

## 12. IoT Alert System

Monitors critical environment threshold conditions (such as smoke, gas leaks, or intrusion) and immediately sends an alert webhook payload.

**API:** Webhook / Notification Gateway (`https://api.example-alerts.com/v1/notify`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";

    // --- Signatures for specific / hardware logic ---
    bool checkEmergencyCondition(); // specific implementation
    void triggerLocalBuzzer();      // specific implementation

    ESP32HTTPClient alertClient("https://api.example-alerts.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        if (checkEmergencyCondition()) {
            triggerLocalBuzzer();

            char incidentId[32] = {0};

            alertClient.post("/v1/notify")
                .header("X-Priority", "High")
                .body("source", "ESP32-Security-Node")
                .body("level", "CRITICAL")
                .body("message", "High gas concentration detected!")
                .getBody("incidentId", incidentId, sizeof(incidentId));

            if (alertClient.isSuccess()) {
                Serial.printf("Emergency alert logged. Incident: %s\n", incidentId);
            }
        }

        delay(2000); // Fast monitoring interval
    }
    ```

---

## 13. REST API POST Example

Creates and sends structured JSON objects to a RESTful service using POST, receiving the generated resource ID and metadata in response.

**API:** [JSONPlaceholder](https://jsonplaceholder.typicode.com/) (`https://jsonplaceholder.typicode.com/posts`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";

    // --- Signatures for specific / hardware logic ---
    void storeCreatedRecord(int newId, const char* title); // specific implementation

    ESP32HTTPClient restClient("https://jsonplaceholder.typicode.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        int createdId = 0;
        char title[64] = "ESP32 IoT Sensor Report";

        restClient.post("/posts")
            .body("title", title)
            .body("body", "Sensor node online with battery at 98%")
            .body("userId", 42)
            .getBody("id", &createdId);

        if (restClient.isSuccess()) {
            Serial.printf("Resource created successfully! ID: %d\n", createdId);
            storeCreatedRecord(createdId, title);
        }

        delay(60000);
    }
    ```

---

## 14. Public API Status Checker

Periodically verifies the availability and health status of an external service or API endpoint by evaluating HTTP response codes and payload health indicators.

**API:** Public Healthcheck API (`https://httpbin.org/status/200`)

??? example "View Code"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";

    // --- Signatures for specific / hardware logic ---
    void updateStatusIndicator(bool isOnline, int statusCode); // specific implementation

    ESP32HTTPClient healthClient("https://httpbin.org");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }

        healthClient.setTimeout(5000); // 5s timeout
    }

    void loop() {
        healthClient.get("/status/200");

        int code = healthClient.getStatusCode();
        bool online = healthClient.isSuccess() && (code == 200);

        updateStatusIndicator(online, code);

        Serial.printf("Service Status: %s (HTTP %d)\n", online ? "HEALTHY" : "DOWN", code);

        delay(30000); // Ping every 30s
    }
    ```

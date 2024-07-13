#include <Arduino.h>
#define FASTLED_INTERNAL
#include <FastLED.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include "font8x8_basic.h"
#include <mutex>

#define NUM_LEDS 50
#define WIDTH 50

CRGB leds[8][NUM_LEDS];
AsyncWebServer server(81);
std::mutex scoreMutex;

int goals = 0;
int points = 0;

const int goalButton = 27;
const int pointButton = 26;
const int resetButton = 25;

void clearDisplay() {
    for (int i = 0; i < 8; i++) {
        fill_solid(leds[i], NUM_LEDS, CRGB::Black);
    }
    FastLED.show();
}

void whiteDisplay() {
    for (int i = 0; i < 8; i++) {
        fill_solid(leds[i], NUM_LEDS, CRGB::White);
    }
    FastLED.show();
}

void displayChar(char letter, int xOffset) {
    if (xOffset >= WIDTH - 8) return; // Boundary check
    int ord = letter; // ASCII value
    const char *bitmap = font8x8_basic[ord]; // Bitmap for character

    int start_row = 0;
    int end_row = 8;
    if (letter == ' ' || letter == '.') {
        start_row = 2;
        end_row = 5;
    }

    for (int col = 0; col < 8; col++) {
        for (int row = start_row; row < end_row; row++) {
            int ledIndex = xOffset + row;
            if (ledIndex >= WIDTH) break; // Ensure we don't go beyond the available LEDs
            bool pixelOn = bitmap[col] & (1 << row);
            leds[col][ledIndex] = pixelOn ? CRGB::White : CRGB::Black;
        }
    }
}

void displayScore(int goals, int points) {
    int total = goals * 6 + points; // Calculate total score
    char scoreStr[20];
    sprintf(scoreStr, "%i.%i %i", goals, points, total); // Format the score string
    Serial.println(scoreStr);
    int length = strlen(scoreStr);
    int xOffset = (WIDTH - length * 9) / 2; // Center the score on the display

    for (int i = 0; scoreStr[i] != '\0'; i++) {
        displayChar(scoreStr[i], xOffset); // Display each character
        if (scoreStr[i] == ' ' || scoreStr[i] == '.') {
            xOffset += 5; // Assuming you move 5 pixels to the right for the next character
        } else {
            xOffset += 9; // Assuming you move 9 pixels to the right for the next character
        }
    }
}

void knightRiderDisplay(int cycles, int delayTime) {
    int segmentLength = WIDTH / 4;  // Calculate segment length as 1/8th of total width

    for (int cycle = 0; cycle < cycles; cycle++) {
        // Move from left to right
        for (int pos = 0; pos <= WIDTH - segmentLength; pos++) {
            for (int i = 0; i < 4; i++) {
                for (int j = pos; j < pos + segmentLength; j++) {
                    leds[i][j] = CRGB::Red;  // Light up a block of LEDs
                }
            }
            FastLED.show();
            clearDisplay();
            delay(delayTime);
        }

        // Move from right to left
        for (int pos = WIDTH - segmentLength; pos >= 0; pos--) {
            for (int i = 0; i < 8; i++) {
                for (int j = pos; j < pos + segmentLength; j++) {
                    leds[i][j] = CRGB::Red;  // Light up a block of LEDs
                }
            }
            FastLED.show();
            clearDisplay();
            delay(delayTime);
        }
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Starting...");
    
    pinMode(goalButton, INPUT_PULLUP);
    pinMode(pointButton, INPUT_PULLUP);
    pinMode(resetButton, INPUT_PULLUP);

    // Initialize the LED strips
    Serial.println("LED init...");
    FastLED.addLeds<WS2812B, 5, GRB>(leds[0], NUM_LEDS);
    FastLED.addLeds<WS2812B, 12, GRB>(leds[1], NUM_LEDS);
    FastLED.addLeds<WS2812B, 13, GRB>(leds[2], NUM_LEDS);
    FastLED.addLeds<WS2812B, 14, GRB>(leds[3], NUM_LEDS);
    FastLED.addLeds<WS2812B, 15, GRB>(leds[4], NUM_LEDS);
    FastLED.addLeds<WS2812B, 18, GRB>(leds[5], NUM_LEDS);
    FastLED.addLeds<WS2812B, 19, GRB>(leds[6], NUM_LEDS);
    FastLED.addLeds<WS2812B, 21, GRB>(leds[7], NUM_LEDS);

    delay(500);
    clearDisplay();

    Serial.println("KITT...");
    knightRiderDisplay(3, 10); // Perform Knight Rider animation three times
    
    Serial.println("Score board on...");
    clearDisplay();
    displayScore(goals, points);
    FastLED.show();

    // Initialize WiFiManager
    WiFiManager wifiManager;
    wifiManager.autoConnect("ESP32-Scoreboard");

    // Start server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = "<html><body><h1>Scoreboard</h1>";
        html += "<p>Goals: " + String(goals) + "</p>";
        html += "<p>Points: " + String(points) + "</p>";
        html += "<p>Total: " + String(goals * 6 + points) + "</p>";
        html += "<a href=\"/goal\">Add Goal</a><br>";
        html += "<a href=\"/point\">Add Point</a><br>";
        html += "<a href=\"/reset\">Reset</a>";
        html += "</body></html>";
        request->send(200, "text/html", html);
    });

    server.on("/goal", HTTP_GET, [](AsyncWebServerRequest *request){
        std::lock_guard<std::mutex> lock(scoreMutex);
        goals++;
        clearDisplay();
        displayScore(goals, points);
        request->redirect("/");
    });

    server.on("/point", HTTP_GET, [](AsyncWebServerRequest *request){
        std::lock_guard<std::mutex> lock(scoreMutex);
        points++;
        clearDisplay();
        displayScore(goals, points);
        request->redirect("/");
    });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
        goals = 0;
        points = 0;
        clearDisplay();
        displayScore(goals, points);
        request->redirect("/");
    });

    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not Found");
    });

    server.begin();
}

void loop() {
    displayScore(goals, points);
    FastLED.show();
    delay(1000);
    yield();
}

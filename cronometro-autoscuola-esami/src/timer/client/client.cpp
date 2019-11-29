#include "client.h"

#include "../timer.h"

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <Wifi.h>
#include <heltec.h>
#include <FastLED.h>


TimerClient::TimerClient() :
    _matrix(leds, 5, 5,
            NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG)
{  
}

void TimerClient::setup(String serverName, String clientName, int pingInterval){
    if(_isSetup){
        Serial.println("[Client] Already setup");
        return;
    }
    _isSetup = true;
    _serverName = serverName;
    _clientName = clientName;
    _pingInterval = pingInterval;
    _t = digitalRead(TIMER_SWITCH);
    _s = digitalRead(STOP_SWITCH);
    _SerialBT.begin(_clientName.c_str());
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(_serverName.c_str());
    for(int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++){
        delay(500);
    }
    _http.setReuse(true);
    sendPing();
    FastLED.addLeds<NEOPIXEL,LEDMATRIX_DATA>(leds, NUMMATRIXX).setCorrection(TypicalLEDStrip);
    _matrix.begin();
    _matrix.setBrightness(20);
    _matrix.setTextWrap(false);
}

void TimerClient::loop(){
    _t = digitalRead(TIMER_SWITCH);
    _s = digitalRead(STOP_SWITCH);
    if(digitalRead(START_BUTTON) == LOW){
        String msg = "";
        _t == LOW ? msg += "t=0" : msg += "t=1";
        _s == LOW ? msg += "&s=0" : msg += "&s=1";
        sendLoRa(msg);
        sendRequest(_endpoint + "timer?" + msg);
    }
    if(digitalRead(RESET_BUTTON) == LOW){
        String msg = "r=1";
        _t == LOW ? msg += "&t=0" : msg += "&t=1";
        _s == LOW ? msg += "&s=0" : msg += "&s=1";
        sendLoRa(msg);
        sendRequest(_endpoint + "timer?" + msg);
    }
    sendPing();
    matrixRefresh();
}

void TimerClient::sendRequest(String path){
    if(WiFi.status() != WL_CONNECTED) return;
    const char * headerkeys[] = {"Connection"};
    size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
    if(_http.begin(_client, "192.168.4.1", 80, path)){
        _http.collectHeaders(headerkeys,headerkeyssize);
        int httpCode = _http.GET();
        if(httpCode > 0){
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);
            if(httpCode == HTTP_CODE_OK) {
                for(int i=0; i<_http.headers(); i++){
                    Serial.printf("HEADER[%s]: %s\n", _http.headerName(i).c_str(), _http.header(i).c_str());
                }
                String payload = _http.getString();
                Serial.println(payload);
            }
        } else {
            Serial.printf("[HTTP] GET... failed, error: %s\n", _http.errorToString(httpCode).c_str());
        }
        _http.end();
    }
}

void TimerClient::sendLoRa(String msg){
    LoRa.beginPacket();
    LoRa.print(msg);
    LoRa.endPacket();
}

void TimerClient::sendPing(){
    if(_lastPing > 0 && _lastPing + _pingInterval > millis()) return;
    _lastPing = millis();
    String msg = "p=1";
    _t == LOW ? msg += "&t=0" : msg += "&t=1";
    _s == LOW ? msg += "&s=0" : msg += "&s=1";
    sendLoRa(msg);
    sendRequest(_endpoint + "timer?" + msg);
}

void TimerClient::matrixRefresh(){
    _matrix.drawLine(0, 0, 4, 4, _matrix.Color(255, 0, 0));
    _matrix.drawLine(4, 0, 0, 4, _matrix.Color(255, 0, 0));

    _matrix.show();
}

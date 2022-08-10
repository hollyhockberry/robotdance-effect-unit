// Copyright (c) 2022 Inaba (@hollyhockberry)
// This software is released under the MIT License.
// http://opensource.org/licenses/mit-license.php

#include <Arduino.h>
#include <esp_now.h>
#include <FastLED.h>
#include <WiFi.h>
#include <algorithm>
#include <vector>

namespace {

bool _moving = false;

// 動作中のノード
std::vector<uint32_t> nodes;

// MACアドレス下位4桁をIDに変換する
uint32_t Addr2Id(const uint8_t* mac_addr) {
  union {
    uint32_t id;
    uint8_t bytes[4];
  } addr;
  for (auto i = 0; i < 4; ++i) addr.bytes[i] = mac_addr[2 + i];
  return addr.id;
}

void recvCallback(const uint8_t* mac_addr, const uint8_t *data, int data_len) {
  if (data_len <= 0) return;

  const auto id = Addr2Id(mac_addr);
  const auto move = data[0] != 0;
  const auto contain = std::find(nodes.begin(), nodes.end(), id) != nodes.end();

  // 動作中ノードのリストを更新する.
  if (move) {
    if (!contain) {
      nodes.push_back(id);
    }
  } else {
    if (contain) {
      for (auto it = nodes.begin(); it != nodes.end();) {
        if (*it == id) {
          it = nodes.erase(it);
        } else {
          ++it;
        }
      }
    }
  }
}

CRGB leds[1];
void show_led(int color) {
  leds[0] = CRGB(color);
  FastLED.show();
}

}  // namespace

void setup() {
  FastLED.addLeds<WS2812, G27>(leds, 1);
  show_led(0);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (::esp_now_init() != ESP_OK) {
    ::esp_restart();
  }
  ::esp_now_register_recv_cb(recvCallback);

  ::ledcSetup(1, 50, 16);
  ::ledcAttachPin(G26, 1);
}

void loop() {
  // 動作中のノードが一つでもあればモータを回したい.
  const bool moving = nodes.size() > 0;

  if (_moving != moving) {
    _moving = moving;
    const uint32_t duty = _moving
      ? 2.0 / 20.0 * 65535
      : 1.5 / 20.0 * 65535;
    ::ledcWrite(1, duty);
    show_led(_moving ? 0xff00 : 0);
  }
}

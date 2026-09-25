#include "driver/twai.h"

#define CAN_TX 21
#define CAN_RX 22

uint16_t encoder_now = 0;
uint16_t encoder_prev = 0;

long encoder_total = 0;

bool first_data = true;


// ========================
// M2006を動かす
// ========================
void sendMotorCurrent(int16_t current) {

  twai_message_t message = {};

  message.identifier = 0x200;
  message.extd = 0;
  message.data_length_code = 8;

  // M2006 ID = 1
  message.data[0] = current >> 8;
  message.data[1] = current & 0xFF;

  message.data[2] = 0;
  message.data[3] = 0;
  message.data[4] = 0;
  message.data[5] = 0;
  message.data[6] = 0;
  message.data[7] = 0;

  twai_transmit(
    &message,
    pdMS_TO_TICKS(10)
  );
}


// ========================
// エンコーダリセット
// ========================
void resetEncoder() {

  encoder_total = 0;

  // 現在位置を新しい基準にする
  encoder_prev = encoder_now;

  Serial.println("Encoder RESET");
}


// ========================
// setup
// ========================
void setup() {

  Serial.begin(115200);

  twai_general_config_t g_config =
    TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)CAN_TX,
      (gpio_num_t)CAN_RX,
      TWAI_MODE_NORMAL
    );

  twai_timing_config_t t_config =
    TWAI_TIMING_CONFIG_1MBITS();

  twai_filter_config_t f_config =
    TWAI_FILTER_CONFIG_ACCEPT_ALL();

  twai_driver_install(
    &g_config,
    &t_config,
    &f_config
  );

  twai_start();

  Serial.println("CAN START");
  Serial.println("r : Encoder RESET");
}


// ========================
// loop
// ========================
void loop() {

  // ======================
  // M2006を回す
  // ======================

  sendMotorCurrent(500);


  // ======================
  // CAN受信
  // ======================

  twai_message_t message;

  if (twai_receive(
        &message,
        pdMS_TO_TICKS(1)
      ) == ESP_OK) {

    // M2006 ID = 1
    if (message.identifier == 0x201) {

      encoder_now =
        ((uint16_t)message.data[0] << 8)
        | message.data[1];


      // 初回
      if (first_data) {

        encoder_prev = encoder_now;
        first_data = false;

      } else {

        int diff =
          encoder_now - encoder_prev;


        // 8191 → 0
        if (diff < -4096) {
          diff += 8192;
        }

        // 0 → 8191
        else if (diff > 4096) {
          diff -= 8192;
        }


        // 積算
        encoder_total += diff;

        encoder_prev = encoder_now;
      }


      // ==================
      // 表示
      // ==================

      Serial.print("Encoder = ");
      Serial.print(encoder_now);

      Serial.print("  Total = ");
      Serial.println(encoder_total);
    }
  }


  // ======================
  // シリアルコマンド
  // ======================

  if (Serial.available()) {

    char command = Serial.read();

    if (command == 'r' || command == 'R') {
      resetEncoder();
    }
  }


  delay(10);
}

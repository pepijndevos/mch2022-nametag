/*
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */

// This file contains a simple Hello World app which you can base you own
// native Badge apps on.

#include "main.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

static pax_buf_t buf;
static pax_buf_t name;
xQueueHandle buttonQueue;

#include <esp_log.h>
static const char *TAG = "mch2022-demo-app";

// Updates the screen with the latest buffer.
void disp_flush() { ili9341_write(get_ili9341(), buf.buf); }

// Exits the app, returning to the launcher.
void exit_to_launcher() {
  REG_WRITE(RTC_CNTL_STORE0_REG, 0);
  esp_restart();
}

uint8_t firebuf[SCREEN_HEIGHT * SCREEN_WIDTH];

void spreadFire(int src) {
  uint8_t pixel = firebuf[src];
  if (pixel == 0) {
    firebuf[src - SCREEN_WIDTH] = 0;
  } else {
    int randIdx = esp_random() & 3;
    int dst = src - randIdx + 1;
    firebuf[dst - SCREEN_WIDTH] = pixel - (randIdx & 1);
  }
}

void doFire() {
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    for (int y = 1; y < SCREEN_HEIGHT; y++) {
      spreadFire(y * SCREEN_WIDTH + x);
    }
  }
}

void app_main() {

  pax_col_t rgbs[] = {
      pax_col_rgb(0x07, 0x07, 0x07), pax_col_rgb(0x1F, 0x07, 0x07),
      pax_col_rgb(0x2F, 0x0F, 0x07), pax_col_rgb(0x47, 0x0F, 0x07),
      pax_col_rgb(0x57, 0x17, 0x07), pax_col_rgb(0x67, 0x1F, 0x07),
      pax_col_rgb(0x77, 0x1F, 0x07), pax_col_rgb(0x8F, 0x27, 0x07),
      pax_col_rgb(0x9F, 0x2F, 0x07), pax_col_rgb(0xAF, 0x3F, 0x07),
      pax_col_rgb(0xBF, 0x47, 0x07), pax_col_rgb(0xC7, 0x47, 0x07),
      pax_col_rgb(0xDF, 0x4F, 0x07), pax_col_rgb(0xDF, 0x57, 0x07),
      pax_col_rgb(0xDF, 0x57, 0x07), pax_col_rgb(0xD7, 0x5F, 0x07),
      pax_col_rgb(0xD7, 0x5F, 0x07), pax_col_rgb(0xD7, 0x67, 0x0F),
      pax_col_rgb(0xCF, 0x6F, 0x0F), pax_col_rgb(0xCF, 0x77, 0x0F),
      pax_col_rgb(0xCF, 0x7F, 0x0F), pax_col_rgb(0xCF, 0x87, 0x17),
      pax_col_rgb(0xC7, 0x87, 0x17), pax_col_rgb(0xC7, 0x8F, 0x17),
      pax_col_rgb(0xC7, 0x97, 0x1F), pax_col_rgb(0xBF, 0x9F, 0x1F),
      pax_col_rgb(0xBF, 0x9F, 0x1F), pax_col_rgb(0xBF, 0xA7, 0x27),
      pax_col_rgb(0xBF, 0xA7, 0x27), pax_col_rgb(0xBF, 0xAF, 0x2F),
      pax_col_rgb(0xB7, 0xAF, 0x2F), pax_col_rgb(0xB7, 0xB7, 0x2F),
      pax_col_rgb(0xB7, 0xB7, 0x37), pax_col_rgb(0xCF, 0xCF, 0x6F),
      pax_col_rgb(0xDF, 0xDF, 0x9F), pax_col_rgb(0xEF, 0xEF, 0xC7),
      pax_col_rgb(0xFF, 0xFF, 0xFF)};

  for (int i = 0; i < SCREEN_WIDTH; i++) {
    firebuf[(SCREEN_HEIGHT - 1) * SCREEN_WIDTH + i] = 36;
  }

  ESP_LOGI(TAG, "Welcome to the template app!");

  // Initialize the screen, the I2C and the SPI busses.
  bsp_init();

  // Initialize the RP2040 (responsible for buttons, etc).
  bsp_rp2040_init();

  // This queue is used to receive button presses.
  buttonQueue = get_rp2040()->queue;

  // Initialize graphics for the screen.
  pax_buf_init(&buf, NULL, 320, 240, PAX_BUF_16_565RGB);

  // Initialize NVS.
  nvs_flash_init();

  // Initialize WiFi. This doesn't connect to Wifi yet.
  wifi_init();

  pax_enable_multicore(1);

  // read PNG
  esp_err_t res  = mount_sd(GPIO_SD_CMD, GPIO_SD_CLK, GPIO_SD_D0, GPIO_SD_PWR, "/sd", false, 5);
  if(res != ESP_OK) ESP_LOGE(TAG, "could not mount SD card");
  FILE* fd = fopen("/sd/doompep.png", "rb");
  if(fd == NULL) ESP_LOGE(TAG, "could not open file");
  if(!pax_decode_png_fd(&name, fd, PAX_BUF_16_565RGB, 0)) ESP_LOGE(TAG, "could not parse png");

  while (1) {
    pax_background(&buf, 0xff000000);
    pax_draw_image(&buf, &name, 86, 82);
    doFire();
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      for (int y = 0; y < SCREEN_HEIGHT; y++) {
        uint8_t pal = firebuf[y * SCREEN_WIDTH + x];
        if (pal) {
          pax_col_t col = rgbs[pal];
          pax_set_pixel(&buf, col, x, y);
        }
      }
    }
    // Draws the entire graphics buffer to the screen.
    disp_flush();

    // Wait for button presses and do another cycle.

    // Structure used to receive data.
    rp2040_input_message_t message;

    // Wait forever for a button press (because of portMAX_DELAY)
    xQueueReceive(buttonQueue, &message, pdMS_TO_TICKS(1));

    // Which button is currently pressed?
    if (message.input == RP2040_INPUT_BUTTON_HOME && message.state) {
      // If home is pressed, exit to launcher.
      exit_to_launcher();
    }
  }
}

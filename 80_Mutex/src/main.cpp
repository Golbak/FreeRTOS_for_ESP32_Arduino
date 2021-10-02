#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_MCP23X17.h>

#define MCP1_ADDR 0x20
#define MCP2_ADDR 0x21

#define LED_PIN 11     // MCP23XXX pin LED is attached to

#define ON  LOW
#define OFF HIGH

struct s_mcp {
  Adafruit_MCP23X17 *mcp;
  uint8_t ui8I2CAddr;
};

static int iApp_cpu = 0;
static SemaphoreHandle_t shMutex;
static s_mcp mcp1 {new Adafruit_MCP23X17(), MCP1_ADDR};
static s_mcp mcp2 {new Adafruit_MCP23X17(), MCP2_ADDR};

//
// Lock i2c bus with mutex
//
static void lock_i2c() {
  BaseType_t rc;

  rc = xSemaphoreTake(shMutex,portMAX_DELAY);
  assert(rc == pdPASS);
}

//
// Unlock i2c bus with mutex
//
static void unlock_i2c() {
  BaseType_t rc;

  rc = xSemaphoreGive(shMutex);
  assert(rc == pdPASS);
}

//
// I2C blink task
//
static void led_task(void *argp) {
  s_mcp _mcp = *(s_mcp*)argp;
  bool bLedStatus = false;
  int rc;

  
  lock_i2c();
  printf("Testing I2C address 0x%02X\n",
    _mcp.ui8I2CAddr);
  rc = _mcp.mcp->begin_I2C(_mcp.ui8I2CAddr, &Wire);
  unlock_i2c();
  if (!rc) {
    printf("Error mcp with address 0x%02X not responding.\n",
      _mcp.ui8I2CAddr);
    vTaskDelete(nullptr);
  } else {
    printf("Correct initilization of mcp with address 0x%02X.\n",
      _mcp.ui8I2CAddr);
  }

  for (;;) {
    lock_i2c();
    bLedStatus ^= true;
    printf("LED 0x%02X %s\n",
      _mcp.ui8I2CAddr,
      bLedStatus ? "on" : "off");
    bLedStatus ? _mcp.mcp->digitalWrite(LED_PIN, ON) : _mcp.mcp->digitalWrite(LED_PIN, OFF);
    unlock_i2c();
    delay(_mcp.ui8I2CAddr == MCP1_ADDR ? 500 : 600);
  }
}


void setup() {
  BaseType_t rc;

  iApp_cpu = xPortGetCoreID();

  shMutex = xSemaphoreCreateMutex();
  assert(shMutex);

  Wire.begin();

  delay(2000);

  printf("\nMutex.ino\n");

  rc = xTaskCreatePinnedToCore(
    led_task,
    "led_task1",
    2000,
    &mcp1,
    1,
    nullptr,
    iApp_cpu
  );
  assert(rc == pdPASS);

  rc = xTaskCreatePinnedToCore(
    led_task,
    "led_task2",
    2000,
    &mcp2,
    1,
    nullptr,
    iApp_cpu
  );
  assert(rc == pdPASS);
}

void loop() {
  // put your main code here, to run repeatedly:
}
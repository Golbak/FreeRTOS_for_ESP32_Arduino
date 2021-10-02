#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_INA219.h>

// Configuration
#define DEV_BNO   0x29
#define INA219_SAMPLERATE_DELAY_MS 2000
#define BNO055_SAMPLERATE_DELAY_MS 500

static Adafruit_INA219 ina219;
static Adafruit_BNO055 bno = Adafruit_BNO055(55, DEV_BNO);

static QueueHandle_t qhComp = nullptr;
static QueueHandle_t qhPow = nullptr;
static SemaphoreHandle_t shChsem = nullptr;
static SemaphoreHandle_t shI2C = nullptr;

struct s_power {
  float fSVolt;
  float fBVolt;
  float fCurr;
  float fLVolt;
  float fPow;
};


//
// Lock I2C Bus
//
static inline void i2c_lock() {
  BaseType_t rc;

  rc = xSemaphoreTake(shI2C,portMAX_DELAY);
  assert(rc == pdPASS);
}

//
// Unlock I2C Bus
//
static inline void i2c_unlock() {
  BaseType_t rc;

  rc = xSemaphoreGive(shI2C);
  assert(rc == pdPASS);
}

//
// INA reading
//
static void pwr_task(void *argp) {
  s_power reading;
  BaseType_t rc;
  TickType_t ttTime;

  i2c_lock();
  if (!ina219.begin()) {
    i2c_unlock();
    printf("INA not found\n");
    vTaskDelete(nullptr);
  }
  i2c_unlock();

  reading.fBVolt  = 0;
  reading.fCurr   = 0;
  reading.fLVolt  = 0;
  reading.fPow    = 0;
  reading.fSVolt  = 0;

  ttTime = xTaskGetTickCount();

  for (;;) {
    i2c_lock();
    reading.fSVolt = ina219.getShuntVoltage_mV();
    reading.fBVolt = ina219.getBusVoltage_V();
    reading.fCurr  = ina219.getCurrent_mA();
    reading.fPow = ina219.getPower_mW();
    i2c_unlock();
    reading.fLVolt = reading.fBVolt + (reading.fSVolt / 1000);

    rc = xQueueOverwrite(qhPow,&reading);
    assert(rc == pdPASS);

    // Notify disp_task
    xSemaphoreGive(shChsem);

    vTaskDelayUntil(&ttTime, INA219_SAMPLERATE_DELAY_MS);
  }
}

//
// Compass reading
//
static void comp_task(void *argp) {
  imu::Vector<3> reading;
  BaseType_t rc;
  TickType_t ttTime;

  i2c_lock();
  if (!bno.begin()) {
    i2c_unlock();
    printf("BNO not found\n");
    vTaskDelete(nullptr);
  }
  i2c_unlock();

  ttTime = xTaskGetTickCount();

  for (;;) {
    i2c_lock();
    reading = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
    i2c_unlock();

    rc = xQueueOverwrite(qhComp,&reading);
    assert(rc == pdPASS);

    // Notify disp_task
    xSemaphoreGive(shChsem);

    vTaskDelayUntil(&ttTime, BNO055_SAMPLERATE_DELAY_MS);
  }
}

//
// Display task (Serial Monitor)
//
static void disp_task(void *argp) {
  imu::Vector<3> comp_reading;
  s_power pwr_reading;
  BaseType_t rc;

  for(;;) {
    // Wait for change notification:
    rc = xSemaphoreTake(shChsem,portMAX_DELAY);
    assert(rc == pdPASS);

    // Grab Power data if any:
    rc = xQueuePeek(qhPow,&pwr_reading,0);
    //rc = xQueueReceive(qhPow,&pwr_reading,0);
    if ( rc == pdPASS ) {
      printf("Power readings: %f, %f, %f\n",
        pwr_reading.fBVolt,
        pwr_reading.fCurr,
        pwr_reading.fPow
        );
    } else {
      printf("Power readings not available.\n");
    }

    // Grab Compass data if any:
    rc = xQueuePeek(qhComp,&comp_reading,0);
    //rc = xQueueReceive(qhComp,&comp_reading,0);
    if ( rc == pdPASS ) {
      printf("Compass readings: %f, %f, %f\n",
        comp_reading.x(),
        comp_reading.y(),
        comp_reading.z()
        );
    }  else {
      printf("Compass readings not available.\n");
    }
  }
}

//
// Program Initialization
//
void setup(void) 
{
  int iApp_cpu = xPortGetCoreID();
  BaseType_t rc;

  // Change notification:
  shChsem = xSemaphoreCreateBinary();
  assert(shChsem);

  //I2C locking semaphore:
  shI2C = xSemaphoreCreateBinary();
  assert(shI2C);
  rc = xSemaphoreGive(shI2C);
  assert( rc == pdPASS);

  //Power Mailbox:
  qhPow = xQueueCreate(1,sizeof(s_power));
  assert(qhPow);

  // Compass Mailbox:
  qhComp = xQueueCreate(1,sizeof(imu::Vector<3>));
  assert(qhComp);

  // Start I2C Bus Support:
  Wire.begin();

  // Allow USB to Serial to start:
  delay(2000);
  printf("\nMailBox.ino\n");

  // Power Reading Task
  rc = xTaskCreatePinnedToCore(
    pwr_task,             // Task
    "pwr_task",           // Friendly name
    2400,                   // Stack size
    nullptr,    // Parameters
    1,                      // Priority
    nullptr, // Handle
    iApp_cpu                // CPU
  );
  assert(rc == pdPASS);

  // Compass Reading Task
  rc = xTaskCreatePinnedToCore(
    comp_task,             // Task
    "comp_task",           // Friendly name
    2400,                   // Stack size
    nullptr,    // Parameters
    1,                      // Priority
    nullptr, // Handle
    iApp_cpu                // CPU
  );
  assert(rc == pdPASS);

  // Compass Reading Task
  rc = xTaskCreatePinnedToCore(
    disp_task,             // Task
    "disp_task",           // Friendly name
    4000,                   // Stack size
    nullptr,    // Parameters
    1,                      // Priority
    nullptr, // Handle
    iApp_cpu                // CPU
  );
  assert(rc == pdPASS);

}

void loop(void) 
{
  vTaskDelete(nullptr);
}
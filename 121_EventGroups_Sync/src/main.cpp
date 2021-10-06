#include <Arduino.h>
#include <stdlib.h>

//LED GPIOs
#define GPIO_LED1   18
#define GPIO_LED2   19
#define GPIO_LED3   21

#define N_LED       3

#define EV_RDY      0b1000
#define EV_ALL      (EV_RDY|0b0111)

static EventGroupHandle_t hevt;
static int leds[N_LED] =
  { GPIO_LED1, GPIO_LED2, GPIO_LED3 };

static void led_task(void *arg) {
  unsigned ledx = (unsigned)arg;    // LED index
  EventBits_t our_ev = 1 << ledx;   // Our Event
  EventBits_t rev;
  TickType_t timeout;
  unsigned seed =ledx;

  assert(ledx < N_LED);
  
  printf("led: %u pin: %u mask: %u\n", ledx, unsigned(leds[ledx]), (1 << ledx) );

  srand(seed);

  for (;;) {
    timeout = rand() % 100 + 10;
    rev = xEventGroupSync(
      hevt,
      our_ev,
      EV_ALL,
      timeout);

     // No time out: blink LED
    if ( (rev & EV_ALL) == EV_ALL )      
      digitalWrite(leds[ledx], !digitalRead(leds[ledx]));
  }
}

void setup() {
  int app_cpu = 0;
  BaseType_t rc;

  // Create Event Group
  hevt = xEventGroupCreate();
  assert(hevt);

  delay(2000);
  printf("\nevntgrpsync.ino\n");

  // Configure LED GPIOs
  for ( int x=0; x < N_LED; ++x ) {
    pinMode(leds[x], OUTPUT);
    digitalWrite(leds[x], LOW);

    rc = xTaskCreatePinnedToCore(
      led_task,
      "ledtsk",
      2100,
      (void*)x,
      1,
      nullptr,
      app_cpu
    );
    assert( rc == pdPASS );
  }
}

void loop() {
  delay(1000);
  xEventGroupSetBits(hevt, EV_RDY);
}
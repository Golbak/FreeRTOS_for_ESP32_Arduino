#undef __STRICT_ANSI__
#include <Arduino.h>
#include <stdlib.h>
#include <FreeRTOS.h>

//#define PREVENT_DEADLOCK
#define N                 4
#define N_EATERS          (N-1)

static QueueHandle_t      qhMsgq;
static SemaphoreHandle_t  shCsem;

enum State {
    Thinking = 0,
    Hungry,
    Eating
};

static const char *state_name[] = {
    "Thinking",
    "Hungry",
    "Eating"
};

struct s_philosopher {
  TaskHandle_t  th;
  unsigned      uNum;
  State         state;
  unsigned      uSeed;
};

struct s_message {
  unsigned      uNum;
  State         state;
};

static s_philosopher spPhilosophers[N];
static SemaphoreHandle_t shForks[N];
static volatile unsigned uLogno = 0;

static void send_state(s_philosopher *philo) {
  s_message msg;

  msg.uNum = philo->uNum;
  msg.state = philo->state;
  xQueueSendToBack(qhMsgq,&msg,portMAX_DELAY);
}

static void philo_task(void *argp) {
  s_philosopher *philo = (s_philosopher*)argp;
  SemaphoreHandle_t smFork1=0, smFork2=0;
  BaseType_t rc;

  delay((rand_r(&philo->uSeed) % 5) + 1 );

  for(;;)
  {
    // Thinking
    philo->state = Thinking;
    send_state(philo);
    delay((rand_r(&philo->uSeed) % 5) + 1);

    // Hungry
    philo->state = Hungry;
    send_state(philo);
    delay((rand_r(&philo->uSeed) % 5) + 1);

    // Pick up forks
    #ifdef PREVENT_DEADLOCK
      rc = xSemaphoreTake(shCsem,portMAX_DELAY);
      assert(rc == pdPASS);
    #endif

    smFork1 = shForks[philo->uNum];
    smFork2 = shForks[(philo->uNum+1) % N];
    
    rc = xSemaphoreTake(smFork1,portMAX_DELAY);
    assert(rc == pdPASS);
    delay((rand_r(&philo->uSeed) % 5) + 1);
    rc = xSemaphoreTake(smFork2,portMAX_DELAY);
    assert(rc == pdPASS);

    // Eating
    philo->state = Eating;
    send_state(philo);
    delay((rand_r(&philo->uSeed) % 5) + 1);

    // Put down forks:
    rc = xSemaphoreGive(smFork1);
    assert(rc == pdPASS);
    delay(1);
    rc = xSemaphoreGive(smFork2);
    assert(rc == pdPASS);

    #ifdef PREVENT_DEADLOCK
      rc = xSemaphoreGive(shCsem);
      assert(rc == pdPASS);
    #endif
  }
}

//
//  Program Initialization
//
void setup() {
  int iApp_cpu = xPortGetCoreID();
  BaseType_t rc;

  qhMsgq = xQueueCreate(30,sizeof(s_message));
  assert(qhMsgq);

  for ( unsigned uX = 0; uX < N; ++uX) {
    shForks[uX] = xSemaphoreCreateBinary();
    assert(shForks[uX]);
    rc = xSemaphoreGive(shForks[uX]);
    assert(rc);
  }

  delay(2000);

  printf("\nThe Dining Philosopher's Problem:\n");
  printf("\nThere are %u Philosophers.\n",N);

  #ifdef PREVENT_DEADLOCK
    shCsem = xSemaphoreCreateCounting(
      N_EATERS,
      N_EATERS
    );
    assert(shCsem);
    printf("With deadlock prevention.\n");
  #else
    shCsem = nullptr;
    printf("Without deadlock prevention.\n");
  #endif

  // Initiliza for task:
  for ( unsigned uX=0 ; uX<N ; ++uX) {
    spPhilosophers[uX].uNum = uX;
    spPhilosophers[uX].state = Thinking;
    //spPhilosophers[uX].uSeed = hallRead();
    spPhilosophers[uX].uSeed = 7369 + uX;
  }

  // Create philosopher tasks:
  for ( unsigned uX=0 ; uX<N ; ++uX) {
    rc = xTaskCreatePinnedToCore(
      philo_task,             // Task
      "philo_task",           // Friendly name
      5000,                   // Stack size
      &spPhilosophers[uX],    // Parameters
      1,                      // Priority
      &spPhilosophers[uX].th, // Handle
      iApp_cpu                // CPU
    );
    assert(rc == pdPASS);
    assert(spPhilosophers[uX].th);
  }  
}

//
//  Report philosopher states:
//
void loop() {
  s_message msg;

  while ( xQueueReceive(qhMsgq,&msg,1) == pdPASS) {
    printf("%05u: Philosopher %u is %s\n",
      ++uLogno,
      msg.uNum,
      state_name[msg.state]);
  }
}
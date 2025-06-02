#ifndef _EVENT_H_
#define _EVENT_H_

#include "process.h"

typedef enum {ARRIVAL, FINISH_CPU, FINISH_IO, FINISH_TIME_SLICE} event_type_t;

struct evt {
  time_ticks_t time;
  event_type_t type;
  struct process* proc;
};

void print_event(const struct evt* event);

#endif /* _EVENT_H_ */


#ifndef _EVENT_QUEUE_H_
#define _EVENT_QUEUE_H_

#include "event.h"

struct evt_node {
  const struct evt* event;
  struct evt_node* next_event;
};

const struct evt* pop_next_event();
void new_event(time_ticks_t time, event_type_t type, struct process* proc);
void remove_events(pid_t pid);
void print_event_queue();

#endif /* _EVENT_QUEUE_H_ */


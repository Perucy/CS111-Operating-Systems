#include "event_queue.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char* event_type_strings[] = {"ARRIVAL", "FINISH CPU", "FINISH I/O", "FINISH TIME SLICE"};

static struct evt_node* event_queue = NULL;


const struct evt* pop_next_event() {
  if (NULL == event_queue)
    return NULL;

  const struct evt* event = event_queue->event;
  const struct evt_node* old_node = event_queue;
  event_queue = event_queue->next_event;
  free((void*)old_node); // free the queue node; caller is responsible for freeing the event itself
  return event;
}


void new_event(time_ticks_t time, event_type_t type, struct process* proc) {
  // Find where to insert the event
  struct evt_node* prev = NULL;
  struct evt_node* next = event_queue;

  // TODO: check for duplicate events while doing this
  while (NULL != next && time >= next->event->time) {
    prev = next;
    next = next->next_event;
  }
  // INVARIANT: at the end of the list (next == NULL)
  //   OR prev.time <= event.time < next.time
  // (both NULL means event_queue was empty)

  // Create the event struct and event queue node, initialize both
  struct evt* event = malloc(sizeof(struct evt));
  memset(event, 0, sizeof(struct evt));
  event->time = time;
  event->type = type;
  event->proc = proc;

#ifdef DEBUG
  fprintf(stderr, "Creating Event: ");
  print_event(event);
#endif // DEBUG

  struct evt_node* event_node = malloc(sizeof(struct evt_node));
  memset(event_node, 0, sizeof(struct evt_node));
  event_node->event = event;
  event_node->next_event = next;

  // Do the actual insert
  if (NULL == prev)
    event_queue = event_node; // evt is first! (also handles empty queue)
  else
    prev->next_event = event_node; // evt is not first
}


void remove_events(pid_t pid) {
  struct evt_node* prev_node = NULL;
  struct evt_node* event_node = event_queue;
  while (NULL != event_node) {
    if (pid == event_node->event->proc->pid) {

#ifdef DEBUG
      fprintf(stderr, "Removing Event: ");
      print_event(event_node->event);
#endif // DEBUG

      if (NULL == prev_node) {
        assert(event_queue == event_node);
        event_queue = event_node->next_event;
        free((void*)event_node->event); // free the event
        free((void*)event_node); // free the queue node
        event_node = event_queue;
      } else {
        assert(prev_node->next_event == event_node);
        prev_node->next_event = event_node->next_event;
        free((void*)event_node->event); // free the event
        free((void*)event_node); // free the queue node
        event_node = prev_node->next_event;
      }

    } else {
      prev_node = event_node;
      event_node = event_node->next_event;
    }
  }
}


void print_event(const struct evt* event) {
  fprintf(stderr, "(t=%d) proc %d %s\n", event->time, event->proc->pid, event_type_strings[event->type]);
}


void print_event_queue() {
  fprintf(stderr, "\nEVENT QUEUE\n");

  for (const struct evt_node* next_event = event_queue;
       NULL != next_event;
       next_event = next_event->next_event) {
    print_event(next_event->event);
  }

  fprintf(stderr, "\n");
}


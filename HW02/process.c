#include "process.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

const char* state_strings[] = {"NOT_ARRIVED", "READY", "BLOCKED", "TERMINATED"};
const char* burst_strings[] = {"CPU", "I/O"};


void print_process(const struct process* proc) {
  fprintf(stderr, "\tPROCESS\n\tpid: %d\n\tstate: %s\n\ttickets: %d\n\tarrival time: %d\n",
          proc->pid, state_strings[proc->state], proc->tickets, proc->arrival_time);
  const struct burst* next_burst = proc->current_burst;
  while (NULL != next_burst) {
    fprintf(stderr, "\t%s burst: %d\n", burst_strings[next_burst->type], next_burst->remaining_time);
    next_burst = next_burst->next_burst;
  }
}


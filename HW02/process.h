#ifndef _PROCESS_H_
#define _PROCESS_H_

typedef unsigned int time_ticks_t;
typedef int pid_t;


typedef enum {CPU_BURST=0, IO_BURST=1} burst_type_t;

struct burst {
  burst_type_t type;
  time_ticks_t remaining_time;
  struct burst* next_burst;
};


typedef enum {NOT_ARRIVED, READY, BLOCKED, TERMINATED} state_t;

struct process {
  pid_t pid;
  state_t state;
  unsigned int tickets;
  time_ticks_t arrival_time;
  struct burst* current_burst;
};


void print_process(const struct process* proc);

#endif /* _PROCESS_H_ */


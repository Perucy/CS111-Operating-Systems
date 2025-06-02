#include "scheduler.h"
#include "event_queue.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

// whitespace characters to use as a delimiter
#define WHITESPACE_DELIM " \t\r\n"

static time_ticks_t INITIAL_TIME_SLICE = 0;
static time_ticks_t TIME_SLICE = 0;

static struct process** process_list = NULL; // array of pointers to processes; array index = pid
static unsigned int num_procs = 0; // number of processes NOT in the TERMINATED state

time_ticks_t current_time = 0;
time_ticks_t time_started = 0;
static const struct process* currently_running = NULL;


pid_t get_current_proc() {
  if (NULL == currently_running)
    return -1;
  else
    return currently_running->pid;
}


time_ticks_t get_time_slice() {
  return TIME_SLICE;
}

void use_time_slice(bool_t use) {
  if (use)
    TIME_SLICE = INITIAL_TIME_SLICE;
  else
    TIME_SLICE = 0;
}


void print_process_list() {
  fprintf(stderr, "\nPROCESS LIST\n");
  unsigned int pid = 0;
  for (const struct process* next_proc = process_list[pid];
       NULL != next_proc;
       next_proc = process_list[++pid]) {
    print_process(next_proc);
    fprintf(stderr, "\n");
  }
}


void terminate_process(struct process* proc) {
  proc->state = TERMINATED;
  --num_procs;
}


void finish_burst(struct process* proc) {
  struct burst* old_burst = proc->current_burst;
  if (NULL != old_burst) {
    proc->current_burst = old_burst->next_burst;
    free(old_burst);
  }

  if (NULL == proc->current_burst) {
    terminate_process(proc);
  } else if (CPU_BURST == proc->current_burst->type)
    proc->state = READY;
  else if (IO_BURST == proc->current_burst->type)
    proc->state = BLOCKED;
}


time_ticks_t deduct_burst(struct process* proc, time_ticks_t amount) {
  if (NULL == proc->current_burst) {
    if (TERMINATED != proc->state) {
      fprintf(stderr,
              "WARNING: Process %d is in state %d, despite having no remaining bursts! Changing state to TERMINATED.\n",
              proc->pid, proc->state);
      terminate_process(proc);
    }
    return 0;
  }
  // INVARIANT: proc->current_burst is valid

  if (amount >= proc->current_burst->remaining_time) {
    finish_burst(proc);
    return 0;
  } else {
    proc->current_burst->remaining_time -= amount;
    assert(proc->current_burst->remaining_time > 0);
    return proc->current_burst->remaining_time;
  }
}


void end_cpu_event() {
  // set up next event on this proc (FINISH_CPU or FINISH_TIME_SLICE)
  assert(CPU_BURST == currently_running->current_burst->type);
  time_ticks_t run_for_time = currently_running->current_burst->remaining_time;
  event_type_t event_type = FINISH_CPU;

  if (get_time_slice() > 0 && get_time_slice() < run_for_time) {
    run_for_time = get_time_slice();
    event_type = FINISH_TIME_SLICE;
  }

  new_event(current_time + run_for_time, event_type, process_list[currently_running->pid]);
}


int context_switch(pid_t pid) {
  if(pid < 0) {
    printf("WARNING: invalid pid value %d\n", pid);
    return -1;
  }
  if (READY != process_list[pid]->state) {
    printf("WARNING: process %d is not in the READY state\n", pid);
    return -1;
  }
  if (NULL != currently_running && currently_running->pid == pid) {
    printf("WARNING: attempt to context switch to currently running process (pid=%d)\n", pid);
    return -1;
  }
  // INVARIANTS: pid is valid, not the currently_running process, and the process is able to run

  if (NULL != currently_running && READY == currently_running->state) {
    remove_events(currently_running->pid); // remove the FINISH_CPU or FINISH_TIME_SLICE event
  }

  currently_running = process_list[pid];
  time_started = current_time;
  printf("(t=%d) running proc %d\n", current_time, currently_running->pid);
  end_cpu_event();
  return 0;
}

time_ticks_t event_loop() {
  for (const struct evt* event = pop_next_event();
       NULL != event && num_procs > 0;
       event = pop_next_event()) {

#ifdef DEBUG
    fprintf(stderr, "Handling Event: ");
    print_event(event);
#endif // DEBUG

    current_time = event->time;
    // update remaining_time on the currently_running process (ending the current burst, if it has finished)
    if (current_time > time_started && NULL != currently_running) {
      deduct_burst(process_list[currently_running->pid], current_time - time_started);
      time_started = current_time;
    }

    switch (event->type) {

    case ARRIVAL:
      assert(CPU_BURST == event->proc->current_burst->type);
      event->proc->state = READY;
      printf("(t=%d) proc %d arrived\n", current_time, event->proc->pid);
      sched_new_process(event->proc);
      break;

    case FINISH_TIME_SLICE:
      assert(CPU_BURST == event->proc->current_burst->type);
      assert(READY == event->proc->state);
      if (TERMINATED == event->proc->state) {
        assert(NULL == event->proc->current_burst);
        sched_terminated(event->proc);
      } else {
        assert(CPU_BURST == event->proc->current_burst->type);
        assert(READY == event->proc->state);
        pid_t prev_proc = currently_running->pid;
        sched_finished_time_slice(event->proc);
        if (prev_proc == currently_running->pid)
          end_cpu_event(); // continuing same proc after time slice requires new time slice event
      }
      break;

    case FINISH_CPU:
      if (TERMINATED == event->proc->state) {
        assert(NULL == event->proc->current_burst);
        sched_terminated(event->proc);

      } else {
        assert(IO_BURST == event->proc->current_burst->type);
        assert(BLOCKED == event->proc->state);
        new_event(current_time + event->proc->current_burst->remaining_time,
                  FINISH_IO,
                  event->proc);
        printf("(t=%d) proc %d blocked for I/O\n", current_time, event->proc->pid);
        sched_blocked(event->proc);
      }
      break;

    case FINISH_IO:
      assert(IO_BURST == event->proc->current_burst->type);
      assert(BLOCKED == event->proc->state);
      finish_burst(event->proc);

      if (TERMINATED == event->proc->state) {
        assert(NULL == event->proc->current_burst);
        sched_terminated(event->proc);

      } else {
        // proc should not be TERMINATED immediately after
        // finishing an I/O burst (only after a CPU burst)
        assert(CPU_BURST == event->proc->current_burst->type);
        assert(READY == event->proc->state);
        printf("(t=%d) proc %d finished I/O\n", current_time, event->proc->pid);
        sched_unblocked(event->proc);
      }
      break;

    default:
      fprintf(stderr, "ERROR: Unrecognized event type %d at time %u; ignoring event...\n", event->type, event->time);
    }
    free((void*)event);
    event = NULL;

    if (NULL != currently_running && READY != currently_running->state) {
        printf("(t=%d) idle\n", current_time);
        currently_running = NULL;
    }
  }
  // INVARIANT: all processes are TERMINATED state AND event loop is empty
  return current_time;
}

void load_file(const char* filename) {
  char line[1024];
  FILE* file = fopen(filename, "r");
  if (NULL == file) {
    perror("ERROR opening file");
    exit(EXIT_FAILURE);
  }
  
  // Get the TIME_SLICE value
  if (NULL == fgets(line, 1024, file)) {
    perror("ERROR reading file");
    fclose(file);
    exit(EXIT_FAILURE);
  }
  size_t first_digit = strcspn(line, "1234567890");
  char* token = &line[first_digit];
  size_t after_last_digit = strspn(line, "1234567890");
  line[after_last_digit] = '\0';
  char* endptr = NULL;
  TIME_SLICE = INITIAL_TIME_SLICE = strtoul(token, &endptr, 10);
  if ('\0' != *endptr) {
    perror("ERROR in file contents");
    fprintf(stderr, "Failed to convert string \"%s\" to TIME_SLICE value\n", token);
    fclose(file);
    exit(EXIT_FAILURE);
  }

  // Get the number of processes
  if (NULL == fgets(line, 1024, file)) {
    perror("ERROR reading file");
    fclose(file);
    exit(EXIT_FAILURE);
  }
  first_digit = strcspn(line, "1234567890");
  token = &line[first_digit];
  after_last_digit = strspn(line, "1234567890");
  line[after_last_digit] = '\0';
  endptr = NULL;
  num_procs = strtoul(token, &endptr, 10);
  if ('\0' != *endptr) {
    perror("ERROR in file contents");
    fprintf(stderr, "Failed to convert string \"%s\" to NUM_PROCS value\n", token);
    fclose(file);
    exit(EXIT_FAILURE);
  }

  // Load the processes
  process_list = malloc((num_procs + 1) * sizeof(struct process*));
  memset(process_list, 0, (num_procs + 1) * sizeof(struct process*));

  for (unsigned int pid = 0; pid < num_procs; ++pid) {
    process_list[pid] = malloc(sizeof(struct process));
    memset(process_list[pid], 0, sizeof(struct process));
    process_list[pid]->pid = pid;
    process_list[pid]->state = NOT_ARRIVED;

    if (NULL == fgets(line, 1024, file)) {
      perror("ERROR reading file");
      fclose(file);
      exit(EXIT_FAILURE);
    }

    token = strtok(line, WHITESPACE_DELIM);
    if (NULL == token) {
      perror("ERROR in file contents");
      fprintf(stderr, "No number of tickets found on process line: %s\n", line);
      fclose(file);
      exit(EXIT_FAILURE);
    }
    endptr = NULL;
    process_list[pid]->tickets = strtoul(token, &endptr, 10);
    if ('\0' != *endptr) {
      perror("ERROR in file contents");
      fprintf(stderr, "Failed to convert string \"%s\" to number of tickets\n", token);
      fclose(file);
      exit(EXIT_FAILURE);
    }

    token = strtok(NULL, WHITESPACE_DELIM);
    if (NULL == token) {
      perror("ERROR in file contents");
      fprintf(stderr, "No arrival time found on process line: %s\n", line);
      fclose(file);
      exit(EXIT_FAILURE);
    }
    endptr = NULL;
    process_list[pid]->arrival_time = strtoul(token, &endptr, 10);
    if ('\0' != *endptr) {
      perror("ERROR in file contents");
      fprintf(stderr, "Failed to convert string \"%s\" to arrival time\n", token);
      fclose(file);
      exit(EXIT_FAILURE);
    }

    // the list of bursts starts as a CPU burst,
    // and then alternates between CPU and I/O bursts
    burst_type_t burst_type = CPU_BURST;
    struct burst** next_burst_ptr = &process_list[pid]->current_burst;
    token = strtok(NULL, WHITESPACE_DELIM);

    while (NULL != token) {
      // create burst
      struct burst* next_burst = malloc(sizeof(struct burst));
      memset(next_burst, 0, sizeof(struct burst));

      // populate burst info
      next_burst->type = burst_type;
      endptr = NULL;
      next_burst->remaining_time = strtoul(token, &endptr, 10);
      if ('\0' != *endptr) {
        perror("ERROR in file contents");
        fprintf(stderr, "Failed to convert string \"%s\" to burst time\n", token);
        fclose(file);
        exit(EXIT_FAILURE);
      }

      // point to burst, then update next pointer
      *next_burst_ptr = next_burst;
      next_burst_ptr = &next_burst->next_burst;

      // change burst type for next burst
      if (CPU_BURST == burst_type)
        burst_type = IO_BURST;
      else
        burst_type = CPU_BURST;

      // get the next burst time token
      token = strtok(NULL, WHITESPACE_DELIM);
    }

    new_event(process_list[pid]->arrival_time, ARRIVAL, process_list[pid]);
  }

  fclose(file);
}


void cleanup_processes() {
  for (unsigned int i = 0; NULL != process_list[i]; ++i) {
    if (TERMINATED != process_list[i]->state) {
      printf("ERROR: Finishing simulation while process %d is not TERMINATED (status=%d)\n",
             process_list[i]->pid, process_list[i]->state);
#ifdef DEBUG
      print_process(process_list[i]);
#endif // DEBUG
    }

    struct burst* this_burst = process_list[i]->current_burst;
    while (NULL != this_burst) {
      fprintf(stderr, "WARNING: Freeing burst type %d with remaining time %d on process %d\n",
              this_burst->type, this_burst->remaining_time, process_list[i]->pid);

      struct burst* prev_burst = this_burst;
      this_burst = this_burst->next_burst;
      free(prev_burst);
    }

    free(process_list[i]);
  }
  free(process_list);
  process_list = NULL;
}


int main(int argc, char** argv) {
  if (argc <= 1) {
    fprintf(stderr, "Usage: ./simulation filename.proc\n");
    return EXIT_FAILURE;
  }
  load_file(argv[1]);

  sched_init();
  time_ticks_t end_time = event_loop();
  // INVARIANT: event queue should now be empty
  printf("Finished at time %d\n", end_time);
  sched_cleanup();

  cleanup_processes();
  return EXIT_SUCCESS;
}


#include "scheduler.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdlib.h>
#include <assert.h>

/***************************************************************************
 * 
 *            Priority Queue Implementation for ready processes
 *
 * *********************************************************************/
typedef struct PQnode {
    const struct process* proc;
    time_ticks_t remaining_time;
    struct PQnode* next;
} PQnode;

typedef struct {
    PQnode* head;
} PQ;
PQ ready_procqueue;



void init_pq(PQ* pq) {
    pq->head = NULL;
}

void add_to_pq(PQ* q, const struct process* proc, time_ticks_t remaining_time) {
    PQnode* new_node = (PQnode*)malloc(sizeof(PQnode));
    assert(new_node != NULL);

    new_node->proc = proc;
    new_node->remaining_time = remaining_time;
    new_node->next = NULL;

    PQnode** current = &(q->head);
    while (*current != NULL) {
        if ((*current)->remaining_time > remaining_time) {
            break;
        } else if ((*current)->remaining_time == remaining_time && (*current)->proc->pid > proc->pid) {
            break;
        }
        current = &((*current)->next);
    }
   

    new_node->next = *current;
    *current = new_node; 
}

void remove_from_pq(PQ* q, const struct process* proc) {
    PQnode** current = &(q->head);
    while (*current != NULL && (*current)->proc != proc) {
        current = &((*current)->next);
    }
    if (*current != NULL) {
        PQnode* temp = *current;
        *current = (*current)->next;
        free(temp);
    }
}

const struct process* get_process(PQ* q) {
    if (q->head == NULL) {
        return NULL;
    }

   return q->head->proc;
}

void free_PQ(PQ* q) {
    PQnode* current = q->head;
    while (current != NULL) {
        PQnode* temp = current;
        current = current->next;
        free(temp);
    }
    q->head = NULL;
}

void print_ready_queue(PQ* q) {
    PQnode* current = q->head;
    printf("Ready Queue: ");
    while (current != NULL) {
        printf("(pid=%d, remaining=%d) ", current->proc->pid, current->remaining_time);
        current = current->next;
    }
    printf("\n");
}
time_ticks_t get_curr_proc_time(PQ* q, pid_t pid) {
    if (q->head == NULL) {
        return -1;
    }
    PQnode* current = q->head;
    while (current != NULL && current->proc->pid != pid) {
        current = current->next;
    }
    return current->proc->current_burst->remaining_time;
}
const struct process* get_curr_proc(PQ* q, pid_t pid) {
    if (q->head == NULL) {
        return NULL;
    }
    PQnode* current = q->head;
    while (current != NULL && current->proc->pid != pid) {
        current = current->next;
    }
    return current->proc;
}
/*******************************************************
 * SHORTEST TIME TO COMPLETION FIRST (STCF) Scheduler  *
 * (also known as Shortest Remaining Time First (SRTF) *
 *******************************************************/


/* sched_init
 *   will be called exactly once before any processes arrive or any other events
 */
void sched_init() {
    use_time_slice(FALSE);

    init_pq(&ready_procqueue);
  
}


/* sched_new_process
 *   will be called when a new process arrives (i.e., fork())
 *
 * proc - the new process that just arrived
 */

 void sched_new_process(const struct process* proc) {
    assert(READY == proc->state);

    if (ready_procqueue.head == NULL) {
        add_to_pq(&ready_procqueue, proc, proc->current_burst->remaining_time);
        context_switch(proc->pid);
        return;
    }
    
    pid_t curr_pid = get_current_proc();
    const struct process* curr_process = get_curr_proc(&ready_procqueue, curr_pid);

    add_to_pq(&ready_procqueue, proc, proc->current_burst->remaining_time);

    const struct process* top_proc = get_process(&ready_procqueue);

    if (curr_process->state == BLOCKED) {
        //printf("current proc is blocked\n");
        sched_blocked(curr_process);
    
    } else if (curr_process->state == TERMINATED) {
        //printf("current proc is terminated\n");
        sched_terminated(curr_process);

    } else if (get_current_proc() == -1) {
        //printf("current proc is idle\n");
        
        context_switch(top_proc->pid);

    } else if (get_current_proc() != top_proc->pid) {
        time_ticks_t time_left = get_curr_proc_time(&ready_procqueue, get_current_proc());
        
        if (time_left >= top_proc->current_burst->remaining_time) {
            context_switch(top_proc->pid);
        }
        
    } 
   
}


/* sched_finished_time_slice
 *   will be called when the currently running process finished a time slice
 *   (This is only called when the time slice ends with time remaining in the
 *   current CPU burst.  If finishing the time slice happens at the same time
 *   that the process blocks / terminates,
 *   then sched_blocked() / sched_terminated() will be called instead).
 *
 * proc - the process whose time slice just ended
 *
 * Note: Time slice end events only occur if use_time_slice() is set to TRUE
 */
void sched_finished_time_slice(const struct process* proc) {
  assert(READY == proc->state);
  // TODO: implement this
}


/* sched_blocked
 *   will be called when the currently running process blocks
 *   (e.g., if it starts an I/O operation that it needs to wait to finish
 *
 * proc - the process that just blocked
 */
void sched_blocked(const struct process* proc) {
    assert(BLOCKED == proc->state);
   
    // Remove the blocked process from the ready queue
    remove_from_pq(&ready_procqueue, proc);


    // switch to the next process in the ready queue
    const struct process* next_proc = get_process(&ready_procqueue);
    if (next_proc == NULL) {
        return;
    }

    if (next_proc->pid != get_current_proc() || get_current_proc() == -1) {
        context_switch(next_proc->pid);
    } 
}

/* sched_unblocked
 *   will be called when a blocked process unblocks
 *   (e.g., if its I/O operation finished)
 *
 * proc - the process that just unblocked
 */
void sched_unblocked(const struct process* proc) {
    assert(READY == proc->state);
   
    sched_new_process(proc);
}


/* sched_terminated
 *   will be called when the currently running process terminates
 *   (i.e., it finished it's last CPU burst)
 *
 * proc - the process that just terminated
 *
 * Note: "kill" commands and other ways to terminate a process that is not
 *       currently running are not being simulated, so only the currently running
 *       process can actually terminate.
 */
void sched_terminated(const struct process* proc) {
    assert(TERMINATED == proc->state);

    // Remove the terminated process from the ready queue
    remove_from_pq(&ready_procqueue, proc);

    const struct process* next_proc = get_process(&ready_procqueue);
    if (next_proc == NULL) {
        return;
    }
    if (next_proc->pid != get_current_proc() || get_current_proc() == -1) {
        context_switch(next_proc->pid);
    } 

}
/* sched_cleanup
 *   will be called exactly once after all processes have terminated and there
 *   are no more events left to occur, just before the simulation exits
 *
 * Note: Calling sched_cleanup() is guaranteed if the simulation has a normal exit
 *       but is not guaranteed in the case of fatal errors, crashes, or other
 *       abnormal exits.
 */
void sched_cleanup() {
  // TODO: implement this
    free_PQ(&ready_procqueue);
}


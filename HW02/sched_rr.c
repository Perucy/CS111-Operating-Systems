#include "scheduler.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



typedef struct Node {
    const struct process* proc;
    struct Node* next;
} Node;

typedef struct {
    Node* front;
    Node* back;
} Queue;

Queue ready_procqueue;
Queue blocked_procqueue;

/************************init_queue******************** */
void init_queue(Queue* q) {
    q->front = NULL;
    q->back = NULL;	
}

/***********************enqueue************************ */
void enqueue(Queue* q, const struct process* proc) {
    Node* new_node = malloc(sizeof(Node));
    assert(new_node != NULL);

    new_node->proc = proc;
    new_node->next = NULL;

    if (q->back == NULL) {
        q->front = q->back = new_node;
    } else {
        q->back->next = new_node;
        q->back = new_node;
    }
}

/***********************dequeue************************ */
const struct process* dequeue(Queue* q) {
    
    if (q->front == NULL) {
        return NULL;
    }

    const struct process* proc = q->front->proc;

    Node* temp = q->front;
    q->front = q->front->next;

    if (q->front == NULL) {
        q->back = NULL;
    }

    free(temp);
    return proc;
}

/**********************free_queue***************************** */
void free_queue(Queue* q) {
    while (q->front != NULL) {
        Node* temp = q->front;
        q->front = q->front->next;
        free(temp);
    }
    q->back = NULL;
}
void dequeue_process(Queue *q, const struct process* proc) {
    Node** current = &(q->front);
    while (*current != NULL && (*current)->proc != proc) {
        current = &((*current)->next);
    }
    //Node* blk = NULL;
    if (*current != NULL) {
        Node* temp = *current;
        //blk = *current;
        *current = (*current)->next;
        if (q->front == NULL) {
            q->back = NULL;
        }
        free(temp);

    }
}
/*************************
 * ROUND ROBIN Scheduler *
 *************************/

/* sched_init
 *   will be called exactly once before any processes arrive or any other events
 */

void sched_init() {
    use_time_slice(TRUE);
  
    /*initialize queue of processes*/
    init_queue(&ready_procqueue);
    init_queue(&blocked_procqueue);
}


/* sched_new_process
 *   will be called when a new process arrives (i.e., fork())
 *
 * proc - the new process that just arrived
 */
void sched_new_process(const struct process* proc) {
    assert(READY == proc->state);

    enqueue(&ready_procqueue, proc);

    pid_t curr = get_current_proc();

    Node* top = (&ready_procqueue)->front;
    if (top == NULL) {
        return;
    }

    if (curr != top->proc->pid) {
        if (curr == -1 && top->proc->state == READY){
            context_switch(top->proc->pid);
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
    
    const struct process* fin_proc = dequeue(&ready_procqueue);

    enqueue(&ready_procqueue, fin_proc);

    Node* top = (&ready_procqueue)->front;

    if (top->proc->pid != get_current_proc() && top->proc->state == READY){
        context_switch(top->proc->pid);
    }
}


/* sched_blocked
 *   will be called when the currently running process blocks
 *   (e.g., if it starts an I/O operation that it needs to wait to finish
 *
 * proc - the process that just blocked
 */
void sched_blocked(const struct process* proc) {
    assert(BLOCKED == proc->state);

    const struct process* blokd_proc = dequeue(&ready_procqueue);
    (void) blokd_proc;

    enqueue(&blocked_procqueue, proc);

    Node* top = (&ready_procqueue)->front;
    if (top == NULL){
        return;
    }

    if (top->proc->pid != get_current_proc() && top->proc->state == READY){
        context_switch(top->proc->pid);
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

    enqueue(&ready_procqueue, proc);

    Node* top = (&ready_procqueue)->front;
    
    if (get_current_proc() != top->proc->pid && get_current_proc() == -1) {
        if (top->proc->state == READY){
            context_switch(top->proc->pid);
        }else{
            context_switch(proc->pid);
        }
    }
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

    const struct process* term_proc = dequeue(&ready_procqueue);
    if (term_proc == NULL) {
        return;
    }
   
    Node* top = (&ready_procqueue)->front;
    if (top == NULL) {
        return;
    }
    
    if (top->proc->pid != get_current_proc() && top->proc->state == READY){
        context_switch(top->proc->pid);
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
    free_queue(&ready_procqueue);
    free_queue(&blocked_procqueue);
}


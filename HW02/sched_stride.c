#include "scheduler.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/********************
 * STRIDE Scheduler *
 ********************/

#define STRIDE_CONSTANT 1000000

/**************************************************************************
 * 
 *            Priority Queue Implementation for ready processes
 * 
 **************************************************************************/
typedef struct PQnode {
    const struct process* proc;
    int stride;
    int pass;
    struct PQnode* next;
} PQnode;
typedef struct {
    PQnode* head;
} PQ;
PQ ready_procqueue;

void init_pq(PQ* pq) {
    pq->head = NULL;
}
void free_PQ(PQ* q) {
    PQnode* curr = q->head;
    while (curr != NULL) {
        PQnode* temp = curr;
        curr = curr->next;
        free(temp);
    }
    q->head = NULL;
}
void add_to_pq(PQ* q, const struct process* proc, int stride, int pass) {
    PQnode* new_node = (PQnode*)malloc(sizeof(PQnode));
    assert(new_node != NULL);

    new_node->proc = proc;
    new_node->stride = stride;
    new_node->pass = pass;
    new_node->next = NULL;

    PQnode** current = &(q->head);
    
    while (*current != NULL) {
        if ((*current)->pass < pass) {
            // Current node has a lower pass value, keep traversing
            current = &((*current)->next);
        } else if ((*current)->pass == pass && (*current)->proc->pid < proc->pid) {
            // Same pass value, skip all nodes with the same pass
            
            current = &((*current)->next);
            
        } else {
            // Found a node with a higher pass value, insert before it
            break;
        }
    }
   
    new_node->next = *current;
    *current = new_node;
    /*PQnode** same = NULL;

    while (*curr != NULL) {
        if ((*curr)->pass > pass) {
            break;
        } else if ((*curr)->pass == pass) {
            same = curr;
        }
        curr = &((*curr)->next);
    }
    if (same != NULL) {
        new_node->next = (*same)->next;
        (*same)->next = new_node;
    } else {
        new_node->next = *curr;
        *curr = new_node;
    }*/
}
void remove_from_pq(PQ* q, const struct process* proc) {
    PQnode** curr = &(q->head);
    while(*curr != NULL && (*curr)->proc->pid != proc->pid) {
        curr = &((*curr)->next);
    }
    if (*curr != NULL) {
        PQnode* temp = *curr;
        *curr = (*curr)->next;
        free(temp);
    }
}
PQnode* get_process_node(PQ* q, pid_t pid) {
    if (q->head == NULL) {
        //printf("head null\n");
        return NULL;
    }
    PQnode* current = q->head;
    while (current != NULL && current->proc->pid != pid) {
        current = current->next;
    }
    if (current != NULL) {
        //printf("in\n");
        return current;
    }
    //printf("end\n");
    return NULL;

}

/*************************************************************************
 * 
 * BlockedQueue Implementation for blocked processes
 * 
 * ************************************************************************/
typedef struct BQNode {
    const struct process* proc;
    struct BQNode* next;
    int p_stride;
    int p_pass;
} BQNode;
typedef struct {
    BQNode* front;
    BQNode* back;
} BQ;
BQ blocked_procqueue;

void init_bq(BQ* bq) {
    bq->front = NULL;
    bq->back = NULL;
}
void enqueue(BQ* bq, const struct process* proc, int p_stride, int p_pass) {
    BQNode* new_node = (BQNode*)malloc(sizeof(BQNode));
    assert(new_node != NULL);

    new_node->proc = proc;
    new_node->next = NULL;
    new_node->p_stride = p_stride;
    new_node->p_pass = p_pass;

    if (bq->back == NULL) {
        bq->front = new_node;
        bq->back = new_node;
    } else {
        bq->back->next = new_node;
        bq->back = new_node;
    }
}
void dequeue_process(BQ* bq, const struct process* proc) {
    BQNode** current = &(bq->front);
    while (*current != NULL && (*current)->proc != proc) {
        current = &((*current)->next);
    }
    if (*current != NULL) {
        BQNode* temp = *current;
        *current = (*current)->next;
        if (bq->front == NULL) {
            bq->back = NULL;
        }
        free(temp);

    }
}
void free_bq(BQ* bq) {
    BQNode* current = bq->front;
    while (current != NULL) {
        BQNode* temp = current;
        current = current->next;
        free(temp);
    }
    bq->front = NULL;
    bq->back = NULL;
}
BQNode* get_blocked_node(BQ* q, pid_t pid) {
    if (q->front == NULL) {
        return NULL;
    }
    BQNode* current = q->front;
    while (current != NULL && current->proc->pid != pid) {
        current = current->next;
    }
    if (current != NULL) {
        return current;
    }
    return NULL;

}
/*************************************************************************
 * 
 *                  STRIDE Scheduler Implementation
 * 
 * ************************************************************************/
/* sched_init
 *   will be called exactly once before any processes arrive or any other events
 */

 void print_ready_queue(PQ* q) {
    if (q->head == NULL) {
        return;
    }
    PQnode* current = q->head;
    while (current != NULL) {
        printf("Process pid: %d, Process state: %d, Process stride: %d, Process pass: %d\n", current->proc->pid, current->proc->state, current->stride, current->pass);
        current = current->next;
        
    }
   
}
void sched_init() {
    use_time_slice(TRUE);
    init_pq(&ready_procqueue);
    init_bq(&blocked_procqueue);    
}


/* sched_new_process
 *   will be called when a new process arrives (i.e., fork())
 *
 * proc - the new process that just arrived
 */
void sched_new_process(const struct process* proc) {
    assert(READY == proc->state);

    int stride_val = STRIDE_CONSTANT / proc->tickets;

    add_to_pq(&ready_procqueue, proc, stride_val, 0);

    /*get current running process*/
    pid_t curr = get_current_proc();

    /*get node on top of PQ*/
    PQnode* top = (&ready_procqueue)->head;

    /**if the top node on PQ is not the current running process,
     * and (top process is READY and cpu is idle)
    */
    if (curr != top->proc->pid) {
        if (curr == -1 && top->proc->state == READY && top->proc->pid != get_current_proc()) {
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

    PQnode* node = get_process_node(&ready_procqueue, proc->pid);
    int node_pass = node->pass;
    int node_stride = node->stride;

    remove_from_pq(&ready_procqueue, proc);
 
    node_pass += node_stride;

    add_to_pq(&ready_procqueue, proc, node_stride, node_pass);

    /*get node on top of PQ*/
    PQnode* top = (&ready_procqueue)->head;
    if (top->proc->pid != get_current_proc()){
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

    //printf("Blocking: %d, State: %d\n", proc->pid, proc->state);

    PQnode* node = get_process_node(&ready_procqueue, proc->pid);
    PQnode* nxt_node = NULL;
    if (node->next != NULL){
        nxt_node = node->next;
    }

    enqueue(&blocked_procqueue, proc, node->stride, node->pass + node->stride);

    remove_from_pq(&ready_procqueue, proc);

    /*get node on top of PQ*/
    PQnode* top = (&ready_procqueue)->head;


    //if next process in queue is available, context switch to it
    //if not then switch to top process in the queue
    if (nxt_node == NULL && top != NULL){
        if (top->proc->pid != get_current_proc()){
            context_switch(top->proc->pid);
        }
    }else if (nxt_node != NULL) {
        if (nxt_node->proc->pid != get_current_proc()){
           context_switch(nxt_node->proc->pid); 
        }
        
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

    BQNode* node = get_blocked_node(&blocked_procqueue, proc->pid);
    if (node == NULL) return;

    add_to_pq(&ready_procqueue, proc, node->p_stride, node->p_pass);

    dequeue_process(&blocked_procqueue, proc);

    /*get current running process*/
    pid_t curr = get_current_proc();

    /*get node on top of PQ*/
    PQnode* top = (&ready_procqueue)->head;

    /**if the top node on PQ is not the current running process,
     * and (top process is READY and cpu is idle)
    */
    if (curr != top->proc->pid) {
        if (curr == -1 && top->proc->state == READY && top->proc->pid != get_current_proc()) {
           context_switch(top->proc->pid); 
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
        
    remove_from_pq(&ready_procqueue, proc);

    PQnode* top = (&ready_procqueue)->head;

    if (top != NULL && top->proc->pid != get_current_proc()){
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
   free_PQ(&ready_procqueue);
   free_bq(&blocked_procqueue);
    
}


#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "process.h"

/*****************************
 * Implement These Functions *
 *****************************/

/* sched_init
 *   will be called exactly once before any processes arrive or any other events
 */
void sched_init();

/* sched_new_process
 *   will be called when a new process arrives (i.e., fork())
 *
 * proc - the new process that just arrived
 */
void sched_new_process(const struct process* proc);

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
void sched_finished_time_slice(const struct process* proc);

/* sched_blocked
 *   will be called when the currently running process blocks
 *   (e.g., if it starts an I/O operation that it needs to wait to finish
 *
 * proc - the process that just blocked
 */
void sched_blocked(const struct process* proc);

/* sched_unblocked
 *   will be called when a blocked process unblocks
 *   (e.g., if its I/O operation finished)
 *
 * proc - the process that just unblocked
 */
void sched_unblocked(const struct process* proc);

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
void sched_terminated(const struct process* proc);

/* sched_cleanup
 *   will be called exactly once after all processes have terminated and there
 *   are no more events left to occur, just before the simulation exits
 *
 * Note: Calling sched_cleanup() is guaranteed if the simulation has a normal exit
 *       but is not guaranteed in the case of fatal errors, crashes, or other
 *       abnormal exits.
 */
void sched_cleanup();


/* since C doesn't have a native boolean type, we made one */
typedef enum {FALSE=0, TRUE=1} bool_t;


/************************************
 * These Are Functions You May Call *
 ************************************/

/* context_switch
 *   call this function to change the currently running process to pid
 *
 * pid - process ID of the process to context switch to
 *
 * returns 0 on success or -1 on failure, in which case the currently running
 * process will not change
 *
 * Note: does NOT set errno on failure (unlike real syscalls), but will print
 *       a warning message saying what went wrong
 */
int context_switch(pid_t pid);

/* get_current_proc
 *   gets the pid of the current process
 *
 * returns the process ID of the currently running process,
 * or -1 if the CPU is idle (i.e., no process is currently running)
 */
pid_t get_current_proc();

/* get_time_slice
 *   gets the time slice parameter value
 *
 * returns the number of ticks in each time slice, or 0 if time slices are not in use
 */
time_ticks_t get_time_slice();

/* use_time_slice
 *   sets whether to use time slices
 *
 * use - TRUE if you want to receive time slice finished events,
 *       or FALSE if you do not (in which case the current process will just
 *       keep running until the next event)
 */
void use_time_slice(bool_t use);

/* print_process_list
 *   prints every process in the simulation to stderr
 *   This reflects all process' current state at the time this function is called.
 */
void print_process_list();

#endif /* _SCHEDULER_H_ */


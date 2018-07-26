/*
 * student.c
 * Multithreaded OS Simulation for CS 2200, Project 4
 * Fall 2014
 *
 * This file contains the CPU scheduler for the simulation.
 * Name: Jongwoo Jang
 * GTID: 903170765
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "os-sim.h"


/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pthread_mutex_t current_mutex;

static pcb_t *head;

static pthread_cond_t cond_idle;
static pthread_mutex_t ready_queue_mutex;

static int scheduler_rr;
static int scheduler_strf;
static int preemption_time;

static int num_cpu;
static void push(pcb_t *pcb);
static pcb_t* pop();
/*
typedef struct _pcb_t {
    const unsigned int pid;
    const char *name;
    unsigned int time_remaining;
    process_state_t state;
    op_t *pc;
    struct _pcb_t *next;
} pcb_t;
*/
static void push(pcb_t *pcb) {
    printf("pushing : %s \n", pcb->name);

    pthread_mutex_lock(&ready_queue_mutex);

    printf("PUSH locking ready queue.\n");

    if (head == NULL) {
        head = pcb;
        pcb -> next = NULL;

    } else if (scheduler_strf == 1) {
        pcb_t *current = head;
        if (pcb -> time_remaining < head -> time_remaining) {
            pcb -> next = current;
            head = pcb;
        } else {
            while (current -> next != NULL && current -> next -> time_remaining < pcb -> time_remaining) {
                current = current -> next;
            }
            pcb -> next = current -> next;
            current -> next = pcb;
        }
    } else {
        pcb_t *current = head;
        while (current -> next != NULL) {
            current = current -> next;
        }
        current -> next = pcb;
        pcb -> next = NULL;
    }
    pthread_mutex_unlock(&ready_queue_mutex);
    printf("PUSH Unlocking ready queue.\n");
    pthread_cond_signal(&cond_idle);
    printf("pushing end\n");
}

static pcb_t* pop() {
    pthread_mutex_lock(&ready_queue_mutex);
    printf("POP locking ready queue.\n");
    printf("we are in pop\n");
    pcb_t *ret_pcb;
    if (head == NULL) {
        ret_pcb = NULL;
    } else {
        printf("popped = %s\n", head->name);
        ret_pcb = head;
        head = head -> next;
    }
    pthread_mutex_unlock(&ready_queue_mutex);
    printf("POP Unlocking ready queue.\n");
    return ret_pcb;
}



/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running process indexed by the cpu id. 
 *	See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id) {
    pcb_t *pcb;

    printf("schedule popping\n");
    pcb = pop();

    if (pcb == NULL) {
        context_switch(cpu_id, NULL, preemption_time);
    } else {
        pcb -> state = PROCESS_RUNNING;
        pthread_mutex_lock(&current_mutex);
        current[cpu_id] = pcb;
        pthread_mutex_unlock(&current_mutex);
        context_switch(cpu_id, pcb, preemption_time);
    }
}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id) {
    printf("in Idle\n");
    pthread_mutex_lock(&current_mutex);
    current[cpu_id] = NULL;
    pthread_mutex_unlock(&current_mutex);
    pthread_mutex_lock(&ready_queue_mutex);
    printf("Idle locking ready queue.\n");

    while (head == NULL) {
        printf("Not Idle\n");
        pthread_cond_wait(&cond_idle, &ready_queue_mutex);
    }
    pthread_mutex_unlock(&ready_queue_mutex);
    printf("Idle Unlocking ready queue.\n");

    schedule(cpu_id);

    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id) {
    pcb_t *pcb;
    pthread_mutex_lock(&current_mutex);
    pcb = current[cpu_id];
    pthread_mutex_unlock(&current_mutex);
    pcb -> state = PROCESS_READY;
    push(pcb);

    schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id) {
    pthread_mutex_lock(&current_mutex);
    current[cpu_id] -> state = PROCESS_WAITING;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id) {
    pthread_mutex_lock(&current_mutex);
    current[cpu_id] -> state = PROCESS_TERMINATED;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is SRTF, wake_up() may need
 *      to preempt the CPU with the highest remaining time left to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with a lower remaining time left than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for 
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process) {
    printf("Wake up state \n");
    int force_preempt_flag = 0;
    int cpu_index;
    int longest_time_remaining_job = 0;

    process -> state = PROCESS_READY;

    push(process);

    if (scheduler_strf) {
        pthread_mutex_lock(&current_mutex);
        for (int i = 0; i < num_cpu; i++) {
            if (current[i] == NULL) {
                force_preempt_flag = 1;
                break;
            }

            if (current[i] != NULL) {
                if (current[i] -> time_remaining > longest_time_remaining_job) {
                    longest_time_remaining_job = current[i] -> time_remaining;
                    cpu_index = i;
                }
            }
        }
        pthread_mutex_unlock(&current_mutex);
        if (force_preempt_flag != 1 && process -> time_remaining < longest_time_remaining_job) {
            force_preempt(cpu_index);
        }
    }
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -s command-line parameters.
 */
int main(int argc, char *argv[]) {
    int cpu_count;
    scheduler_rr = 0;
    scheduler_strf = 0;

    /* Parse command-line arguments */
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -s ]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler\n"
            "         -s : Shortest Remaining Time First Scheduler\n\n");
        return -1;
    }
    cpu_count = atoi(argv[1]);
    num_cpu = cpu_count;
    // ./os-sim(0) <# CPUs>(1) -r(2) 5(3)
    if (argc == 2) {
        preemption_time = -1;
    } else if (argc == 3) {
        scheduler_strf = 1;
        preemption_time = -1;
    } else if (argc == 4) {
        scheduler_rr = 1;
        preemption_time = atoi(argv[3]);
    }

    head = NULL;



    /* FIX ME - Add support for -r and -s parameters*/

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
    pthread_mutex_init(&ready_queue_mutex, NULL);
    pthread_cond_init(&cond_idle, NULL);


    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}



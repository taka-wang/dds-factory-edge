#include "threadpool.h"
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include "factory.h"                // generated code

#define AND &&
#define TOPIC_LIGHT   "LIGHT"       // DDS topic name
#define TOPIC_ALARM   "ALARM"       // DDS topic name
#define TOPIC_WARNING "WARNING"     // DDS topic name

enum 
{ 
    MAX_SAMPLES = 200,              // max num of sample for each take
    THREAD      = 32,               // num of the threads in thread pool
    QUEUE       = 128               // num of the task queue
};

typedef enum 
{ 
    light_e, 
    alarm_e,
    warning_e
} reader_type;  

// global variables
static dds_condition_t terminated_cond;     // terminated condition variable

extern void thread_task(void *arg);         // thread handler

// compound sample with sample pointer and sample info arrays
typedef struct compound_sample_s 
{
    void*              samples_ptr [MAX_SAMPLES];
    dds_sample_info_t  samples_info[MAX_SAMPLES];
    int sample_count;
    int counter;
    reader_type type;
} compound_sample_t;

// handle ctrl+c signal
static void sigint_handler (int fdw_ctrl_type)
{
    dds_guard_trigger (terminated_cond);    // set trigger_value
}

// thread handler
void thread_task(void *arg) 
{
    compound_sample_t* compound_samples_ptr = (compound_sample_t*)arg;
    DDS_ERR_CHECK (compound_samples_ptr->sample_count, DDS_CHECK_REPORT);
    
    light_color_t * sample_ptr1 = NULL;
    alarm_msg_t   * sample_ptr2 = NULL;
    warning_msg_t * sample_ptr3 = NULL;

    // give chance to catch terminate signal
    for (uint16_t i = 0; !dds_condition_triggered (terminated_cond) AND 
                    (i < compound_samples_ptr->sample_count); i++)
    {
        // verify sample from sample_info
        if (compound_samples_ptr->samples_info[i].valid_data)
        {
            switch (compound_samples_ptr->type)
            {
                case light_e: 
                    sample_ptr1 = compound_samples_ptr->samples_ptr[i];
                    printf("read color(%d): %d, %s, %s, %d\n", 
                        compound_samples_ptr->counter,
                        sample_ptr1->machine_id, 
                        sample_ptr1->date,
                        sample_ptr1->time, 
                        sample_ptr1->color);
                    break;
                case alarm_e: 
                    sample_ptr2 = compound_samples_ptr->samples_ptr[i];
                    printf("read alarm(%d): %d, %s, %s, %s, %s, %s\n", 
                        compound_samples_ptr->counter,
                        sample_ptr2->machine_id, 
                        sample_ptr2->date, 
                        sample_ptr2->time, 
                        sample_ptr2->major, 
                        sample_ptr2->minor, 
                        sample_ptr2->msg);
                    break;
                case warning_e:
                    sample_ptr3 = compound_samples_ptr->samples_ptr[i];
                    printf("read warning(%d): %d, %s, %s, %s, %s\n", 
                        compound_samples_ptr->counter,
                        sample_ptr3->machine_id, 
                        sample_ptr3->date, 
                        sample_ptr3->time, 
                        sample_ptr3->msg_num,
                        sample_ptr3->msg);
                    break;
                default:
                    break;
            }
        }
    }
    //dds_sleepfor (DDS_MSECS(1000)); // simulate busy task, can be remove

    free(compound_samples_ptr);
    assert(arg == NULL);
}

int main (int argc, char *argv[])
{
    // Change signal disposition
    struct sigaction sat;
    sat.sa_handler = sigint_handler;
    sigemptyset (&sat.sa_mask);
    sat.sa_flags = 0;
    sigaction (SIGINT, &sat, NULL);

    // Init thread pool
    threadpool_t* pool = threadpool_create(THREAD, QUEUE, 0);
    fprintf(stderr, "Pool started with %d threads and queue size of %d\n", THREAD, QUEUE);
    
    // Declare dds entities ------------------------
    int counter                         = 0;

    dds_qos_t*      qos                 = NULL;
    dds_entity_t    domain_participant  = NULL;
    dds_entity_t    subscriber          = NULL;

    dds_entity_t    light_topic         = NULL;
    dds_entity_t    light_reader        = NULL;
    dds_condition_t light_cond;         // condition variable

    dds_entity_t    alarm_topic         = NULL;
    dds_entity_t    alarm_reader        = NULL;
    dds_condition_t alarm_cond;         // condition variable

    dds_entity_t    warning_topic       = NULL;
    dds_entity_t    warning_reader      = NULL;
    dds_condition_t warning_cond;       // condition variable

    dds_waitset_t   ws;
    dds_attach_t    ws_results[2];
    size_t          ws_result_size      = 2u;
    dds_time_t      ws_timeout          = DDS_SECS (20);

    uint32_t        sample_mask         = 0;
    uint32_t        reader_status       = 0;

    // Initialize DDS ------------------------
    int status = dds_init ( 0, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create domain participant
    status = dds_participant_create (   // factory method to create domain participant
                &domain_participant,    // pointer to created domain participant entity
                DDS_DOMAIN_DEFAULT,     // domain id (DDS_DOMAIN_DEFAULT = -1)
                qos,                    // Qos on created domain participant (can be NULL)
                NULL                    // Listener on created domain participant (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create a subscriber
    status = dds_subscriber_create (    // factory method to create subscriber
                domain_participant,     // domain participant entity
                &subscriber,            // pointer to created subscriber entity
                qos,                    // Qos on created subscriber (can be NULL)
                NULL                    // Listener on created subscriber (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create topic light
    status = dds_topic_create (         // factory method to create topic
                domain_participant,     // domain participant entity
                &light_topic,           // pointer to created topic entity
                &light_color_t_desc,    // pointer to IDL generated topic descriptor
                TOPIC_LIGHT,            // name of created topic
                NULL,                   // Qos on created topic (can be NULL)
                NULL                    // Listener on created topic (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create topic alarm
    status = dds_topic_create (         // factory method to create topic
                domain_participant,     // domain participant entity
                &alarm_topic,           // pointer to created topic entity
                &alarm_msg_t_desc,      // pointer to IDL generated topic descriptor
                TOPIC_ALARM,            // name of created topic
                NULL,                   // Qos on created topic (can be NULL)
                NULL                    // Listener on created topic (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create topic warning
    status = dds_topic_create (         // factory method to create topic
                domain_participant,     // domain participant entity
                &warning_topic,         // pointer to created topic entity
                &warning_msg_t_desc,    // pointer to IDL generated topic descriptor
                TOPIC_WARNING,          // name of created topic
                NULL,                   // Qos on created topic (can be NULL)
                NULL                    // Listener on created topic (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create qos ------------------------------------------------------------------
    qos = dds_qos_create ();
    dds_qset_reliability (              // Set the reliability policy in the qos structure
        qos,                            // The pointer to the qos structure
        DDS_RELIABILITY_RELIABLE,       // kind: {RELIABLE, BEST_EFFORT}
        DDS_SECS (10)                   // max_blocking_time, default: 100ms
    );
    //dds_qset_history (qos, DDS_HISTORY_KEEP_ALL, 0);

    // Create light reader with qos
    status = dds_reader_create (        // factory method to create typed reader
                subscriber,             // domain participant entity or subscriber entity
                &light_reader,          // pointer to created reader entity
                light_topic,            // topic entity
                qos,                    // Qos on created reader (can be NULL)
                NULL                    // Listener on created reader (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create alarm reader with qos
    status = dds_reader_create (        // factory method to create typed reader
                subscriber,             // domain participant entity or subscriber entity
                &alarm_reader,          // pointer to created reader entity
                alarm_topic,            // topic entity
                qos,                    // Qos on created reader (can be NULL)
                NULL                    // Listener on created reader (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create warning reader with qos
    status = dds_reader_create (        // factory method to create typed reader
                subscriber,             // domain participant entity or subscriber entity
                &warning_reader,        // pointer to created reader entity
                warning_topic,          // topic entity
                qos,                    // Qos on created reader (can be NULL)
                NULL                    // Listener on created reader (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    dds_qos_delete (qos);               // delete qos

    ws = dds_waitset_create ();         // create waitset

    terminated_cond = dds_guardcondition_create (); // Init guard condition
    status = dds_waitset_attach (       // attach guard condition to waitset
                ws,                     // pointer to a waitset
                terminated_cond,        // pointer to a condition to wait for the trigger value
                terminated_cond         // attach condition, could be used to know the reason for the waitset to unblock (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // light read cond ------------------------------------
    light_cond = dds_readcondition_create ( 
                    light_reader,       // Reader entity on which the condition is created
                    DDS_ANY_STATE       // mask set the sample_state, instance_state and view_state of the sample
                   ); 
    dds_status_set_enabled ( 
        light_reader,                   // Entity to enable the status
        DDS_DATA_AVAILABLE_STATUS       // mask Status value that indicates the status to be enabled
    );

    // Attach condition variable to waitset
    status = dds_waitset_attach (
                ws,                     // pointer to a waitset
                light_cond,             // pointer to a condition to wait for the trigger value 
                light_reader            // attach condition, could be used to know the reason for the waitset to unblock (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // alarm read cond ------------------------------------
    alarm_cond = dds_readcondition_create ( 
                    alarm_reader,       // Reader entity on which the condition is created
                    DDS_ANY_STATE       // mask set the sample_state, instance_state and view_state of the sample
                   ); 
    dds_status_set_enabled ( 
        alarm_reader,                   // Entity to enable the status
        DDS_DATA_AVAILABLE_STATUS       // mask Status value that indicates the status to be enabled
    );

    // Attach condition variable to waitset
    status = dds_waitset_attach (
                ws,                     // pointer to a waitset
                alarm_cond,             // pointer to a condition to wait for the trigger value 
                alarm_reader            // attach condition, could be used to know the reason for the waitset to unblock (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);


    // warning read cond ------------------------------------
    warning_cond = dds_readcondition_create ( 
                    warning_reader,     // Reader entity on which the condition is created
                    DDS_ANY_STATE       // mask set the sample_state, instance_state and view_state of the sample
                   ); 
    dds_status_set_enabled ( 
        warning_reader,                 // Entity to enable the status
        DDS_DATA_AVAILABLE_STATUS       // mask Status value that indicates the status to be enabled
    );

    // Attach condition variable to waitset
    status = dds_waitset_attach (
                ws,                     // pointer to a waitset
                warning_cond,           // pointer to a condition to wait for the trigger value 
                warning_reader          // attach condition, could be used to know the reason for the waitset to unblock (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Waiting for writer appear -------------------------

    printf ("Waiting for writer...\n");
    status = dds_waitset_wait (
                ws,                     // pointer to a waitset
                ws_results,             // pointer to an array of attached_conditions based on the conditions associated with a waitset (can be NULL)
                ws_result_size,         // number of attached conditions (can be zero)
                DDS_INFINITY            // timeout value associated with a waitset (can be INFINITY or some value)
            ); // inf block
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    
    // Loop ----------------------------------------------

    while (!dds_condition_triggered (terminated_cond) )
    {
        // blocking
        status = dds_waitset_wait ( 
                    ws,                 // pointer to a waitset
                    ws_results,         // pointer to an array of attached_conditions based on the conditions associated with a waitset (can be NULL)
                    ws_result_size,     // number of attached conditions (can be zero)
                    ws_timeout          // timeout value associated with a waitset (can be INFINITY or some value)
                );
        DDS_ERR_CHECK (status, DDS_CHECK_REPORT);

        // timeout checking
        if (status == 0 )
        {
            printf ("Timed out while waiting for an event.\n");
            break;
        }

        // num of signaled waitset conditions
        if (status > 0)
        {
            counter++;

            compound_sample_t* compound_samples_ptr = NULL;

            for (uint16_t i = 0; i < ws_result_size; i++)
            {
                if (ws_results[i] != NULL)
                {
                    compound_samples_ptr = (compound_sample_t*) calloc (1, sizeof(compound_sample_t));
                    compound_samples_ptr->counter = counter;

                    status = dds_status_take (ws_results[i], &reader_status, DDS_DATA_AVAILABLE_STATUS);

                    if (dds_statuscondition_get (light_reader) == dds_statuscondition_get (ws_results[i])) 
                    {
                        compound_samples_ptr->type = light_e;
                    } 
                    else if (dds_statuscondition_get (alarm_reader) == dds_statuscondition_get (ws_results[i]))
                    {
                        compound_samples_ptr->type = alarm_e;
                    }
                    else if (dds_statuscondition_get (warning_reader) == dds_statuscondition_get (ws_results[i]))
                    {
                        compound_samples_ptr->type = warning_e;
                    }
                    else
                    {
                        free (compound_samples_ptr);
                        compound_samples_ptr = NULL;
                        continue;
                    }

                    compound_samples_ptr->sample_count = dds_take (
                        ws_results[i],                          // reader entity 
                        compound_samples_ptr->samples_ptr,      // (void **) array of pointers to samples (pointers can be NULL)
                        MAX_SAMPLES,                            // max num of samples to read
                        compound_samples_ptr->samples_info,     // pointer to an array of dds_sample_info_t
                        sample_mask                             // filter mask, [sample, view, instance state]
                    );

                    if (compound_samples_ptr->sample_count > 0 )
                    {
                        threadpool_add (
                            pool, 
                            &thread_task, 
                            compound_samples_ptr, 
                            0
                        );
                    }
                    else // no sample
                    {
                        free (compound_samples_ptr);
                        compound_samples_ptr = NULL;
                    }
                }
            }
        }
    }

    printf ("Cleaning up...\n");

    threadpool_destroy (pool, threadpool_graceful);
    dds_sleepfor (DDS_SECS(1));

    // detach light read cond
    status = dds_waitset_detach (
                ws, 
                light_cond
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_condition_delete (light_cond);

    // detach alarm read cond
    status = dds_waitset_detach (
            ws, 
            alarm_cond
        );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_condition_delete (alarm_cond);


    // detach warning read cond
    status = dds_waitset_detach (
            ws, 
            warning_cond
        );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_condition_delete (warning_cond);


    // detach terminal guard cond
    status = dds_waitset_detach (       // Disassociate the condition attached with a waitset
                ws,                     // pointer to a waitset
                terminated_cond         // pointer to a condition to wait for the trigger value
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_condition_delete (terminated_cond);
    
    // delete waitset
    status = dds_waitset_delete (ws);
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    dds_entity_delete (domain_participant);
    dds_fini ();
    printf ("Finished.\n");
    exit(0);
}
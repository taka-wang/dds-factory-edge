#include "factory.h"                    // generated code and headers

#define MY_TOPIC "WARNING"              // DDS topic name

static volatile int do_loop = 1;        // global variables
static void sigint_handler (int sig)    // signal handler
{
    do_loop = 0;
}

int main (int argc, char *argv[])
{
    // Change signal disposition
    struct sigaction sat;
    sat.sa_handler = sigint_handler;
    sigemptyset (&sat.sa_mask);
    sat.sa_flags = 0;
    sigaction (SIGINT, &sat, NULL);

    // Declare dds entities ------------------------
    int status;
    dds_qos_t*   qos                = NULL;
    dds_entity_t domain_participant = NULL;
    dds_entity_t warning_topic      = NULL;
    dds_entity_t publisher          = NULL;  
    dds_entity_t warning_writer     = NULL;
    
    // Initialize DDS ------------------------
    status = dds_init (0, NULL);
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create participant
    status = dds_participant_create (   // factory method to create domain participant
                &domain_participant,    // pointer to created domain participant entity
                DDS_DOMAIN_DEFAULT,     // domain id (DDS_DOMAIN_DEFAULT = -1)
                qos,                    // Qos on created domain participant (can be NULL)
                NULL                    // Listener on created domain participant (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create a publisher
    status = dds_publisher_create (     // factory method to create publisher
                domain_participant,     // domain participant entity
                &publisher,             // pointer to created publisher entity
                qos,                    // Qos on created publisher (can be NULL)
                NULL                    // Listener on created publisher (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create topic for writer
    status = dds_topic_create (         // factory method to create topic
                domain_participant,     // domain participant entity
                &warning_topic,         // pointer to created topic entity
                &warning_msg_t_desc,    // pointer to IDL generated topic descriptor
                MY_TOPIC,               // name of created topic
                NULL,                   // Qos on created topic (can be NULL)
                NULL                    // Listener on created topic (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create writer without Qos
    status = dds_writer_create (        // factory method to create typed writer
                domain_participant,     // domain participant entity or publisher entity
                &warning_writer,        // pointer to created writer entity
                warning_topic,          // topic entity
                NULL,                   // Qos on created writer (can be NULL)
                NULL                    // Listener on created writer (can be NULL)
            );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Prepare samples  ------------------------
    warning_msg_t writer_msg;
    writer_msg.date = "2017/03/15";
    writer_msg.time = "17:00";
    writer_msg.msg_num = "messge number";
    writer_msg.msg = "Your software just isn’t mission critical";

    srand ((unsigned int)time(NULL));

    while ( do_loop ) 
    {
        // dds write
        writer_msg.machine_id = rand() % 200;
        status = dds_write ( 
                    warning_writer,     // writer entity
                    &writer_msg         // pointer to topic structure
                );
        DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

        printf ("write: %d\n", writer_msg.machine_id);
        dds_sleepfor (DDS_MSECS(200));
    }

    // release resources
    printf ("Sanitize\n");
    dds_entity_delete (domain_participant);
    dds_fini ();
    exit (0);
}
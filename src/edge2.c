/*
 * Copyright (C) Taka Wang. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.
 */

#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include "zdb.h"
#include "threadpool.h"
#include "iniparser.h"
#include "factory.h" // generated code

#define AND &&
#define STREQ( s1, s2 ) ( strcasecmp ( ( s1 ), ( s2 ) ) == 0 )
#define MAX_SAMPLES 200 // max num of sample for each take
#define MAX_URL_LEN 256 // max SQL length

// global variables
static IniParserPtr_T parser            = NULL;     // config parser context
static threadpool_t* pool               = NULL;     // thread pool context
static ConnectionPool_T connection_pool = NULL;     // connection pool
static URL_T url                        = NULL;     // connection url

static dds_condition_t terminated_cond;     // terminated condition variable

// reader type
typedef enum 
{ 
    LIGHT_T, 
    ALARM_T,
    WARN_T
} reader_type_t;  

// compound sample with sample pointer and sample info arrays
typedef struct compound_sample_s 
{
    void*              samples_ptr [MAX_SAMPLES];
    dds_sample_info_t  samples_info[MAX_SAMPLES];
    int                sample_count;
    reader_type_t      type;
} compound_sample_t;

extern void thread_task(void *arg); // thread handler

static void handle_int_signal (int fdw_ctrl_type)
{
    dds_guard_trigger (terminated_cond);    // set trigger_value
}

static void change_signal_disposition (void)
{
    struct sigaction sat;
    sat.sa_handler = handle_int_signal;
    sigemptyset ( &sat.sa_mask );
    sat.sa_flags = 0;
    sigaction ( SIGINT, &sat, NULL );
}

static bool load_config ( const char *const filename )
{
    parser = IniParser_Create ( filename );
    if (0 == IniParser_CheckInstanceError ( parser ))
    {
        return true;
    }
    else
    {
        fprintf ( stderr, "Failed to load config\n");
        return false;
    }
}

static bool init_connection_pool (void)
{
    char * buf = (char *)calloc((MAX_URL_LEN), sizeof(char));
    snprintf(buf, MAX_URL_LEN, "mysql://%s:%d/%s?user=%s&password=%s", 
        IniParser_GetString (parser,  "db", "url", "localhost"),
        IniParser_GetInteger(parser,  "db", "port", 3306),
        IniParser_GetString (parser,  "db", "name", "test"),
        IniParser_GetString (parser,  "db", "user", "root"),
        IniParser_GetString (parser,  "db", "password", "")
    );
    //printf("Connection string: %s\n", buf);

    url = URL_new(buf);
    free(buf);
    connection_pool = ConnectionPool_new(url);
    assert(connection_pool);
    ConnectionPool_start(connection_pool);
    Connection_T conn = ConnectionPool_getConnection(connection_pool);
    assert(conn);
    
    // create tables if not exists
    TRY
    {
        // light table
        Connection_execute(conn, "CREATE TABLE IF NOT EXISTS %s %s", 
            IniParser_GetString (parser,  "light", "table_name", "light"), 
            "(id INT NOT NULL, date VARCHAR(20), time VARCHAR(20), color INT);");

        // alarm table
        Connection_execute(conn, "CREATE TABLE IF NOT EXISTS %s %s", 
            IniParser_GetString (parser,  "alarm", "table_name", "alarm"), 
            "(id INT NOT NULL, date VARCHAR(20), time VARCHAR(20), major VARCHAR(200), minor VARCHAR(200), msg VARCHAR(200));");
        
        // warning table
        Connection_execute(conn, "CREATE TABLE IF NOT EXISTS %s %s", 
            IniParser_GetString (parser,  "warning", "table_name", "warning"), 
            "(id INT NOT NULL, date VARCHAR(20), time VARCHAR(20), msg_num VARCHAR(200), msg VARCHAR(200));");
    }
    CATCH(SQLException)
    {
        printf("SQLException -- %s\n", Exception_frame.message);
        return false;
    }
    FINALLY
    {
        Connection_close(conn);
        return true;
    }
    END_TRY;
}

static bool init_thread ( int max_thread, int max_queue )
{
    pool = threadpool_create ( max_thread, max_queue, 0 );
    if (pool != NULL)
    {
        fprintf ( stderr, "Pool started with %d threads and queue size of %d\n", max_thread, max_queue );
        return true;
    }
    else
    {
        fprintf ( stderr, "Failed to start pool with %d threads and queue size of %d\n", max_thread, max_queue );
        return false;
    }
}

static void init ( const char *const ini_filename )
{
    change_signal_disposition ();
    if ( !load_config (ini_filename)) 
    { 
        exit (EXIT_FAILURE);
    }
    if ( !init_connection_pool())
    {
        exit (EXIT_FAILURE);
    }
    if ( !init_thread ( IniParser_GetInteger( parser, "basic", "max_thread", 32 ), 
                        IniParser_GetInteger( parser, "basic", "max_queue", 128 ))) 
    {
        exit (EXIT_FAILURE);
    }
}

static void fini (void)
{
    ConnectionPool_free(&connection_pool); URL_free(&url);
    IniParser_Destroy ( parser );
    threadpool_destroy ( pool, threadpool_graceful );
    sleep (1);
}

// thread handler
void thread_task(void *arg) 
{
    compound_sample_t* cs_ptr = (compound_sample_t*)arg;
    DDS_ERR_CHECK (cs_ptr->sample_count, DDS_CHECK_REPORT);
    
    light_color_t * ptr1 = NULL;
    alarm_msg_t   * ptr2 = NULL;
    warning_msg_t * ptr3 = NULL;

    Connection_T conn = ConnectionPool_getConnection(connection_pool);
    if ( NULL != conn ) 
    {
        // give chance to catch terminate signal
        for (uint16_t i = 0; !dds_condition_triggered (terminated_cond) AND (i < cs_ptr->sample_count); i++)
        {
            // verify sample from sample_info
            if (cs_ptr->samples_info[i].valid_data)
            {
                switch (cs_ptr->type)
                {
                    case LIGHT_T: 
                        ptr1 = cs_ptr->samples_ptr[i];
                        Connection_execute (conn, "INSERT INTO %s VALUES (%d, \"%s\", \"%s\", %d)", 
                            IniParser_GetString (parser,  "light", "table_name", "light"), 
                            ptr1->machine_id, ptr1->date, ptr1->time, ptr1->color);
                        //printf("read color: %d, %s, %s, %d\n", ptr1->machine_id, ptr1->date, ptr1->time, ptr1->color);
                        break;
                    case ALARM_T: 
                        ptr2 = cs_ptr->samples_ptr[i];
                        Connection_execute (conn, "INSERT INTO %s VALUES (%d, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")", 
                            IniParser_GetString (parser,  "alarm", "table_name", "alarm"), 
                            ptr2->machine_id, ptr2->date, ptr2->time, ptr2->major, ptr2->minor, ptr2->msg);
                        //printf("read alarm: %d, %s, %s, %s, %s, %s\n", ptr2->machine_id, ptr2->date, ptr2->time, ptr2->major, ptr2->minor, ptr2->msg);
                        break;
                    case WARN_T:
                        ptr3 = cs_ptr->samples_ptr[i];
                        Connection_execute (conn, "INSERT INTO %s VALUES (%d, \"%s\", \"%s\" ,\"%s\" ,\"%s\")", 
                            IniParser_GetString (parser,  "warning", "table_name", "warning"), 
                            ptr3->machine_id, ptr3->date, ptr3->time, ptr3->msg_num, ptr3->msg);
                        //printf("read warning: %d, %s, %s, %s, %s\n", ptr3->machine_id, ptr3->date, ptr3->time, ptr3->msg_num, ptr3->msg);
                        break;
                    default:
                        break;
                }
            }
        }
        Connection_close(conn);
    }
    else
    {
        printf("Fail to get context from connection pool!\n");
    }
    free(cs_ptr);
    assert(arg == NULL);
}

int main (int argc, char *argv[])
{
    // 0. 
    const char * config_name = (argc > 1) ? argv[1] : "/etc/edge/config.ini";
    init (config_name);
    
    // 1. Initialize DDS ------------------------------------
    int status = dds_init ( 0, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // 2. Create domain participant  ------------------------
    dds_entity_t dp  = NULL;
    status = dds_participant_create ( &dp, DDS_DOMAIN_DEFAULT, NULL, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // 3. Create a subscriber  ------------------------------
    dds_entity_t subscriber = NULL;
    status = dds_subscriber_create ( dp, &subscriber, NULL, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // 4. Create waitset ------------------------------------
    dds_waitset_t ws = dds_waitset_create ();
    
    // 5. Init guard condition ------------------------------
    terminated_cond = dds_guardcondition_create (); 
    status = dds_waitset_attach ( ws, terminated_cond, terminated_cond );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // 6.1 Create light topic  ------------------------------
    dds_entity_t light_topic = NULL;
    status = dds_topic_create ( dp, &light_topic, &light_color_t_desc, 
                IniParser_GetString (parser, "light", "topic", "LIGHT"), NULL, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // 6.2 Create light qos
    dds_qos_t* light_qos = dds_qos_create ();
    dds_reliability_kind_t light_reliability = 
        STREQ (IniParser_GetString (parser, "light", "reliability", ""), "RELIABLE") ? 
            DDS_RELIABILITY_RELIABLE : DDS_RELIABILITY_BEST_EFFORT;
    dds_qset_reliability ( light_qos, light_reliability, DDS_SECS (10) );

    // 6.3 Create light reader with qos
    dds_entity_t light_reader = NULL;
    status = dds_reader_create ( subscriber, &light_reader, light_topic, light_qos, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_qos_delete (light_qos); // delete qos

    // 6.4 Create light read cond
    dds_condition_t light_cond = dds_readcondition_create ( light_reader, DDS_ANY_STATE ); 
    dds_status_set_enabled ( light_reader, DDS_DATA_AVAILABLE_STATUS );

    // 6.5 Attach light condition variable to waitset
    status = dds_waitset_attach ( ws, light_cond, light_reader);
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // 7.1 Create topic alarm  ------------------------------
    dds_entity_t alarm_topic = NULL;
    status = dds_topic_create ( dp, &alarm_topic, &alarm_msg_t_desc, 
                IniParser_GetString (parser, "alarm", "topic", "ALARM"), NULL, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // 7.2 Create alarm qos
    dds_qos_t* alarm_qos = dds_qos_create ();
    dds_reliability_kind_t alarm_reliability = 
        STREQ (IniParser_GetString (parser, "alarm", "reliability", ""), "RELIABLE") ? 
            DDS_RELIABILITY_RELIABLE : DDS_RELIABILITY_BEST_EFFORT;
    dds_qset_reliability ( alarm_qos, alarm_reliability, DDS_SECS (10) );

    // 7.3 Create alarm reader with qos
    dds_entity_t alarm_reader = NULL;
    status = dds_reader_create ( subscriber, &alarm_reader, alarm_topic, alarm_qos, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_qos_delete (alarm_qos); // delete qos

    // 7.4 Create alarm read cond
    dds_condition_t alarm_cond = dds_readcondition_create ( alarm_reader, DDS_ANY_STATE ); 
    dds_status_set_enabled ( alarm_reader, DDS_DATA_AVAILABLE_STATUS );

    // 7.5 Attach alarm condition variable to waitset
    status = dds_waitset_attach ( ws, alarm_cond, alarm_reader);
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // 8.1 Create topic warning  ----------------------------
    dds_entity_t warning_topic = NULL;
    status = dds_topic_create ( dp, &warning_topic, &warning_msg_t_desc, 
                IniParser_GetString (parser, "warning", "topic", "WARNING"), NULL, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // 8.2 Create warning qos
    dds_qos_t* warning_qos = dds_qos_create ();
    dds_reliability_kind_t warning_reliability = 
        STREQ (IniParser_GetString (parser, "warning", "reliability", ""), "RELIABLE") ? 
            DDS_RELIABILITY_RELIABLE : DDS_RELIABILITY_BEST_EFFORT;
    dds_qset_reliability ( warning_qos, warning_reliability, DDS_SECS (10) );

    // 8.3 Create warning reader with qos
    dds_entity_t warning_reader = NULL;
    status = dds_reader_create ( subscriber, &warning_reader, warning_topic, warning_qos, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_qos_delete (warning_qos); // delete qos

    // 8.4 Create warning read cond
    dds_condition_t warning_cond = dds_readcondition_create ( warning_reader, DDS_ANY_STATE ); 
    dds_status_set_enabled ( warning_reader, DDS_DATA_AVAILABLE_STATUS );

    // 8.5 Attach warning condition variable to waitset
    status = dds_waitset_attach ( ws, warning_cond, warning_reader);
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // 9. Waiting for writer appear -------------------------
    printf ("Waiting for writer...\n");
    size_t ws_size = 2u;
    dds_attach_t ws_results[2];
    status = dds_waitset_wait (ws, ws_results, ws_size, DDS_INFINITY ); // inf block
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // 10. Loop ----------------------------------------------
    uint32_t reader_status = 0;
    dds_time_t ws_timeout = DDS_SECS (20);
    while ( !dds_condition_triggered (terminated_cond) )
    {
        // blocking
        status = dds_waitset_wait ( ws, ws_results, ws_size, ws_timeout );
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
            compound_sample_t * cs_ptr = NULL;
            
            for ( uint16_t i = 0; i < ws_size; i++ )
            {
                if ( ws_results[i] != NULL )
                {
                    dds_condition_t cond = dds_statuscondition_get (ws_results[i]);
                    cs_ptr = (compound_sample_t*) calloc (1, sizeof(compound_sample_t));
                    status = dds_status_take ( ws_results[i], &reader_status, DDS_DATA_AVAILABLE_STATUS );

                    if (dds_statuscondition_get (light_reader) == cond) 
                    {
                        cs_ptr->type = LIGHT_T;
                    } 
                    else if (dds_statuscondition_get (alarm_reader) == cond)
                    {
                        cs_ptr->type = ALARM_T;
                    }
                    else if (dds_statuscondition_get (warning_reader) == cond)
                    {
                        cs_ptr->type = WARN_T;
                    }
                    else
                    {
                        free (cs_ptr);
                        cs_ptr = NULL;
                        continue;
                    }

                    cs_ptr->sample_count = dds_take ( ws_results[i], cs_ptr->samples_ptr, MAX_SAMPLES, cs_ptr->samples_info, 0 );

                    if (cs_ptr->sample_count > 0 )
                    {
                        threadpool_add ( pool, &thread_task, cs_ptr, 0 );
                    }
                    else // no sample
                    {
                        free (cs_ptr);
                        cs_ptr = NULL;
                    }
                }
            }
        }
    }

    printf ("Cleaning up...\n");

    // detach terminal guard cond
    status = dds_waitset_detach ( ws, terminated_cond );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_condition_delete (terminated_cond);

    // detach light read cond
    status = dds_waitset_detach ( ws, light_cond );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_condition_delete (light_cond);

    // detach alarm read cond
    status = dds_waitset_detach ( ws, alarm_cond );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_condition_delete (alarm_cond);

    // detach warning read cond
    status = dds_waitset_detach ( ws, warning_cond );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    dds_condition_delete (warning_cond);
    
    // delete waitset
    status = dds_waitset_delete (ws);
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // delete dds entities
    dds_entity_delete (dp);
    dds_fini ();

    // release all resources
    fini ();
    
    printf ("Finished.\n");
    exit (EXIT_SUCCESS);
}

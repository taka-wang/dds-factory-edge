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


#include "share.h"
#define TOPIC "ALARM"                   // DDS topic name

static volatile int do_loop = 1;        // global variables
static void sigint_handler (int sig)    // signal handler
{
    do_loop = 0;
}

static void do_job ( dds_entity_t * writer )
{
    // Prepare samples  ------------------------
    alarm_msg_t writer_msg;
    writer_msg.date = "2017/03/15";
    writer_msg.time = "17:00";
    writer_msg.major = "this is major";
    writer_msg.minor = "this is minor";
    writer_msg.msg   = "Hello world, today is another day";
    writer_msg.machine_id = rand() % 200;
    int status = dds_write ( *writer, &writer_msg );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    printf ("alarm: %d\n", writer_msg.machine_id);
    dds_sleepfor (DDS_MSECS(200));
}

int main (int argc, char *argv[])
{
    init (sigint_handler);
    dds_entity_t dp = NULL;
    init_dp (&dp);
    dds_entity_t writer = NULL;
    init_writer(&dp, &writer, &alarm_msg_t_desc, TOPIC);

    while ( do_loop ) 
    {
        do_job(&writer);
    }

    fini(&dp);
}

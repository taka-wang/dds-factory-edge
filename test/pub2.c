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
#define TOPIC "ALARM" // DDS topic name

static volatile int do_loop = 1;
static void sigint_handler (int sig) { do_loop = 0; }

static void emit ( dds_entity_t * writer )
{
    alarm_msg_t writer_msg;
    writer_msg.date = random_date();
    writer_msg.time = random_time();
    writer_msg.major = random_sentence();
    writer_msg.minor = random_sentence();
    writer_msg.msg   = "Hello world, today is another day";
    writer_msg.machine_id = random_number(200);
    
    int status = dds_write ( *writer, &writer_msg );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
    printf ("alarm: %d\n", writer_msg.machine_id);

    free(writer_msg.date);
    free(writer_msg.time);
    free(writer_msg.major);
    free(writer_msg.minor);
    
    dds_sleepfor (DDS_MSECS(200));
}

int main (int argc, char *argv[])
{
    init (sigint_handler);
    dds_entity_t dp = NULL;
    init_dp (&dp);
    dds_entity_t writer = NULL;
    init_writer (&dp, &writer, &alarm_msg_t_desc, TOPIC);

    while (do_loop) 
    {
        emit (&writer);
    }

    fini (&dp);
    exit (EXIT_SUCCESS);
}

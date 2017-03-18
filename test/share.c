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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

void init ( signal_handler handle )
{
    struct sigaction sat;
    sat.sa_handler = handle;
    sigemptyset (&sat.sa_mask);
    sat.sa_flags = 0;
    sigaction (SIGINT, &sat, NULL);
    
    // random seed
    srand ((unsigned int)time(NULL));
}

void init_dp ( dds_entity_t *dp )
{
    // Initialize DDS ------------------------
    int status = dds_init (0, NULL);
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create participant
    status = dds_participant_create ( dp, DDS_DOMAIN_DEFAULT, NULL, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create publisher
    dds_entity_t publisher = NULL;
    status = dds_publisher_create ( *dp, &publisher, NULL, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
}

void init_writer (  dds_entity_t * dp, 
                    dds_entity_t * writer, 
                    const dds_topic_descriptor_t * desc, 
                    const char * topic_name )
{
    // Create topic for writer
    dds_entity_t topic = NULL;
    int status = dds_topic_create ( *dp, &topic, desc, topic_name, NULL, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);

    // Create writer without Qos
    status = dds_writer_create ( *dp, writer, topic, NULL, NULL );
    DDS_ERR_CHECK (status, DDS_CHECK_REPORT | DDS_CHECK_EXIT);
}


void fini ( dds_entity_t *dp )
{
    printf ("fini\n");
    dds_entity_delete ( *dp );
    dds_fini ();
    exit (0);
}

const int sentence_length = 80;
const char * articles[] = { "the", "a", "one"};
const char * nouns[]    = { "boy", "girl", "dog", "cat", "bird" };
const char * verbs[]    = { "play", "listened", "see", "walked", "jump" };
const char * prepositions[] =  { "to", "from", "on", "under", "over" };
const int articles_count = sizeof(articles) / sizeof(articles[0]);
const int nouns_count = sizeof(nouns) / sizeof(nouns[0]);
const int verbs_count = sizeof(verbs) / sizeof(verbs[0]);
const int prepositions_count = sizeof(prepositions) / sizeof(prepositions[0]);

char * random_sentence(void) 
{
    char * sentence = calloc((sentence_length + 1), sizeof(char));
    strcat(sentence, articles[ rand() % articles_count ]);
    strcat(sentence, " ");
    strcat(sentence, nouns[ rand() % nouns_count ]);
    strcat(sentence, " ");
    strcat(sentence, verbs[ rand() % verbs_count ]);
    strcat(sentence, " ");
    strcat(sentence, prepositions[ rand() % prepositions_count ]);
    return sentence;
}

int random_number(int max)
{
    return rand() % max;
}

const int date_length = 25;
char * random_date(void)
{
    char * buf = (char *)calloc((date_length), sizeof(char));
    time_t epoch = (rand() % 90000000) + 1400000000;
    struct tm * timeinfo = localtime(&epoch);
    strftime(buf, date_length, "%Y/%m/%d", timeinfo);
    return buf;
}

const int time_length = 25;
char * random_time(void)
{
    char * buf = (char *)calloc((time_length), sizeof(char));
    time_t epoch = (rand() % 90000000) + 1400000000;
    struct tm * timeinfo = localtime(&epoch);
    strftime(buf, time_length, "%H:%M:%S", timeinfo);
    return buf;
}
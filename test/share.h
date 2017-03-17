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


#pragma once

#include "factory.h"

typedef void (*signal_handler)(int);

// change signal disposition
void init ( signal_handler handle );

// create domain participant and publisher
void init_dp ( dds_entity_t *dp );

// create typed writer
void init_writer ( dds_entity_t * dp, 
                   dds_entity_t * writer, 
                   const dds_topic_descriptor_t * desc, 
                   const char * topic_name );

// release all participants
void fini ( dds_entity_t *dp );

char * random_sentence(void);
int random_number(int max);
char * random_date(void);
char * random_time(void);

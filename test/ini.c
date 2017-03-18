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


#include <stdio.h>
#include "iniparser.h"

int main(void)
{
    parser = IniParser_Create ("config.ini");
    printf ("Is error: %" PRId16 "\n", IniParser_CheckInstanceError(parser));

    const char * v01 = IniParser_GetString (parser, "db", "url", "");
    printf ("db-url: %s\n", v01);

    const char * v02 = IniParser_GetString (parser, "db", "port", "");
    printf ("db-port: %s\n", v02);

    const char * v03 = IniParser_GetString (parser, "db", "user", "");
    printf ("db-user: %s\n", v03);

    const char * v04 = IniParser_GetString (parser, "db", "password", "");
    printf ("db-password: %s\n", v04);

    const char * v05 = IniParser_GetString (parser, "db", "name", "");
    printf ("db-name: %s\n", v05);

    int32_t v06 = IniParser_GetInteger (parser, "basic", "max_sample", 0);
    printf ("basic-max_sample: %" PRId32 "\n", v06);

    int32_t v07 = IniParser_GetInteger (parser, "basic", "max_thread", 0);
    printf ("basic-max_thread: %" PRId32 "\n", v07);

    int32_t v08 = IniParser_GetInteger (parser, "basic", "max_queue", 0);
    printf ("basic-max_queue: %" PRId32 "\n", v08);

    const char * v09 = IniParser_GetString (parser, "light", "prefix", "");
    printf ("light-prefix: %s\n", v09);
    const char * v10 = IniParser_GetString (parser, "light", "topic", "");
    printf ("light-topic: %s\n", v10);
    const char * v11 = IniParser_GetString (parser, "light", "reliability", "");
    printf ("light-reliability: %s\n", v11);


    const char * v12 = IniParser_GetString (parser, "alarm", "prefix", "");
    printf ("alarm-prefix: %s\n", v12);
    const char * v13 = IniParser_GetString (parser, "alarm", "topic", "");
    printf ("alarm-topic: %s\n", v13);
    const char * v14 = IniParser_GetString (parser, "alarm", "reliability", "");
    printf ("alarm-reliability: %s\n", v14);

    const char * v15 = IniParser_GetString (parser, "warning", "prefix", "");
    printf ("light-prefix: %s\n", v15);
    const char * v16 = IniParser_GetString (parser, "warning", "topic", "");
    printf ("light-topic: %s\n", v16);
    const char * v17 = IniParser_GetString (parser, "warning", "reliability", "");
    printf ("light-reliability: %s\n", v17);

    const char * v18 = IniParser_GetString (parser, "machine_status", "prefix", "");
    printf ("light-prefix: %s\n", v18);
    const char * v19 = IniParser_GetString (parser, "machine_status", "topic", "");
    printf ("light-topic: %s\n", v19);
    const char * v20 = IniParser_GetString (parser, "machine_status", "reliability", "");
    printf ("light-reliability: %s\n", v20);

    return 0;
}

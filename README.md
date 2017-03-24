# DDS-Factory-Edge

[![Build Status](https://travis-ci.org/taka-wang/dds-factory-edge.svg?branch=master)](https://travis-ci.org/taka-wang/dds-factory-edge)

DDS subscriber for Edge server based on Vortex Lite.

## // Continuous Integration

Please check the [log](https://travis-ci.org/taka-wang/dds-factory-edge) and [script](.travis.yml) to know how to build this software.

## // Build from scratch

```bash
# Install dev-deps
sudo apt-get update
sudo apt-get install -y git pkg-config automake cmake libtool re2c libmysqlclient-dev mysql-client-core-5.6

# Create database (assume local mysql server with empty password)
mysql -e 'CREATE DATABASE IF NOT EXISTS factory;'

# Setup repo
git clone https://github.com/taka-wang/dds-factory-edge.git
cd dds-factory-edge
git submodule update --init --recursive

# Install vortex lite
yes 'y' | prismtech/P704-VortexLite-2.0.4-Ubuntu1404-x86_64-installer.run
sudo mv y /opt/Prismtech
sudo ln -s /opt/Prismtech/Vortex_v2/Device/VortexLite/2.0.4/lib/linux_gcc_x86/libdds*.* /usr/lib/

# Install libzdb
cd src/libzdb
autoreconf --force --install
./configure --enable-optimized --without-postgresql --without-sqlite --prefix=/usr
make && sudo make install
cd ../..

# Build repo
mkdir build && cd build
cmake .. && make && sudo make install

# Run test
./sub2 &
./pub1 20
./pub2 20
./pub3 20
```

## // Configuration

```ini
; Config sample

[db]
url         = localhost     ; mysql url
port        = 3306          ; mysql port
user        = root          ; mysql account
password    =               ; mysql password
name        = factory       ; db name

[basic]
max_sample  = 200           ; max DDS sample per take
max_thread  = 32            ; max thread
max_queue   = 128           ; max queue

; DDS settings

[light]
table_name  = light         ; table name
topic       = LIGHT         ; dds topic
reliability = RELIABLE

[alarm]
table_name  = alarm         ; table name
topic       = ALARM         ; dds topic
reliability = RELIABLE

[warning]
table_name  = warning       ; table name
topic       = WARNING       ; dds topic
reliability = RELIABLE

[machine_status]
topic       = OT            ; dds topic
reliability = BEST_EFFORT

```

## // DDS IDL

```c
/**
 * @brief Light color enum type for #light_color_t.
 *
 */
enum light_color {
    BLUE,                       /**< enum value BLUE.                       */
    YELLOW,                     /**< enum value YELLOW.                     */
    GREEN,                      /**< enum value GREEN.                      */
    RED,                        /**< enum value RED.                        */
    UNKNOW                      /**< enum value UNKNOW.                     */
};

/**
 * @brief Tower light topic type.
 *
 */
struct light_color_t {
    unsigned short machine_id;  /**< machine id.                            */
    string date;                /**< date string.                           */
    string time;                /**< time string.                           */
    light_color color;          /**< light color.                           */
};
#pragma keylist light_color_t machine_id


/**
 * @brief Warning message topic type.
 *
 */
struct warning_msg_t {
    unsigned short machine_id;  /**< machine id.                            */
    string date;                /**< date string.                           */
    string time;                /**< time string.                           */    
    string msg_num;             /**< warning number                         */
    string msg;                 /**< warning message                        */
};
#pragma keylist warning_msg_t machine_id


/**
 * @brief Alarm message topic type.
 *
 */
struct alarm_msg_t {
    unsigned short machine_id;  /**< machine id.                            */
    string date;                /**< date string.                           */
    string time;                /**< time string.                           */
    string major;               /**< major number                           */
    string minor;               /**< minor number                           */
    string msg;                 /**< alarm message                          */
};
#pragma keylist alarm_msg_t machine_id


/**
 * @brief OT status topic type.
 *
 */
struct machine_status_t {
    unsigned short machine_id;  /**< machine id.                            */
    string date;                /**< date string.                           */
    string time;                /**< time string.                           */
    string values;              /**< machine status in csv string.          */
};
#pragma keylist machine_status_t machine_id

/**
 * @brief Database table schema type.
 *
 * A keyless topic.
 */
struct table_schema_t {
    unsigned short machine_id;  /**< machine id.                            */
    string value;               /**< specific table schema (json string).   */
};
#pragma keylist table_schema_t  /**< keyless.                               */
```

## // Table Schema

### Light table

```
+-------+-------------+------+-----+---------+-------+
| Field | Type        | Null | Key | Default | Extra |
+-------+-------------+------+-----+---------+-------+
| id    | int(11)     | NO   |     | NULL    |       |
| date  | varchar(20) | YES  |     | NULL    |       |
| time  | varchar(20) | YES  |     | NULL    |       |
| color | int(11)     | YES  |     | NULL    |       |
+-------+-------------+------+-----+---------+-------+
```

### Alarm messages table
```
+-------+--------------+------+-----+---------+-------+
| Field | Type         | Null | Key | Default | Extra |
+-------+--------------+------+-----+---------+-------+
| id    | int(11)      | NO   |     | NULL    |       |
| date  | varchar(20)  | YES  |     | NULL    |       |
| time  | varchar(20)  | YES  |     | NULL    |       |
| major | varchar(200) | YES  |     | NULL    |       |
| minor | varchar(200) | YES  |     | NULL    |       |
| msg   | varchar(200) | YES  |     | NULL    |       |
+-------+--------------+------+-----+---------+-------+
```

### Warning messages table

```
+---------+--------------+------+-----+---------+-------+
| Field   | Type         | Null | Key | Default | Extra |
+---------+--------------+------+-----+---------+-------+
| id      | int(11)      | NO   |     | NULL    |       |
| date    | varchar(20)  | YES  |     | NULL    |       |
| time    | varchar(20)  | YES  |     | NULL    |       |
| msg_num | varchar(200) | YES  |     | NULL    |       |
| msg     | varchar(200) | YES  |     | NULL    |       |
+---------+--------------+------+-----+---------+-------+
```

## // Dependencies

- [thread pool](https://github.com/mbrossard/threadpool): A simple C Thread pool implementation.
- [c-ini-parser](https://github.com/taka-wang/c-ini-parser): An simple INI parser based on inih in OOP style C99.
- [libzdb](https://github.com/taka-wang/libzdb): A thread-safe multi database connection pool library.
- [Vortex Lite](http://www.prismtech.com/vortex/software-downloads): A DDS library.

## // License

Copyright 2017 - Taka Wang.

Software licensed under the GPL version 2 available in GPLv3.TXT and online on [http://www.gnu.org/licenses/gpl.txt](http://www.gnu.org/licenses/gpl.txt).

Use parts from other developers, sometimes with small changes, references on autorship and specific licenses are on individual source files.
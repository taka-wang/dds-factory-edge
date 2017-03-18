# DDS-Factory-Edge

[![Build Status](https://travis-ci.org/taka-wang/dds-factory-edge.svg?branch=master)](https://travis-ci.org/taka-wang/dds-factory-edge)

DDS subscriber for Edge server

## Continuous Integration

Please check the [build log](https://travis-ci.org/taka-wang/dds-factory-edge) and [travis config](.travis.yml).

## DDS IDL

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

## License

Copyright 2017 - Taka Wang.

Software licensed under the GPL version 2 available in GPLv3.TXT and online on [http://www.gnu.org/licenses/gpl.txt](http://www.gnu.org/licenses/gpl.txt).

Use parts from other developers, sometimes with small changes, references on autorship and specific licenses are on individual source files.
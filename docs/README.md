# Design of DDS Topic & Database

## 名詞解釋

- DDS topic: 程式語言中的 class 或 struct，對應物件導向術語中的 class。
- sample: 不同數值的實體 instance，對應物件導向術語中的 instance。
- table schema topic: 五種 topic 中的一種，用來定義 machine status 的 "values" 欄位如何解讀與儲存， table schema topic 中的 "value" 欄位用 JSON string 表達 machine status 對應的 database table 的結構，此 JSON string 的撰寫需要遵守 JSON schema 規範。
- JSON schema: 為了確保 table schema topic 中的 JSON string "value" 格式，此 JSON schema 當作 metadata 用。此 JSON schema 很單純的定義一個 "table name", 一個 table "column array"，每個 element 指定 "欄位名稱"、"資料型態" 跟 "可省略的字串長度"，僅此而以。
- values string: machine status 對應的 sample，其中 "values" 這個 string 欄位，需要符合對應的 table schema sample 中 JSON string "value" 所描述的資料模型，用逗點字串 (CSV) 表達 (by design)。

Ambiguous concept： JSON schema 只是溝通語言，是 database table schema 的 metadata，metadata 本身不能改變 (by design)。但 table schema topic 中的 JSON string "value"，可以根據前端使用者介面設定動態數量的資料欄位，例如 50 個 OT status 或 200 個 OT status，只要按照 JSON schema 方式去描述，撰寫 table schema topic 中的 JSON string "value"，就可以定義出來。

---

## Assumptions & Constraints

- Without loss of generality, we assume that there are 200 machine, 5 DDS topics each, respectively:

    1. tower light
    2. warning message 
    3. alarm message
    4. machine operation status
    5. table schema

- tower light topic 中的顏色可能會增加
- warning, alarm, status topic 中的欄位可能會增加 (除定義在 status topic value 中的擴展不用修改程式，其他都需要修改收發端)
- 支援欄位格式包括 `integer`, `float`, `string` 三種 (by design)，其中 `string` 類型必須指定最大長度 (若不指定，預設長度 100)

---


## Work Flow

For simplicity's sake，we call publisher as "client" and subscriber as "server".

-  每一台 client 必須先傳遞 "machine status" 的 table schema topic sample 給 server，server 根據 sample 中的 JSON string "value"，建立 "machine status" 對應的 database table (若不存在)，不同 machine 有自己獨立的 database table。因此每台 machine 一開始要傳一筆 schema sample 給 server，之後 table schema topic 就不會再用到。
-  machine status 的 sample，必須晚於 sample of schema，此時 server 才能根據 table schema 處理此 topic。
-  tower light, warning message 與 alarm message 的 DDS topic 結構是寫死的，因此 server 端在初始化時就會建立對應的 database table，client 端可以直接傳送 sample instance。

---

## DDS Topics

- topic 中的 machine id 對應每個 machine 的是 unique 的。其中只有 table schema 這個 topic 是 keyless 的，其他四個 topic 都是 keyed (machine id)，因此所有的 machine 對於此 table schema topic 共用同一個 DDS queue (否則就是 200 個 queue，而 queue 中有意義的 sample 只有一筆)
- 以 200 台 machine 計算，總共只有 200 筆 table schema sample，所以 table schema topic 的 value string 選用 JSON 表達，並不會造成效能上的負擔。

---

## IDL Example: Light Color Topic

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
```

---

## IDL Example: Warning/Alarm Message Topics

```c
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
```
---

## IDL Example: Machine Status/Table Schema Topics 

```c
/**
 * @brief OT status topic type.
 *
 */
struct machine_status_t {
    unsigned short machine_id;  /**< machine id.                            */
    string date;                /**< date string.                           */
    string time;                /**< time string.                           */
    string values;              /**< machine status in CSV string.          */
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

---

## JSON schema for table schema topic's "value" field

JSON string 大部分情況下，本身是不定義 schema。但為了建立共同語言，確保 table schema topic 內的 JSON string "value" 有一致的格式，我們採用 [json-schema.org](http://json-schema.org/) 的 [draft-04](https://tools.ietf.org/html/draft-zyp-json-schema-04) 當成溝通工具。

JSON schema 主要定義兩個 properties:

1. **table_name**: desired table name string (server will append machine id to the table name)
2. **schema**: JSON array (item 的順序是有意義的), 每個 item 包括
    - name: filed name string
    - type: type string (integer, float, string)
    - length: string lengh (integer, float 可省略， string 若無指定，預設長度 100)

---

## Partial JSON schema

這裡僅節錄 properties 中的 schema array 部分，這是 client/server 對於 table schema topic 中 JSON string "value" 的共識，請把它想成 metadata。注意到 required 中並沒有 length，所以 length 是 **optional** 的 (預設長度 100)。

```js
"schema": {
  "type": "array",
  "items": {
    "type": "object",
    "properties": {
      "name": {
        "type": "string"
      },
      "type": {
        "type": "string"
      },
      "length": {
        "type": "integer"
      }
    },
    "required": [
      "name",
      "type"
    ]
  }
```

---


## Example: Table schema topic's value field

Machine status 對應的 table schema topic 的 JSON string "value" 範例如下，schema array 的 item 都是可以增加刪減的：

```js
{
    "table": "machine_status",
    "schema":
    [
        {
            "name": "Operation Time",
            "type": "string",
            "length": 20
        },
        {
            "name": "Successful pickup rate",
            "type": "float"
        },
        {
            "name": "Cycle Time",
            "type": "integer"
        }
    ]
}
```

---

## Example: SQL command to create 'machine status' table

接續上頁，若 machine id 為 20，則 table name 為 `machine_status_20`，對應的 SQL command 為：

```sql
CREATE TABLE `machine_status_20` 
(
	id INT NOT NULL, 
	date VARCHAR(20) NOT NULL,
	time VARCHAR(20) NOT NULL,
	`Operation Time` VARCHAR(20),
	`Successful pickup rate` FLOAT,
	`Cycle Time` INT
);
```

---

## Example: Machine status sample

接續上上頁，每一筆 machine status 中的 values 為三個值的 CSV string (根據 table schema topic 裡的定義)，例如 `15H33M16S,0.98,1`，因此 client 端可以這樣建立 sample，以下以 `C語言` 為範例：

```c
machine_status_t mcn_msg;                // DDS topic struct
mcn_msg.machine_id = 20;                 // machine ID
mcn_msg.date       = "18/11/2016";       // machine 日期
mcn_msg.time       = "15:34:36";         // machine 時間
mcn_msg.values     = "15H33M16S,0.98,1"; // CSV string comform to schema
```

其中 "15H33M16S" 為 Operation Time, 1 為 Cycle Time，以此類推。

---

## Example: Alarm status sample

以下以 `C語言` 為範例：

```c
alarm_msg_t alarm_msg;                // DDS topic struct
alarm_msg.machine_id = 8;             // machine ID
alarm_msg.date       = "18/11/2016";  // machine 日期
alarm_msg.time       = "15:34:36";    // machine 時間
alarm_msg.major      = "5930001E";    // alarm major code
alarm_msg.minor      = "1005";        // alarm minor code
alarm_msg.msg        = "A failure occurred during XY-axis operation.";
```

---

## Take aways

- 我們已經預先定義 table schema topic 中 JSON string "value" 的 JSON schema，開發者與用戶只需要遵守。
- 有五種 DDS topic, 其中 table schema topic 每個 machine 要最先送出一次，讓 server 端建立 machine status 的 database table (if not exist)。
- client 可以透過 table schema topic 的 value string 定義 "table名稱" 與 "欄位名稱"、"資料類型"、 "資料長度"，此 JSON string "value" 必須符合我們預先定義的 JSON schema 格式，方便 client/server 間溝通。

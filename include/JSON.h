#pragma once

#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <errno.h>

#include "HashMap.h"
#include "Vector.h"

#define JSON_INITIAL_ITEM_COUNT 16
#define JSON_ARRAY_INITIAL_ITEM_COUNT 8

typedef enum JSONStatus {
    JSON_OK,
    JSON_ERROR_EMPTY_TEXT,
    JSON_ERROR_UNCLOSED_COMMENT,
    JSON_ERROR_UNTERMINATED_STRING,
    JSON_ERROR_MISSING_VALUE,
    JSON_ERROR_MISSING_START_PARENTHESIS,
    JSON_ERROR_MISSING_END_PARENTHESIS,
    JSON_ERROR_MISSING_KEY_VALUE_SEPARATOR,
    JSON_ERROR_WRONG_VALUE_END,
    JSON_ERROR_WRONG_KEY_START
} JSONStatus;

typedef enum JSONType {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_TEXT,
    JSON_BOOLEAN,
    JSON_INTEGER,
    JSON_LONG,
    JSON_DOUBLE,
    JSON_NULL
} JSONType;

typedef struct JSONValue {
    JSONType type;
    void *value;
} JSONValue;

typedef struct JSONTokener {
    char *jsonBufferPointer;
    char *jsonStringEnd;
    uint32_t jsonStringLength;
    JSONStatus jsonStatus;
} JSONTokener;

typedef struct JSONObject {
    JSONTokener *jsonTokener;
    HashMap jsonMap;
} JSONObject;

typedef struct JSONArray {
    JSONTokener *jsonTokener;
    Vector jsonVector;
} JSONArray;


// JSON parse
JSONTokener getJSONTokener(char *jsonString, uint32_t stringLength);
JSONObject jsonObjectParse(JSONTokener *jsonTokener);
JSONArray jsonArrayParse(JSONTokener *jsonTokener);

// JSON Object Get
JSONValue *getJsonObjectValue(JSONObject *jsonObject, JSONType type, const char *key);
bool getJsonObjectBoolean(JSONObject *jsonObject, const char *key);
double getJsonObjectDouble(JSONObject *jsonObject, const char *key);
int32_t getJsonObjectInt(JSONObject *jsonObject, const char *key);
int64_t getJsonObjectLong(JSONObject *jsonObject, const char *key);
char *getJsonObjectString(JSONObject *jsonObject, const char *key);

JSONArray getJSONArrayFromObject(JSONObject *jsonObject, const char *key);    // Returns: A JSONArray which is the value.
JSONObject getJSONObjectFromObject(JSONObject *jsonObject, const char *key);  // Returns: A JSONObject which is the value.

bool jsonObjectHasKey(JSONObject *jsonObject, const char *key); // Returns: true if the key exists in the JSONObject.
bool isJsonObjectValueNull(JSONObject *jsonObject, const char *key);  // Returns: true if there is no value associated with the key or if the value is the "null".
uint32_t getJsonObjectLength(JSONObject *jsonObject);               // Returns: The number of keys in the JSONObject.

// JSON Object Get Optional
bool getJsonObjectOptBoolean(JSONObject *jsonObject, const char *key, bool defaultValue);// Get an optional boolean associated with a key. It returns the defaultValue if there is no such key, or if it is not a Boolean or the String "true" or "false" (case sensitive).
double getJsonObjectOptDouble(JSONObject *jsonObject, const char *key, double defaultValue);// Get an optional double associated with a key, or the defaultValue if there is no such key or if its value is not a number.
int32_t getJsonObjectOptInt(JSONObject *jsonObject, const char *key, int32_t defaultValue);// Get an optional int value associated with a key, or the default if there is no such key or if the value is not a number.
int64_t getJsonObjectOptLong(JSONObject *jsonObject, const char *key, int64_t defaultValue);
char *getJsonObjectOptString(JSONObject *jsonObject, const char *key, char *defaultValue);// Get an optional string associated with a key. It returns the defaultValue if there is no such key.

// JSON Array Get
JSONValue *getJsonArrayValue(JSONArray *jsonArray, JSONType type, uint32_t index);
bool getJsonArrayBoolean(JSONArray *jsonArray, uint32_t index);
double getJsonArrayDouble(JSONArray *jsonArray, uint32_t index);
int32_t getJsonArrayInt(JSONArray *jsonArray, uint32_t index);
int64_t getJsonArrayLong(JSONArray *jsonArray, uint32_t index);
char *getJsonArrayString(JSONArray *jsonArray, uint32_t index);

JSONArray getJSONArrayFromArray(JSONArray *jsonArray, uint32_t index);
JSONObject getJSONObjectFromArray(JSONArray *jsonArray, uint32_t index);

bool isJsonArrayValueNull(JSONArray *jsonArray, uint32_t index); // Returns: true if the value at the index is null, or if there is no value.
uint32_t getJsonArrayLength(JSONArray *jsonArray);

// JSON Array Get Optional
bool getJsonArrayOptBoolean(JSONArray *jsonArray, uint32_t index, bool defaultValue);
double getJsonArrayOptDouble(JSONArray *jsonArray, uint32_t index, double defaultValue);
int32_t getJsonArrayOptInt(JSONArray *jsonArray, uint32_t index, int32_t defaultValue);
int64_t getJsonArrayOptLong(JSONArray *jsonArray, uint32_t index, int64_t defaultValue);
char *getJsonArrayOptString(JSONArray *jsonArray, uint32_t index, char *defaultValue);

// JSON Object Create/Update
JSONTokener createEmptyJSONTokener();
JSONObject createJsonObject(JSONTokener *jsonTokener);
void jsonObjectPut(JSONObject *jsonObject, const char *key, char *value); // Put a key/value pair in the JSONObject.
void jsonObjectRemove(JSONObject *jsonObject, const char *key); // Remove a name and its value, if present.
void jsonObjectAddObject(JSONObject *jsonObject, const char *key, JSONObject *innerObject);
void jsonObjectAddArray(JSONObject *jsonObject, const char *key, JSONArray *innerArray);

// JSON Array Create/Update
JSONArray createJsonArray(JSONTokener *jsonTokener);
void jsonArrayPut(JSONArray *jsonArray, char *value);
void jsonArrayRemove(JSONArray *jsonArray, uint32_t index);
void jsonArrayPutAll(JSONArray *destination, JSONArray *source);
void jsonArrayAddArray(JSONArray *jsonArray, JSONArray *innerArray);
void jsonArrayAddObject(JSONArray *jsonArray, JSONObject *innerObject);

// JSON to String
/*
* Make a pretty printed JSON text of this JSONObject.
* Params: indentFactor – The number of spaces to add to each level of indentation.
 *        topLevelIndent – The indentation of the top level.
* Returns: a printable, displayable, transmittable representation of the object, beginning with '{'(left brace) and ending with '}'(right brace).
*/
void jsonObjectToStringPretty(JSONObject *jsonObject, char *resultBuffer, uint32_t bufferSize, uint16_t indentFactor, uint16_t topLevelIndent);
void jsonArrayToStringPretty(JSONArray *jsonArray, char *resultBuffer, uint32_t bufferSize, uint16_t indentFactor, uint16_t topLevelIndent);

// Make an JSON text of this JSONObject. For compactness, no whitespace and new line is added.
void jsonObjectToString(JSONObject *jsonObject, char *resultBuffer, uint32_t bufferSize);
void jsonArrayToString(JSONArray *jsonArray, char *resultBuffer, uint32_t bufferSize);

// JSON Delete
void deleteJSONObject(JSONObject *jsonObject);
void deleteJSONArray(JSONArray *jsonArray);

// Helper methods
static inline bool isJsonObjectOk(JSONObject *jsonObject) {
    return jsonObject->jsonTokener->jsonStatus == JSON_OK;
}

static inline bool isJsonArrayOk(JSONArray *jsonArray) {
    return jsonArray->jsonTokener->jsonStatus == JSON_OK;
}
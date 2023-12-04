#include "JSON.h"

#define JSON_TEXT_MIN_LENGTH 2  // empty json -> {}

#define JSON_NULL_CHAR         '\0'
#define JSON_DOUBLE_QUOTE_CHAR '"'

#define JSON_COMMENT_START              '/'
#define JSON_SINGLE_LINE_COMMENT_START  '/'
#define JSON_MULTI_LINE_COMMENT_START   '*'
#define JSON_COMMENT_END                '/'
#define JSON_HASH_CHAR                  '#'

#define JSON_KEY_BEGIN_CHAR              '"'
#define JSON_KEY_VALUE_DELIMITER_CHAR    ':'
#define JSON_VALUE_BEGIN_CHAR            '"'
#define JSON_OBJECT_BEGIN_CHAR           '{'
#define JSON_OBJECT_END_CHAR             '}'
#define JSON_ARRAY_BEGIN_CHAR            '['
#define JSON_ARRAY_END_CHAR              ']'
#define JSON_NEXT_VALUE_COMMA_CHAR       ','
#define JSON_NEXT_VALUE_SEMICOLON_CHAR   ';'

typedef struct InnerJsonBuffer {
    char *buffer;
    uint32_t length;
    uint32_t capacity;
} InnerJsonBuffer;

static char nextJsonChar(JSONTokener *jsonTokener);
static void backJsonChar(JSONTokener *jsonTokener);
static char nextCleanJsonChar(JSONTokener *jsonTokener);

static char *nextJsonKey(JSONTokener *jsonTokener);
static JSONValue *nextJsonValue(JSONTokener *jsonTokener);
static char *nextJsonString(JSONTokener *jsonTokener);

static void skipJsonChars(JSONTokener *jsonTokener);
static void skipJsonMultiLineCommentChars(JSONTokener *jsonTokener);
static JSONValue *handleUnquotedText(JSONTokener *jsonTokener);
static inline bool isNotEscapedCharArray(char jsonChar);
static JSONType detectJsonValueType(char *jsonTextValue, uint32_t valueLength);
static JSONValue *getValueInstance(JSONTokener *jsonTokener, JSONType type, void *value);

static void jsonHashMapToString(HashMap jsonMap, InnerJsonBuffer *jsonBuffer, uint16_t indentFactor, uint16_t topLevelIndent);
static void jsonVectorToString(Vector jsonVector, InnerJsonBuffer *jsonBuffer, uint16_t indentFactor, uint16_t topLevelIndent);
static void jsonHashMapToStringCompact(HashMap jsonMap, InnerJsonBuffer *jsonBuffer);
static void jsonVectorToStringCompact(Vector jsonVector, InnerJsonBuffer *jsonBuffer);
static void quoteJsonString(InnerJsonBuffer *jsonBuffer, const char *value);
static void appendJsonValue(InnerJsonBuffer *jsonBuffer, JSONValue *jsonValue, uint16_t indentFactor, uint16_t topLevelIndent);
static void appendJsonValueCompact(InnerJsonBuffer *jsonBuffer, JSONValue *jsonValue);
static void jsonBufferCatStr(InnerJsonBuffer *jsonBuffer, const char *str);

static void deleteJsonObject(HashMap jsonObjectMap);
static void deleteJsonArray(Vector jsonVector);

static inline bool hasMoreJsonChars(JSONTokener *jsonTokener) {
    return (jsonTokener->jsonStringEnd - jsonTokener->jsonBufferPointer) < jsonTokener->jsonStringLength;
}

static inline void terminateJsonString(JSONTokener *jsonTokener) {
    *jsonTokener->jsonStringEnd = JSON_NULL_CHAR;
}

static inline bool isValidParsedLength(const char *valuePointer, const char *endPointer, uint32_t valueLength) {
    return endPointer == NULL || *endPointer == '\0' || endPointer - valuePointer == valueLength;
}

static inline bool isJsonIntegerValid(int32_t number,
                                      const char *valuePointer,
                                      const char *endPointer,
                                      uint32_t expectedValueLength) {
    return (valuePointer == endPointer) ||              // no digits found
           (errno == ERANGE && number == LONG_MIN) ||   // underflow occurred
           (errno == ERANGE && number == LONG_MAX) ||   // overflow occurred
           !isValidParsedLength(valuePointer, endPointer, expectedValueLength)
           ? false : true;
}

static inline bool isJsonLongValid(int64_t number,
                                   const char *valuePointer,
                                   const char *endPointer,
                                   uint32_t expectedValueLength) {
    return (valuePointer == endPointer) ||               // no digits found
           (errno == ERANGE && number == LLONG_MIN) ||   // underflow occurred
           (errno == ERANGE && number == LLONG_MAX) ||   // overflow occurred
           !isValidParsedLength(valuePointer, endPointer, expectedValueLength)
           ? false : true;
}

static inline bool isJsonDoubleNumberValid(double number,
                                           const char *valuePointer,
                                           const char *endPointer,
                                           uint32_t expectedValueLength) {
    return (valuePointer == endPointer) ||             // no digits found
           (errno == ERANGE && number == DBL_MIN) ||   // underflow occurred
           (errno == ERANGE && number == DBL_MAX) ||   // overflow occurred
           !isValidParsedLength(valuePointer, endPointer, expectedValueLength)
           ? false : true;
}


JSONTokener getJSONTokener(char *jsonString, uint32_t stringLength) {
    JSONTokener jsonTokener = {
            .jsonBufferPointer = jsonString,
            .jsonStringEnd = jsonString,
            .jsonStringLength = stringLength,
            .jsonStatus = JSON_OK};
    return jsonTokener;
}

JSONObject jsonObjectParse(JSONTokener *jsonTokener) {
    JSONObject jsonObject = {.jsonMap = NULL, .jsonTokener = jsonTokener};

    if (jsonTokener == NULL || jsonTokener->jsonBufferPointer == NULL) {
        return jsonObject;
    }

    if (jsonTokener->jsonStringLength < JSON_TEXT_MIN_LENGTH) {
        jsonTokener->jsonStatus = JSON_ERROR_EMPTY_TEXT;
        return jsonObject;
    }

    if (nextCleanJsonChar(jsonTokener) != JSON_OBJECT_BEGIN_CHAR) {
        jsonTokener->jsonStatus = JSON_ERROR_MISSING_START_PARENTHESIS;
        return jsonObject;
    }

    if (jsonTokener->jsonStatus != JSON_OK) {
        return jsonObject;
    }

    jsonObject.jsonTokener = jsonTokener;
    jsonObject.jsonMap = getHashMapInstance(JSON_INITIAL_ITEM_COUNT);
    nextJsonChar(jsonTokener);  // skip json start '{'

    while (true) {
        char jsonChar = nextCleanJsonChar(jsonTokener);

        if (jsonTokener->jsonStatus != JSON_OK) {
            return jsonObject;

        } else if (jsonChar == JSON_NULL_CHAR) {
            jsonTokener->jsonStatus = JSON_ERROR_MISSING_END_PARENTHESIS;
            deleteJsonObject(jsonObject.jsonMap);
            return jsonObject;

        } else if (jsonChar == JSON_OBJECT_END_CHAR) {
            nextJsonChar(jsonTokener);
            return jsonObject;
        }

        char const *jsonKey = nextJsonKey(jsonTokener);
        if (jsonTokener->jsonStatus != JSON_OK) {
            deleteJsonObject(jsonObject.jsonMap);
            return jsonObject;
        }

        JSONValue *jsonValue = nextJsonValue(jsonTokener);
        if (jsonTokener->jsonStatus != JSON_OK) {
            deleteJsonObject(jsonObject.jsonMap);
            return jsonObject;
        }

        hashMapPut(jsonObject.jsonMap, jsonKey, jsonValue);

        jsonChar = nextCleanJsonChar(jsonTokener);
        if (jsonChar == JSON_NEXT_VALUE_SEMICOLON_CHAR || jsonChar == JSON_NEXT_VALUE_COMMA_CHAR) {
            terminateJsonString(jsonTokener);   // remove value separator (',' or ';')
            nextJsonChar(jsonTokener);
            if (nextCleanJsonChar(jsonTokener) == JSON_OBJECT_END_CHAR) {
                return jsonObject;
            }

        } else if (jsonChar == JSON_OBJECT_END_CHAR) {
            terminateJsonString(jsonTokener);   // remove object end char for unquoted values. Example: {key:100} -> 100\0
            nextJsonChar(jsonTokener);
            return jsonObject;

        } else {
            jsonTokener->jsonStatus = JSON_ERROR_WRONG_VALUE_END;
            deleteJsonObject(jsonObject.jsonMap);
            return jsonObject;
        }
    }
}

JSONArray jsonArrayParse(JSONTokener *jsonTokener) {
    JSONArray jsonArray = {.jsonTokener = jsonTokener, .jsonVector = NULL};

    char jsonChar = nextCleanJsonChar(jsonTokener);
    if (jsonChar != JSON_ARRAY_BEGIN_CHAR) {
        jsonTokener->jsonStatus = JSON_ERROR_MISSING_START_PARENTHESIS;
        return jsonArray;
    }

    nextJsonChar(jsonTokener);
    jsonChar = nextCleanJsonChar(jsonTokener);
    if (jsonChar == JSON_ARRAY_END_CHAR) {
        nextJsonChar(jsonTokener);
        return jsonArray;
    }
    jsonArray.jsonVector = getVectorInstance(JSON_ARRAY_INITIAL_ITEM_COUNT);

    while (true) {
        JSONValue *jsonValue = (jsonChar == JSON_NEXT_VALUE_COMMA_CHAR) ? NULL : nextJsonValue(jsonTokener);
        if (jsonTokener->jsonStatus != JSON_OK) {
            deleteJsonArray(jsonArray.jsonVector);
            return jsonArray;
        }
        vectorAdd(jsonArray.jsonVector, jsonValue);

        jsonChar = nextCleanJsonChar(jsonTokener);
        if (jsonChar == JSON_NEXT_VALUE_SEMICOLON_CHAR || jsonChar == JSON_NEXT_VALUE_COMMA_CHAR) {
            terminateJsonString(jsonTokener);
            nextJsonChar(jsonTokener);
            jsonChar = nextCleanJsonChar(jsonTokener);
            if (jsonChar == JSON_ARRAY_END_CHAR) {
                nextJsonChar(jsonTokener);
                return jsonArray;
            }

        } else if (jsonChar == JSON_ARRAY_END_CHAR) {
            terminateJsonString(jsonTokener);
            nextJsonChar(jsonTokener);
            return jsonArray;
        } else {
            jsonTokener->jsonStatus = JSON_ERROR_MISSING_END_PARENTHESIS;
            deleteJsonArray(jsonArray.jsonVector);
            return jsonArray;
        }
    }
}

JSONValue *getJsonObjectValue(JSONObject *jsonObject, JSONType type, const char *key) {
    if (jsonObject != NULL) {
        JSONValue *jsonValue = hashMapGet(jsonObject->jsonMap, key);
        jsonObject->jsonTokener->jsonStatus = (jsonValue != NULL && jsonValue->type == type) ? JSON_OK : JSON_ERROR_MISSING_VALUE;
        return jsonValue;
    }
    return NULL;
}

bool getJsonObjectBoolean(JSONObject *jsonObject, const char *key) {
    JSONValue const *jsonValue = getJsonObjectValue(jsonObject, JSON_BOOLEAN, key);
    return jsonValue != NULL ? (strcmp(jsonValue->value, "true") == 0) : false;
}

double getJsonObjectDouble(JSONObject *jsonObject, const char *key) {
    JSONValue const *jsonValue = getJsonObjectValue(jsonObject, JSON_DOUBLE, key);
    return jsonValue != NULL ? strtod(jsonValue->value, NULL) : 0;
}

int32_t getJsonObjectInt(JSONObject *jsonObject, const char *key) {
    JSONValue const *jsonValue = getJsonObjectValue(jsonObject, JSON_INTEGER, key);
    return jsonValue != NULL ? strtol(jsonValue->value, NULL, 10) : 0;
}

int64_t getJsonObjectLong(JSONObject *jsonObject, const char *key) {
    JSONValue const *jsonValue = hashMapGet(jsonObject->jsonMap, key);
    if (jsonValue != NULL && jsonValue->type == JSON_INTEGER) {
        jsonObject->jsonTokener->jsonStatus = JSON_OK;
        return strtol(jsonValue->value, NULL, 10);

    } else if (jsonValue != NULL && jsonValue->type == JSON_LONG) {
        jsonObject->jsonTokener->jsonStatus = JSON_OK;
        return strtoll(jsonValue->value, NULL, 10);
    }
    jsonObject->jsonTokener->jsonStatus = JSON_ERROR_MISSING_VALUE;
    return 0;
}

char *getJsonObjectString(JSONObject *jsonObject, const char *key) {
    JSONValue const *jsonValue = getJsonObjectValue(jsonObject, JSON_TEXT, key);
    return (isJsonObjectOk(jsonObject) && jsonValue != NULL) ? jsonValue->value : NULL;
}

JSONArray getJSONArrayFromObject(JSONObject *jsonObject, const char *key) {
    JSONArray innerArray = {NULL, NULL};
    JSONValue *jsonValue = getJsonObjectValue(jsonObject, JSON_ARRAY, key);
    if (jsonValue != NULL) {
        innerArray.jsonVector = jsonValue->value;
        innerArray.jsonTokener = jsonObject->jsonTokener;
        return innerArray;
    }
    return innerArray;
}

JSONObject getJSONObjectFromObject(JSONObject *jsonObject, const char *key) {
    JSONObject innerObject = {NULL, NULL};
    JSONValue *jsonValue = getJsonObjectValue(jsonObject, JSON_OBJECT, key);
    if (jsonValue != NULL) {
        innerObject.jsonMap = jsonValue->value;
        innerObject.jsonTokener = jsonObject->jsonTokener;
        return innerObject;
    }
    return innerObject;
}

bool jsonObjectHasKey(JSONObject *jsonObject, const char *key) {
    return jsonObject != NULL ? isHashMapContainsKey(jsonObject->jsonMap, key) : false;
}

bool isJsonObjectValueNull(JSONObject *jsonObject, const char *key) {
    JSONValue *jsonValue = getJsonObjectValue(jsonObject, JSON_NULL, key);
    return jsonValue != NULL;
}

uint32_t getJsonObjectLength(JSONObject *jsonObject) {
    return jsonObject != NULL ? getHashMapSize(jsonObject->jsonMap) : 0;
}

bool getJsonObjectOptBoolean(JSONObject *jsonObject, const char *key, bool defaultValue) {
    bool boolValue = getJsonObjectBoolean(jsonObject, key);
    return isJsonObjectOk(jsonObject) ? boolValue : defaultValue;
}

double getJsonObjectOptDouble(JSONObject *jsonObject, const char *key, double defaultValue) {
    double doubleValue = getJsonObjectDouble(jsonObject, key);
    return isJsonObjectOk(jsonObject) ? doubleValue : defaultValue;
}

int32_t getJsonObjectOptInt(JSONObject *jsonObject, const char *key, int32_t defaultValue) {
    int32_t intValue = getJsonObjectInt(jsonObject, key);
    return isJsonObjectOk(jsonObject) ? intValue : defaultValue;
}

int64_t getJsonObjectOptLong(JSONObject *jsonObject, const char *key, int64_t defaultValue) {
    int64_t longValue = getJsonObjectLong(jsonObject, key);
    return isJsonObjectOk(jsonObject) ? longValue : defaultValue;
}

char *getJsonObjectOptString(JSONObject *jsonObject, const char *key, char *defaultValue) {
    char *stringValue = getJsonObjectString(jsonObject, key);
    return (isJsonObjectOk(jsonObject) && stringValue != NULL) ? stringValue : defaultValue;
}

JSONValue *getJsonArrayValue(JSONArray *jsonArray, JSONType type, uint32_t index) {
    if (jsonArray != NULL) {
        JSONValue *jsonValue = vectorGet(jsonArray->jsonVector, index);
        jsonArray->jsonTokener->jsonStatus = (jsonValue != NULL && jsonValue->type == type) ? JSON_OK : JSON_ERROR_MISSING_VALUE;
        return jsonValue;
    }
    return NULL;
}

bool getJsonArrayBoolean(JSONArray *jsonArray, uint32_t index) {
    JSONValue const *jsonValue = getJsonArrayValue(jsonArray, JSON_BOOLEAN, index);
    return jsonValue != NULL ? (strcmp(jsonValue->value, "true") == 0) : false;
}

double getJsonArrayDouble(JSONArray *jsonArray, uint32_t index) {
    JSONValue const *jsonValue = getJsonArrayValue(jsonArray, JSON_DOUBLE, index);
    return jsonValue != NULL ? strtod(jsonValue->value, NULL) : 0;
}

int32_t getJsonArrayInt(JSONArray *jsonArray, uint32_t index) {
    JSONValue const *jsonValue = getJsonArrayValue(jsonArray, JSON_INTEGER, index);
    return jsonValue != NULL ? strtol(jsonValue->value, NULL, 10) : 0;
}

int64_t getJsonArrayLong(JSONArray *jsonArray, uint32_t index) {
    JSONValue *jsonValue = vectorGet(jsonArray->jsonVector, index);
    if (jsonValue != NULL && jsonValue->type == JSON_INTEGER) {
        jsonArray->jsonTokener->jsonStatus = JSON_OK;
        return strtol(jsonValue->value, NULL, 10);
    } else if (jsonValue != NULL && jsonValue->type == JSON_LONG) {
        jsonArray->jsonTokener->jsonStatus = JSON_OK;
        return strtoll(jsonValue->value, NULL, 10);
    }
    jsonArray->jsonTokener->jsonStatus = JSON_ERROR_MISSING_VALUE;
    return 0;
}

char *getJsonArrayString(JSONArray *jsonArray, uint32_t index) {
    JSONValue const *jsonValue = getJsonArrayValue(jsonArray, JSON_TEXT, index);
    return (isJsonArrayOk(jsonArray) && jsonValue != NULL) ? jsonValue->value : NULL;
}

JSONArray getJSONArrayFromArray(JSONArray *jsonArray, uint32_t index) {
    JSONArray innerArray = {NULL, NULL};
    JSONValue const *jsonValue = getJsonArrayValue(jsonArray, JSON_ARRAY, index);
    if (jsonValue != NULL) {
        innerArray.jsonVector = jsonValue->value;
        innerArray.jsonTokener = jsonArray->jsonTokener;
        return innerArray;
    }
    return innerArray;
}

JSONObject getJSONObjectFromArray(JSONArray *jsonArray, uint32_t index) {
    JSONObject innerObject = {NULL, NULL};
    JSONValue const *jsonValue = getJsonArrayValue(jsonArray, JSON_OBJECT, index);
    if (jsonValue != NULL) {
        innerObject.jsonMap = jsonValue->value;
        innerObject.jsonTokener = jsonArray->jsonTokener;
        return innerObject;
    }
    return innerObject;
}

bool isJsonArrayValueNull(JSONArray *jsonArray, uint32_t index) {
    JSONValue const *jsonValue = getJsonArrayValue(jsonArray, JSON_NULL, index);
    return jsonValue != NULL && isJsonArrayOk(jsonArray);
}

uint32_t getJsonArrayLength(JSONArray *jsonArray) {
    return jsonArray != NULL ? getVectorSize(jsonArray->jsonVector) : 0;
}

bool getJsonArrayOptBoolean(JSONArray *jsonArray, uint32_t index, bool defaultValue) {
    bool boolValue = getJsonArrayBoolean(jsonArray, index);
    return isJsonArrayOk(jsonArray) ? boolValue : defaultValue;
}

double getJsonArrayOptDouble(JSONArray *jsonArray, uint32_t index, double defaultValue) {
    double doubleValue = getJsonArrayDouble(jsonArray, index);
    return isJsonArrayOk(jsonArray) ? doubleValue : defaultValue;
}

int32_t getJsonArrayOptInt(JSONArray *jsonArray, uint32_t index, int32_t defaultValue) {
    int32_t intValue = getJsonArrayInt(jsonArray, index);
    return isJsonArrayOk(jsonArray) ? intValue : defaultValue;
}

int64_t getJsonArrayOptLong(JSONArray *jsonArray, uint32_t index, int64_t defaultValue) {
    int64_t longValue = getJsonArrayLong(jsonArray, index);
    return isJsonArrayOk(jsonArray) ? longValue : defaultValue;
}

char *getJsonArrayOptString(JSONArray *jsonArray, uint32_t index, char *defaultValue) {
    char *stringValue = getJsonArrayString(jsonArray, index);
    return stringValue != NULL ? stringValue : defaultValue;
}

JSONTokener createEmptyJSONTokener() {
    return getJSONTokener(NULL, 0);
}

JSONObject createJsonObject(JSONTokener *jsonTokener) {
    JSONObject jsonObject = {
            .jsonMap = getHashMapInstance(JSON_INITIAL_ITEM_COUNT),
            .jsonTokener = jsonTokener
    };
    return jsonObject;
}

JSONArray createJsonArray(JSONTokener *jsonTokener) {
    JSONArray jsonArray = {
            .jsonVector = getVectorInstance(JSON_ARRAY_INITIAL_ITEM_COUNT),
            .jsonTokener = jsonTokener
    };
    return jsonArray;
}

void jsonObjectPut(JSONObject *jsonObject, const char *key, char *value) {
    if (jsonObject != NULL && key != NULL) {
        if (value != NULL) {
            JSONType jsonType = detectJsonValueType(value, strlen(value));
            JSONValue *jsonValue = getValueInstance(jsonObject->jsonTokener, jsonType, value);
            hashMapPut(jsonObject->jsonMap, key, jsonValue);
        } else {
            hashMapRemove(jsonObject->jsonMap, key);
        }
    }
}

void jsonObjectRemove(JSONObject *jsonObject, const char *key) {
    if (jsonObject != NULL) {
        hashMapRemove(jsonObject->jsonMap, key);
    }
}

void jsonObjectAddObject(JSONObject *jsonObject, const char *key, JSONObject *innerObject) {
    if (jsonObject != NULL && key != NULL && innerObject != NULL) {
        JSONValue *jsonValue = getValueInstance(jsonObject->jsonTokener, JSON_OBJECT, innerObject->jsonMap);
        hashMapPut(jsonObject->jsonMap, key, jsonValue);
    }
}

void jsonObjectAddArray(JSONObject *jsonObject, const char *key, JSONArray *innerArray) {
    if (jsonObject != NULL && key != NULL && innerArray != NULL) {
        JSONValue *jsonValue = getValueInstance(jsonObject->jsonTokener, JSON_ARRAY, innerArray->jsonVector);
        hashMapPut(jsonObject->jsonMap, key, jsonValue);
    }
}

void jsonArrayPut(JSONArray *jsonArray, char *value) {
    if (jsonArray != NULL) {
        if (value != NULL) {
            JSONType jsonType = detectJsonValueType(value, strlen(value));
            JSONValue *jsonValue = getValueInstance(jsonArray->jsonTokener, jsonType, value);
            vectorAdd(jsonArray->jsonVector, jsonValue);
        } else {
            vectorAdd(jsonArray->jsonVector, NULL);
        }
    }
}

void jsonArrayRemove(JSONArray *jsonArray, uint32_t index) {
    if (jsonArray != NULL) {
        vectorRemoveAt(jsonArray->jsonVector, index);
    }
}

void jsonArrayPutAll(JSONArray *destination, JSONArray *source) {
    if (destination != NULL && source != NULL) {
        for (uint32_t i = 0; i < getVectorSize(source->jsonVector); i++) {
            vectorAdd(destination->jsonVector, vectorGet(source->jsonVector, i));
        }
    }
}

void jsonArrayAddArray(JSONArray *jsonArray, JSONArray *innerArray) {
    if (jsonArray != NULL && innerArray != NULL) {
        JSONValue *jsonValue = getValueInstance(jsonArray->jsonTokener, JSON_ARRAY, innerArray->jsonVector);
        vectorAdd(jsonArray->jsonVector, jsonValue);
    }
}

void jsonArrayAddObject(JSONArray *jsonArray, JSONObject *innerObject) {
    if (jsonArray != NULL && innerObject != NULL) {
        JSONValue *jsonValue = getValueInstance(jsonArray->jsonTokener, JSON_OBJECT, innerObject->jsonMap);
        vectorAdd(jsonArray->jsonVector, jsonValue);
    }
}

void jsonObjectToStringPretty(JSONObject *jsonObject, char *resultBuffer, uint32_t bufferSize, uint16_t indentFactor, uint16_t topLevelIndent) {
    if (jsonObject == NULL || resultBuffer == NULL) return;
    InnerJsonBuffer jsonBuffer = {.buffer = resultBuffer, .length = 0, .capacity = bufferSize};
    jsonHashMapToString(jsonObject->jsonMap, &jsonBuffer, indentFactor, topLevelIndent);
}

void jsonArrayToStringPretty(JSONArray *jsonArray, char *resultBuffer, uint32_t bufferSize, uint16_t indentFactor, uint16_t topLevelIndent) {
    if (jsonArray == NULL || resultBuffer == NULL) return;
    InnerJsonBuffer jsonBuffer = {.buffer = resultBuffer, .length = 0, .capacity = bufferSize};
    jsonVectorToString(jsonArray->jsonVector, &jsonBuffer, indentFactor, topLevelIndent);
}

void jsonObjectToString(JSONObject *jsonObject, char *resultBuffer, uint32_t bufferSize) {
    if (jsonObject == NULL || resultBuffer == NULL) return;
    InnerJsonBuffer jsonBuffer = {.buffer = resultBuffer, .length = 0, .capacity = bufferSize};
    jsonHashMapToStringCompact(jsonObject->jsonMap, &jsonBuffer);
}

void jsonArrayToString(JSONArray *jsonArray, char *resultBuffer, uint32_t bufferSize) {
    if (jsonArray == NULL || resultBuffer == NULL) return;
    InnerJsonBuffer jsonBuffer = {.buffer = resultBuffer, .length = 0, .capacity = bufferSize};
    jsonVectorToStringCompact(jsonArray->jsonVector, &jsonBuffer);
}

void deleteJSONObject(JSONObject *jsonObject) {
    if (jsonObject != NULL) {
        deleteJsonObject(jsonObject->jsonMap);
    }
}

void deleteJSONArray(JSONArray *jsonArray) {
    if (jsonArray != NULL) {
        deleteJsonArray(jsonArray->jsonVector);
    }
}

static char nextJsonChar(JSONTokener *jsonTokener) {
    if (hasMoreJsonChars(jsonTokener)) {
        jsonTokener->jsonStringEnd++;
        return *jsonTokener->jsonStringEnd;
    }
    return JSON_NULL_CHAR;
}

static void backJsonChar(JSONTokener *jsonTokener) {
    if (jsonTokener->jsonStringEnd > jsonTokener->jsonBufferPointer) {
        jsonTokener->jsonStringEnd--;
    }
}

static char nextCleanJsonChar(JSONTokener *jsonTokener) {
    char jsonChar = *jsonTokener->jsonStringEnd;

    while (true) {
        if (jsonChar == JSON_COMMENT_START) {  // skip comments
            jsonChar = nextJsonChar(jsonTokener);
            if (jsonChar == JSON_SINGLE_LINE_COMMENT_START) {
                skipJsonChars(jsonTokener);

            } else if (jsonChar == JSON_MULTI_LINE_COMMENT_START) {
                skipJsonMultiLineCommentChars(jsonTokener);
                if (jsonTokener->jsonStatus == JSON_ERROR_UNCLOSED_COMMENT) {
                    return JSON_NULL_CHAR;
                }

            } else {
                backJsonChar(jsonTokener);
                return JSON_COMMENT_END;
            }

        } else if (jsonChar == JSON_HASH_CHAR) {
            skipJsonChars(jsonTokener);

        } else if (jsonChar == JSON_NULL_CHAR || jsonChar > ' ') {
            return jsonChar;
        }

        jsonChar = nextJsonChar(jsonTokener);
    }
}

static char *nextJsonKey(JSONTokener *jsonTokener) {
    char jsonChar = nextCleanJsonChar(jsonTokener);
    if (jsonChar == JSON_KEY_BEGIN_CHAR) {
        char *jsonKey = nextJsonString(jsonTokener);
        if (jsonKey == NULL) {
            return jsonKey;
        }

        if (*jsonTokener->jsonStringEnd != JSON_KEY_VALUE_DELIMITER_CHAR) {
            jsonTokener->jsonStatus = JSON_ERROR_MISSING_KEY_VALUE_SEPARATOR;
            return NULL;
        }
        nextJsonChar(jsonTokener);  // skip key value separator ':'
        return jsonKey;

    } else {
        jsonTokener->jsonStatus = JSON_ERROR_WRONG_KEY_START;
        return NULL;
    }
}

static JSONValue *nextJsonValue(JSONTokener *jsonTokener) {
    char jsonChar = nextCleanJsonChar(jsonTokener);

    if (jsonChar == JSON_VALUE_BEGIN_CHAR) {
        char *jsonStringValue = nextJsonString(jsonTokener);
        return getValueInstance(jsonTokener, JSON_TEXT, jsonStringValue);

    } else if (jsonChar == JSON_OBJECT_BEGIN_CHAR) {
        uint32_t currentJsonLength = jsonTokener->jsonStringLength - (jsonTokener->jsonStringEnd - jsonTokener->jsonBufferPointer);
        JSONTokener currentTokener = getJSONTokener(jsonTokener->jsonStringEnd, currentJsonLength);
        JSONObject jsonObject = jsonObjectParse(&currentTokener);
        jsonTokener->jsonStatus = currentTokener.jsonStatus;
        uint32_t objectSize = (jsonObject.jsonTokener->jsonStringEnd - jsonTokener->jsonStringEnd);
        jsonTokener->jsonStringEnd += objectSize;   // set pointer to object end
        return getValueInstance(jsonTokener, JSON_OBJECT, jsonObject.jsonMap);

    } else if (jsonChar == JSON_ARRAY_BEGIN_CHAR) {
        JSONArray jsonArray = jsonArrayParse(jsonTokener);
        uint32_t objectSize = (jsonArray.jsonTokener->jsonStringEnd - jsonTokener->jsonStringEnd);
        jsonTokener->jsonStringEnd += objectSize;
        return getValueInstance(jsonTokener, JSON_ARRAY, jsonArray.jsonVector);

    } else {
        return handleUnquotedText(jsonTokener);
    }
}

static char *nextJsonString(JSONTokener *jsonTokener) {
    char jsonChar = *jsonTokener->jsonStringEnd;

    if (jsonChar == JSON_DOUBLE_QUOTE_CHAR) {
        jsonChar = nextJsonChar(jsonTokener);  // skip quote char
        char *jsonString = jsonTokener->jsonStringEnd;

        while (jsonChar != JSON_DOUBLE_QUOTE_CHAR) {
            if (jsonChar == '\\') {
                nextJsonChar(jsonTokener);
            }

            if (jsonChar == JSON_NULL_CHAR) {
                jsonTokener->jsonStatus = JSON_ERROR_UNTERMINATED_STRING;
                return NULL;
            }
            jsonChar = nextJsonChar(jsonTokener);
        }

        terminateJsonString(jsonTokener);
        nextJsonChar(jsonTokener);
        return jsonString;

    } else {
        jsonTokener->jsonStatus = JSON_ERROR_UNTERMINATED_STRING;
        return NULL;
    }
}

static void skipJsonChars(JSONTokener *jsonTokener) {
    char jsonChar;
    do {
        jsonChar = nextJsonChar(jsonTokener);
    } while (jsonChar != '\n' && jsonChar != '\r' && jsonChar != JSON_NULL_CHAR);
}

static void skipJsonMultiLineCommentChars(JSONTokener *jsonTokener) {
    while (true) {
        char jsonChar = nextJsonChar(jsonTokener);
        if (jsonChar == JSON_NULL_CHAR) {
            jsonTokener->jsonStatus = JSON_ERROR_UNCLOSED_COMMENT;
            return;
        }

        if (jsonChar == '*') {
            if (nextJsonChar(jsonTokener) == '/') {
                break;
            }
            backJsonChar(jsonTokener);
        }
    }
}

static JSONValue *handleUnquotedText(JSONTokener *jsonTokener) {
    char *jsonTextValue = jsonTokener->jsonStringEnd;

    uint32_t valueLength = 0;
    char jsonChar = *jsonTokener->jsonStringEnd;
    while (jsonChar >= ' ' && isNotEscapedCharArray(jsonChar)) { // Accumulate characters until reach the end of the text or a formatting character.
        jsonChar = nextJsonChar(jsonTokener);
        valueLength++;
    }

    if (valueLength == 0) {
        jsonTokener->jsonStatus = JSON_ERROR_MISSING_VALUE;
        return NULL;
    }

    if (jsonChar == '\n') { // cut value when at new line
        terminateJsonString(jsonTokener);
        nextJsonChar(jsonTokener);
    }

    JSONType jsonType = detectJsonValueType(jsonTextValue, valueLength);
    return getValueInstance(jsonTokener, jsonType, jsonTextValue);
}

static inline bool isNotEscapedCharArray(char jsonChar) {
    switch (jsonChar) {
        case ',':
        case ':':
        case ']':
        case '}':
        case '/':
        case '"':
        case '[':
        case '{':
        case ';':
        case '=':
        case '#':
        case '\\':
            return false;
        default:
            return true;
    }
}

static JSONType detectJsonValueType(char *jsonTextValue, uint32_t valueLength) {
    if (strncmp(jsonTextValue, "true", valueLength) == 0 || strncmp(jsonTextValue, "false", valueLength) == 0) {
        return JSON_BOOLEAN;

    } else if (strncmp(jsonTextValue, "null", valueLength) == 0) {
        return JSON_NULL;
    }

    char value = *jsonTextValue;
    if (isdigit(value) || value == '.' || value == '-' || value == '+') {
        bool isDoubleValue = false;
        for (uint32_t i = 0; i < valueLength; i++) {
            char numberChar = jsonTextValue[i];
            if (numberChar == '.' || numberChar == 'e' || numberChar == 'E') {
                isDoubleValue = true;
                break;
            }
        }

        errno = 0;
        char *endPointer = NULL;
        if (isDoubleValue) {
            double doubleNumber = strtod(jsonTextValue, &endPointer);   // try to parse double number
            if (isJsonDoubleNumberValid(doubleNumber, jsonTextValue, endPointer, valueLength)) {
                if (doubleNumber != 0 || valueLength == 1) {
                    return JSON_DOUBLE;
                }
            }
        }

        errno = 0;
        endPointer = NULL;
        int32_t intNumber = strtol(jsonTextValue, &endPointer, 10);     // try to parse decimal number
        if (isJsonIntegerValid(intNumber, jsonTextValue, endPointer, valueLength)) {
            if (intNumber != 0 || valueLength == 1) {   // check that 0 value is not an error and also no additional chars
                return JSON_INTEGER;
            }
        }

        errno = 0;
        endPointer = NULL;
        int64_t longNumber = strtoll(jsonTextValue, &endPointer, 10);
        if (isJsonLongValid(longNumber, jsonTextValue, endPointer, valueLength)) {
            if (longNumber != 0 || valueLength == 1) {
                return JSON_LONG;
            }
        }
    }

    return JSON_TEXT;
}

static JSONValue *getValueInstance(JSONTokener *jsonTokener, JSONType type, void *value) {
    if (jsonTokener->jsonStatus == JSON_OK) {
        JSONValue *jsonValue = malloc(sizeof(struct JSONValue));
        if (jsonValue == NULL) return NULL;
        jsonValue->value = value;
        jsonValue->type = type;
        return jsonValue;
    }
    return NULL;
}

static void jsonHashMapToString(HashMap jsonMap, InnerJsonBuffer *jsonBuffer, uint16_t indentFactor, uint16_t topLevelIndent) {
    uint32_t objectLength = getHashMapSize(jsonMap);
    if (objectLength == 0) {
        jsonBufferCatStr(jsonBuffer, "{}");
        return;
    }
    uint16_t totalIndent = indentFactor + topLevelIndent;

    jsonBufferCatStr(jsonBuffer, "{");
    if (objectLength == 1) {
        HashMapIterator iterator = getHashMapIterator(jsonMap);
        hashMapHasNext(&iterator);
        JSONValue *jsonValue = iterator.value;
        quoteJsonString(jsonBuffer, iterator.key);
        jsonBufferCatStr(jsonBuffer, ": ");
        appendJsonValue(jsonBuffer, jsonValue, indentFactor, 0);

    } else {
        uint32_t rowCounter = 0;
        HashMapIterator iterator = getHashMapIterator(jsonMap);
        while (hashMapHasNext(&iterator)) {
            rowCounter++;

            if (rowCounter > 1) {
                jsonBufferCatStr(jsonBuffer, ",\n");
            } else {
                jsonBufferCatStr(jsonBuffer, "\n");
            }

            for (uint16_t i = 0; i < totalIndent; i++) {
                jsonBufferCatStr(jsonBuffer, " ");
            }
            quoteJsonString(jsonBuffer, iterator.key);
            jsonBufferCatStr(jsonBuffer, ": ");
            JSONValue *jsonValue = iterator.value;
            appendJsonValue(jsonBuffer, jsonValue, indentFactor, totalIndent);
        }

        if (rowCounter > 1) {
            jsonBufferCatStr(jsonBuffer, "\n");
            for (uint16_t i = 0; i < topLevelIndent; i++) {
                jsonBufferCatStr(jsonBuffer, " ");
            }
        }
    }
    jsonBufferCatStr(jsonBuffer, "}");
}

static void jsonVectorToString(Vector jsonVector, InnerJsonBuffer *jsonBuffer, uint16_t indentFactor, uint16_t topLevelIndent) {
    uint32_t arrayLength = getVectorSize(jsonVector);
    if (arrayLength == 0) {
        jsonBufferCatStr(jsonBuffer, "[]");
        return;
    }

    jsonBufferCatStr(jsonBuffer, "[");
    if (arrayLength == 1) {
        JSONValue *jsonValue = vectorGet(jsonVector, 0);
        appendJsonValue(jsonBuffer, jsonValue, indentFactor, topLevelIndent);

    } else {

        uint16_t totalIndent = indentFactor + topLevelIndent;
        jsonBufferCatStr(jsonBuffer, "\n");
        for (uint32_t i = 0; i < arrayLength; i++) {
            if (i > 0) {
                jsonBufferCatStr(jsonBuffer, ",\n");
            }

            for (uint16_t j = 0; j < totalIndent; j++) {
                jsonBufferCatStr(jsonBuffer, " ");
            }
            JSONValue *jsonValue = vectorGet(jsonVector, i);
            appendJsonValue(jsonBuffer, jsonValue, indentFactor, topLevelIndent);
        }

        jsonBufferCatStr(jsonBuffer, "\n");
        for (uint16_t i = 0; i < topLevelIndent; i++) {
            jsonBufferCatStr(jsonBuffer, " ");
        }
    }
    jsonBufferCatStr(jsonBuffer, "]");
}

static void jsonHashMapToStringCompact(HashMap jsonMap, InnerJsonBuffer *jsonBuffer) {
    jsonBufferCatStr(jsonBuffer, "{");

    uint32_t rowCounter = 0;
    HashMapIterator iterator = getHashMapIterator(jsonMap);
    while (hashMapHasNext(&iterator)) {
        rowCounter++;
        if (rowCounter > 1) {
            jsonBufferCatStr(jsonBuffer, ",");
        }

        quoteJsonString(jsonBuffer, iterator.key);
        jsonBufferCatStr(jsonBuffer, ":");
        JSONValue *jsonValue = iterator.value;
        appendJsonValueCompact(jsonBuffer, jsonValue);
    }
    jsonBufferCatStr(jsonBuffer, "}");
}

static void jsonVectorToStringCompact(Vector jsonVector, InnerJsonBuffer *jsonBuffer) {
    jsonBufferCatStr(jsonBuffer, "[");
    for (uint32_t i = 0; i < getVectorSize(jsonVector); i++) {
        if (i > 0) {
            jsonBufferCatStr(jsonBuffer, ",");
        }
        JSONValue *jsonValue = vectorGet(jsonVector, i);
        appendJsonValueCompact(jsonBuffer, jsonValue);
    }
    jsonBufferCatStr(jsonBuffer, "]");
}

static void quoteJsonString(InnerJsonBuffer *jsonBuffer, const char *value) {
    jsonBufferCatStr(jsonBuffer, "\"");
    jsonBufferCatStr(jsonBuffer, value);
    jsonBufferCatStr(jsonBuffer, "\"");
}

static void appendJsonValue(InnerJsonBuffer *jsonBuffer, JSONValue *jsonValue, uint16_t indentFactor, uint16_t topLevelIndent) {
    switch (jsonValue->type) {
        case JSON_OBJECT:
            jsonHashMapToString((HashMap) jsonValue->value, jsonBuffer, indentFactor, topLevelIndent);
            break;
        case JSON_ARRAY:
            jsonVectorToString((Vector) jsonValue->value, jsonBuffer, indentFactor, topLevelIndent);
            break;
        case JSON_BOOLEAN:
        case JSON_INTEGER:
        case JSON_DOUBLE:
        case JSON_LONG:
        case JSON_NULL:
            jsonBufferCatStr(jsonBuffer, jsonValue->value);
            break;
        case JSON_TEXT:
            quoteJsonString(jsonBuffer, jsonValue->value);
            break;
        default:
            return;
    }
}

static void appendJsonValueCompact(InnerJsonBuffer *jsonBuffer, JSONValue *jsonValue) {
    switch (jsonValue->type) {
        case JSON_OBJECT:
            jsonHashMapToStringCompact((HashMap) jsonValue->value, jsonBuffer);
            break;
        case JSON_ARRAY:
            jsonVectorToStringCompact((Vector) jsonValue->value, jsonBuffer);
            break;
        case JSON_BOOLEAN:
        case JSON_INTEGER:
        case JSON_DOUBLE:
        case JSON_LONG:
        case JSON_NULL:
            jsonBufferCatStr(jsonBuffer, jsonValue->value);
            break;
        case JSON_TEXT:
            quoteJsonString(jsonBuffer, jsonValue->value);
            break;
        default:
            return;
    }
}

static void jsonBufferCatStr(InnerJsonBuffer *jsonBuffer, const char *str) {
    uint32_t length = strnlen(str, jsonBuffer->capacity + 1);
    if (length >= (jsonBuffer->capacity - jsonBuffer->length)) return;
    memcpy(jsonBuffer->buffer + jsonBuffer->length, str, length);
    jsonBuffer->length += length;
    jsonBuffer->buffer[jsonBuffer->length] = '\0';
}

static void deleteJsonObject(HashMap jsonObjectMap) {
    HashMapIterator iterator = getHashMapIterator(jsonObjectMap);
    while (hashMapHasNext(&iterator)) {
        JSONValue *jsonValue = iterator.value;
        if (jsonValue != NULL && jsonValue->type == JSON_OBJECT) {
            deleteJsonObject(jsonValue->value);
        } else if (jsonValue != NULL && jsonValue->type == JSON_ARRAY) {
            deleteJsonArray(jsonValue->value);
        } else {
            free(jsonValue);
        }
    }
    hashMapDelete(jsonObjectMap);
}

static void deleteJsonArray(Vector jsonVector) {
    for (uint32_t i = 0; i < getVectorSize(jsonVector); i++) {
        JSONValue *jsonValue = vectorGet(jsonVector, i);
        if (jsonValue != NULL && jsonValue->type == JSON_OBJECT) {
            deleteJsonObject(jsonValue->value);
        } else if (jsonValue != NULL && jsonValue->type == JSON_ARRAY) {
            deleteJsonArray(jsonValue->value);
        } else {
            free(jsonValue);
        }
    }
    vectorDelete(jsonVector);
}
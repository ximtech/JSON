#pragma once

#include "BaseTestTemplate.h"
#include "JSON.h"

static MunitResult updateJsonObjectTest(const MunitParameter params[], void *data) {
    const char *testUpdateJson = "{\"name\":\"John\", \"age\":30, \"car\":null, \"house\":null}";
    char buffer[256] = {0};
    strcpy(buffer, testUpdateJson);

    JSONTokener jsonTokener = getJSONTokener(buffer, strlen(buffer));
    JSONObject jsonObject = jsonObjectParse(&jsonTokener);
    assert_true(isJsonObjectOk(&jsonObject));

    jsonObjectRemove(&jsonObject, "car");

    jsonObjectPut(&jsonObject, "key1", "true");
    jsonObjectPut(&jsonObject, "key2", "false");
    jsonObjectPut(&jsonObject, "key3", "true");
    jsonObjectPut(&jsonObject, "key4", "some text value");

    char intBuffer[20] = {0};
    sprintf(intBuffer, "%d", 123);
    jsonObjectPut(&jsonObject, "key5", intBuffer);

    char doubleBuffer[20] = {0};
    sprintf(doubleBuffer, "%.3f", 123.456);
    jsonObjectPut(&jsonObject, "key6", doubleBuffer);

    char longBuffer[100] = {0};
    sprintf(longBuffer, "%lld", 1223423568889378999);
    jsonObjectPut(&jsonObject, "key7", longBuffer);

    JSONObject innerObj = createJsonObject(&jsonTokener);
    jsonObjectPut(&innerObj, "innerKey1", "value");
    jsonObjectPut(&innerObj, "innerKey2", "true");
    jsonObjectPut(&innerObj, "innerKey3", "200");
    jsonObjectAddObject(&jsonObject, "innerObject", &innerObj);

    JSONArray innerArray = createJsonArray(&jsonTokener);
    jsonArrayPut(&innerArray, "123");
    jsonArrayPut(&innerArray, "false");
    jsonArrayPut(&innerArray, "text value");
    jsonArrayPut(&innerArray, "33.333");
    jsonArrayPut(&innerArray, "3333344445555");
    jsonObjectAddArray(&jsonObject, "valueList", &innerArray);

    // Validate updated JSON
    char resBuffer[512] = {0};
    jsonObjectToStringPretty(&jsonObject, resBuffer, ARRAY_SIZE(resBuffer), 3, 0);
    JSONTokener checkTokener = getJSONTokener(resBuffer, strlen(resBuffer));
    JSONObject checkObject = jsonObjectParse(&checkTokener);
    assert_true(isJsonObjectOk(&checkObject));

    assert_false(jsonObjectHasKey(&checkObject, "car"));

    JSONValue *name = getJsonObjectValue(&checkObject, JSON_TEXT, "name");
    JSONValue *age = getJsonObjectValue(&checkObject, JSON_INTEGER, "age");
    JSONValue *house = getJsonObjectValue(&checkObject, JSON_NULL, "house");
    JSONValue *key1 = getJsonObjectValue(&checkObject, JSON_BOOLEAN, "key1");
    JSONValue *key2 = getJsonObjectValue(&checkObject, JSON_BOOLEAN, "key2");
    JSONValue *key3 = getJsonObjectValue(&checkObject, JSON_BOOLEAN, "key3");
    JSONValue *key4 = getJsonObjectValue(&checkObject, JSON_TEXT, "key4");
    JSONValue *key5 = getJsonObjectValue(&checkObject, JSON_INTEGER, "key5");
    JSONValue *key6 = getJsonObjectValue(&checkObject, JSON_DOUBLE, "key6");
    JSONValue *key7 = getJsonObjectValue(&checkObject, JSON_LONG, "key7");

    assert_true(name != NULL && JSON_TEXT == name->type);
    assert_true(age != NULL && JSON_INTEGER == age->type);
    assert_true(house != NULL && JSON_NULL == house->type);
    assert_true(key1 != NULL && JSON_BOOLEAN == key1->type);
    assert_true(key2 != NULL && JSON_BOOLEAN == key2->type);
    assert_true(key3 != NULL && JSON_BOOLEAN == key3->type);
    assert_true(key4 != NULL && JSON_TEXT == key4->type);
    assert_true(key5 != NULL && JSON_INTEGER == key5->type);
    assert_true(key6 != NULL && JSON_DOUBLE == key6->type);
    assert_true(JSON_LONG == key7->type || JSON_INTEGER == key7->type);

    JSONObject innerObjCheck = getJSONObjectFromObject(&checkObject, "innerObject");
    assert_true(isJsonObjectOk(&checkObject));

    JSONValue *innerKey1 = getJsonObjectValue(&innerObjCheck, JSON_ARRAY, "innerKey1");
    JSONValue *innerKey2 = getJsonObjectValue(&innerObjCheck, JSON_ARRAY, "innerKey2");
    JSONValue *innerKey3 = getJsonObjectValue(&innerObjCheck, JSON_ARRAY, "innerKey3");

    assert_true(JSON_TEXT == innerKey1->type);
    assert_true(JSON_BOOLEAN == innerKey2->type);
    assert_true(JSON_INTEGER == innerKey3->type);

    JSONArray innerArrayCheck = getJSONArrayFromObject(&checkObject, "valueList");
    assert_true(isJsonObjectOk(&checkObject));

    JSONValue *arrayValue1 = getJsonArrayValue(&innerArrayCheck, JSON_INTEGER, 0);
    JSONValue *arrayValue2 = getJsonArrayValue(&innerArrayCheck, JSON_BOOLEAN, 1);
    JSONValue *arrayValue3 = getJsonArrayValue(&innerArrayCheck, JSON_TEXT, 2);
    JSONValue *arrayValue4 = getJsonArrayValue(&innerArrayCheck, JSON_DOUBLE, 3);
    JSONValue *arrayValue5 = getJsonArrayValue(&innerArrayCheck, JSON_LONG, 4);

    assert_not_null(arrayValue1);
    assert_not_null(arrayValue2);
    assert_not_null(arrayValue3);
    assert_not_null(arrayValue4);
    assert_not_null(arrayValue5);

    deleteJSONObject(&jsonObject);
    deleteJSONObject(&checkObject);
    return MUNIT_OK;
}

static MunitResult updateJsonArrayTest(const MunitParameter params[], void *data) {
    const char *testUpdateJson = "[123, false, \"text value\", 33.333, 3333344445555]";
    char buffer[256] = {0};
    strcpy(buffer, testUpdateJson);

    JSONTokener jsonTokener = getJSONTokener(buffer, strlen(buffer));
    JSONArray jsonArray = jsonArrayParse(&jsonTokener);
    assert_true(isJsonArrayOk(&jsonArray));

    jsonArrayRemove(&jsonArray, 1);
    jsonArrayPut(&jsonArray, "true");

    JSONTokener arrayTokener = createEmptyJSONTokener();
    JSONArray nextArray = createJsonArray(&arrayTokener);
    jsonArrayPut(&nextArray, "text");
    jsonArrayPut(&nextArray, "12.22");
    jsonArrayPutAll(&jsonArray, &nextArray);

    // Validate updated JSON
    char resBuffer[256] = {0};
    jsonArrayToStringPretty(&jsonArray, resBuffer, ARRAY_SIZE(resBuffer), 3, 0);
    assert_string_equal("[\n"
                        "   123,\n"
                        "   \"text value\",\n"
                        "   33.333,\n"
                        "   3333344445555,\n"
                        "   true,\n"
                        "   \"text\",\n"
                        "   12.22\n"
                        "]", resBuffer);

    deleteJSONArray(&jsonArray);
    return MUNIT_OK;
}

static MunitTest jsonUpdateTests[] = {
        {.name = "Test OK - should correctly add/remove data to existing json object", .test = updateJsonObjectTest},
        {.name = "Test OK - should correctly add/remove data to existing json array", .test = updateJsonArrayTest},

        END_OF_TESTS
};

static const MunitSuite jsonUpdateTestSuite = {
        .prefix = "JSON Update: ",
        .tests = jsonUpdateTests,
        .suites = NULL,
        .iterations = 1,
        .options = MUNIT_SUITE_OPTION_NONE
};
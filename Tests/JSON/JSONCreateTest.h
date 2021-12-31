#pragma once

#include "BaseTestTemplate.h"
#include "JSON.h"

static MunitResult createJsonObjectTest(const MunitParameter params[], void *data) {
    JSONTokener jsonTokener = createEmptyJSONTokener();
    JSONObject jsonObject = createJsonObject(&jsonTokener);
    jsonObjectPut(&jsonObject, "key1", "123");
    jsonObjectPut(&jsonObject, "key2", "null");
    jsonObjectPut(&jsonObject, "key3", "false");
    jsonObjectPut(&jsonObject, "key4", "text");

    JSONObject innerObject = createJsonObject(&jsonTokener);
    jsonObjectPut(&innerObject, "innerKey1", "321456");
    jsonObjectPut(&innerObject, "innerKey2", "true");
    jsonObjectPut(&innerObject, "innerKey3", "some text");
    jsonObjectPut(&innerObject, "innerKey4", "1.255");

    JSONArray jsonArray = createJsonArray(&jsonTokener);
    jsonArrayPut(&jsonArray, "1");
    jsonArrayPut(&jsonArray, "2");
    jsonArrayPut(&jsonArray, "3");

    jsonObjectAddObject(&jsonObject, "innerObject", &innerObject);
    jsonObjectAddArray(&jsonObject, "innerArray", &jsonArray);

    char resBuffer[256] = {0};
    jsonObjectToStringPretty(&jsonObject, resBuffer, 3, 0);
    assert_string_equal("{\n"
                        "   \"innerObject\": {\n"
                        "      \"innerKey2\": true,\n"
                        "      \"innerKey3\": \"some text\",\n"
                        "      \"innerKey1\": 321456,\n"
                        "      \"innerKey4\": 1.255\n"
                        "   },\n"
                        "   \"innerArray\": [\n"
                        "      1,\n"
                        "      2,\n"
                        "      3\n"
                        "   ],\n"
                        "   \"key1\": 123,\n"
                        "   \"key4\": \"text\",\n"
                        "   \"key2\": null,\n"
                        "   \"key3\": false\n"
                        "}", resBuffer);

    memset(resBuffer, 0, 256);
    jsonObjectToString(&jsonObject, resBuffer);
    assert_string_equal(
            "{\"innerObject\":"
            "{\"innerKey2\":true,"
            "\"innerKey3\":\"some text\","
            "\"innerKey1\":321456,"
            "\"innerKey4\":1.255},"
            "\"innerArray\":[1,2,3],"
            "\"key1\":123,"
            "\"key4\":\"text\","
            "\"key2\":null,"
            "\"key3\":false}",
            resBuffer);

    deleteJSONObject(&jsonObject);
    return MUNIT_OK;
}

static MunitResult createJsonArrayTest(const MunitParameter params[], void *data) {
    JSONTokener arrayTokener = createEmptyJSONTokener();
    JSONArray jsonArray = createJsonArray(&arrayTokener);

    jsonArrayPut(&jsonArray, "text");
    jsonArrayPut(&jsonArray, "12.22");
    jsonArrayPut(&jsonArray, "222");
    jsonArrayPut(&jsonArray, "true");
    jsonArrayPut(&jsonArray, "null");
    jsonArrayPut(&jsonArray, "234123423543");

    JSONTokener innerArrayTokener = createEmptyJSONTokener();
    JSONArray innerArray = createJsonArray(&innerArrayTokener);
    jsonArrayPut(&innerArray, "1");
    jsonArrayPut(&innerArray, "2");
    jsonArrayPut(&innerArray, "3");

    jsonArrayAddArray(&jsonArray, &innerArray);

    JSONObject innerObject = createJsonObject(&innerArrayTokener);
    jsonObjectPut(&innerObject, "key1", "12345");
    jsonArrayAddObject(&jsonArray, &innerObject);

    char resBuffer[256] = {0};
    jsonArrayToString(&jsonArray, resBuffer);
    assert_string_equal("[\"text\",12.22,222,true,null,234123423543,[1,2,3],{\"key1\":12345}]", resBuffer);

    deleteJSONArray(&jsonArray);
    return MUNIT_OK;
}

static MunitTest jsonCreateTests[] = {
        {.name = "Test OK - should correctly create json object", .test = createJsonObjectTest},
        {.name = "Test OK - should correctly create json array", .test = createJsonArrayTest},

        END_OF_TESTS
};

static const MunitSuite jsonCreateTestSuite = {
        .prefix = "JSON Create: ",
        .tests = jsonCreateTests,
        .suites = NULL,
        .iterations = 1,
        .options = MUNIT_SUITE_OPTION_NONE
};

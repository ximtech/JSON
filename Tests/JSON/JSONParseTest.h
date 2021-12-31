#pragma once

#include "BaseTestTemplate.h"
#include "JSON.h"

typedef struct JSONBadFormat {
    char *jsonText;
    JSONStatus status;
} JSONBadFormat;

static const JSONBadFormat JSON_BAD_FORMAT_ARRAY[] = {
        {"",                                       JSON_ERROR_EMPTY_TEXT},
        {"{/*unclosed comment\n \"a\":1,\"b\":2}", JSON_ERROR_UNCLOSED_COMMENT},
        {"{\"var:true}",                           JSON_ERROR_UNTERMINATED_STRING},
        {"{\"var\":true,",                         JSON_ERROR_MISSING_END_PARENTHESIS},
        {"{\"var\":true",                          JSON_ERROR_WRONG_VALUE_END},
        {"{\"var\":,9}",                           JSON_ERROR_MISSING_VALUE},
        {"{\"var\":}",                             JSON_ERROR_MISSING_VALUE},
        {"{\"var\":,}",                            JSON_ERROR_MISSING_VALUE},
        {"\"qwerty\":false,}",                     JSON_ERROR_MISSING_START_PARENTHESIS},
        {"{\"qwerty\":false,",                     JSON_ERROR_MISSING_END_PARENTHESIS},
        {"{\"qwerty\"false}",                      JSON_ERROR_MISSING_KEY_VALUE_SEPARATOR},
        {"{\"qwerty\":false \"next\":123}",        JSON_ERROR_WRONG_VALUE_END},
        {"{qwerty\":false,}",                      JSON_ERROR_WRONG_KEY_START},
        {"{\"qwerty\":[false,}",                   JSON_ERROR_MISSING_VALUE},
        {"{\"qwerty\":[false}",                    JSON_ERROR_MISSING_END_PARENTHESIS},
};

static const char * const testJson =
        "{\n"
        "   \"firstName\":\"Bidhan\",\n"
        "   \"lastName\":\"Chatterjee\",\n"
        "   \"age\":40,\n"
        "   \"address\":{\n"
        "      \"streetAddress\":\"144 J B Hazra Road\",\n"
        "      \"city\":\"Burdwan\",\n"
        "      \"state\":\"Paschimbanga\",\n"
        "      \"postalCode\":\"713102\"\n"
        "   },\n"
        "   \"phoneList\":[\n"
        "      {\n"
        "         \"type\":\"personal\",\n"
        "         \"number\":\"09832209761\"\n"
        "      },\n"
        "      {\n"
        "         \"type\":\"fax\",\n"
        "         \"number\":\"91-342-2567692\",\n"
        "         \"tel\":+913422567692\n"
        "      }\n"
        "   ]\n"
        "}";

static const char *const testJsonOpt =
        "{   \"widget\": \"on\",\n"
        "    \"debug\": \"on\",\n"
        "    \"window\": {\n"
        "        \"title\": \"Sample Konfabulator Widget\",\n"
        "        \"name\": \"main_window\",\n"
        "        \"width\": 500,\n"
        "        \"height\": 50078314454\n"
        "    },\n"
        "    \"image\": {\n"
        "        \"src\": \"Images/Sun.png\",\n"
        "        \"name\": \"sun1\",\n"
        "        \"enabled\": true,\n"
        "        \"deleted\": false,\n"
        "        \"alignment\": \"center\"\n"
        "    },\n"
        "    \"text\": {\n"
        "        \"data\": \"Click Here\",\n"
        "        \"size\": 36,\n"
        "        \"style\": \"bold\",\n"
        "        \"name\": \"text1\",\n"
        "        \"hOffset\": 25.30,\n"
        "        \"alignment\": \"center\",\n"
        "        \"onMouseUp\": \"sun1.opacity = (sun1.opacity / 100) * 90;\"\n"
        "    }\n"
        "}";

static MunitResult emptyJsonTest(const MunitParameter params[], void *data) {
    {
        char jsonBuffer[20] = {0};
        strcpy(jsonBuffer, "{}");
        JSONTokener jsonTokener = getJSONTokener(jsonBuffer, strlen(jsonBuffer));
        JSONObject jsonObject = jsonObjectParse(&jsonTokener);
        assert_true(isJsonObjectOk(&jsonObject));
        assert_uint32(0, ==, getJsonObjectLength(&jsonObject));
        deleteJSONObject(&jsonObject);
    }
    {
        char jsonBuffer[20] = {0};
        strcpy(jsonBuffer, "{\"a\":[]}");
        JSONTokener jsonTokener = getJSONTokener(jsonBuffer, strlen(jsonBuffer));
        JSONObject jsonObject = jsonObjectParse(&jsonTokener);
        assert_true(isJsonObjectOk(&jsonObject));
        assert_uint32(1, ==, getJsonObjectLength(&jsonObject));

        JSONArray jsonArray = getJSONArrayFromObject(&jsonObject, "a");
        assert_true(isJsonArrayOk(&jsonArray));
        assert_uint32(0, ==, getJsonArrayLength(&jsonArray));
        deleteJSONObject(&jsonObject);
    }
    {
        char jsonBuffer[20] = {0};
        strcpy(jsonBuffer, "{\"a\":[{},{}]}");
        JSONTokener jsonTokener = getJSONTokener(jsonBuffer, strlen(jsonBuffer));
        JSONObject jsonObject = jsonObjectParse(&jsonTokener);
        assert_true(isJsonObjectOk(&jsonObject));
        assert_uint32(1, ==, getJsonObjectLength(&jsonObject));

        JSONArray jsonArray = getJSONArrayFromObject(&jsonObject, "a");
        assert_true(isJsonArrayOk(&jsonArray));
        assert_uint32(2, ==, getJsonArrayLength(&jsonArray));

        JSONObject innerObj_1 = getJSONObjectFromArray(&jsonArray, 0);
        JSONObject innerObj_2 = getJSONObjectFromArray(&jsonArray, 1);
        assert_uint32(0, ==, getJsonObjectLength(&innerObj_1));
        assert_uint32(0, ==, getJsonObjectLength(&innerObj_2));
        deleteJSONObject(&jsonObject);
    }

    return MUNIT_OK;
}

static MunitResult primitiveValueJsonTest(const MunitParameter params[], void *data) {
    char jsonBuffer[256] = {0};
    strcpy(jsonBuffer, "{"
                       "\"max\":        9223372036854775807,"
                       "\"min\":        -9223372036854775807,"
                       "\"boolvar0\":   false,"
                       "\"boolvar1\":   true,"
                       "\"nullvar\":    null,"
                       "\"scientific\": 5368.32e-3,"
                       "\"real\":       -0.25,"
                       "}");

    JSONTokener jsonTokener = getJSONTokener(jsonBuffer, strlen(jsonBuffer));
    JSONObject jsonObject = jsonObjectParse(&jsonTokener);
    assert_true(isJsonObjectOk(&jsonObject));

    bool boolvar0 = getJsonObjectBoolean(&jsonObject, "boolvar0");
    assert_true(isJsonObjectOk(&jsonObject));
    assert_false(boolvar0);

    bool boolvar1 = getJsonObjectBoolean(&jsonObject, "boolvar1");
    assert_true(isJsonObjectOk(&jsonObject));
    assert_true(boolvar1);

    bool isNullVar = isJsonObjectValueNull(&jsonObject, "nullvar");
    assert_true(isJsonObjectOk(&jsonObject));
    assert_true(isNullVar);

    int64_t max = getJsonObjectLong(&jsonObject, "max");
    assert_true(isJsonObjectOk(&jsonObject));
    assert_int64(9223372036854775807, ==, max);

    int64_t min = getJsonObjectLong(&jsonObject, "min");
    assert_true(isJsonObjectOk(&jsonObject));
    assert_int64(-9223372036854775807, ==, min);

    double real = getJsonObjectDouble(&jsonObject, "real");
    assert_true(isJsonObjectOk(&jsonObject));
    assert_double(-0.25, ==, real);

    double scientific = getJsonObjectDouble(&jsonObject, "scientific");
    assert_true(isJsonObjectOk(&jsonObject));
    assert_double(5368.32e-3, ==, scientific);
    deleteJSONObject(&jsonObject);

    return MUNIT_OK;
}

static MunitResult textValueJsonTest(const MunitParameter params[], void *data) {
    {
        char jsonBuffer[128] = {0};
        strcpy(jsonBuffer, "{\"a\":\"\tThis text: \\\"Hello\\\".\n\"}");
        JSONTokener jsonTokener = getJSONTokener(jsonBuffer, strlen(jsonBuffer));
        JSONObject jsonObject = jsonObjectParse(&jsonTokener);
        assert_true(isJsonObjectOk(&jsonObject));

        char *string = getJsonObjectString(&jsonObject, "a");
        assert_true(isJsonObjectOk(&jsonObject));
        assert_string_equal("\tThis text: \\\"Hello\\\".\n", string);
        deleteJSONObject(&jsonObject);
    }
    {
        char jsonBuffer[128] = {0};
        strcpy(jsonBuffer, "{\"name\":\"Christiane Eluère\"}");
        JSONTokener jsonTokener = getJSONTokener(jsonBuffer, strlen(jsonBuffer));
        JSONObject jsonObject = jsonObjectParse(&jsonTokener);
        assert_true(isJsonObjectOk(&jsonObject));

        char *string = getJsonObjectString(&jsonObject, "name");
        assert_true(isJsonObjectOk(&jsonObject));
        assert_string_equal("Christiane Eluère", string);
        deleteJSONObject(&jsonObject);
    }

    return MUNIT_OK;
}

static MunitResult validFormatJsonTest(const MunitParameter params[], void *data) {
    char jsonBuffer[128] = {0};
    const char *validJsonFormat = munit_parameters_get(params, "validJsonFormat");
    strcpy(jsonBuffer, validJsonFormat);
    JSONTokener jsonTokener = getJSONTokener(jsonBuffer, strlen(jsonBuffer));
    JSONObject jsonObject = jsonObjectParse(&jsonTokener);
    assert_true(isJsonObjectOk(&jsonObject));
    deleteJSONObject(&jsonObject);
    return MUNIT_OK;
}

static MunitResult badFormatJsonTest(const MunitParameter params[], void *data) {
    for (int i = 0; i < ARRAY_SIZE(JSON_BAD_FORMAT_ARRAY); i++) {
        JSONBadFormat badFormat = JSON_BAD_FORMAT_ARRAY[i];
        char jsonBuffer[64] = {0};
        strcpy(jsonBuffer, badFormat.jsonText);

        JSONTokener jsonTokener = getJSONTokener(jsonBuffer, strlen(jsonBuffer));
        jsonObjectParse(&jsonTokener);

        bool isStatusCorrect = jsonTokener.jsonStatus == badFormat.status;
        if (!isStatusCorrect) {
            printf(". Test No: [%d], Json: [%s], Expected: [%d] Actual: [%d]", i, badFormat.jsonText, badFormat.status, jsonTokener.jsonStatus);
            return MUNIT_FAIL;
        }
    }
    return MUNIT_OK;
}

static MunitResult parseJsonTest(const MunitParameter params[], void *data) {
    char jsonBuffer[1024] = {0};
    strcpy(jsonBuffer, testJson);

    JSONTokener jsonTokener = getJSONTokener(jsonBuffer, strlen(jsonBuffer));
    JSONObject jsonObject = jsonObjectParse(&jsonTokener);
    assert_true(isJsonObjectOk(&jsonObject));

    int firstNameAsInt = getJsonObjectInt(&jsonObject, "firstName");
    assert_false(isJsonObjectOk(&jsonObject));  // fistName is string, int is not valid type
    assert_int(0, ==, firstNameAsInt);

    char *firstName = getJsonObjectString(&jsonObject, "firstName");
    assert_true(isJsonObjectOk(&jsonObject));
    assert_string_equal("Bidhan", firstName);

    char *lastName = getJsonObjectString(&jsonObject, "lastName");
    assert_true(isJsonObjectOk(&jsonObject));
    assert_string_equal("Chatterjee", lastName);

    int32_t age = getJsonObjectInt(&jsonObject, "age");
    assert_true(isJsonObjectOk(&jsonObject));
    assert_int32(40, ==, age);

    JSONArray addressArray = getJSONArrayFromObject(&jsonObject, "address");
    assert_false(isJsonArrayOk(&addressArray));  // address is object, array is not valid type

    JSONObject addressObject = getJSONObjectFromObject(&jsonObject, "address");
    assert_true(isJsonObjectOk(&addressObject));

    char *streetAddress = getJsonObjectString(&addressObject, "streetAddress");
    assert_true(isJsonObjectOk(&addressObject));
    assert_string_equal("144 J B Hazra Road", streetAddress);

    char *city = getJsonObjectString(&addressObject, "city");
    assert_true(isJsonObjectOk(&addressObject));
    assert_string_equal("Burdwan", city);

    char *state = getJsonObjectString(&addressObject, "state");
    assert_true(isJsonObjectOk(&addressObject));
    assert_string_equal("Paschimbanga", state);

    char *postalCode = getJsonObjectString(&addressObject, "postalCode");
    assert_true(isJsonObjectOk(&addressObject));
    assert_string_equal("713102", postalCode);

    JSONArray phoneList = getJSONArrayFromObject(&jsonObject, "phoneList");
    assert_true(isJsonArrayOk(&phoneList));

    JSONObject item_1 = getJSONObjectFromArray(&phoneList, 0);
    assert_true(isJsonArrayOk(&phoneList));

    char *type_1 = getJsonObjectString(&item_1, "type");
    assert_true(isJsonObjectOk(&item_1));
    assert_string_equal("personal", type_1);

    char *number_1 = getJsonObjectString(&item_1, "number");
    assert_true(isJsonObjectOk(&item_1));
    assert_string_equal("09832209761", number_1);

    JSONObject item_2 = getJSONObjectFromArray(&phoneList, 1);
    assert_true(isJsonArrayOk(&phoneList));

    char *type_2 = getJsonObjectString(&item_2, "type");
    assert_true(isJsonObjectOk(&item_2));
    assert_string_equal("fax", type_2);

    char *number_2 = getJsonObjectString(&item_2, "number");
    assert_true(isJsonObjectOk(&item_2));
    assert_string_equal("91-342-2567692", number_2);

    char *telAsString = getJsonObjectString(&item_2, "tel");
    assert_false(isJsonObjectOk(&item_2));
    assert_null(telAsString);

    int64_t tel = getJsonObjectLong(&item_2, "tel");
    assert_true(isJsonObjectOk(&item_2));
    assert_int64(913422567692, ==, tel);

    deleteJSONObject(&jsonObject);
    return MUNIT_OK;
}

static MunitResult parseJsonOptTest(const MunitParameter params[], void *data) {
    char jsonBuffer[1024] = {0};
    strcpy(jsonBuffer, testJsonOpt);

    JSONTokener jsonTokener = getJSONTokener(jsonBuffer, strlen(jsonBuffer));
    JSONObject jsonObject = jsonObjectParse(&jsonTokener);
    assert_true(isJsonObjectOk(&jsonObject));

    char *debug = getJsonObjectOptString(&jsonObject, "debug", "off");
    char *prod = getJsonObjectOptString(&jsonObject, "prod", "off");
    assert_string_equal("on", debug);
    assert_string_equal("off", prod);

    JSONObject windowObject = getJSONObjectFromObject(&jsonObject, "window");
    assert_true(isJsonObjectOk(&windowObject));
    int32_t width = getJsonObjectOptInt(&windowObject, "width", 400);
    assert_int32(500, ==, width);
    int32_t offset = getJsonObjectOptInt(&windowObject, "offset", 60);
    assert_int32(60, ==, offset);

    int64_t height = getJsonObjectOptLong(&windowObject, "height", 600);
    assert_int64(50078314454, ==, height);
    int64_t length = getJsonObjectOptLong(&windowObject, "length", 600);
    assert_int64(600, ==, length);

    JSONObject imageObject = getJSONObjectFromObject(&jsonObject, "image");
    assert_true(isJsonObjectOk(&windowObject));
    bool isEnabled = getJsonObjectOptBoolean(&imageObject, "enabled", false);
    assert_true(isEnabled);

    bool isDeletedKeyPresent = jsonObjectHasKey(&imageObject, "deleted");
    bool isTargetKeyPresent = jsonObjectHasKey(&imageObject, "target");
    assert_true(isDeletedKeyPresent);
    assert_false(isTargetKeyPresent);

    JSONObject textObject = getJSONObjectFromObject(&jsonObject, "text");
    assert_true(isJsonObjectOk(&textObject));
    double hOffset = getJsonObjectOptDouble(&textObject, "hOffset", 4.12);
    assert_double(25.30, ==, hOffset);

    double vOffset = getJsonObjectOptDouble(&textObject, "vOffset", 12.12);
    assert_double(12.12, ==, vOffset);

    deleteJSONObject(&jsonObject);
    return MUNIT_OK;
}

static MunitResult parseJsonArrayTest(const MunitParameter params[], void *data) {
    char jsonBuffer[128] = {0};
    strcpy(jsonBuffer, "[\"one\", 2, 3.33, null, 45677889900, true, [false, 12, \"text\"], {}]");

    JSONTokener jsonTokener = getJSONTokener(jsonBuffer, strlen(jsonBuffer));
    JSONArray jsonArray = jsonArrayParse(&jsonTokener);
    assert_true(isJsonArrayOk(&jsonArray));

    char *stringVal = getJsonArrayString(&jsonArray, 0);
    char *stringValOpt = getJsonArrayOptString(&jsonArray, 1, "default String");
    assert_string_equal("one", stringVal);
    assert_string_equal("default String", stringValOpt);

    int32_t intValue = getJsonArrayInt(&jsonArray, 1);
    int32_t intValueOpt = getJsonArrayOptInt(&jsonArray, 56, 456);
    assert_int32(2, ==, intValue);
    assert_int32(456, ==, intValueOpt);

    double doubleVal = getJsonArrayDouble(&jsonArray, 2);
    double doubleValOpt = getJsonArrayOptDouble(&jsonArray, 3, -45.12);
    assert_double(3.33, ==, doubleVal);
    assert_double(-45.12, ==, doubleValOpt);

    assert_true(isJsonArrayValueNull(&jsonArray, 3));
    assert_false(isJsonArrayValueNull(&jsonArray, 0));

    int64_t longValue = getJsonArrayLong(&jsonArray, 4);
    int64_t longValueOpt = getJsonArrayOptLong(&jsonArray, 5, 1234);
    assert_int64(45677889900, ==, longValue);
    assert_int64(1234, ==, longValueOpt);

    bool boolValue = getJsonArrayBoolean(&jsonArray, 5);
    bool boolValueOpt = getJsonArrayOptBoolean(&jsonArray, 3, false);
    assert_true(boolValue);
    assert_false(boolValueOpt);

    JSONArray innerArray = getJSONArrayFromArray(&jsonArray, 6);
    assert_true(isJsonArrayOk(&jsonArray));
    assert_uint32(3, ==, getJsonArrayLength(&innerArray));

    JSONObject innerObject = getJSONObjectFromArray(&jsonArray, 7);
    assert_true(isJsonArrayOk(&jsonArray));
    assert_uint32(0, ==, getJsonObjectLength(&innerObject));

    deleteJSONArray(&jsonArray);
    return MUNIT_OK;
}

static char *validJsonFormats[] = {
        "{\"qwerty\":false,}",
        "{\"a\":[0,]}",
        "{\"a\":[0],}",
        "{\"qwerty\":654,}",
        "{\"qwerty\":\"asdfgh\",}",
        "{  \t \"qwerty\":\t\"asdfgh\",}",
        "{\"a\":1, \t   \"b\":2,  \t }",
        "{/*multi line comment*/\"a\":1, \"b\":2, }",
        "{\"a\":1, \"b\":2,} //single line comment",
        "{\"var\":tr}",

        NULL
};

static MunitParameterEnum jsonTestParameters_1[] = {
        {.name = "validJsonFormat", .values = validJsonFormats},
        END_OF_PARAMETERS
};

static MunitTest jsonParseTests[] = {
        {.name = "Test OK - should correctly parse empty json", .test = emptyJsonTest},
        {.name = "Test OK - should correctly parse json primitive values", .test = primitiveValueJsonTest},
        {.name = "Test OK - should correctly parse json text(quoted) values", .test = textValueJsonTest},
        {.name = "Test OK - check valid json formats", .test = validFormatJsonTest, .parameters = jsonTestParameters_1},
        {.name = "Test FAIL - check bad json formats", .test = badFormatJsonTest},
        {.name = "Test OK - should correctly parse nested json", .test = parseJsonTest},
        {.name = "Test OK - should get optional values", .test = parseJsonOptTest},
        {.name = "Test OK - should correctly parse json array", .test = parseJsonArrayTest},

        END_OF_TESTS
};

static const MunitSuite jsonParseTestSuite = {
        .prefix = "JSON Parse: ",
        .tests = jsonParseTests,
        .suites = NULL,
        .iterations = 1,
        .options = MUNIT_SUITE_OPTION_NONE
};
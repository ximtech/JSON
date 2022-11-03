# JSON

[![tests](https://github.com/ximtech/JSON/actions/workflows/cmake-ci.yml/badge.svg)](https://github.com/ximtech/JSON/actions/workflows/cmake-ci.yml)
[![codecov](https://codecov.io/gh/ximtech/JSON/branch/main/graph/badge.svg?token=0ScIsutp3U)](https://codecov.io/gh/ximtech/JSON)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/293fcd59c381460c99896911d3e5609c)](https://www.codacy.com/gh/ximtech/JSON/dashboard)

Full feature library for parsing, editing and creating JSON strings. Written in C and suitable for embedded system.
User-friendly interface and high level functions, provide easy manipulating with JSON.

## Features

- Object-Oriented like solution
- All in one:
    - Parsing and validating
    - Editing existing json
    - Create new json string
- Strings are not copied. The reference of original string is returned
- No limit for nested levels in array or json objects.
- Advanced json type checking and validation 
- Easy to free resources after work is done
- Easy to integrate. Single header file `JSON.h`
- Extra lightweight design and low memory consumption

### Add as CPM project dependency

How to add CPM to the project, check the [link](https://github.com/cpm-cmake/CPM.cmake)

```cmake
CPMAddPackage(
        NAME JSON
        GITHUB_REPOSITORY ximtech/JSON
        GIT_TAG origin/main)

target_link_libraries(${PROJECT_NAME} JSON)
```

```cmake
add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})
# For Clion STM32 plugin generated Cmake use 
target_link_libraries(${PROJECT_NAME}.elf JSON)
```
## Usage
- ***JSON Parsing***
```c
    static const char *const testJson =
            "{\n"
            "  \"squadName\": \"Super hero squad\",\n"
            "  \"homeTown\": \"Metro City\",\n"
            "  \"formed\": 2016,\n"
            "  \"active\": true,\n"
            "  \"members\": [\n"
            "    {\n"
            "      \"name\": \"Molecule Man\",\n"
            "      \"age\": 29,\n"
            "      \"secretIdentity\": \"Dan Jukes\",\n"
            "      \"powers\": [\n"
            "        \"Radiation resistance\",\n"
            "        \"Turning tiny\",\n"
            "        \"Radiation blast\"\n"
            "      ]\n"
            "    }\n"
            "  ]\n"
            "}";

    char buffer[512] = {0};
    strcpy(buffer, testJson);

    JSONTokener jsonTokener = getJSONTokener(buffer, strlen(buffer));   // create tokener from string
    JSONObject rootObject = jsonObjectParse(&jsonTokener);              // create root object
    if (!isJsonObjectOk(&rootObject)) {
        printf("JSON is not valid. Error code: [%d]\n", rootObject.jsonTokener->jsonStatus);
        return EXIT_FAILURE;
    }

    // Get values from json root object
    char *squadName = getJsonObjectString(&rootObject, "squadName");
    assert(isJsonObjectOk(&rootObject));    // validate that value present

    char *homeTown = getJsonObjectString(&rootObject, "homeTown");
    assert(isJsonObjectOk(&rootObject));

    int32_t formed = getJsonObjectInt(&rootObject, "formed");
    assert(isJsonObjectOk(&rootObject));

    char *secretBase = getJsonObjectOptString(&rootObject, "secretBase", "Metro City");    // if value not present, default value will be returned, no need to check
    bool active = getJsonObjectBoolean(&rootObject, "active");
    assert(isJsonObjectOk(&rootObject));

    // Get inner array
    JSONArray membersArray = getJSONArrayFromObject(&rootObject, "members");
    assert(isJsonObjectOk(&rootObject));

    // Get object from array
    JSONObject heroObject = getJSONObjectFromArray(&membersArray, 0);   // receiving object from array
    assert(isJsonArrayOk(&membersArray));

    char *name = getJsonObjectString(&heroObject, "name");
    assert(isJsonObjectOk(&heroObject));

    int32_t age = getJsonObjectOptInt(&heroObject, "age", 18);  // Optional age value
    char *secretIdentity = getJsonObjectString(&heroObject, "secretIdentity");
    assert(isJsonObjectOk(&heroObject));

    // Last array of strings
    JSONArray powersArray = getJSONArrayFromObject(&heroObject, "powers");
    assert(isJsonObjectOk(&heroObject));

    // Print received data
    printf("Squad name: [%s]\n", squadName);
    printf("Home town: [%s]\n", homeTown);
    printf("Formed in: [%d]\n", formed);
    printf("Secret base: [%s]\n", secretBase);
    printf("Is active: [%s]\n", active ? "true" : "false");

    printf("Hero name: [%s]\n", name);
    printf("Age: [%d]\n", age);
    printf("ID: [%s]\n", secretIdentity);

    printf("Powers: \n");
    for (int i = 0; i < getJsonArrayLength(&powersArray); i++) {
        char *superPower = getJsonArrayString(&powersArray, i);
        assert(isJsonArrayOk(&powersArray));
        printf("  [%s]\n", superPower);
    }

    deleteJSONObject(&rootObject);  // only root object is required, all nested data will be released
```

- ***JSON edit/update***
```c
    const char *testJson = "{\"name\":\"John\", \"age\":30, \"car\":null}";

    char buffer[128] = {0};
    strcpy(buffer, testJson);

    JSONTokener jsonTokener = getJSONTokener(buffer, strlen(buffer));   // create tokener from string
    JSONObject rootObject = jsonObjectParse(&jsonTokener);              // create root object
    if (!isJsonObjectOk(&rootObject)) {
        printf("JSON is not valid. Error code: [%d]\n", rootObject.jsonTokener->jsonStatus);
        return EXIT_FAILURE;
    }

    jsonObjectPut(&rootObject, "age", "25");    // change existing value
    jsonObjectPut(&rootObject, "married", "true");    // add new value

    JSONArray jsonArray = createJsonArray(&jsonTokener);    // create array and add values
    jsonArrayPut(&jsonArray, "1");
    jsonArrayPut(&jsonArray, "2");
    jsonArrayPut(&jsonArray, "3");
    jsonObjectAddArray(&rootObject, "values", &jsonArray);  // attach array to root object

    char resBuffer[128] = {0};
    jsonObjectToStringPretty(&rootObject, resBuffer, 3, 0);   // set pretty print with idents - 3, and root level - 0
    printf("%s", resBuffer);

        /* Result: {
       "car": null,
       "name": "John",
       "married": true,
       "values": [
          1,
          2,
          3
       ],
       "age": 25
    }*/
        
    deleteJSONObject(&rootObject);  // free resources
```

- ***JSON create***
```c
    JSONTokener jsonTokener = createEmptyJSONTokener(); // create empty tokener
    JSONObject rootObject = createJsonObject(&jsonTokener);  // create root Object "{}"
    jsonObjectPut(&rootObject, "key1", "1234");     // add values
    jsonObjectPut(&rootObject, "key2", "text value");
    jsonObjectPut(&rootObject, "key3", "12.22");
    jsonObjectPut(&rootObject, "key4", "5368.32e-3");
    jsonObjectPut(&rootObject, "key5", "null");
    jsonObjectPut(&rootObject, "key6", "true");

    JSONObject innerObject = createJsonObject(&jsonTokener);    // create nested object
    jsonObjectPut(&innerObject, "innerKey1", "6789");
    jsonObjectPut(&innerObject, "innerKey2", "text value 2");
    jsonObjectPut(&innerObject, "innerKey3", "null");
    jsonObjectAddObject(&rootObject, "innerObject", &innerObject); // attach to root object

    JSONArray innerArray = createJsonArray(&jsonTokener);   // create nested array
    jsonArrayPut(&innerArray, "10");    // add some values
    jsonArrayPut(&innerArray, "1.34");
    jsonArrayPut(&innerArray, "abcdf");
    jsonArrayPut(&innerArray, "false");
    jsonObjectAddArray(&rootObject, "innerArray", &innerArray); // attach to root

    char buffer[512] = {0};
    jsonObjectToStringPretty(&rootObject, buffer, 3, 0);    // for compact json string, use jsonObjectToString()

    printf("%s", buffer); // Result:
        /*{
       "innerObject": {
          "innerKey2": "text value 2",
          "innerKey3": null,
          "innerKey1": 6789
       },
       "innerArray": [
          10,
          1.34,
          "abcdf",
          false
       ],
       "key1": 1234,
       "key4": 5368.32e-3,
       "key2": "text value",
       "key5": null,
       "key3": 12.22,
       "key6": true
    }*/

    deleteJSONObject(&rootObject);  // free resources
```

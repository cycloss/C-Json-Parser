
#include "jsonParser.h"
#include <stdio.h>

//TODO write a readme

void printStatus(void* jsonStructure) {
    hashMap* meta = getValueForKey(jsonStructure, "meta");
    int* status = getValueForKey(meta, "status");
    printf("Status: %d\n", *status);
}

void printSlug(void* jsonStructure) {
    arrayList* data = getValueForKey(jsonStructure, "data");
    hashMap* firstData = getItemAt(data, 0);
    char* slug = getValueForKey(firstData, "slug");
    printf("Slug: %s\n", slug);
}

int main() {

    jsonBundle* bundle = parseJson("testJsonFiles/simple.json");
    bool* fooVal = getValueForKey(bundle->structure, "foo");
    puts(*fooVal == 0 ? "false" : "true");
    arrayList* bazVal = getValueForKey(bundle->structure, "baz");
    char* secondItem = getItemAt(bazVal, 1);
    puts(secondItem);
    hashMap* quuzVal = getValueForKey(bundle->structure, "quuz");
    void* corgeVal = getValueForKey(quuzVal, "corge");
    puts(corgeVal ? "Corge val was not null" : "Corge val was null");
    freeJsonBundle(bundle);

    jsonBundle* bundle2 = parseJson("testJsonFiles/testjson.json");
    printStatus(bundle2->structure);
    printSlug(bundle2->structure);
    freeJsonBundle(bundle2);
}
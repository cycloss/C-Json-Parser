
#include "jsonParser.h"
#include <stdio.h>

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
    void* json = parseJson("testjson.json");
    printStatus(json);
    printSlug(json);
}
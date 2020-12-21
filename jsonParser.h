#ifndef JSON_PARSE_H
#define JSON_PARSE_H

#include <arrayList.h>
#include <hashMap.h>

typedef struct {
    //the root of the json structure, either an arrayList of hashMap
    void* structure;
    //do not attempt to modify or memory leaks will occur. Required because of opaqueness of void pointers.
    arrayList* _allocatedMemory;
} jsonBundle;

/**
 * NOT thread safe due to use of global variable memList in jsonParser.c
 */
jsonBundle* parseJson(char* fileName);

/**
 * Essential that this function is called on the jsonBundle pointer returned from parseJson function, or massive memory leaks will occur
 */
void freeJsonBundle(jsonBundle* bundlep);

#endif
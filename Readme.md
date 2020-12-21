# A simple Json parsing library

In simple terms, the library translates a valid json file into a structure of nested array lists and hash maps.
</br>

## Dependencies:

* C Generic Hash Map
* C Generic Linked List
* C Generic Array List
* C Utilities

All of these can be found on my github [page](https://github.com/lucas979797?tab=repositories).
</br>

## A simple example

```json
{
    "foo": true,
    "bar": 123,
    "baz": [
        "qux",
        "quux"
    ],
    "quuz": {
        "corge": null
    }
}
```

Calling `parseJson(simple.json)` will return a `jsonBundle*`. This jsonBundle contains a pointer to the root element of the json structure (member: `structure`); which is either an `arrayList*` or `hashMap*` (stored as `void*` to enable genericity). It also contains a private variable containing a list of all memory allocated during the parsing, which is needed to properly free the parsed structure and prevent memory leaks (see below).

The root element of `simple.json` will be a `hashMap` with four keys:

1. foo
2. bar
3. baz
4. quuz

To print out the value in foo, call `getValueForKey(structure, "foo")`, which will return a `void*` which can be casted to a `bool*` and then printed:

```c
jsonBundle* bundle = parseJson("testJsonFiles/simple.json");
bool* fooVal = getValueForKey(bundle->structure, "foo");
puts(*fooVal == 0 ? "false" : "true");
```

Similarly for bar:

```c
jsonBundle* bundle = parseJson("testJsonFiles/simple.json");
int* barVal = getValueForKey(bundle->structure, "bar");
printf("%d\n", *barVal);
```

For baz, because its value is an array, it will be represented in `structure` as an `arrayList`. You will therefore have to first get the pointer to this list and then use `arrayList` methods to get the enclosed values:

```c
jsonBundle* bundle = parseJson("testJsonFiles/simple.json");
arrayList* bazVal = getValueForKey(bundle->structure, "baz");
char* secondItem = getItemAt(bazVal, 1);
puts(secondItem);
```

For quuz, because its value is another json object, it will be represented in `structure` as a further `hashMap`. You will therefore have to first get the pointer to this map and then use `getValueForKey(structure, "foo")` again to get the enclosed values. It you wanted to get the value for 'corge':

```c
jsonBundle* bundle = parseJson("testJsonFiles/simple.json");
hashMap* quuzVal = getValueForKek(bundle->structure, "quuz");
void* corgeVal = getValueForKey(quuzVal, "corge");
puts(corgeVal ? "Corge val was not null" : "Corge val was null");
```

</br>

## Freeing the jsonBundle

The function `freeJsonBundle(jsonBundle* bundle)` MUST be called on the bundle returned by `parseJson` or memory leaks will occur. After this has been called, the json structure and all of its values will be freed from memory and will therefore not be accessible anymore.

```c
freeJsonBundle(bundle);
```

</br>

## Other considerations

* The parser itself is not thread safe, although I could make it so if there was demand.

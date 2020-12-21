#include "jsonParser.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utilities.h>

//TODO add support for floats, remove number and add integer and floatingPoint, will have to change number parsing
//currently only supports integers
//TODO add a table of malloc'd pointers to enable the structure to be freed
//TODO track line number in parsing, possibly modify the token struct to include line num

typedef enum {
    openObject,
    closeObject,
    comma,
    colon,
    openArray,
    closeArray,
    string,
    number,
    boolean,
    null,
} tokenType;

char* names[] = {
    "openObject",
    "closeObject",
    "comma",
    "colon",
    "openArray",
    "closeArray",
    "string",
    "number",
    "boolean",
    "null"
};

typedef struct {
    tokenType type;
    char* rawText;
} token;

typedef struct {
    arrayList* tokens;
    //index of token waiting to be parsed
    int currentIndex;
} parseInfo;

typedef enum {
    simple,
    arrLst,
    hshMp
} pointerType;

typedef struct {
    void* mallocdPointer;
    pointerType type;
} memoryBundle;

//an array list of memory bundles
arrayList* memList;

void processFile(FILE* in, arrayList* al);
char* generateStringLiteral(FILE* in);
void generateLiteral(FILE* in, token* tp);
void generateNumberToken(FILE* in, token* tp);
void generateBoolToken(FILE* in, token* tp);
void generateNullToken(FILE* in, token* tp);

hashMap* generateObject(parseInfo* pi);
void optionallyConsumeComma(parseInfo* pi);
void* generateBool(parseInfo* pi);
void* generateString(parseInfo* pi);
void* generateNumber(parseInfo* pi);
arrayList* generateArray(parseInfo* pi);
void* processArrayValue(parseInfo* pi);
void* processObjValue(parseInfo* pi);
void consumeColon(parseInfo* pi);
bool match(tokenType type, int args, ...);

void printToken(void* tkn);

void addMemoryBundleToList(void* mallocdPointer, pointerType type);

void* fatalError(char* formatString, ...) {
    printf("Fatal error: ");
    va_list args;
    va_start(args, formatString);
    vprintf(formatString, args);
    puts("");
    exit(1);
    return NULL;
}

//returns a list of tokens
arrayList* lexJson(char* jsonFile) {

    FILE* in = fopen(jsonFile, "r");

    if (!in) {
        fatalError("Could not open the json file at: %s", jsonFile);
    }
    //TODO consider moving this to global scope so the list of processed tokens can be printed in the event of an error
    arrayList* al = createArrayList();
    processFile(in, al);
    fclose(in);
    return al;
}

void processFile(FILE* in, arrayList* al) {
    int lineNumber = 0;
    for (char c = fgetc(in); c != EOF; c = fgetc(in)) {
        token* t = malloc(sizeof(token));
        t->rawText = NULL;
        switch (c) {
            case '\n':
                lineNumber++;
            case ' ':
                free(t);
                continue;
            case '{':
                t->type = openObject;
                break;
            case '}':
                t->type = closeObject;
                break;
            case '[':
                t->type = openArray;
                break;
            case ']':
                t->type = closeArray;
                break;
            case ':':
                t->type = colon;
                break;
            case ',':
                t->type = comma;
                break;
            case '"':
                t->type = string;
                char* str = generateStringLiteral(in);
                t->rawText = str;
                break;
            default:
                fseek(in, -1, SEEK_CUR);
                generateLiteral(in, t);
        }
        // printToken(t);
        appendToAl(al, t);
    }
}

char* generateStringLiteral(FILE* in) {
    stringBuilder* sb = createStringBuilder();
    for (char c = fgetc(in); c != '"'; c = fgetc(in)) {
        if (c == EOF) {
            fatalError("Unterminated string");
        } else {
            appendCharToBuilder(sb, c);
        }
    }
    char* str = sb->string;
    addMemoryBundleToList(str, simple);
    freeBuilder(sb, false);
    return str;
}

//generates boolean, number or null token
void generateLiteral(FILE* in, token* tp) {
    char c = fgetc(in);
    fseek(in, -1, SEEK_CUR);
    if (c <= '9' && c >= '0' || c == '-') {
        generateNumberToken(in, tp);
    } else if (c == 'f' || c == 't') {
        generateBoolToken(in, tp);
    } else if (c == 'n') {
        generateNullToken(in, tp);
    } else {
        fatalError("Unknown value starting with char: %c", c);
    }
}

void generateNumberToken(FILE* in, token* tp) {
    stringBuilder* sb = createStringBuilder();
    for (char c = fgetc(in); c != EOF; c = fgetc(in)) {
        if (c <= '9' && c >= '0' || c == '.' || c == '-') {
            appendCharToBuilder(sb, c);
        } else {
            break;
        }
    }
    addMemoryBundleToList(sb->string, simple);
    tp->rawText = sb->string;
    tp->type = number;
    freeBuilder(sb, false);
    fseek(in, -1, SEEK_CUR);
}

void generateBoolToken(FILE* in, token* tp) {
    char c = fgetc(in);
    stringBuilder* sb = createStringBuilder();
    appendCharToBuilder(sb, c);
    char* match;
    if (c == 'f') {
        match = "false";
    } else {
        match = "true";
    }

    for (char c = fgetc(in), i = 1; i < strlen(match); c = fgetc(in), i++) {
        if (c == match[i]) {
            appendCharToBuilder(sb, c);
        } else {
            fatalError("Malformed boolean value");
        }
    }
    addMemoryBundleToList(sb->string, simple);
    tp->rawText = sb->string;
    tp->type = boolean;
    freeBuilder(sb, false);
}

void generateNullToken(FILE* in, token* tp) {
    char* match = "null";
    for (int i = 0; i < 4; i++) {
        char c = getc(in);
        if (c != match[i]) {
            fatalError("Malformed null value");
        }
    }
    tp->type = null;
}

//returns either an arrayList or HashMap depending on whether the json started with an array or an object
void* generateJsonStructure(arrayList* tokens) {
    parseInfo pi = { tokens, 0 };
    token* first = getItemAt(tokens, pi.currentIndex++);
    if (first->type == openObject) {
        return generateObject(&pi);
    } else if (first->type == openArray) {
        return generateArray(&pi);
    } else {
        return fatalError("Invalid start token");
    }
}

hashMap* generateObject(parseInfo* pi) {
    hashMap* object = createHashMap(strHash, strComp);
    addMemoryBundleToList(object, hshMp);
    token* t = getItemAt(pi->tokens, pi->currentIndex++);
    while (match(t->type, 1, string)) {
        void* val = processObjValue(pi);
        addToMap(object, t->rawText, val, true);
        optionallyConsumeComma(pi);
        t = getItemAt(pi->tokens, pi->currentIndex++);
    }
    pi->currentIndex--;
    token* closet = getItemAt(pi->tokens, pi->currentIndex++);
    if (match(closet->type, 1, closeObject)) {
        return object;
    } else {
        return fatalError("Expected object close but got type: %s", names[closet->type]);
    }
}

void optionallyConsumeComma(parseInfo* pi) {
    token* possibleComma = getItemAt(pi->tokens, pi->currentIndex);
    if (possibleComma->type == comma) {
        pi->currentIndex++;
    }
}

void* processObjValue(parseInfo* pi) {
    consumeColon(pi);
    token* valueToken = getItemAt(pi->tokens, pi->currentIndex);
    tokenType type = valueToken->type;
    if (type == boolean) {
        return generateBool(pi);
    } else if (type == string) {
        return generateString(pi);
    } else if (type == number) {
        return generateNumber(pi);
    } else if (type == null) {
        pi->currentIndex++;
        return NULL;
    } else if (type == openObject) {
        pi->currentIndex++;
        return generateObject(pi);
    } else if (type == openArray) {
        pi->currentIndex++;
        return generateArray(pi);
    } else {
        return fatalError("Illegal token for value: %s", names[type]);
    }
}

void consumeColon(parseInfo* pi) {
    token* colonToken = getItemAt(pi->tokens, pi->currentIndex++);
    if (colonToken->type != colon) {
        fatalError("Expecting colon but got: %s", names[colonToken->type]);
    }
}

void* generateBool(parseInfo* pi) {
    token* boolToken = getItemAt(pi->tokens, pi->currentIndex++);
    bool* boolVal = malloc(sizeof(bool));
    addMemoryBundleToList(boolVal, simple);
    if (strcmp("true", boolToken->rawText) == 0) {
        *boolVal = true;
    } else if (strcmp("false", boolToken->rawText) == 0) {
        *boolVal = false;
    } else {
        fatalError("Expected bool text but raw value was %s", boolToken->rawText);
    }
    return boolVal;
}

void* generateString(parseInfo* pi) {
    token* strToken = getItemAt(pi->tokens, pi->currentIndex++);
    return strToken->rawText;
}

void* generateNumber(parseInfo* pi) {
    token* numToken = getItemAt(pi->tokens, pi->currentIndex++);
    int parsednum = atoi(numToken->rawText);
    //TODO add error handling to disern when atoi failed and whether it was just a zero
    int* nump = malloc(sizeof(int));
    addMemoryBundleToList(nump, simple);
    *nump = parsednum;
    return nump;
}

arrayList* generateArray(parseInfo* pi) {
    arrayList* array = createArrayList();
    addMemoryBundleToList(array, arrLst);
    //don't want to increment here because need to see what it was in the prcessArrayValue method
    token* t = getItemAt(pi->tokens, pi->currentIndex);
    while (match(t->type, 5, string, number, boolean, null, openObject)) {
        void* val = processArrayValue(pi);
        appendToAl(array, val);
        optionallyConsumeComma(pi);
        t = getItemAt(pi->tokens, pi->currentIndex);
    }
    token* closet = getItemAt(pi->tokens, pi->currentIndex++);
    if (match(closet->type, 1, closeArray)) {
        return array;
    } else {
        return fatalError("Expected array close but got type: %s", names[closet->type]);
    }
}

void* processArrayValue(parseInfo* pi) {
    token* t = getItemAt(pi->tokens, pi->currentIndex);
    tokenType type = t->type;
    if (type == string) {
        return generateString(pi);
    } else if (type == number) {
        return generateNumber(pi);
    } else if (type == boolean) {
        return generateBool(pi);
    } else if (type == null) {
        return NULL;
    } else if (type == openObject) {
        pi->currentIndex++;
        return generateObject(pi);
    } else {
        return fatalError("Expected array value but got token type: %s", names[type]);
    }
}

bool match(tokenType type, int args, ...) {
    va_list ls;
    va_start(ls, args);

    for (int i = 0; i < args; i++) {
        if (type == va_arg(ls, tokenType)) {
            return true;
        }
    }

    return false;
}

void printToken(void* tkn) {
    token* tp = (token*)tkn;
    printf("Token type: %s, raw: %s\n", names[tp->type], tp->rawText ? tp->rawText : "null");
}

void printTokenList(arrayList* tokenList) {
    iterateListItems(tokenList, printToken);
}

jsonBundle* parseJson(char* fileName) {
    memList = createArrayList();
    arrayList* tokenList = lexJson(fileName);
    if (!memList) {
        fatalError("Memory list not allocated");
    }
    // printTokenList(tokenList);
    void* structure = generateJsonStructure(tokenList);
    freeAl(tokenList, true);
    jsonBundle* jb = malloc(sizeof(jsonBundle));
    *jb = (jsonBundle) { structure, memList };
    memList = NULL;
    return jb;
}

void addMemoryBundleToList(void* mallocdPointer, pointerType type) {
    memoryBundle* mb = malloc(sizeof(memoryBundle));
    *mb = (memoryBundle) { mallocdPointer, type };
    appendToAl(memList, mb);
}

void freeJsonBundle(jsonBundle* bundlep) {
    for (int i = 0; i < getSize(bundlep->_allocatedMemory); i++) {
        memoryBundle* mb = getItemAt(bundlep->_allocatedMemory, i);
        pointerType type = mb->type;
        if (type == simple) {
            free(mb->mallocdPointer);
        } else if (type == arrLst) {
            freeAl(mb->mallocdPointer, false);
        } else {
            freeMap(mb->mallocdPointer, false);
        }
        free(mb);
    }
    freeAl(bundlep->_allocatedMemory, false);
    free(bundlep);
}
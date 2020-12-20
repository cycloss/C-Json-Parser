#include <arrayList.h>
#include <stack.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utilities.h>

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
    eof,
} tokenType;

typedef struct {
    tokenType type;
    char* rawText;
} token;

void processFile(FILE* in, arrayList* al);
char* getString(FILE* in);
void generateLiteral(FILE* in, token* tp);
void generateNumberToken(FILE* in, token* tp);
void generateBoolToken(FILE* in, token* tp);
void generateNullToken(FILE* in, token* tp);

void fatalError(char* message) {
    printf("Fatal error: %s\n", message);
    exit(1);
}

//returns a list of tokens
arrayList* lexJson(char* jsonFile) {

    FILE* in = fopen(jsonFile, "r");

    if (!in) {
        fatalError("Could not open specified json file");
    }
    //TODO consider moving this to global scope so the list of processed tokens can be printed in the event of an error
    arrayList* al = createArrayList();
    processFile(in, al);
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
                char* str = getString(in);
                t->rawText = str;
                break;
            default:
                fseek(in, -1, SEEK_CUR);
                generateLiteral(in, t);
        }
        appendToAl(al, t);
    }
    token* t = malloc(sizeof(token));
    t->type = eof;
    appendToAl(al, t);
}

char* getString(FILE* in) {
    stringBuilder* sb = createStringBuilder();
    for (char c = fgetc(in); c != '"'; c = fgetc(in)) {
        if (c == EOF) {
            fatalError("Unterminated string");
        } else {
            appendCharToBuilder(sb, c);
        }
    }
    char* str = sb->string;
    freeBuilder(sb, false);
    return str;
}

//generates boolean, number or null token
void generateLiteral(FILE* in, token* tp) {
    char c = fgetc(in);
    fseek(in, -1, SEEK_CUR);
    if (c <= '9' && c >= '0') {
        generateNumberToken(in, tp);
    } else if (c == 'f' || c == 't') {
        generateBoolToken(in, tp);
    } else if (c == 'n') {
        generateNullToken(in, tp);
    } else {
        char uk[40];
        snprintf(uk, 40, "Unknown value starting with char: %c", c);
        fatalError(uk);
    }
}

void generateNumberToken(FILE* in, token* tp) {
    stringBuilder* sb = createStringBuilder();
    for (char c = fgetc(in); c != EOF; c = fgetc(in)) {
        if (c <= '9' && c >= '0' || c == '.') {
            appendCharToBuilder(sb, c);
        } else {
            break;
        }
    }
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
    "null",
    "eof"
};

void printToken(void* tkn) {
    token* tp = (token*)tkn;
    printf("Token type: %s, raw: %s\n", names[tp->type], tp->rawText ? tp->rawText : "null");
}

void printTokenList(arrayList* tokenList) {
    iterateListItems(tokenList, printToken);
}

int main() {
    arrayList* tokenList = lexJson("testjson.json");
    printTokenList(tokenList);
}


test: jsonParser.c
	gcc -lGenericStructures jsonParser.c -o jsonParser
	./jsonParser
	
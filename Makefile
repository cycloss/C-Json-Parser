

test: jsonParser.c jsonParserTest.c
	gcc -lGenericStructures jsonParser.c jsonParserTest.c -o jsonParserTest
	./jsonParserTest
	
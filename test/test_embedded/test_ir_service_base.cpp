#ifdef __CDT_PARSER__
#define UNIT_TEST
#endif

#ifdef UNIT_TEST
#include <Arduino.h>
#include <unity.h>


#define DEBUG_LEVEL_VERBOSE
#include "IrParser.h"

#define INFRARED_LED_PIN 12 // ############# CHECK IF THE LED OR TRANSISTOR (RECOMMENDED) IS ACTUALLY ATTACHED TO THIS PIN
#define RECV_PIN  D1 // an IR detector/demodulatord
#define UDP_PORT 9131
#define HTTP_SERVER_PORT 80
#define ITACH_SERVER_PORT 4998
#define LIRCD_SERVER_PORT 4999
#define DEBUG_LISTENER_PORT 22

String case1 = "GC-IRE,55000,22,340,24,156,24,158,23,92,23,157,23,157,23,92,23,156,23,156,23,157,23,157,23,156,23,310,23,311,23,156,24,156,23,157,23,1100";
String case2 = "GC-IRE,55000,22,340,24,156B,23,92BBCBBBBB,23,310DBBB,23,1100";
//22,340,24,156,24,158,23,92,23,157,23,157,23,92,23,156,23,156,23,157,23,157,23,156,23,310,23,311,23,156,24,156,23,157,23,1100
//22,340,24,156,24,156,23,92BBCBBBBB,23,310DBBB,23,1100
unsigned int values[] = { 22,340,24,156,24,158,23,92,23,157,23,157,23,92,23,156,23,156,23,157,23,157,23,156,23,310,23,311,23,156,24,156,23,157,23,1100 };

//ServerEngine server(INFRARED_LED_PIN,RECV_PIN,80, 4999, 22,UDP_PORT, 4998);
bool isSameWithTolerance(unsigned int val1, unsigned int val2)
{
	DEBUG_PRINTF("Tolerance 3, checking %d - %d <= 3 <=> abs(%d)=%d <=3?\n",val1,val2,(long)val1-(long)val2,abs((long)val1-(long)val2));
	return abs((long)val1-(long)val2)<=3;
}
//Service itach(UDP_PORT, ITACH_SERVER_PORT, INFRARED_LED_PIN,RECV_PIN, DEBUG_LISTENER_PORT, "Welcome debug client!");
void test_ir_parsing_compressed(void) {
	iTach::IrParser parser(13,case1);
	iTach::IrParser parser2(13,case2);

	while (parser.getNext())
	{
		DEBUG_RESET_WDT_IF_PRINT;
		TEST_ASSERT_TRUE(parser2.getNext());
		DEBUG_PRINTF("Checking pair [%d, %d] ?= [%d,%d]", parser.value1, parser.value2, parser2.value1, parser2.value2);
		TEST_ASSERT_TRUE_MESSAGE(isSameWithTolerance(parser.value1,parser2.value1), "1st values in the pair are different");
		TEST_ASSERT_TRUE_MESSAGE(isSameWithTolerance(parser.value2,parser2.value2),"2nd values in the pair are different");
	}
	TEST_ASSERT_EQUAL_UINT_MESSAGE(parser.totalNumberOfValues,parser2.totalNumberOfValues, "Number of values found in uncompressed vs compressed");
}
void test_ir_parsing_simple(void) {
	iTach::IrParser parser(13,case1);
	uint8_t pos = 0;
	while (parser.getNext())
	{
		DEBUG_RESET_WDT_IF_PRINT;
		DEBUG_PRINTF("Checking pair [%d, %d] ?= [%d,%d]\n", parser.value1, parser.value2, values[pos], values[pos+1]);
		TEST_ASSERT_EQUAL_UINT_MESSAGE(parser.value1,values[pos], "1st values in the pair are different");
		TEST_ASSERT_EQUAL_UINT_MESSAGE(parser.value2,values[pos+1],"2nd values in the pair are different");
		pos+=2;
	}
	TEST_ASSERT_EQUAL_UINT_MESSAGE(pos,parser.totalNumberOfValues, "Number of values found is wrong");
}
void test_ir_compression_works()
{
	iTach::IrParser parser(13,case1);
	while (parser.getNext())
	{
		// parse/build compressed string
	}
	DEBUG_PRINTF("Comparing non compressed string with compressed string\n%s\n%s\n",
			parser.getCompressedString().c_str(),
			case2.substring(12).c_str());
	TEST_ASSERT_TRUE_MESSAGE(parser.getCompressedString() == case2.substring(12), "Compressed string is wrong");
}
void test_ir_decompress_compress_works()
{
	iTach::IrParser parser(13,case2);
	while (parser.getNext())
	{
		// parse compressed string
	}
	DEBUG_PRINTF("Comparing non compressed string with compressed string\n%s\n%s\n",
			parser.getCompressedString().c_str(),
			case2.substring(12).c_str());
	TEST_ASSERT_TRUE_MESSAGE(parser.getCompressedString() == case2.substring(12), "Compressed string is wrong");
}


void setup() {
    UNITY_BEGIN();    // IMPORTANT LINE!
    RUN_TEST(test_ir_parsing_simple);
    RUN_TEST(test_ir_compression_works);
    RUN_TEST(test_ir_parsing_compressed);
    RUN_TEST(test_ir_decompress_compress_works);

    UNITY_END(); // stop unit testing

}



void loop() {

}

#endif

#ifndef UNITY_FRAMEWORK_H
#define UNITY_FRAMEWORK_H

#include "unity_internals.h"

void setUp(void);
void tearDown(void);

#define UNITY_BEGIN() UnityBegin(__FILE__)
#define UNITY_END()   UnityEnd()

#define RUN_TEST(func) UnityDefaultTestRun(func, #func, __LINE__)

#define TEST_ASSERT_EQUAL_INT(expected, actual) UnityAssertEqualNumber((int32_t)(expected), (int32_t)(actual), NULL, __LINE__)
#define TEST_ASSERT_EQUAL_UINT32(expected, actual) UnityAssertEqualNumber((int32_t)(expected), (int32_t)(actual), NULL, __LINE__)
#define TEST_ASSERT_EQUAL_HEX32(expected, actual) UnityAssertEqualNumber((int32_t)(expected), (int32_t)(actual), NULL, __LINE__)
#define TEST_ASSERT_EQUAL_STRING(expected, actual) UnityAssertEqualString((const char*)(expected), (const char*)(actual), NULL, __LINE__)
#define TEST_ASSERT_EQUAL_PTR(expected, actual) UnityAssertEqualPtr((const void*)(expected), (const void*)(actual), NULL, __LINE__)

#define TEST_ASSERT_TRUE(condition) UnityAssertEqualNumber(1, (condition) ? 1 : 0, "Expected TRUE", __LINE__)
#define TEST_ASSERT_FALSE(condition) UnityAssertEqualNumber(0, (condition) ? 1 : 0, "Expected FALSE", __LINE__)
#define TEST_ASSERT_NULL(pointer) UnityAssertEqualPtr(NULL, (const void*)(pointer), "Expected NULL", __LINE__)
#define TEST_ASSERT_NOT_NULL(pointer) if ((pointer) == NULL) UnityAssertEqualPtr((void*)1, NULL, "Expected NOT NULL", __LINE__)

#endif /* UNITY_FRAMEWORK_H */

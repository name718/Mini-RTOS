#include "unity.h"
#include <string.h>

UNITY_STORAGE_T Unity;

void UnityBegin(const char* filename)
{
    Unity.TestFile = filename;
    Unity.CurrentTestName = NULL;
    Unity.CurrentTestLineNumber = 0;
    Unity.NumberOfTests = 0;
    Unity.TestFailures = 0;
    Unity.TestIgnores = 0;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;

    printf("\n=======================================================\n");
    printf("        Unity C Unit Testing Framework (MiniRTOS)       \n");
    printf("=======================================================\n");
}

int UnityEnd(void)
{
    printf("\n----------------------- SUMMARY -----------------------\n");
    printf("%u Tests, %u Failures, %u Ignored\n", 
           (unsigned int)Unity.NumberOfTests, 
           (unsigned int)Unity.TestFailures, 
           (unsigned int)Unity.TestIgnores);
    if (Unity.TestFailures == 0)
    {
        printf("RESULT: SUCCESS (ALL TESTS PASSED)\n");
    }
    else
    {
        printf("RESULT: FAIL (%u TESTS FAILED)\n", (unsigned int)Unity.TestFailures);
    }
    printf("=======================================================\n\n");
    return (int)Unity.TestFailures;
}

void UnityConcludeTest(void)
{
    if (Unity.CurrentTestFailed)
    {
        Unity.TestFailures++;
        printf(" -> [FAIL]\n");
    }
    else if (Unity.CurrentTestIgnored)
    {
        Unity.TestIgnores++;
        printf(" -> [IGNORE]\n");
    }
    else
    {
        printf(" -> [PASS]\n");
    }
}

void UnityDefaultTestRun(void (*Func)(void), const char* FuncName, const int FuncLineNum)
{
    Unity.CurrentTestName = FuncName;
    Unity.CurrentTestLineNumber = (uint32_t)FuncLineNum;
    Unity.NumberOfTests++;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;

    printf("[TEST %u] %s (line %d)", (unsigned int)Unity.NumberOfTests, FuncName, FuncLineNum);
    fflush(stdout);

    if (setjmp(Unity.AbortFrame) == 0)
    {
        setUp();
        Func();
    }
    tearDown();

    UnityConcludeTest();
}

void UnityAssertEqualNumber(const int32_t expected, const int32_t actual, const char* msg, const uint32_t lineNumber)
{
    if (expected != actual)
    {
        Unity.CurrentTestFailed = 1;
        printf("\n   [FAIL at line %u] Expected %d, got %d. %s", 
               (unsigned int)lineNumber, (int)expected, (int)actual, (msg ? msg : ""));
        longjmp(Unity.AbortFrame, 1);
    }
}

void UnityAssertEqualString(const char* expected, const char* actual, const char* msg, const uint32_t lineNumber)
{
    if (expected == NULL || actual == NULL || strcmp(expected, actual) != 0)
    {
        Unity.CurrentTestFailed = 1;
        printf("\n   [FAIL at line %u] Expected string '%s', got '%s'. %s", 
               (unsigned int)lineNumber, (expected ? expected : "NULL"), (actual ? actual : "NULL"), (msg ? msg : ""));
        longjmp(Unity.AbortFrame, 1);
    }
}

void UnityAssertEqualPtr(const void* expected, const void* actual, const char* msg, const uint32_t lineNumber)
{
    if (expected != actual)
    {
        Unity.CurrentTestFailed = 1;
        printf("\n   [FAIL at line %u] Expected pointer %p, got %p. %s", 
               (unsigned int)lineNumber, expected, actual, (msg ? msg : ""));
        longjmp(Unity.AbortFrame, 1);
    }
}

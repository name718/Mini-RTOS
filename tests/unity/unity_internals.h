#ifndef UNITY_INTERNALS_H
#define UNITY_INTERNALS_H

#include <stdio.h>
#include <setjmp.h>
#include <stdint.h>
#include <stddef.h>

#define UNITY_INT_WIDTH 32

typedef struct UNITY_STORAGE_T
{
    const char* TestFile;
    const char* CurrentTestName;
    uint32_t CurrentTestLineNumber;
    uint32_t NumberOfTests;
    uint32_t TestFailures;
    uint32_t TestIgnores;
    uint32_t CurrentTestFailed;
    uint32_t CurrentTestIgnored;
    jmp_buf AbortFrame;
} UNITY_STORAGE_T;

extern UNITY_STORAGE_T Unity;

void UnityBegin(const char* filename);
int UnityEnd(void);
void UnityConcludeTest(void);
void UnityDefaultTestRun(void (*Func)(void), const char* FuncName, const int FuncLineNum);

void UnityAssertEqualNumber(const int32_t expected, const int32_t actual, const char* msg, const uint32_t lineNumber);
void UnityAssertEqualString(const char* expected, const char* actual, const char* msg, const uint32_t lineNumber);
void UnityAssertEqualPtr(const void* expected, const void* actual, const char* msg, const uint32_t lineNumber);

#endif /* UNITY_INTERNALS_H */

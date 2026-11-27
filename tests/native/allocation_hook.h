#pragma once
// Parse all library/system headers before replacing only source-level malloc calls.
#include <GithubReleaseOTA.h>
#include <utility>
void* otaTestMalloc(size_t size);
void otaTestFree(void* pointer);
#define malloc otaTestMalloc
#define free otaTestFree

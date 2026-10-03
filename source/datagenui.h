#pragma once
#include "datagen.h"
#include "stopwatch.h"

namespace DATAGEN {

void PrintProgress(const DatagenStats& stats, int targetPositions, Stopwatch &stopwatch, int threads);
void PrintSummary(const DatagenStats& stats, int targetPositions, Stopwatch &stopwatch, int threads);
void PrintUsage();

} // namespace DATAGEN

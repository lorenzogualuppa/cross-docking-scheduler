#ifndef CD_GREEDY_HH
#define CD_GREEDY_HH
#include "CD_Data.hh"

// Greedy ERT (Earliest Release Time):
//   Schedule inbound trucks in non-decreasing order of release time.
//   Schedule outbound trucks in non-decreasing order of loading time (SPT).
void GreedyCDSolver(const CD_Input& in, CD_Output& out);

#endif


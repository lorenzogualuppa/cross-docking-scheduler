#include "CD_Greedy.hh"
#include <algorithm>
#include <numeric>

void GreedyCDSolver(const CD_Input& in, CD_Output& out)
{
  // --- Inbound sequence: ERT, ties broken by SPT ---
  vector<unsigned> in_seq(in.InboundTrucks());
  iota(in_seq.begin(), in_seq.end(), 0);  // {0, 1, ..., n-1}

  sort(in_seq.begin(), in_seq.end(), [&](unsigned a, unsigned b)
  {
    if (in.ReleaseTime(a) != in.ReleaseTime(b))
      return in.ReleaseTime(a) < in.ReleaseTime(b);  // primary: earliest release
    return in.UnloadTime(a) < in.UnloadTime(b);      // tie-break: shortest unload
  });

  // --- Outbound sequence: SPT (shortest load time first) ---
  vector<unsigned> out_seq(in.OutboundTrucks());
  iota(out_seq.begin(), out_seq.end(), 0);  // {0, 1, ..., m-1}

  sort(out_seq.begin(), out_seq.end(), [&](unsigned a, unsigned b)
  {
    return in.LoadTime(a) < in.LoadTime(b);
  });

  out.SetInboundSeq(in_seq);
  out.SetOutboundSeq(out_seq);
}

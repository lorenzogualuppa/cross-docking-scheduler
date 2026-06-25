#ifndef CD_DATA_HH
#define CD_DATA_HH
#include <iostream>
#include <vector>
#include <string>
#include <cassert>

using namespace std;

class CD_Input
{
  friend ostream& operator<<(ostream& os, const CD_Input& in);
public:
  CD_Input(string file_name);
  unsigned InboundTrucks()  const { return n_inbound; }
  unsigned OutboundTrucks() const { return n_outbound; }
  unsigned ReleaseTime(unsigned i)        const { return release_time[i]; }
  unsigned UnloadTime(unsigned i)         const { return unload_time[i]; }
  unsigned LoadTime(unsigned j)           const { return load_time[j]; }
  unsigned TransferTime(unsigned i, unsigned j) const { return transfer_time[i][j]; }

private:
  unsigned n_inbound;   // number of inbound trucks
  unsigned n_outbound;  // number of outbound trucks
  vector<unsigned> release_time;  // r[i]: earliest arrival of inbound truck i
  vector<unsigned> unload_time;   // p[i]: unloading duration of inbound truck i
  vector<unsigned> load_time;     // q[j]: loading duration of outbound truck j
  vector<vector<unsigned>> transfer_time; // t[i][j]: transfer time goods i -> j
};


class CD_Output
{
  friend ostream& operator<<(ostream& os, const CD_Output& out);
public:
  explicit CD_Output(const CD_Input& in);
  CD_Output& operator=(const CD_Output& other);
  void Reset();

  // Solution access
  unsigned InboundOrder(unsigned pos)  const { return inbound_seq[pos]; }
  unsigned OutboundOrder(unsigned pos) const { return outbound_seq[pos]; }
  const vector<unsigned>& InboundSeq()  const { return inbound_seq; }
  const vector<unsigned>& OutboundSeq() const { return outbound_seq; }

  // Solution building
  void SetInboundSeq(const vector<unsigned>& seq)  { inbound_seq  = seq; }
  void SetOutboundSeq(const vector<unsigned>& seq) { outbound_seq = seq; }

  // Objective function
  unsigned ComputeMakespan() const;

private:
  const CD_Input& in;
  vector<unsigned> inbound_seq;   // permutation of inbound  trucks (order of docking)
  vector<unsigned> outbound_seq;  // permutation of outbound trucks (order of docking)
};
#endif
#ifndef CD_DATA_HH
#define CD_DATA_HH
#include <iostream>
#include <vector>
#include <string>

#include <utility>

using namespace std;

class CD_Input
{
  friend ostream& operator<<(ostream& os, const CD_Input& in);
public:
  CD_Input(string file_name);
  unsigned InboundTrucks()   const { return n_inbound; }
  unsigned OutboundTrucks()  const { return n_outbound; }
  unsigned InboundDoors()    const { return d_inbound; }
  unsigned OutboundDoors()   const { return d_outbound; }
  unsigned ReleaseTime(unsigned i)              const { return release_time[i]; }
  unsigned UnloadTime(unsigned i)               const { return unload_time[i]; }
  unsigned LoadTime(unsigned j)                 const { return load_time[j]; }
  unsigned TransferTime(unsigned i, unsigned j) const { return transfer_time[i][j]; }

private:
  unsigned n_inbound;   // number of inbound trucks
  unsigned n_outbound;  // number of outbound trucks
  unsigned d_inbound;   // number of inbound doors
  unsigned d_outbound;  // number of outbound doors
  vector<unsigned> release_time;
  vector<unsigned> unload_time;
  vector<unsigned> load_time;
  vector<vector<unsigned>> transfer_time;
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

  // Door assignment access
  unsigned InboundDoor(unsigned pos)  const { return inbound_door[pos]; }
  unsigned OutboundDoor(unsigned pos) const { return outbound_door[pos]; }

  // Solution building
  void SetInboundSeq(const vector<unsigned>& seq)  { inbound_seq  = seq; }
  void SetOutboundSeq(const vector<unsigned>& seq) { outbound_seq = seq; }
  void SetInboundDoor(const vector<unsigned>& d)   { inbound_door  = d; }
  void SetOutboundDoor(const vector<unsigned>& d)  { outbound_door = d; }

  
  // Objective function
  vector<unsigned> ComputeGoodsReadyFromCurrentInbound() const;
  unsigned ComputeMakespan() const;
  unsigned ComputeLowerBound() const;
  pair<double, double> ComputeAverageWaits() const;

private:
  const CD_Input& in;
  vector<unsigned> inbound_seq;
  vector<unsigned> outbound_seq;
  vector<unsigned> inbound_door;   // door assigned at each position of inbound_seq
  vector<unsigned> outbound_door;  // door assigned at each position of outbound_seq
};
#endif
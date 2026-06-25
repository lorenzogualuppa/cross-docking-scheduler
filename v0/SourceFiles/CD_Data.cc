#include "CD_Data.hh"
#include <fstream>
#include <algorithm>
#include <numeric>

CD_Input::CD_Input(string file_name)
{
    const unsigned MAX_DIM = 256;
    unsigned i, j;
    char ch, buffer[MAX_DIM];

    ifstream is(file_name);
    if (!is)
    {
        cerr << "Cannot open input file: " << file_name << endl;
        exit(1);
    }

    // InboundTrucks = N;
    is >> buffer >> ch >> n_inbound >> ch;
    // OutboundTrucks = M;
    is >> buffer >> ch >> n_outbound >> ch;

    release_time.resize(n_inbound);
    unload_time.resize(n_inbound);
    load_time.resize(n_outbound);
    transfer_time.resize(n_inbound, vector<unsigned>(n_outbound));

    // ReleaseTime
    is.ignore(MAX_DIM, '[');
    for (i = 0; i < n_inbound; i++)
        is >> release_time[i] >> ch;

    // UnloadTime
    is.ignore(MAX_DIM, '[');
    for (i = 0; i < n_inbound; i++)
        is >> unload_time[i] >> ch;

    // LoadTime 
    is.ignore(MAX_DIM, '[');
    for (j = 0; j < n_outbound; j++)
        is >> load_time[j] >> ch;

    // TransferTime
    is.ignore(MAX_DIM, '[');
    is >> ch;  // consume first '|'
    for (i = 0; i < n_inbound; i++)
        for (j = 0; j < n_outbound; j++)
            is >> transfer_time[i][j] >> ch;  // value then ',' or '|'
}

ostream& operator<<(ostream& os, const CD_Input& in)
{
  os << "InboundTrucks  = " << in.n_inbound  << ";\n";
  os << "OutboundTrucks = " << in.n_outbound << ";\n\n";

  os << "ReleaseTime = [";
  for (unsigned i = 0; i < in.n_inbound; i++)
    os << in.release_time[i] << (i+1 < in.n_inbound ? ", " : "];\n");

  os << "UnloadTime  = [";
  for (unsigned i = 0; i < in.n_inbound; i++)
    os << in.unload_time[i] << (i+1 < in.n_inbound ? ", " : "];\n");

  os << "LoadTime    = [";
  for (unsigned j = 0; j < in.n_outbound; j++)
    os << in.load_time[j] << (j+1 < in.n_outbound ? ", " : "];\n");

  os << "TransferTime = [|\n";
  for (unsigned i = 0; i < in.n_inbound; i++)
  {
    os << "  ";
    for (unsigned j = 0; j < in.n_outbound; j++)
      os << in.transfer_time[i][j] << (j+1 < in.n_outbound ? "," : "|");
    os << "\n";
  }
  os << "];\n";

  return os;
}



CD_Output::CD_Output(const CD_Input& my_in)
  : in(my_in),
    inbound_seq(my_in.InboundTrucks()),
    outbound_seq(my_in.OutboundTrucks())
{
  Reset();
}

CD_Output& CD_Output::operator=(const CD_Output& other)
{
  inbound_seq  = other.inbound_seq;
  outbound_seq = other.outbound_seq;
  return *this;
}

void CD_Output::Reset()
{
  // Identity permutations: 0, 1, 2, ...
  iota(inbound_seq.begin(),  inbound_seq.end(),  0);
  iota(outbound_seq.begin(), outbound_seq.end(), 0);
}

unsigned CD_Output::ComputeMakespan() const
{
  // Step 1: compute finish time of each inbound truck
  vector<unsigned> finish_unload(in.InboundTrucks());
  unsigned inbound_door_time = 0;

  for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
  {
    finish_unload[inbound_seq[pos]] = max(inbound_door_time, in.ReleaseTime(inbound_seq[pos])) + in.UnloadTime(inbound_seq[pos]);
    inbound_door_time = finish_unload[inbound_seq[pos]];
  }


  // Step 2: compute completion time of each outbound truck
  unsigned outbound_door_time = 0;
  unsigned makespan = 0;

  for (unsigned pos = 0; pos < in.OutboundTrucks(); pos++)
  {
    unsigned goods_ready = 0;
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
      goods_ready = max(goods_ready, finish_unload[i] + in.TransferTime(i, outbound_seq[pos]));

    outbound_door_time = max(outbound_door_time, goods_ready) + in.LoadTime(outbound_seq[pos]);
    makespan = max(makespan, outbound_door_time);
  }

  return makespan;
}

ostream& operator<<(ostream& os, const CD_Output& out)
{
  os << "Inbound  sequence: ";
  for (unsigned pos = 0; pos < out.in.InboundTrucks(); pos++)
    os << out.inbound_seq[pos] << (pos+1 < out.in.InboundTrucks() ? " -> " : "\n");

  os << "Outbound sequence: ";
  for (unsigned pos = 0; pos < out.in.OutboundTrucks(); pos++)
    os << out.outbound_seq[pos] << (pos+1 < out.in.OutboundTrucks() ? " -> " : "\n");

  return os;
}

#include "CD_Data.hh"
#include <fstream>
#include <algorithm>
#include <numeric>

CD_Input::CD_Input(string file_name)
{
    const int MAX_DIM = 256;
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
    // InboundDoors = D_IN;
    is >> buffer >> ch >> d_inbound >> ch;
    // OutboundDoors = D_OUT;
    is >> buffer >> ch >> d_outbound >> ch;

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
            is >> transfer_time[i][j] >> ch;
}

ostream& operator<<(ostream& os, const CD_Input& in)
{
  os << "InboundTrucks  = " << in.n_inbound  << ";\n";
  os << "OutboundTrucks = " << in.n_outbound << ";\n";
  os << "InboundDoors   = " << in.d_inbound  << ";\n";
  os << "OutboundDoors  = " << in.d_outbound << ";\n\n";

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
    outbound_seq(my_in.OutboundTrucks()),
    inbound_door(my_in.InboundTrucks(), 0),
    outbound_door(my_in.OutboundTrucks(), 0)
{
  Reset();
}

CD_Output& CD_Output::operator=(const CD_Output& other)
{
  inbound_seq   = other.inbound_seq;
  outbound_seq  = other.outbound_seq;
  inbound_door  = other.inbound_door;
  outbound_door = other.outbound_door;
  return *this;
}

void CD_Output::Reset()
{
  iota(inbound_seq.begin(),  inbound_seq.end(),  0);
  iota(outbound_seq.begin(), outbound_seq.end(), 0);
  fill(inbound_door.begin(),  inbound_door.end(),  0);
  fill(outbound_door.begin(), outbound_door.end(), 0);
}

vector<unsigned> CD_Output::ComputeGoodsReadyFromCurrentInbound() const
{
  vector<unsigned> finish_unload(in.InboundTrucks(), 0);
  vector<unsigned> door_free_in(in.InboundDoors(), 0);

  for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
  {
    assert(inbound_door[pos] < in.InboundDoors());

    unsigned truck = inbound_seq[pos];
    unsigned door  = inbound_door[pos];

    unsigned start_unload = max(door_free_in[door], in.ReleaseTime(truck));
    unsigned finish = start_unload + in.UnloadTime(truck);

    finish_unload[truck] = finish;
    door_free_in[door] = finish;
  }

  vector<unsigned> goods_ready(in.OutboundTrucks(), 0);
  for (unsigned j = 0; j < in.OutboundTrucks(); j++)
  {
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
    {
      if (in.TransferTime(i, j) > 0)
      {
        goods_ready[j] = max(goods_ready[j], finish_unload[i] + in.TransferTime(i, j));
      }
    }
  }

  return goods_ready;
}


unsigned CD_Output::ComputeMakespan() const
{
  vector<unsigned> goods_ready = ComputeGoodsReadyFromCurrentInbound();
  vector<unsigned> door_free_out(in.OutboundDoors(), 0);
  unsigned makespan = 0;

  for (unsigned pos = 0; pos < in.OutboundTrucks(); pos++)
  {
    unsigned truck = outbound_seq[pos];
    unsigned door  = outbound_door[pos];

    unsigned start_load = max(door_free_out[door], goods_ready[truck]);
    unsigned finish = start_load + in.LoadTime(truck);

    door_free_out[door] = finish;
    makespan = max(makespan, finish);
  }

  return makespan;
}

// LB1 (Critical Path Outbound) e LB4 (Critical Path Inbound)

unsigned CD_Output::ComputeLowerBound() const
{
    // LB1 — Critical Path per ogni outbound truck j
    unsigned lb1 = 0;
    for (unsigned j = 0; j < in.OutboundTrucks(); j++)
    {
        unsigned best_j = UINT_MAX;
        for (unsigned i = 0; i < in.InboundTrucks(); i++)
        {
            if (in.TransferTime(i, j) > 0) 
            {
                unsigned candidate = in.ReleaseTime(i) + in.UnloadTime(i) + in.TransferTime(i, j);
                if (candidate < best_j)
                    best_j = candidate;
            }
        }
        if (best_j < UINT_MAX)
            lb1 = max(lb1, best_j + in.LoadTime(j));
    }

    // LB4 — Critical Path per ogni inbound truck i
    unsigned lb4 = 0;
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
    {
        unsigned min_transfer = UINT_MAX;
        for (unsigned j = 0; j < in.OutboundTrucks(); j++)
        {
            if (in.TransferTime(i, j) > 0 && in.TransferTime(i, j) < min_transfer)
                min_transfer = in.TransferTime(i, j);
        }
        if (min_transfer < UINT_MAX) 
        {
            unsigned path_i = in.ReleaseTime(i) + in.UnloadTime(i) + min_transfer;
            if (path_i > lb4)
                lb4 = path_i;
        }
    }
    
    return max({lb1, lb4});
}

pair<double, double> CD_Output::ComputeAverageWaits() const
{
    //Inbound waits
    vector<unsigned> door_free_in(in.InboundDoors(), 0);
    double total_wait_in = 0;

    for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
    {
        unsigned truck = inbound_seq[pos];
        unsigned start_unload = max(door_free_in[inbound_door[pos]], in.ReleaseTime(truck));
        total_wait_in += (start_unload - in.ReleaseTime(truck));

        unsigned finish = start_unload + in.UnloadTime(truck);
        door_free_in[inbound_door[pos]] = finish;
    }

    //Outbound waits
    vector<unsigned> goods_ready = ComputeGoodsReadyFromCurrentInbound();
    vector<unsigned> door_free_out(in.OutboundDoors(), 0);
    double total_wait_out = 0;

    for (unsigned pos = 0; pos < in.OutboundTrucks(); pos++)
    {
        unsigned start_load = max(door_free_out[outbound_door[pos]], goods_ready[outbound_seq[pos]]);
        total_wait_out += (start_load - goods_ready[outbound_seq[pos]]);

        unsigned finish = start_load + in.LoadTime(outbound_seq[pos]);
        door_free_out[outbound_door[pos]] = finish;
    }

    return { total_wait_in  / in.InboundTrucks(), total_wait_out / in.OutboundTrucks() };
}

ostream& operator<<(ostream& os, const CD_Output& out)
{
  unsigned max_in = std::min((unsigned)20, out.in.InboundTrucks());
  os << "Inbound sequence : ";
  for (unsigned i = 0; i < max_in; i++)
    os << out.inbound_seq[i] << " ";
  os << endl;

  os << "Inbound doors    : ";
  for (unsigned i = 0; i < max_in; i++)
    os << out.inbound_door[i] << " ";
  os << endl;

  unsigned max_out = std::min((unsigned)20, out.in.OutboundTrucks());
  os << "Outbound sequence: ";
  for (unsigned j = 0; j < max_out; j++)
    os << out.outbound_seq[j] << " ";
  os << endl;

  os << "Outbound doors   : ";
  for (unsigned j = 0; j < max_out; j++)
    os << out.outbound_door[j] << " ";
  os << endl;

  return os;
}
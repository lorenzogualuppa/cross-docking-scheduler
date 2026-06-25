#include "CD_Greedy.hh"
#include <algorithm>
#include <numeric>


void GreedyCDSolver(const CD_Input& in, CD_Output& out)
{  
  // --- A. Sequencing Inbound: ERT primario, tie-break SPT ---
  vector<unsigned> in_seq(in.InboundTrucks());
  iota(in_seq.begin(), in_seq.end(), 0);

  sort(in_seq.begin(), in_seq.end(), [&](unsigned a, unsigned b)
  {
    // Regola Primaria (ERT - Earliest Release Time)
    if (in.ReleaseTime(a) != in.ReleaseTime(b))
      return in.ReleaseTime(a) < in.ReleaseTime(b);
      
    // Regola di Spareggio (SPT - Shortest Processing Time)
    return in.UnloadTime(a) < in.UnloadTime(b);
  });

  out.SetInboundSeq(in_seq);

  // --- B. Routing Inbound: Politica EAD (Earliest Available Door) ---
  vector<unsigned> door_free_in(in.InboundDoors(), 0);
  vector<unsigned> assigned_door_in(in.InboundTrucks(), 0);

  for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
  {
    unsigned best_door = 0;
    for (unsigned d = 1; d < in.InboundDoors(); d++) {
      if (door_free_in[d] < door_free_in[best_door]) {
        best_door = d;
      }
    }

    unsigned truck = in_seq[pos];
    
    // Calcolo tempistica fisica
    door_free_in[best_door] = max(door_free_in[best_door], in.ReleaseTime(truck)) + in.UnloadTime(truck);
      
    assigned_door_in[pos] = best_door;
  }

  out.SetInboundDoor(assigned_door_in);

  // --- A. Sequencing Outbound: LDF (Least Dependent First) + SPT ---  
  vector<unsigned> out_seq(in.OutboundTrucks());
  iota(out_seq.begin(), out_seq.end(), 0);

  sort(out_seq.begin(), out_seq.end(), [&](unsigned a, unsigned b)
  {
    unsigned dipendenze_a = 0, dipendenze_b = 0;
    
    // Contiamo le dipendenze
    for(unsigned i = 0; i < in.InboundTrucks(); i++) 
    {
        if (in.TransferTime(i, a) > 0)
        {
          dipendenze_a++;
        }
        if (in.TransferTime(i, b) > 0) 
        {
          dipendenze_b++;
        }
    }
    
    // Regola Primaria (LDF)
    if (dipendenze_a != dipendenze_b) 
    {
        return dipendenze_a < dipendenze_b; 
    }
    
    // Regola di Spareggio (SPT)
    return in.LoadTime(a) < in.LoadTime(b); 
  });

  out.SetOutboundSeq(out_seq);

  // --- B. Routing Outbound: EAD basato sui vincoli di merce ---
  
  // Calcolo disponibilità merce
  vector<unsigned> goods_ready = out.ComputeGoodsReadyFromCurrentInbound();
  
  vector<unsigned> door_free_out(in.OutboundDoors(), 0);
  vector<unsigned> assigned_door_out(in.OutboundTrucks(), 0);

  for (unsigned pos = 0; pos < in.OutboundTrucks(); pos++)
  {
    unsigned best_door = 0;
    for (unsigned d = 1; d < in.OutboundDoors(); d++)
    {
      if (door_free_out[d] < door_free_out[best_door])
      {
        best_door = d;
      }
    }

    unsigned truck = out_seq[pos];
    
    // Calcolo tempistica fisica
    door_free_out[best_door] = max(door_free_out[best_door], goods_ready[truck]) + in.LoadTime(truck);
    
    assigned_door_out[pos] = best_door;
  }

  out.SetOutboundDoor(assigned_door_out);
}
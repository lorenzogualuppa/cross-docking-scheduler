#include "CD_Greedy.hh"
#include <algorithm>
#include <numeric>

// ---------------------------------------------------------------------------
// GreedyCDSolver (v0.1.1 Variant)
//
// Questo algoritmo greedy pianifica le operazioni di un terminal Cross-Docking.
// Il terminal ha porte dedicate all'arrivo merci (Inbound) e alla partenza (Outbound).
// La logica si divide in due fasi principali per ogni lato (Inbound e Outbound):
// 1. SEQUENCING: In che ordine gestiamo i camion (la "fila" teorica).
// 2. ROUTING/SCHEDULING: A quale porta mandiamo il camion e a che ora inizia l'operazione.
// ---------------------------------------------------------------------------
void GreedyCDSolver(const CD_Input& in, CD_Output& out)
{
  // =========================================================================
  // FASE 1: GESTIONE INBOUND (Camion in arrivo)
  // =========================================================================
  
  // --- A. Sequencing Inbound: ERT primario, tie-break SPT ---
  // Vogliamo rispettare l'ordine naturale di arrivo dei camion.
  
  // 1. Creiamo un vettore vuoto 'in_seq' che conterrà gli ID di tutti i camion in ingresso.
  vector<unsigned> in_seq(in.InboundTrucks());
  
  // 2. Riempiamo il vettore con numeri da 0 fino a (n_inbound - 1). 
  // La funzione 'iota' è una comodità del C++ per non dover fare un ciclo for.
  iota(in_seq.begin(), in_seq.end(), 0);

  // 3. Ordiniamo la "fila" dei camion Inbound usando una funzione personalizzata (lambda).
  sort(in_seq.begin(), in_seq.end(), [&](unsigned a, unsigned b)
  {
    // Regola Primaria (ERT - Earliest Release Time):
    // Se i camion 'a' e 'b' arrivano in orari diversi, diamo la priorità a chi arriva prima.
    if (in.ReleaseTime(a) != in.ReleaseTime(b))
      return in.ReleaseTime(a) < in.ReleaseTime(b);
      
    // Regola di Spareggio (SPT - Shortest Processing Time):
    // Se arrivano nello stesso esatto momento, gestiamo per primo quello che si 
    // scarica più in fretta, per liberare rapidamente la porta.
    return in.UnloadTime(a) < in.UnloadTime(b);
  });

  // 4. Salviamo la sequenza appena calcolata nell'oggetto di output.
  out.SetInboundSeq(in_seq);

  // --- B. Routing Inbound: Politica EAD (Earliest Available Door) ---
  // Ora che sappiamo in che ordine gestire i camion, dobbiamo assegnarli fisicamente alle porte.
  
  // Vettore che traccia l'orario in cui ogni porta Inbound tornerà libera. 
  // All'inizio (tempo 0) tutte le porte sono libere.
  vector<unsigned> door_free_in(in.InboundDoors(), 0);
  
  // Vettore che ricorderà a quale porta abbiamo mandato ogni singolo camion.
  vector<unsigned> assigned_door_in(in.InboundTrucks(), 0);

  // Scorriamo i camion nell'ordine stabilito dal Sequencing (in_seq)
  for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
  {
    // Partiamo supponendo che la porta migliore (quella che si libera prima) sia la numero 0
    unsigned best_door = 0;
    
    // Controlliamo tutte le altre porte per vedere se ce n'è una che si libera ancora prima
    for (unsigned d = 1; d < in.InboundDoors(); d++) {
      if (door_free_in[d] < door_free_in[best_door]) {
        best_door = d; // Trovata una porta migliore, aggiorniamo l'indice
      }
    }

    // Prendiamo l'ID reale del camion che stiamo processando
    unsigned truck = in_seq[pos];
    
    // CALCOLO TEMPISTICA FISICA:
    // Il camion inizia a scaricare solo quando si verificano ENTRAMBE le condizioni:
    // 1. La porta è libera (door_free_in[best_door])
    // 2. Il camion è fisicamente arrivato al terminal (in.ReleaseTime(truck))
    // La funzione max() prende il maggiore tra questi due tempi. A questo sommiamo 
    // il tempo necessario allo scarico per calcolare quando la porta tornerà libera.
    door_free_in[best_door] = max(door_free_in[best_door], in.ReleaseTime(truck)) + in.UnloadTime(truck);
      
    // Salviamo l'informazione di quale porta ha gestito questo camion
    assigned_door_in[pos] = best_door;
  }

  // Passiamo tutte le assegnazioni delle porte Inbound all'oggetto di output
  out.SetInboundDoor(assigned_door_in);


  // =========================================================================
  // FASE 2: GESTIONE OUTBOUND (Camion in partenza)
  // =========================================================================

  // --- A. Sequencing Outbound: LDF (Least Dependent First) + SPT ---
  // A differenza del v0.1 che guarda a 'goods_ready', qui guardiamo alla complessità 
  // strutturale. Vogliamo far partire per primi i camion che aspettano merce da 
  // pochi camion Inbound, riducendo così il rischio di tenere le porte bloccate.
  
  vector<unsigned> out_seq(in.OutboundTrucks());
  iota(out_seq.begin(), out_seq.end(), 0);

  sort(out_seq.begin(), out_seq.end(), [&](unsigned a, unsigned b)
  {
    unsigned dipendenze_a = 0, dipendenze_b = 0;
    
    // Contiamo da quanti camion Inbound riceveranno merce i camion 'a' e 'b'
    // Se il TransferTime è > 0, significa che c'è uno scambio di merce.
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
    
    // Regola Primaria (LDF):
    // Diamo la priorità (mettiamo prima nella lista) al camion che ha Meno dipendenze.
    // Un camion che aspetta merce da 1 solo Inbound è più facile da chiudere rispetto 
    // a uno che aspetta pallet da 10 Inbound diversi.
    if (dipendenze_a != dipendenze_b) 
    {
        return dipendenze_a < dipendenze_b; 
    }
    
    // Regola di Spareggio (SPT):
    // A parità di complessità di network, facciamo passare per primo chi si carica più velocemente.
    return in.LoadTime(a) < in.LoadTime(b); 
  });

  out.SetOutboundSeq(out_seq);

  // --- B. Routing Outbound: EAD basato sui vincoli di merce ---
  // Ora calcoliamo i tempi reali basandoci anche sul flusso fisico della merce nel piazzale.
  
  // goods_ready contiene per ogni camion Outbound l'orario in cui l'ULTIMO pallet 
  // a lui destinato sarà arrivato e trasferito dall'Inbound all'Outbound.
  vector<unsigned> goods_ready = out.ComputeGoodsReadyFromCurrentInbound();
  
  // Inizializziamo le disponibilità delle porte Outbound
  vector<unsigned> door_free_out(in.OutboundDoors(), 0);
  vector<unsigned> assigned_door_out(in.OutboundTrucks(), 0);

  for (unsigned pos = 0; pos < in.OutboundTrucks(); pos++)
  {
    // Cerchiamo la porta Outbound che si libera per prima (stessa logica dell'Inbound)
    unsigned best_door = 0;
    for (unsigned d = 1; d < in.OutboundDoors(); d++)
    {
      if (door_free_out[d] < door_free_out[best_door])
      {
        best_door = d;
      }
    }

    unsigned truck = out_seq[pos];
    
    // CALCOLO TEMPISTICA FISICA (Cruciale!):
    // Il camion inizia a caricare SOLO QUANDO:
    // 1. La porta è libera dal camion precedente (door_free_out[best_door])
    // 2. TUTTA la sua merce ha attraversato il magazzino ed è pronta (goods_ready[truck])
    // Se la porta è libera ma la merce non c'è, la porta rimane in attesa (Idle Time).
    // È qui che la nostra scelta dell'ordinamento LDF mostra la sua forza, cercando di
    // ridurre al minimo questo Idle Time.
    door_free_out[best_door] = max(door_free_out[best_door], goods_ready[truck]) + in.LoadTime(truck);
    
    assigned_door_out[pos] = best_door;
  }

  // Salviamo l'assegnazione finale delle porte Outbound
  out.SetOutboundDoor(assigned_door_out);
}
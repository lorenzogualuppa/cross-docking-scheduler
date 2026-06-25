#!/usr/bin/env python3
"""
Cross-Docking Instance Generator

File naming: cd_n{N:04d}_m{M:04d}_d{D:02d}_{scenario}_d{density}_s{seed}.dzn

Truck sizes  : 10, 20, 30, 50       (same count for inbound and outbound)
Door counts  : 4, 8, 15             (same count for inbound and outbound)

RNG streams use large seed offsets for stable reproducibility:
  rng_release  = Random(seed)           # arrival times
  rng_process  = Random(seed + 1000)    # unload / load times
  rng_transfer = Random(seed + 2000)    # transfer time matrix
Adding/removing calls in one stream never affects the others.

TransferTime[i][j] semantics:
  - 0        : no goods flow from inbound truck i to outbound truck j
  - t > 0    : internal handling/transfer time in minutes (forklift, conveyor, etc.)
               between inbound dock and outbound dock for this pair.

Unità temporale: 1 u.t. = 1 minuto

Densità supportate: 0.25, 0.35, 0.50, 0.75


"""

import random
import os

TRUCK_SIZES = [10, 20, 30, 50]
DOOR_COUNTS = [4, 8, 15]

SEEDS = [1, 2, 3]


# DZN serialisation
def _dzn_array(values):
    return "[" + ",".join(str(v) for v in values) + "]"


def _dzn_matrix(matrix):
    rows = " | ".join(",".join(str(v) for v in row) for row in matrix)
    return "[| " + rows + " |]"


def write_dzn(path, n_in, n_out, d_in, d_out, release, unload, load, transfer):
    lines = [
        f"InboundTrucks  = {n_in};",
        f"OutboundTrucks = {n_out};",
        f"InboundDoors   = {d_in};",
        f"OutboundDoors  = {d_out};",
        f"ReleaseTime    = {_dzn_array(release)};",
        f"UnloadTime     = {_dzn_array(unload)};",
        f"LoadTime       = {_dzn_array(load)};",
        f"TransferTime   = {_dzn_matrix(transfer)};",
    ]
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")


def _transfer_matrix(n_in, n_out, rng, prob_fn, val_fn):
    """
    Build an n_in x n_out transfer matrix.
    prob_fn(i, j) -> float  : probability that cell (i,j) is non-zero
    val_fn(rng)   -> int    : internal transfer TIME (minutes) when non-zero
    Always draws exactly TWO values from rng per cell (one for probability,
    one for the value), regardless of the outcome, so the stream is stable.
    """
    matrix = []
    for i in range(n_in):
        row = []
        for j in range(n_out):
            p = rng.random()              # probability draw
            v = val_fn(rng, i, j)         # value draw (always consumed)
            row.append(v if p < prob_fn(i, j) else 0)
        matrix.append(row)
    return matrix


# Scenario generators

def gen_uniform(n_in, n_out, seed):
    """
    Hub a regime – arrivi distribuiti [0,120], densità 75%.
    Transfer time: magazzino standard, 2–8 min per qualsiasi coppia.
    """
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    release  = [rng_r.randint(0, 120) for _ in range(n_in)]
    unload   = [rng_p.randint(10, 35) for _ in range(n_in)]
    load     = [rng_p.randint(5,  20) for _ in range(n_out)]
    transfer = _transfer_matrix(n_in, n_out, rng_t,
                                prob_fn=lambda i, j: 0.75,
                                val_fn =lambda r, i, j: r.randint(2, 8))
    return release, unload, load, transfer, 75


def gen_sparse(n_in, n_out, seed):
    """
    Hub nicchia – stesse release/process del base, matrice rada 25%.
    Transfer time: poche coppie attive, magazzino standard, 2–8 min.
    """
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    release  = [rng_r.randint(0, 120) for _ in range(n_in)]
    unload   = [rng_p.randint(10, 35) for _ in range(n_in)]
    load     = [rng_p.randint(5,  20) for _ in range(n_out)]
    transfer = _transfer_matrix(n_in, n_out, rng_t,
                                prob_fn=lambda i, j: 0.25,
                                val_fn =lambda r, i, j: r.randint(2, 8))
    return release, unload, load, transfer, 25


def gen_clustered(n_in, n_out, seed, n_clusters=3):
    """
    Hub multi-cliente – intra-cluster 80%, inter-cluster 8%, densità ~35%.
    Transfer time: intra-cluster più veloci (1–5 min, stessi bay contigui),
                   inter-cluster più lenti (5–12 min, bay distanti).
    """
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    release = [rng_r.randint(0, 120) for _ in range(n_in)]
    unload  = [rng_p.randint(10, 35) for _ in range(n_in)]
    load    = [rng_p.randint(5,  20) for _ in range(n_out)]

    in_cl  = [i % n_clusters for i in range(n_in)]
    out_cl = [j % n_clusters for j in range(n_out)]

    def prob_fn(i, j):
        return 0.80 if in_cl[i] == out_cl[j] else 0.08
        
    def val_fn(r, i, j):
        return r.randint(1, 5) if in_cl[i] == out_cl[j] else r.randint(5, 12)
    transfer = _transfer_matrix(n_in, n_out, rng_t, prob_fn, val_fn)

    return release, unload, load, transfer, 35

def gen_asymmetric(n_in, n_out, seed):
    """
    Collo di bottiglia – scarico lento (15-40 min), carico veloce (5-15 min),
    densità 50%.
    Transfer time: magazzino standard, 2–8 min (il bottleneck è unload/load,
    non il trasferimento interno).
    """
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    release  = [rng_r.randint(0, 120) for _ in range(n_in)]
    unload   = [rng_p.randint(15, 40) for _ in range(n_in)]
    load     = [rng_p.randint(5,  15) for _ in range(n_out)]
    transfer = _transfer_matrix(n_in, n_out, rng_t,
                                prob_fn=lambda i, j: 0.50,
                                val_fn =lambda r, i, j: r.randint(2, 8))

    return release, unload, load, transfer, 50


def gen_congested(n_in, n_out, seed):
    """
    Ora di punta – tutti arrivano in [0,30], tempi alti, densità 75%.
    Transfer time: magazzino congestionato → corridoi affollati → 5–15 min.
    """
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    release  = [rng_r.randint(0, 30)   for _ in range(n_in)]
    unload   = [rng_p.randint(10, 35) for _ in range(n_in)]
    load     = [rng_p.randint(5, 20) for _ in range(n_out)]
    transfer = _transfer_matrix(n_in, n_out, rng_t,
                                prob_fn=lambda i, j: 0.75,
                                val_fn =lambda r, i, j: r.randint(5, 15))
    return release, unload, load, transfer, 75


def gen_urgent(n_in, n_out, seed, urgent_frac=0.20):
    """
    Corriere espresso – 20% truck urgenti: arrivo [0,20], scarico [5,15].
    Restanti 80%: arrivo [20,120], scarico [10,35].
    Lato outbound: 20% truck urgenti con carico [5,15], non urgenti [10,25].
    Transfer: urgenti→urgenti 60% (transfer veloce 1–4 min, corsia prioritaria),
              urgenti→normali  5% (transfer standard 3–8 min),
              normali→*       35% (transfer standard 3–8 min).
    """
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    n_urg_in  = max(1, round(n_in  * urgent_frac))
    n_urg_out = max(1, round(n_out * urgent_frac))

    release = [rng_r.randint(0,  20) if i < n_urg_in else rng_r.randint(20, 120)
               for i in range(n_in)]
    unload  = [rng_p.randint(5,  15) if i < n_urg_in else rng_p.randint(10,  35)
               for i in range(n_in)]
    load    = [rng_p.randint(5,  15) if j < n_urg_out else rng_p.randint(10, 25)
               for j in range(n_out)]

    def prob_fn(i, j):
        if i < n_urg_in:
            return 0.60 if j < n_urg_out else 0.05
        return 0.35

    def val_fn(r, i, j):
        # Truck urgenti verso outbound urgenti: corsia dedicata, molto veloce
        if i < n_urg_in and j < n_urg_out:
            return r.randint(1, 4)
        # Tutte le altre coppie: transfer standard
        return r.randint(3, 8)

    transfer = _transfer_matrix(n_in, n_out, rng_t, prob_fn, val_fn)
    
    return release, unload, load, transfer, 35


def gen_mixed(n_in, n_out, seed, n_clusters=3):
    """
    Giornata imprevedibile – cluster + variabilità a 3 velocità.
    Intra-cluster 70%, inter 15%; densità ~50%.
    Transfer time: intra-cluster 1–6 min (dock contigui),
                   inter-cluster 4–12 min (dock distanti).
    Niente outlier esagerati: la variabilità è nel processo (unload/load),
    non nel transfer fisico.
    """
    rng_r = random.Random(seed)
    rng_p = random.Random(seed + 1000)
    rng_t = random.Random(seed + 2000)

    release = [rng_r.randint(0, 120) for _ in range(n_in)]

    _cats = ["fast", "medium", "slow"]
    unload = []
    for _ in range(n_in):
        c = rng_p.choice(_cats)
        unload.append(rng_p.randint(5, 15)  if c == "fast"
                      else rng_p.randint(15, 25) if c == "medium"
                      else rng_p.randint(25, 35))
    load = []
    for _ in range(n_out):
        c = rng_p.choice(_cats)
        load.append(rng_p.randint(5, 15)  if c == "fast"
                    else rng_p.randint(10, 20) if c == "medium"
                    else rng_p.randint(15, 25))

    in_cl  = [i % n_clusters for i in range(n_in)]
    out_cl = [j % n_clusters for j in range(n_out)]

    def prob_fn(i, j):
        return 0.70 if in_cl[i] == out_cl[j] else 0.15
        
    def val_fn(r, i, j):
        return r.randint(1, 6) if in_cl[i] == out_cl[j] else r.randint(4, 12)
    transfer = _transfer_matrix(n_in, n_out, rng_t, prob_fn, val_fn)

    return release, unload, load, transfer, 50


SCENARIOS = {
    "uniform":    gen_uniform,
    "sparse":     gen_sparse,
    "clustered":  gen_clustered,
    "asymmetric": gen_asymmetric,
    "congested":  gen_congested,
    "urgent":     gen_urgent,
    "mixed":      gen_mixed,
}


# Main generation loop

def generate_all(output_dir="Instances"):
    os.makedirs(output_dir, exist_ok=True)
    count = 0
    for n in TRUCK_SIZES:
        for d in DOOR_COUNTS:
            # Evitiamo istanze irrealistiche con più porte che camion
            if d > n:
                continue
                
            for scenario_name, gen_fn in SCENARIOS.items():
                for seed in SEEDS:
                    release, unload, load, transfer, density = gen_fn(n, n, seed)
                    fname = (f"cd_n{n:04d}_m{n:04d}_d{d:02d}_{scenario_name}"
                             f"_d{density}_s{seed}.dzn")
                    write_dzn(os.path.join(output_dir, fname),
                              n, n, d, d,
                              release, unload, load, transfer)
                    count += 1
    print(f"Generated {count} instances in '{output_dir}/'")
    return count
if __name__ == "__main__":
    generate_all()
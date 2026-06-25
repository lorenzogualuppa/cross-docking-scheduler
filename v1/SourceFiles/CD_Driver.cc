#include <iostream>
#include <cstdlib>
#include <iomanip>
#include <vector>
#include <numeric>
#include <algorithm>
#include "CD_Data.hh"
#include "CD_Greedy.hh"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    CD_Input  in(argv[1]);
    CD_Output out(in);

    std::vector<unsigned> init_seq(in.InboundTrucks());
    std::iota(init_seq.begin(), init_seq.end(), 0);
    std::sort(init_seq.begin(), init_seq.end(), [&in](unsigned a, unsigned b) {
        return in.ReleaseTime(a) < in.ReleaseTime(b);
    });

    std::cout << "Initial Arrival  : ";
    unsigned max_init = std::min((unsigned)20, in.InboundTrucks());
    for (unsigned i = 0; i < max_init; i++) {
        std::cout << init_seq[i] << " ";
    }
    std::cout << std::endl;

    GreedyCDSolver(in, out);

    std::cout << out; // Stampa Inbound e Outbound sequences

    auto [avg_wait_in, avg_wait_out] = out.ComputeAverageWaits();

    unsigned lb = out.ComputeLowerBound();
    unsigned makespan = out.ComputeMakespan();

    std::cout << "Lower Bound : " << lb << std::endl;
    std::cout << "Makespan    : " << makespan << std::endl;

    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Avg Wait In  : " << avg_wait_in << std::endl;
    std::cout << "Avg Wait Out : " << avg_wait_out << std::endl;

    std::cout << std::fixed << std::setprecision(2);
    if (lb > 0)
        std::cout << "Gap (%)     : " << 100.0 * (makespan - lb) / (double)lb << "%" << std::endl;
    else
        std::cout << "Gap (%)     : n/a" << std::endl;

    return 0;
}
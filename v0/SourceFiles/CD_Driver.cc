#include <iostream>
#include <cstdlib>
#include "CD_Data.hh"
#include "CD_Greedy.hh"

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    cerr << "Usage: " << argv[0] << " <input_file>" << endl;
    exit(1);
  }

  CD_Input  in(argv[1]);
  CD_Output out(in);

  // Print instance if small enough
  if (in.InboundTrucks() <= 20)
    cout << in << endl;

  GreedyCDSolver(in, out);

  if (in.InboundTrucks() <= 20)
      cout << out << endl;

  cout << "Makespan: " << out.ComputeMakespan() << endl;
  return 0;
}
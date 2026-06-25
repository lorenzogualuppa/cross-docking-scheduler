OPTIONS = -std=c++17 -Wall -Wfatal-errors -O3


CD_Test.exe: CD_Driver.o CD_Data.o CD_Greedy.o
	g++ -o CD_Test.exe CD_Driver.o CD_Data.o CD_Greedy.o


CD_Data.o: CD_Data.cc CD_Data.hh
	g++ $(OPTIONS) -c CD_Data.cc


CD_Greedy.o: CD_Greedy.cc CD_Greedy.hh CD_Data.hh
	g++ $(OPTIONS) -c CD_Greedy.cc


CD_Driver.o: CD_Driver.cc CD_Data.hh CD_Greedy.hh
	g++ $(OPTIONS) -c CD_Driver.cc


clean:
	rm -f CD_Data.o CD_Greedy.o CD_Driver.o CD_Test.exe

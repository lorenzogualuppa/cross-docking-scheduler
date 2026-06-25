# CXX      = g++
# CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# TARGET  = CD_Test
# SOURCES = CD_Data.cc CD_Greedy.cc CD_Driver.cc
# OBJECTS = $(SOURCES:.cc=.o)

# all: $(TARGET)

# $(TARGET): $(OBJECTS)
# 	$(CXX) $(CXXFLAGS) -o $@ $^

# %.o: %.cc
# 	$(CXX) $(CXXFLAGS) -c $<

# clean:
# 	rm -f $(OBJECTS) $(TARGET)

# .PHONY: all clean


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




# IMPORTANTE DA CAPIRE IL MAKEFILE PERCHé DIVERSO DAL PROF IN QUALCOSA

# Le scelte fatte:

# -O3 al posto di -O2 — come il prof

# -Wfatal-errors al posto di -Wextra — si ferma al primo errore come il prof

# -std=c++17 mantenuto — il prof non ce l'ha ma il tuo codice ne ha bisogno per le lambda e altri costrutti moderni

# Ogni regola è esplicita con le sue dipendenze — identico allo stile del prof
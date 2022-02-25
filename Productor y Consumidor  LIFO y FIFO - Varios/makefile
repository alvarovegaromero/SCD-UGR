.SUFFIXES:
.PHONY: x1, x2, clean

compilador:=g++
opcionesc:= -std=c++11 -pthread -Wfatal-errors
hmonsrcs:= HoareMonitor.h HoareMonitor.cpp Semaphore.h Semaphore.cpp

x4: prodconsLIFO_su_exe
	./$<

x5: prodconsLIFO_su_exe
	./$<

prodconsLIFO_su_exe: prodconsLIFO_su.cpp $(hmonsrcs)
	$(compilador) $(opcionesc)  -o $@ $< HoareMonitor.cpp Semaphore.cpp

prodconsFIFO_su_exe: prodconsFIFO_su.cpp $(hmonsrcs)
	$(compilador) $(opcionesc)  -o $@ $< HoareMonitor.cpp Semaphore.cpp


clean:
	rm -f *_exe

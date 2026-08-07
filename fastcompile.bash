#This is used for max performance. The binaries are usually system spesific. You need to recompile in a diffrent cpu structure or generally a diffrent system.
g++  -Ofast -march=native -flto -fno-plt -fomit-frame-pointer lexer.cpp parser.cpp interpreter.cpp graphite.cpp Header_LPI/parseHeader.cpp -o graphite -lreadline 

#This is used for max performance. The binaries are usually system spesific. You need to recompile in a diffrent cpu structure or generally a diffrent system.
g++ -O3 -march=native -flto -fno-plt -fomit-frame-pointer \
    -ffunction-sections -fdata-sections -fno-rtti \
    -Wl,--gc-sections -Wl,--as-needed -s \
    lexer.cpp parser.cpp interpreter.cpp graphite.cpp Header_LPI/parseHeader.cpp \
    -o graphite 



#for debug
#g++ -O0 -g -ggdb -fno-omit-frame-pointer \
#    lexer.cpp parser.cpp interpreter.cpp graphite.cpp \
#    -o graphite
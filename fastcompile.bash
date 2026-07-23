#for fast compilation use this bash. as far as i know nothing breaks in this compilation but i am not sure.
#pls report to me if you spot any diffrences between the normal compilation and the fast version

g++ -Ofast -march=native -flto -fno-plt -fomit-frame-pointer lexer.cpp parser.cpp interpreter.cpp graphite.cpp -o graphite -lreadline

# les compilateurs possible 
   # gcc
   # Visual C++
   # Intel C++ compiler
   # Clang
   # Clang++
   # g++

# Les options de compilateur
   # -c : indique de ne pas linker
   # -v : mode verbeux. Affiche plus de détails
   # -I : spécifie le ou les répertoire(s) de headers
   # -L : spécifie le ou les répertoire(s) de la livrairie statique
   # -l : spécifie le ou les répertoire(s) de la livrairie dynamique
   # -Ox : indique le degré d'optimisation (x varie de 1 à 6) levels 0-1-2-3 -Os(size) -g(debugging) -fast
   # -w : supprime tous les avertissements(warnings)
   # -Wall : GCC est très exigeant !!
   # -Werror : tous les warning deviennent des erreurs
   # -g : mode Debugger
   
   # exemple: g++ -O2 -W -Wextra -Wall -ansi -pedantic

# Make file
#cible: dependences
#	commandes
	




# les compilateurs possible 
   # gcc
   # Visual C++
   # Intel C++ compiler
   # Clang
   # Clang++
   # g++

# Les options de compilateur
   # -c : indique de ne pas linker
   # -v : mode verbeux. Affiche plus de détails
   # -I : spécifie le ou les répertoire(s) de headers
   # -L : spécifie le ou les répertoire(s) de la livrairie statique
   # -l : spécifie le ou les répertoire(s) de la livrairie dynamique
   # -Ox : indique le degré d'optimisation (x varie de 1 à 6) levels 0-1-2-3 -Os(size) -g(debugging) -fast
   # -w : supprime tous les avertissements(warnings)
   # -Wall : GCC est très exigeant !!
   # -Werror : tous les warning deviennent des erreurs
   # -g : mode Debugger
   
   # exemple: g++ -O2 -W -Wextra -Wall -ansi -pedantic

# Make file
#cible: dependences
#	commandes
	



# les compilateurs possible 
   # gcc
   # Visual C++
   # Intel C++ compiler
   # Clang
   # Clang++
   # g++

# Les options de compilateur
   # -c : indique de ne pas linker
   # -v : mode verbeux. Affiche plus de détails
   # -I : spécifie le ou les répertoire(s) de headers
   # -L : spécifie le ou les répertoire(s) de la livrairie statique
   # -l : spécifie le ou les répertoire(s) de la livrairie dynamique
   # -Ox : indique le degré d'optimisation (x varie de 1 à 6) levels 0-1-2-3 -Os(size) -g(debugging) -fast
   # -w : supprime tous les avertissements(warnings)
   # -Wall : GCC est très exigeant !!
   # -Werror : tous les warning deviennent des erreurs
   # -g : mode Debugger
   
   # exemple: g++ -O2 -W -Wextra -Wall -ansi -pedantic

# Make file
#cible: dependences
#	commandes
	



CXX := clang++
CPP := g++
CC  := gcc

#CXXFLAGS := -Wall -g -O2 -D DIGITHIN
#CXXFLAGS := -Wall -g -O2 -Wno-unused-variable -Wno-unused-function
CXXFLAGS := -Wall -g -O2 -Werror
LDFLAGS := -lm -lrt -lpthread -lX11

all:	kisspectrum.run

kisspectrum.run : src/main_kisspectrum.cpp
		$(CPP) $(CXXFLAGS) -o bin/kisspectrum.run src/main_kisspectrum.cpp $(LDFLAGS)
			
clean:
	rm -f bin/kisspectrum.run bin/*.o 

#####################################################################
# Copyright 2003-2009 Santi Ontanon (Brain Games)
# Work continued by 2017-2018 Carlos Donizete Froes [a.k.a coringao]
#
# This file is part of Road Fighter Remake.
# This is free software, licensed under the GPLv2 license.
#####################################################################

EXE = roadfighter
SRCDIR = src
OBJS = \
	$(SRCDIR)/CCarObject.o			$(SRCDIR)/CEnemyCarObject.o		\
	$(SRCDIR)/CEnemyFastCarObject.o		$(SRCDIR)/CEnemyNormalCarObject.o	\
	$(SRCDIR)/CEnemyRacerCarObject.o	$(SRCDIR)/CEnemySlidderCarObject.o	\
	$(SRCDIR)/CEnemyTruckObject.o		$(SRCDIR)/CExplosionObject.o		\
	$(SRCDIR)/CFuelObject.o			$(SRCDIR)/CGame.o			\
	$(SRCDIR)/CObject.o			$(SRCDIR)/CPlayerCarObject.o		\
	$(SRCDIR)/CRoadFighter.o		$(SRCDIR)/CSemaphoreObject.o		\
	$(SRCDIR)/CTile.o			$(SRCDIR)/collision.o			\
	$(SRCDIR)/auxiliar.o			$(SRCDIR)/configuration.o		\
	$(SRCDIR)/debug.o			$(SRCDIR)/filehandling.o		\
	$(SRCDIR)/gameover_state.o		$(SRCDIR)/interlevel_state.o		\
	$(SRCDIR)/konami_state.o		$(SRCDIR)/loadmg2.o			\
	$(SRCDIR)/menu_state.o			$(SRCDIR)/playing_state.o		\
	$(SRCDIR)/presentation_state.o		$(SRCDIR)/quick_tables.o		\
	$(SRCDIR)/sound.o								\
	$(SRCDIR)/main.o

CXX = g++
# Isto e' um projeto C++ (usa new/delete, classes, etc). "gcc" ate compila
# arquivos .cpp (ele detecta a linguagem pela extensao), mas na hora de
# linkar o binario final ele nao linka libstdc++ automaticamente como o
# g++ faz -- isso e' invisivel a maior parte do tempo se alguma outra lib
# do link puxa libstdc++ transitivamente, mas falha ("undefined reference
# to operator new/delete") em ambientes com um toolchain mais minimo, como
# dentro do sandbox de build do Flatpak.
# SDL3: SDL3_mixer ainda nao esta empacotado (nem no Fedora, nem neste
# ambiente) -- ver RHBZ #2454358. Chamamos o pkg-config dele separado dos
# outros (com stderr silenciado) de proposito: se um unico `pkg-config`
# recebe varios modulos e UM deles nao existe, o comando inteiro falha e
# devolve string vazia pra TODOS -- isso quebraria a build mesmo pros
# pacotes (sdl3/sdl3-image/sdl3-ttf) que ja estao instalados e funcionam.
CXXFLAGS += -std=gnu++14 -g3 -O3 -fPIE -D_FORTIFY_SOURCE=2 `pkg-config --cflags sdl3 sdl3-image sdl3-ttf` `pkg-config --cflags sdl3-mixer 2>/dev/null`
LDFLAGS += -fPIE -pie -lm `pkg-config --libs sdl3 sdl3-image sdl3-ttf` `pkg-config --libs sdl3-mixer 2>/dev/null`

RM = rm -rf
ECHO = echo
STRIP = strip

all: $(EXE)

%.o: %.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

$(EXE): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)
	@$(STRIP) $@
	@$(ECHO) "Compiled successfully and generated binary of the game"

clean:
	@$(RM) $(SRCDIR)/*.o $(SRCDIR)/*.bak core $(EXE)
	@$(ECHO) "Completed source cleanup"

.PHONY: all clean

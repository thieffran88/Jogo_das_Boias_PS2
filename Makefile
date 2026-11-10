# Jogo das Boias PS2
#
#   make          -> gera jogo_das_boias.elf (precisa do PS2SDK / ps2dev)
#   make iso      -> gera jogo_das_boias.iso (precisa de xorriso ou mkisofs)
#   make test     -> roda os testes das regras no PC
#   make sim      -> compila o simulador de PC (capturas de tela)

EE_BIN       = jogo_das_boias_unpacked.elf
EE_BIN_PKD   = jogo_das_boias.elf
EE_OBJS      = src/main_ps2.o src/ui.o src/render.o src/game.o src/ai.o src/assets.o
EE_INCS     += -I$(GSKIT)/include -Isrc
EE_LDFLAGS  += -L$(GSKIT)/lib
EE_LIBS      = -lgskit_toolkit -lgskit -ldmakit -lpad -lpatches -lm
EE_STRIP    ?= mips64r5900el-ps2-elf-strip
EE_CFLAGS   += -O2 -Wall -Wno-format-security -Wno-format-truncation

GSKIT ?= $(PS2DEV)/gsKit

ISO_NAME   = jogo_das_boias.iso
ISO_BOOT   = BOIA_000.01

HOST_CC   ?= cc
HOST_SRCS  = src/ui.c src/render.c src/game.c src/ai.c src/assets.c

all: $(EE_BIN_PKD)

$(EE_BIN_PKD): $(EE_BIN)
	$(EE_STRIP) -o build_stripped.elf $<
	@if command -v ps2-packer >/dev/null 2>&1 && ps2-packer build_stripped.elf $@ >/dev/null 2>&1; then \
		echo "ELF comprimido com ps2-packer"; \
	else \
		cp build_stripped.elf $@; \
	fi
	rm -f build_stripped.elf

# Regenera a fonte/formas (precisa de Python 3 + Pillow e da fonte DejaVu).
atlas:
	python3 tools/gen_atlas.py > src/atlas.h

# Recorta os sprites das folhas em assets/src (Python 3 + Pillow + numpy).
assets:
	python3 tools/gen_assets.py

src/render.o src/ui.o src/main_ps2.o: src/atlas.h src/assets.h
src/assets.o: assets/assets.bin src/assets.h

iso: $(EE_BIN_PKD)
	rm -rf build/iso && mkdir -p build/iso
	cp $(EE_BIN_PKD) build/iso/$(ISO_BOOT)
	cp iso/SYSTEM.CNF build/iso/SYSTEM.CNF
	@if command -v xorriso >/dev/null 2>&1; then \
		xorriso -as mkisofs -V JOGO_DAS_BOIAS -sysid PLAYSTATION -l -o $(ISO_NAME) build/iso; \
	else \
		mkisofs -V JOGO_DAS_BOIAS -sysid PLAYSTATION -l -o $(ISO_NAME) build/iso; \
	fi

test:
	mkdir -p build
	$(HOST_CC) -O2 -Wall -Wextra -o build/test_rules tests/test_rules.c src/game.c src/ai.c
	./build/test_rules

sim:
	mkdir -p build
	$(HOST_CC) -O2 -Wall -Wno-format-security -Wno-format-truncation -o build/sim tools/sim.c $(HOST_SRCS) -lm

clean:
	rm -f $(EE_OBJS) $(EE_BIN) $(EE_BIN_PKD) $(ISO_NAME) build_stripped.elf
	rm -rf build

.PHONY: all iso test sim clean atlas assets

ifdef PS2SDK
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
endif

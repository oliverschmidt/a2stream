# https://cc65.github.io/
CL65 ?= cl65

# https://github.com/cc65/ip65/wiki
IP65 ?= ip65

# https://applecommander.github.io/
AC ?= ac.jar

all:
	$(CL65) -t apple2enh --start-addr 0x4000 -Wl -D,__STACKSIZE__=0x0400 -Wl -D,__HIMEM__=0xBF00 \
	-D NDEBUG -D HAVE_ETH -D SINGLE_SOCKET -I $(IP65) -Oir -Cl -m a2stream.map \
	a2stream.c linenoise.c player.c w5100_http.c w5100.c $(IP65)/ip65.lib $(IP65)/ip65_apple2_uther2.lib

dsk: all
	copy prodos.dsk A2Stream.dsk
	java -jar $(AC) -as A2Stream.dsk a2stream            < a2stream
	java -jar $(AC) -p  A2Stream.dsk a2stream.system sys < $(shell $(CL65) --print-target-path)/apple2enh/util/loader.system

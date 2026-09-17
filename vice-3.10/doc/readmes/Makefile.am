
SDLREADME = Readme-SDL.txt
SDL2README = Readme-SDL2.txt

READMES = \
	$(SDLREADME) \
	$(SDL2README)

if USE_SDL2UI

dist_doc_DATA = $(SDL2README)

else
if USE_SDLUI

dist_doc_DATA = $(SDLREADME)

endif
endif

EXTRA_DIST = $(READMES)

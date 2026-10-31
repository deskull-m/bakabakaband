## Makefile.am -- Process this file with automake to process Makefile.in

EXTRA_DIST = \
	delete.me \
	music.cfg

## BGM (mp3/ogg/mid/wav) は配布物・リポジトリに含めない (music/.gitignore) ため、
## インストールの時点で lib/xtra/music に置かれている BGM があれば、music.cfg と
## 合わせてインストールする。
xtra_music_installdir = @DEFAULT_LIB_PATH@/xtra/music

install-data-hook:
if SET_GID
	@set -- "$(srcdir)"/*.mp3 "$(srcdir)"/*.ogg "$(srcdir)"/*.mid "$(srcdir)"/*.wav; \
	have_bgm=no; \
	for f in "$$@"; do \
	  if test -f "$$f"; then have_bgm=yes; break; fi; \
	done; \
	if test "$$have_bgm" = yes; then \
	  $(MKDIR_P) "$(DESTDIR)$(xtra_music_installdir)" || exit 1; \
	  for f in "$(srcdir)/music.cfg" "$$@"; do \
	    if test -f "$$f"; then \
	      echo " $(INSTALL_DATA) $$f $(DESTDIR)$(xtra_music_installdir)"; \
	      $(INSTALL_DATA) "$$f" "$(DESTDIR)$(xtra_music_installdir)" || exit 1; \
	    fi; \
	  done; \
	fi
endif

uninstall-hook:
if SET_GID
	@set -- "$(srcdir)"/*.mp3 "$(srcdir)"/*.ogg "$(srcdir)"/*.mid "$(srcdir)"/*.wav; \
	have_bgm=no; \
	for f in "$$@"; do \
	  if test -f "$$f"; then have_bgm=yes; break; fi; \
	done; \
	if test "$$have_bgm" = yes; then \
	  for f in "$(srcdir)/music.cfg" "$$@"; do \
	    if test -f "$$f"; then \
	      rm -f "$(DESTDIR)$(xtra_music_installdir)/$${f##*/}"; \
	    fi; \
	  done; \
	fi
endif

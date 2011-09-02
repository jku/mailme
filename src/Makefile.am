bin_PROGRAMS = mailme

AM_CFLAGS = \
	-Wall \
	-DLOCALEDIR=\"$(localedir)\" \
	$(MAILME_CFLAGS) \
	-I$(top_srcdir)

mailme_SOURCES = \
	mailme-telepathy.c \
	mailme-telepathy.h \
	mailme-telepathy-account.c \
	mailme-telepathy-account.h \
	mailme.c

mailme_LDADD = $(MAILME_LIBS)

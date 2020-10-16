# Makefile for select/cons_strawman plugin

AUTOMAKE_OPTIONS = foreign

PLUGIN_FLAGS = -module -avoid-version --export-dynamic

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src/common

pkglib_LTLIBRARIES = select_cons_strawman.la

# Consumable resources node selection plugin.
select_cons_strawman_la_SOURCES =  select_cons_strawman.c select_cons_strawman.h \
                              dist_tasks.c dist_tasks.h \
			      job_test.c job_test.h
select_cons_strawman_la_LDFLAGS = $(PLUGIN_FLAGS)

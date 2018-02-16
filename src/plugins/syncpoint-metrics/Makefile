#*****************************************************************************
#     Makefile Build System for Fawkes: syncpoints metrics
#                            -------------------
#   Created on Sat May 06 19:17:11 2017
#   Copyright (C) 2017 by Tim Niemueller
#
#*****************************************************************************
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#*****************************************************************************

BASEDIR = ../../..
include $(BASEDIR)/etc/buildsys/config.mk
include $(BUILDCONFDIR)/webview/webview.mk
include $(BASEDIR)/src/plugins/metrics/metrics.mk
include $(BUILDSYSDIR)/protobuf.mk


LIBS_syncpoints_metrics = fawkescore fawkesutils fawkesaspects	\
  fawkesmetricsaspect	metrics_msgs

OBJS_syncpoints_metrics = syncpoints_metrics_plugin.o syncpoints_metrics_thread.o
OBJS_all    = $(OBJS_syncpoints_metrics)

ifeq ($(HAVE_METRICS)$(HAVE_CPP11)$(HAVE_PROTOBUF),111)
  PLUGINS_all = $(PLUGINDIR)/syncpoints-metrics.$(SOEXT)
  CFLAGS  += $(CFLAGS_CPP11) $(CFLAGS_PROTOBUF)
  LDFLAGS += $(LDFLAGS_PROTOBUF)
else
  ifneq ($(HAVE_CPP11),1)
    WARN_TARGETS = warning_cpp11
  endif
  ifneq ($(HAVE_METRICS),1)
    WARN_TARGETS += warning_metrics
  endif
  ifneq ($(HAVE_PROTOBUF),1)
    WARN_TARGETS += warning_protobuf
  endif
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS)

.PHONY: warning_cpp11 warning_metrics warning_protobuf
warning_cpp11:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Omitting syncpoints-metrics plugin$(TNORMAL) (C++11 support required)"
warning_metrics:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Omitting syncpoints-metrics plugin$(TNORMAL) (metrics plugin not available)"
warning_protobuf:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Omitting syncpoints-metrics plugin$(TNORMAL) (protobuf not available)"
endif

include $(BUILDSYSDIR)/base.mk

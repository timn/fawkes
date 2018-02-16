
/***************************************************************************
 *  metrics_plugin.cpp - Metrics exporter plugin
 *
 *  Created: Sat Jul 29 22:50:13 2017 (Aonami Line from Kinjo Futo to Nagoya)
 *  Copyright  2017  Tim Niemueller [www.niemueller.de]
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#include <core/plugin.h>

#include "syncpoints_metrics_thread.h"

using namespace fawkes;

/** Collect sync points metrics.
 * @author Tim Niemueller
 */
class SyncPointsMetricsPlugin : public fawkes::Plugin
{
 public:
	/** Constructor.
	 * @param config Fawkes configuration
	 */
	SyncPointsMetricsPlugin(Configuration *config) : Plugin(config)
	{
		thread_list.push_back(new SyncPointsMetricsThread());
	}
};

PLUGIN_DESCRIPTION("Collect metrics for sync points")
EXPORT_PLUGIN(SyncPointsMetricsPlugin)

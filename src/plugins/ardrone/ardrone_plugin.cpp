
/***************************************************************************
 *  ardrone_plugin.cpp - Plugin to interface with an AR.Drone
 *
 *  Created: Thu Jan 06 19:06:27 2010
 *  Copyright  2006-2010  Tim Niemueller [www.niemueller.de]
 *
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

#include "ardrone_plugin.h"

using namespace fawkes;

/** @class ArDronePlugin "ardrone_plugin.h"
 * Plugin to interface with an AR.Drone.
 * @author Tim Niemueller
 */

/** Constructor.
 * @param config Fawkes configuration
 */
ArDronePlugin::ArDronePlugin(Configuration *config)
  : Plugin(config)
{
}


PLUGIN_DESCRIPTION("Connector plugin for an AR.Drone.")
EXPORT_PLUGIN(ArDronePlugin)

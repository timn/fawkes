
/***************************************************************************
 *  kinova_plugin.cpp - Fawkes Kinova Plugin
 *
 *  Created: Tue Jun 04 13:13:20 2013
 *  Copyright  2013  Bahram Maleki-Fard
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

#include "kinova_plugin.h"

#include "info_thread.h"
#include "act_thread.h"
#include "goto_thread.h"
#include "openrave_single_thread.h"
#include "openrave_dual_thread.h"

using namespace fawkes;

/** @class KinovaPlugin <plugins/kinova/kinova_plugin.h>
 * Kinova Jaco Arm plugin.
 *
 * @author Bahram Maleki-Fard
 */

/** Constructor.
 * @param config Fawkes configuration
 */
KinovaPlugin::KinovaPlugin(Configuration *config)
  : Plugin(config)
{
  KinovaInfoThread *info_thread = new KinovaInfoThread();
  thread_list.push_back(info_thread);

  KinovaGotoThread *goto_thread = new KinovaGotoThread("KinovaGotoThread");
  thread_list.push_back(goto_thread);

  bool is_dual_arm = config->get_bool("/hardware/jaco/dual_arm/active");

  KinovaGotoThread *goto_thread_2nd = NULL;
  if( is_dual_arm ) {
    goto_thread_2nd = new KinovaGotoThread("KinovaGotoThread2");
    thread_list.push_back(goto_thread_2nd);
  }

  KinovaOpenraveBaseThread *openrave_thread = NULL;
#ifdef HAVE_OPENRAVE
  if( is_dual_arm )
    openrave_thread = new KinovaOpenraveDualThread();
  else
    openrave_thread = new KinovaOpenraveSingleThread();
  thread_list.push_back(openrave_thread);
#endif

  thread_list.push_back(new KinovaActThread(info_thread, goto_thread, goto_thread_2nd, openrave_thread));
}


PLUGIN_DESCRIPTION("Kinova Plugin")
EXPORT_PLUGIN(KinovaPlugin)

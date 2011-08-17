
/***************************************************************************
 *  naofawkes_module.cpp - NaoQi module for Fawkes integration
 *
 *  Created: Thu Jul 03 17:59:29 2008
 *  Copyright  2006-2011  Tim Niemueller [www.niemueller.de]
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

#include "naoqi_broker.h"

#include <baseapp/run.h>
#include <baseapp/main_thread.h>
#include <core/exception.h>
#include <core/threading/thread.h>
#include <plugin/manager.h>
#include <utils/system/dynamic_module/module.h>

#include <alcore/altypes.h>
#include <alcommon/albroker.h>
#include <alcommon/albrokermanager.h>
#include <alcommon/almodule.h>
#include <alproxies/allauncherproxy.h>
#include <alproxies/alaudioplayerproxy.h>

#include <cstdio>
#include <dlfcn.h>

using namespace std;


/** Nao Fawkes Module.
 * This module is instantiated in NaoQi and embeds its own Fawkes instance.
 */
class NaoFawkesModule : public AL::ALModule
{
 public:

  /** Constructor.
   * Initializes and starts the embedded Fawkes, and loads the nao plugin
   * without precondition.
   * @param broker NaoQi broker to use, will be forwarded to the nao plugin
   * @param name name of the module (no idea why NaoQi wants to pass it
   * as a parameter)
   */
  NaoFawkesModule(AL::ALPtr<AL::ALBroker> broker, const std::string &name)
    : AL::ALModule(broker, name), broker(broker)
  {
    setModuleDescription("Fawkes integration module");

    try {
      printf("*** Initializing embedded Fawkes\n");

      // The module flags hack is required because otherwise NaoQi segfaults
      // due to problems with boost static initialization after a module
      // has been closed once, unfortunately that prevents loading a
      // new version of a plugin without a restart.

      fawkes::runtime::InitOptions init_options = 
	fawkes::runtime::InitOptions("naofawkes")
	.plugin_module_flags(fawkes::Module::MODULE_FLAGS_DEFAULT |
			     fawkes::Module::MODULE_NODELETE)
	.net_service_name("NaoQi Fawkes on %h")
	.load_plugins("naoqi,webview")
        .default_plugin("nao_default");

      if (fawkes::runtime::init(init_options) != 0) {
	//throw AL::ALError(name, "ctor", "Initializing Fawkes failed");
	printf("--- Fawkes initialization failed\n");
      } else {

	printf("*** Starting embedded Fawkes\n");
	fawkes::runtime::main_thread->full_start();
	//printf("*** Loading plugins\n");
	//fawkes::runtime::plugin_manager->load("naoqi,naoqi-motion,webview");
      }
    } catch (fawkes::Exception &e) {
      printf("--- Fawkes initialization failed, exception follows.\n");
      e.print_trace();
      //throw AL::ALError(name, "ctor", e.what());
    }

    printf("*** Embedded Fawkes initialization done\n");
    play_startup_sound();
  }

  /** Destructor.
   * Stops the Fawkes main thread and cleans up the embedded Fawkes.
   */
  virtual ~NaoFawkesModule()
  {
    fawkes::runtime::main_thread->cancel();
    fawkes::runtime::main_thread->join();
    fawkes::runtime::cleanup();
  }

  /** Play startup sound. */
  void
  play_startup_sound()
  {
    // Is the auplayer running ?
    try {
      AL::ALPtr<AL::ALLauncherProxy> launcher(new AL::ALLauncherProxy(broker));
      bool is_auplayer_available = launcher->isModulePresent("ALAudioPlayer");

      if (is_auplayer_available) {
        AL::ALPtr<AL::ALAudioPlayerProxy>
          auplayer(new AL::ALAudioPlayerProxy(broker));
        auplayer->playFile(RESDIR"/sounds/naostartup.wav");
      }
    } catch (AL::ALError& e) {} // ignored
  }
  private:
    AL::ALPtr<AL::ALBroker> broker;
};

#ifdef __cplusplus
extern "C"
{
#endif


int
_createModule(AL::ALPtr<AL::ALBroker> broker)
{      
  // init broker with the main broker inctance 
  // from the parent executable

  printf("*** Setting broker stuff\n");
  AL::ALBrokerManager::setInstance(broker->fBrokerManager.lock());
  AL::ALBrokerManager::getInstance()->addBroker(broker);

  fawkes::naoqi::broker = broker;
    
  // create modules instance
  printf("*** Instantiating Module\n");
  AL::ALModule::createModule<NaoFawkesModule>(broker, "NaoFawkesModule");

  return 0;
}

int
_closeModule()
{
  // Delete module instance
  return 0;
}

# ifdef __cplusplus
}
# endif


/***************************************************************************
 *  clips_env_manager.cpp - CLIPS environment manager
 *
 *  Created: Thu Aug 15 18:57:58 2013
 *  Copyright  2006-2013  Tim Niemueller [www.niemueller.de]
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. A runtime exception applies to
 *  this software (see LICENSE.GPL_WRE file mentioned below for details).
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL_WRE file in the doc directory.
 */

#include <plugins/clips/aspect/clips_env_manager.h>
#include <logging/logger.h>

#include <clipsmm.h>
#include <cstring>

extern "C" {
#include <clips/clips.h>
}

namespace fawkes {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif


/// @cond INTERNALS
class CLIPSLogger
{
 public:
  CLIPSLogger(Logger *logger, const char *component = NULL)
  {
    logger_ = logger;
    if (component) {
      component_ = strdup(component);
    } else {
      component_ = NULL;
    }
  }

  ~CLIPSLogger()
  {
    if (component_) {
      free(component_);
    }
  }

  void log(const char *logical_name, const char *str)
  {
    if (strcmp(str, "\n") == 0) {
      if (strcmp(logical_name, "debug") == 0 || strcmp(logical_name, "logdebug") == 0 ||
	  strcmp(logical_name, WTRACE) == 0)
      {
	logger_->log_debug(component_ ? component_ : "CLIPS", "%s", buffer_.c_str());
      } else if (strcmp(logical_name, "warn") == 0 || strcmp(logical_name, "logwarn") == 0 ||
		 strcmp(logical_name, WWARNING) == 0)
      {
	logger_->log_warn(component_ ? component_ : "CLIPS", "%s", buffer_.c_str());
      } else if (strcmp(logical_name, "error") == 0 || strcmp(logical_name, "logerror") == 0 ||
		 strcmp(logical_name, WERROR) == 0)
      {
	logger_->log_error(component_ ? component_ : "CLIPS", "%s", buffer_.c_str());
      } else {
	logger_->log_info(component_ ? component_ : "CLIPS", "%s", buffer_.c_str());
      }

      buffer_.clear();
    } else {
      buffer_ += str;
    }
  }

 private:
  Logger *logger_;
  char *component_;
  std::string buffer_;
};

class CLIPSContextMaintainer {
 public:
  CLIPSContextMaintainer(Logger *logger, const char *log_component_name)
  {
    this->logger = new CLIPSLogger(logger, log_component_name);
  }

  ~CLIPSContextMaintainer()
  {
    delete logger;
  }

 public:
  CLIPSLogger *logger;
};


static int
log_router_query(void *env, char *logical_name)
{
  if (strcmp(logical_name, "l") == 0) return TRUE;
  if (strcmp(logical_name, "info") == 0) return TRUE;
  if (strcmp(logical_name, "debug") == 0) return TRUE;
  if (strcmp(logical_name, "warn") == 0) return TRUE;
  if (strcmp(logical_name, "error") == 0) return TRUE;
  if (strcmp(logical_name, "loginfo") == 0) return TRUE;
  if (strcmp(logical_name, "logdebug") == 0) return TRUE;
  if (strcmp(logical_name, "logwarn") == 0) return TRUE;
  if (strcmp(logical_name, "logerror") == 0) return TRUE;
  if (strcmp(logical_name, "stdout") == 0) return TRUE;
  if (strcmp(logical_name, WTRACE) == 0) return TRUE;
  if (strcmp(logical_name, WWARNING) == 0) return TRUE;
  if (strcmp(logical_name, WERROR) == 0) return TRUE;
  if (strcmp(logical_name, WDISPLAY) == 0) return TRUE;
  return FALSE;
}

static int
log_router_print(void *env, char *logical_name, char *str)
{
  void *rc = GetEnvironmentRouterContext(env);
  CLIPSLogger *logger = static_cast<CLIPSLogger *>(rc);
  logger->log(logical_name, str);
  return TRUE;
}

static int
log_router_exit(void *env, int exit_code)
{
  return TRUE;
}

/// @endcond


/** @class CLIPSEnvManager <plugins/clips/aspect/clips_env_manager.h>
 * CLIPS environment manager.
 * The CLIPS environment manager creates and maintains CLIPS
 * environments, registers features and provides them to the CLIPS
 * environments, and allows access to any and all CLIPS environments.
 * @author Tim Niemueller
 */

/** Constructor.
 * @param logger logger to log messages from created environments
 */
CLIPSEnvManager::CLIPSEnvManager(Logger *logger)
{
  logger_ = logger;
}

/** Destructor. */
CLIPSEnvManager::~CLIPSEnvManager()
{
}


/** Create a new environment.
 * This function creates a new plain environment and sets up logging etc.
 * @param log_component_name prefix for log entries
 * @return readily initialized CLIPS environment
 */
LockPtr<CLIPS::Environment>
CLIPSEnvManager::new_env(const std::string &log_component_name)
{
  // CLIPS overwrites the SIGINT handler, restore it after
  // initializing the environment
  struct sigaction oldact;
  if (sigaction(SIGINT, NULL, &oldact) == 0) {
    LockPtr<CLIPS::Environment> clips(new CLIPS::Environment(),
				      /* recursive mutex */ true);

    CLIPSContextMaintainer *cm =
      new CLIPSContextMaintainer(logger_, log_component_name.c_str());

    void *env = clips->cobj();

    SetEnvironmentContext(env, cm);

    EnvAddRouterWithContext(env, (char *)"fawkeslog",
                            /* exclusive */ 30,
                            log_router_query,
                            log_router_print,
                            /* getc */   NULL,
                            /* ungetc */ NULL,
                            log_router_exit,
                            cm->logger);

    // restore old action
    sigaction(SIGINT, &oldact, NULL);

    return clips;
  } else {
    throw Exception("CLIPS: Unable to backup "
		    "SIGINT sigaction for restoration.");
  }
}


/** Create a new environment.
 * The environment is registered internally under the specified name.
 * It must be destroyed when done with it. Only a single environment
 * can be created for a particular environment name.
 * @param env_name name by which to register environment
 * @param log_component_name prefix for log entries
 * @return readily initialized CLIPS environment
 */
LockPtr<CLIPS::Environment>
CLIPSEnvManager::create_env(const std::string &env_name, const std::string &log_component_name)
{
  LockPtr<CLIPS::Environment> clips;
  if (envs_.find(env_name) != envs_.end()) {
    throw Exception("CLIPS environment '%s' already exists", env_name.c_str());
  }

  clips = new_env(log_component_name);

  if (clips) {
    envs_[env_name].env = clips;
    return clips;
  } else {
    throw Exception("Failed to initialize CLIPS environment '%s'", env_name.c_str());
  }
}

/** Destroy the named environment.
 * Only ever destroy environments which you have created yourself.
 * @param env_name name of the environment to destroy
 */
void
CLIPSEnvManager::destroy_env(const std::string &env_name)
{
  if (envs_.find(env_name) != envs_.end()) {
    void *env = envs_[env_name].env->cobj();
    CLIPSContextMaintainer *cm =
      static_cast<CLIPSContextMaintainer *>(GetEnvironmentContext(env));

    EnvDeleteRouter(env, (char *)"fawkeslog");
    SetEnvironmentContext(env, NULL);
    delete cm;

    envs_.erase(env_name);
  }
}

} // end namespace fawkes

/***************************************************************************
 *  or_thread.cpp - OpenRAVE Thread
 *
 *  Created: Fri Feb 25 15:08:00 2011
 *  Copyright  2011  Bahram Maleki-Fard
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

#include "or_thread.h"

#include "environment.h"
#include "robot.h"
#include "manipulator.h"

#include <openrave-core.h>
#include <core/exceptions/software.h>

using namespace fawkes;

/** @class OpenRAVEThread "or_thread.h"
 * OpenRAVE Thread.
 * This thread maintains an active connection to OpenRAVE and provides an
 * aspect to access OpenRAVE to make it convenient for other threads to use
 * OpenRAVE.
 *
 * @author Bahram Maleki-Fard
 */

/** Constructor. */
OpenRAVEThread::OpenRAVEThread()
  : Thread("OpenRAVEThread", Thread::OPMODE_CONTINUOUS),
    AspectProviderAspect("OpenRAVEAspect", &__or_aspectIniFin),
  __or_aspectIniFin( this ),
  __OR_env( 0 ),
  __OR_robot( 0 )

{
}


/** Destructor. */
OpenRAVEThread::~OpenRAVEThread()
{
}


void
OpenRAVEThread::init()
{
  try {
    OpenRAVE::RaveInitialize(true);
  } catch(const OpenRAVE::openrave_exception &e) {
    logger->log_error(name(), "Could not initialize OpenRAVE. Ex:%s", e.what());
  }

  __OR_env = new OpenRAVEEnvironment(logger);

  __OR_env->create();
  __OR_env->enable_debug(); // TODO: cfg

  __OR_env->lock();
}


void
OpenRAVEThread::finalize()
{
  __OR_env->destroy();
}


void
OpenRAVEThread::loop()
{
}

/* ##########################################################
 * #    methods form OpenRAVEConnector (or_connector.h)     #
 * ########################################################*/
/** Get pointer to OpenRAVEEnvironment object.
 * @return pointer
 */
OpenRAVEEnvironment*
OpenRAVEThread::get_environment() const
{
  return __OR_env;
}

/** Get pointer to currently used OpenRAVERobot object.
 * @return pointer
 */
OpenRAVERobot*
OpenRAVEThread::get_active_robot() const
{
  return __OR_robot;
}

/** Set robot to be used
 * @param robot OpenRAVERobot that should be used implicitly in other methods
 */
void
OpenRAVEThread::set_active_robot(OpenRAVERobot* robot)
{
  __OR_robot = robot;
}

/** Set OpenRAVEManipulator object for robot, and calculate
 * coordinate-system offsets or set them directly.
 * Make sure to update manip angles before calibrating!
 * @param robot pointer to OpenRAVERobot object, explicitly set
 * @param manip pointer to OpenRAVManipulator that is set for robot
 * @param trans_x transition offset on x-axis
 * @param trans_y transition offset on y-axis
 * @param trans_z transition offset on z-axis
 * @param calibrate decides whether to calculate offset (true )or set them directly (false; default)
 */
void
OpenRAVEThread::set_manipulator(OpenRAVERobot* robot, OpenRAVEManipulator* manip, float trans_x, float trans_y, float trans_z, bool calibrate)
{
  robot->set_manipulator(manip);
  if( calibrate )
    {robot->calibrate(trans_x, trans_y, trans_z);}
  else
    {robot->set_offset(trans_x, trans_y, trans_z);}
}
/** Set OpenRAVEManipulator object for robot, and calculate
 * coordinate-system offsets or set them directly.
 * Make sure to update manip angles before calibrating!
 * Uses default OpenRAVERobot object.
 * @param manip pointer to OpenRAVManipulator that is set for robot
 * @param trans_x transition offset on x-axis
 * @param trans_y transition offset on y-axis
 * @param trans_z transition offset on z-axis
 * @param calibrate decides whether to calculate offset (true )or set them directly (false; default)
 */
void
OpenRAVEThread::set_manipulator(OpenRAVEManipulator* manip, float trans_x, float trans_y, float trans_z, bool calibrate)
{
  set_manipulator(__OR_robot, manip, trans_x, trans_y, trans_z, calibrate);
}


/** Start Viewer */
void
OpenRAVEThread::start_viewer() const
{
  __OR_env->start_viewer();
}

/** Run planner on previously set target.
 * @param robot robot to use planner on. If none is given, the currently used robot is taken
 */
void
OpenRAVEThread::run_planner(OpenRAVERobot* robot)
{
  if(!robot)
    robot = __OR_robot;

  __OR_env->run_planner(robot);
}

/** Add a new robot to the environment, and set it as the currently active one.
 * @param filename_robot path to robot's xml file
 * @param autogenerate_IK if true: autogenerate IKfast IK solver for robot
 * @return pointer to new OpenRAVERobot object
 */
OpenRAVERobot*
OpenRAVEThread::add_robot(const std::string& filename_robot, bool autogenerate_IK)
{
  OpenRAVERobot* robot = new OpenRAVERobot(logger);
  robot->load(filename_robot, __OR_env);
  __OR_env->add_robot(robot);
  robot->set_ready();

  if( autogenerate_IK )
    __OR_env->load_IK_solver(robot);

  set_active_robot(robot);

  return robot;
}

/* ##########################################################
 * #    object handling (mainly from environment.h)        #
* ########################################################*/
/** Add an object to the environment.
 * @param name name that should be given to that object
 * @param filename path to xml file of that object (KinBody)
 * @return true if successful */
bool OpenRAVEThread::add_object(const std::string& name, const std::string& filename) {
  return __OR_env->add_object(name, filename); }

/** Remove object from environment.
 * @param name name of the object
 * @return true if successful */
bool OpenRAVEThread::delete_object(const std::string& name) {
  return __OR_env->delete_object(name); }

/** Rename object.
 * @param name current name of the object
 * @param new_name new name of the object
 * @return true if successful */
bool OpenRAVEThread::rename_object(const std::string& name, const std::string& new_name) {
  return __OR_env->rename_object(name, new_name); }

/** Move object in the environment.
 * Distances are given in meters
 * @param name name of the object
 * @param trans_x transition along x-axis
 * @param trans_y transition along y-axis
 * @param trans_z transition along z-axis
 * @param robot if given, move relatively to robot (in most simple cases robot is at position (0,0,0) anyway, so this has no effect)
 * @return true if successful */
bool OpenRAVEThread::move_object(const std::string& name, float trans_x, float trans_y, float trans_z, OpenRAVERobot* robot) {
  return __OR_env->move_object(name, trans_x, trans_y, trans_z, robot); }

/** Rotate object along its axis.
 * Rotation angles should be given in radians.
 * @param name name of the object
 * @param rot_x 1st rotation, along x-axis
 * @param rot_y 2nd rotation, along y-axis
 * @param rot_z 3rd rotation, along z-axis
 * @return true if successful */
bool OpenRAVEThread::rotate_object(const std::string& name, float rot_x, float rot_y, float rot_z) {
  return __OR_env->rotate_object(name, rot_x, rot_y, rot_z); }

/** Set an object as the target.
 * Currently the object should be cylindric, and stand upright. It may
 * also be rotated on its x-axis, but that rotation needs to be given in an argument
 * to calculate correct position for endeffecto. This is only temporary until
 * proper graps planning for 5DOF in OpenRAVE is provided.
 * @param name name of the object
 * @param robot pointer to OpenRAVERobot that the target is set for
 * @param rot_x rotation of object on x-axis (radians)
 * @return true if IK solvable
 */
bool
OpenRAVEThread::set_target_object(const std::string& name, OpenRAVERobot* robot, float rot_x)
{
  OpenRAVE::Transform transform = __OR_env->get_env_ptr()->GetKinBody(name)->GetTransform();

  return robot->set_target_object_position(transform.trans[0], transform.trans[1], transform.trans[2], rot_x);
}

/** Attach a kinbody to the robot.
 * @param name name of the object
 * @param robot pointer to OpenRAVERobot that object is attached to
 * @return true if IK solvable
 */
bool
OpenRAVEThread::attach_object(const std::string& name, OpenRAVERobot* robot)
{
  OpenRAVE::KinBodyPtr body = __OR_env->get_env_ptr()->GetKinBody(name);

  if( !robot )
    { robot = __OR_robot; }

  return robot->get_robot_ptr()->Grab(body);
}

/** Release a kinbody from the robot.
 * @param name name of the object
 * @param robot pointer to OpenRAVERobot that object is released from
 */
void
OpenRAVEThread::release_object(const std::string& name, OpenRAVERobot* robot)
{
  OpenRAVE::KinBodyPtr body = __OR_env->get_env_ptr()->GetKinBody(name);

  if( !robot )
    { robot = __OR_robot; }

  robot->get_robot_ptr()->Release(body);
}

/** Release all grabbed kinbodys from the robot.
 * @param robot pointer to OpenRAVERobot that objects are released from
 */
void
OpenRAVEThread::release_all_objects(OpenRAVERobot* robot)
{
  if( !robot )
    { robot = __OR_robot; }

  robot->get_robot_ptr()->ReleaseAllGrabbed();
}
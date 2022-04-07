/*
 * Copyright (C) 2019 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef IGNITION_GAZEBO_SYSTEMS_PROTOPNEUMATICSPRING_HH_
#define IGNITION_GAZEBO_SYSTEMS_PROTOPNEUMATICSPRING_HH_

#include <ignition/gazebo/System.hh>
#include <memory>

namespace ignition
{
namespace gazebo
{
// Inline bracket to help doxygen filtering.
inline namespace IGNITION_GAZEBO_VERSION_NAMESPACE {
namespace systems
{

    enum class SpringType { linear, pneumatic_adiabatic, pneumatic_calibrated};

  // Forward declaration
  class ProtoPneumaticSpringPrivate;

  /// \brief This can be attached to a model with a reference
  /// to a single prismatic joint. A force proportional to the
  /// joint displacement will be applied along the axis of the joint.
  /// This is an added force that will sum to other forces that may be present.
  ///
  /// ## System Parameters
  ///
  /// xml tags in Ignition Gazebo .sdf file define behavior as follows:
  ///
  /// \brief <JointName>  The name of the joint to control. Required parameter.
  /// 
  /// <SpringType> \brief Type of Spring, options are 'linear', 'pneumatic_adiabatic', 'pneumatic_calibrated'  - Currently Unused
  /// 
  /// <SpringConst> \brief The spring constant.  Required and used when 'SpringType' is 'trivial'.  The default is 1.
  /// 
  /// <PistonDiam> \brief Piston Diam (inches) Required and used whenever 'SpringType' is not 'trivial'.  The default is 5.  - Currently Unused
  /// 
  /// <RodDiam> \brief Rod Diamter (inches) Required and used whenever 'SpringType' is not 'trivial'.  The default is 1.5.  - Currently Unused
  /// 
  /// <PistonEndVolume> \brief Piston End Dead Volume when position is 0 (inches^3). Required and used whenever 'SpringType' is not 'trivial'.  The default is 1430.  - Currently Unused
  /// 
  /// <RodEndVolume> \brief Rod End Dead Volume when position is 0 (inches^3).  Required and used whenever 'SpringType' is not 'trivial'.  The default is 4700.  - Currently Unused
  /// 
  /// <PistonEndPressure> \brief Piston End pressure when position is 0 (psia).  Required and used whenever 'SpringType' is not 'trivial'.  The default is 65.  - Currently Unused
  /// 
  /// <RodEndPressure> \brief Rod End pressure when position is 0 (psia).  Required and used whenever 'SpringType' is not 'trivial'.  The default is 160.  - Currently Unused
  ///
  /// <AmbientTemp> \brief Ambient Temperature (degrees C).  Required and used whenever 'SpringType' is not 'trivial'.  The default is 15.  - Currently Unused
  /// 



  class ProtoPneumaticSpring
      : public System,
        public ISystemConfigure,
        public ISystemPreUpdate
  {
    /// \brief Constructor
    public: ProtoPneumaticSpring();

    /// \brief Destructor
    public: ~ProtoPneumaticSpring() override = default;

    // Documentation inherited
    public: void Configure(const Entity &_entity,
                           const std::shared_ptr<const sdf::Element> &_sdf,
                           EntityComponentManager &_ecm,
                           EventManager &_eventMgr) override;

    // Documentation inherited
    public: void PreUpdate(
                const ignition::gazebo::UpdateInfo &_info,
                ignition::gazebo::EntityComponentManager &_ecm) override;

    /// \brief Private data pointer
    private: std::unique_ptr<ProtoPneumaticSpringPrivate> dataPtr;
  };
  }
}
}
}

#endif
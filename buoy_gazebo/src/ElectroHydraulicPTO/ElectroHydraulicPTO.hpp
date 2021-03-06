// Copyright 2022 Open Source Robotics Foundation, Inc. and Monterey Bay Aquarium Research Institute
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ELECTROHYDRAULICPTO__ELECTROHYDRAULICPTO_HPP_
#define ELECTROHYDRAULICPTO__ELECTROHYDRAULICPTO_HPP_

#include <optional>

#include <ignition/gazebo/System.hh>
#include <ignition/transport.hh>
#include <memory>

namespace buoy_gazebo
{
// Forward declaration
class ElectroHydraulicPTOPrivate;

/// \brief To use, several parameters are required.
/// Two ignition gazebo joints that are either prismatic or continous with 1DOF each,
/// a desciption of the connectoins between actuator (joints),
/// and an oil characteristic specification.
///
/// ## System Parameters
///
/// xml tags in Ignition Gazebo .sdf file define behavior as follows:
///
/// \brief <PrismaticJointName>
///
///  For each actuator of prismatic type, the following nested tags are required:
///     <Area_A>  Piston area on A end of cylinder
///
///     <Area_B>  Piston area on B end of Cylinder
///
///
/// \brief <RevoluteJointName>
///   For each actuator of revolute type, the following nested tags are required:
///
///     <Displacement>  Displacement per revolution of rotary pump/motor.
class ElectroHydraulicPTO : public ignition::gazebo::System,
  public ignition::gazebo::ISystemConfigure,
  public ignition::gazebo::ISystemPreUpdate
{
public:
  /// \brief Constructor
  ElectroHydraulicPTO();

  /// \brief Destructor
  ~ElectroHydraulicPTO() override = default;

  // Documentation inherited
  void Configure(
    const ignition::gazebo::Entity & _entity,
    const std::shared_ptr<const sdf::Element> & _sdf,
    ignition::gazebo::EntityComponentManager & _ecm,
    ignition::gazebo::EventManager & _eventMgr) override;

  // Documentation inherited
  void PreUpdate(
    const ignition::gazebo::UpdateInfo & _info,
    ignition::gazebo::EntityComponentManager & _ecm) override;

private:
  ignition::transport::Node node;
  ignition::transport::Node::Publisher pistonvel_pub, rpm_pub, deltaP_pub, targwindcurr_pub,
    windcurr_pub,
    battcurr_pub, loadcurr_pub, scalefactor_pub, retractfactor_pub;

  /// \brief Private data pointer
  std::unique_ptr<ElectroHydraulicPTOPrivate> dataPtr;
};
}  // namespace buoy_gazebo

#endif  // ELECTROHYDRAULICPTO__ELECTROHYDRAULICPTO_HPP_

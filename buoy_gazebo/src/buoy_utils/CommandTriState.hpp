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

#ifndef BUOY_UTILS__COMMANDTRISTATE_HPP_
#define BUOY_UTILS__COMMANDTRISTATE_HPP_


namespace buoy_utils
{
/// \brief Command state variable that tracks if command is running, finished, or was ever active
template<typename T = double>
class CommandTriState
{
public:
  typedef T valuetype;

  bool isRunning() const  // rising edge
  {
    return !left_ && right_;
  }

  bool isFinished() const  // falling edge
  {
    return left_ && !right_;
  }

  bool active() const  // running or finished
  {
    return left_ || right_;
  }

  operator bool() const
  {
    return isRunning();
  }

  void reset()
  {
    left_ = right_ = false;  // no command activity
  }

  void operator=(const bool state)
  {
    if (state) {
      if (!active()) {
        right_ = true;
      }
    } else {
      if (isRunning()) {
        left_ = true;
        right_ = false;
      }
    }
  }

  void operator=(const valuetype & command)
  {
    value_ = command;
    *this = true;
  }

  const valuetype & value() const
  {
    return value_;
  }

private:
  bool left_{false};
  bool right_{false};
  valuetype value_;
};

}  // namespace buoy_utils

#endif  // BUOY_UTILS__COMMANDTRISTATE_HPP_

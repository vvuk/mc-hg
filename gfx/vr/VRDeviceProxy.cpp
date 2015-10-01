/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <math.h>

#include "prlink.h"
#include "prmem.h"
#include "prenv.h"
#include "gfxPrefs.h"
#include "nsString.h"
#include "mozilla/Preferences.h"
#include "mozilla/unused.h"
#include "nsServiceManagerUtils.h"


#ifdef XP_WIN
#include "../layers/d3d11/CompositorD3D11.h"
#endif

#include "VRDeviceProxy.h"
#include "VRManagerChild.h"

using namespace mozilla;
using namespace mozilla::gfx;

VRDeviceProxy::VRDeviceProxy(const VRDeviceUpdate& aDeviceUpdate)
  : mDeviceInfo(aDeviceUpdate.mDeviceInfo)
  , mSensorState(aDeviceUpdate.mSensorState)
{
  MOZ_COUNT_CTOR(VRDeviceProxy);
}

VRDeviceProxy::~VRDeviceProxy() {
  MOZ_COUNT_DTOR(VRDeviceProxy);
}

void
VRDeviceProxy::UpdateDeviceInfo(const VRDeviceUpdate& aDeviceUpdate)
{
  mDeviceInfo = aDeviceUpdate.mDeviceInfo;
  mSensorState = aDeviceUpdate.mSensorState;
}

bool
VRDeviceProxy::SetFOV(const VRFieldOfView& aFOVLeft, const VRFieldOfView& aFOVRight,
                       double zNear, double zFar)
{
  VRManagerChild *vm = VRManagerChild::Get();
  vm->SendSetFOV(mDeviceInfo.mDeviceID, aFOVLeft, aFOVRight, zNear, zFar);
  return true;
}

void
VRDeviceProxy::ZeroSensor()
{
  VRManagerChild *vm = VRManagerChild::Get();
  vm->SendResetSensor(mDeviceInfo.mDeviceID);
}

VRHMDSensorState
VRDeviceProxy::GetSensorState()
{
  VRManagerChild *vm = VRManagerChild::Get();
  Unused << vm->SendKeepSensorTracking(mDeviceInfo.mDeviceID);
  return mSensorState;
}

VRHMDSensorState
VRDeviceProxy::GetImmediateSensorState()
{
  // XXX TODO - Need to perform IPC call to get the current sensor
  // state rather than the predictive state used for the frame rendering.
  return GetSensorState();
}

void
VRDeviceProxy::UpdateSensorState(const VRHMDSensorState& aSensorState)
{
  mSensorState = aSensorState;
}

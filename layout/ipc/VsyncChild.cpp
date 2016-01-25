/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VsyncChild.h"
#include "gfxVsync.h"

#include "nsThreadUtils.h"

extern PRLogModuleInfo *gVsyncLog;
#define VSYNC_LOG(...)  MOZ_LOG(gVsyncLog, mozilla::LogLevel::Debug, (__VA_ARGS__))

namespace mozilla {
namespace layout {

VsyncChild::VsyncChild(const nsID& aDisplayIdentifier)
  : VsyncSource(aDisplayIdentifier)
  , mObservingVsync(false)
  , mIsShutdown(false)
  , mVsyncInterval(TimeDuration::Forever())
{
  char idstr[NSID_LENGTH];
  mID.ToProvidedString(idstr);
  VSYNC_LOG("[%s]: VsyncChild %p created for display ID %s\n", XRE_GetProcessTypeString(), this, idstr);

  MOZ_ASSERT(NS_IsMainThread());
}

VsyncChild::~VsyncChild()
{
  VSYNC_LOG("[%s]: VsyncChild %p destroyed\n", XRE_GetProcessTypeString(), this);
  MOZ_ASSERT(NS_IsMainThread());
}

void
VsyncChild::EnableVsync()
{
  SendObserve();
}

void
VsyncChild::DisableVsync()
{
  SendUnobserve();
}

bool
VsyncChild::SendObserve()
{
  MOZ_ASSERT(NS_IsMainThread());
  VSYNC_LOG("[%s]: VsyncChild %p observing\n", XRE_GetProcessTypeString(), this);
  if (!mObservingVsync && !mIsShutdown) {
    mObservingVsync = true;
    PVsyncChild::SendObserve();
  }
  return true;
}

bool
VsyncChild::SendUnobserve()
{
  MOZ_ASSERT(NS_IsMainThread());
  VSYNC_LOG("[%s]: VsyncChild %p unobserving\n", XRE_GetProcessTypeString(), this);
  if (mObservingVsync && !mIsShutdown) {
    mObservingVsync = false;
    PVsyncChild::SendUnobserve();
  }
  return true;
}

void
VsyncChild::ActorDestroy(ActorDestroyReason aActorDestroyReason)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mIsShutdown);
  mIsShutdown = true;

  // clear up the VsyncSource's observers
  Shutdown();

  VSYNC_LOG("[%s]: VsyncChild %p actor destroyed\n", XRE_GetProcessTypeString(), this);
}

bool
VsyncChild::RecvNotify(const TimeStamp& aVsyncTimestamp)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mIsShutdown);
  if (mObservingVsync) {
    OnVsync(aVsyncTimestamp);
  }
  return true;
}

TimeDuration
VsyncChild::GetVsyncInterval()
{
  if (mVsyncInterval == TimeDuration::Forever()) {
    PVsyncChild::SendRequestVsyncInterval();
  }

  return mVsyncInterval;
}

bool
VsyncChild::RecvVsyncInterval(const float& aVsyncInterval)
{
  mVsyncInterval = TimeDuration::FromMilliseconds(aVsyncInterval);
  return true;
}

} // namespace layout
} // namespace mozilla

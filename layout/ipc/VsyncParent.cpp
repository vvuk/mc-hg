/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VsyncParent.h"

#include "BackgroundParent.h"
#include "BackgroundParentImpl.h"
#include "gfxPlatform.h"
#include "mozilla/unused.h"
#include "nsIThread.h"
#include "nsThreadUtils.h"
#include "gfxVsync.h"

namespace mozilla {

using namespace ipc;

namespace layout {

/*static*/ already_AddRefed<VsyncParent>
VsyncParent::Create(const nsID& aSourceID)
{
  AssertIsOnBackgroundThread();
  RefPtr<VsyncParent> vsyncParent = new VsyncParent();
  vsyncParent->mSourceID = aSourceID;
  return vsyncParent.forget();
}

VsyncParent::VsyncParent()
  : mObservingVsync(false)
  , mDestroyed(false)
  , mBackgroundThread(NS_GetCurrentThread())
{
  MOZ_ASSERT(mBackgroundThread);
  AssertIsOnBackgroundThread();
}

VsyncParent::~VsyncParent()
{
  // Since we use NS_INLINE_DECL_THREADSAFE_REFCOUNTING, we can't make sure
  // VsyncParent is always released on the background thread.
}

void
VsyncParent::NotifyVsync(TimeStamp aTimeStamp)
{
  // Called on hardware vsync thread. We should post to current ipc thread.
  MOZ_ASSERT(!IsOnBackgroundThread());

  // If we were destroyed, don't bother dispatching to the background thread
  if (mDestroyed)
    return;

  nsCOMPtr<nsIRunnable> vsyncEvent =
    NS_NewRunnableMethodWithArg<TimeStamp>(this,
                                           &VsyncParent::DispatchVsyncEvent,
                                           aTimeStamp);
  // This dispatch might fail, if we're in the middle of shutting down when
  // a vsync comes in.
  mBackgroundThread->Dispatch(vsyncEvent, NS_DISPATCH_NORMAL);
}

void
VsyncParent::DispatchVsyncEvent(TimeStamp aTimeStamp)
{
  AssertIsOnBackgroundThread();

  // If we call NotifyVsync() when we handle ActorDestroy() message, we might
  // still call DispatchVsyncEvent().
  // Similarly, we might also receive RecvUnobserveVsync() when call
  // NotifyVsync(). We use mObservingVsync and mDestroyed flags to skip this
  // notification.
  if (mObservingVsync && !mDestroyed) {
    Unused << SendNotify(aTimeStamp);
  }
}

bool
VsyncParent::RecvRequestVsyncRate()
{
  AssertIsOnBackgroundThread();
  RefPtr<gfx::VsyncSource> source = gfxPlatform::GetPlatform()->GetHardwareVsync()->GetSource(mSourceID);
  Unused << SendVsyncRate(source->GetVsyncRate().ToMilliseconds());
  return true;
}

bool
VsyncParent::RecvObserve()
{
  AssertIsOnBackgroundThread();
  if (!mObservingVsync) {
    RefPtr<gfx::VsyncSource> source = gfxPlatform::GetPlatform()->GetHardwareVsync()->GetSource(mSourceID);
    source->AddVsyncObserver(this);
    mObservingVsync = true;
    return true;
  }
  return false;
}

bool
VsyncParent::RecvUnobserve()
{
  AssertIsOnBackgroundThread();
  if (mObservingVsync) {
    AttachedSource()->RemoveVsyncObserver(this);
    mObservingVsync = false;
    return true;
  }
  return false;
}

void
VsyncParent::ActorDestroy(ActorDestroyReason aReason)
{
  MOZ_ASSERT(!mDestroyed);
  AssertIsOnBackgroundThread();
  if (mObservingVsync && AttachedSource()) {
    AttachedSource()->RemoveVsyncObserver(this);
    mObservingVsync = false;
  }
  mDestroyed = true;
}

} // namespace layout
} // namespace mozilla

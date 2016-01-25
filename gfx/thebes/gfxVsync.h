/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GFX_VSYNC_H
#define GFX_VSYNC_H

#include "nsTArray.h"
#include "mozilla/RefPtr.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "nsISupportsImpl.h"
#include "nsID.h"

class CancelableTask;
namespace base {
class Thread;
}

namespace mozilla {
namespace gfx {

class VsyncSource;

// An observer class, with a single notify method that is called when
// vsync occurs.
class VsyncObserver
{
  friend class VsyncSource;

  // This class can be mixed into classes that are already refcounted;
  // as such, implementors of this class will need to provide their
  // own *thread safe* AddRef and Release.  This can be done with
  // NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VsyncObserverImplClass, override)
  NS_INLINE_DECL_PURE_VIRTUAL_REFCOUNTING

public:
  // An observer receives a NotifyVsync call when a vsync occurs.  In
  // general, a vsync notification will occur on a non-main thread,
  // usually either the hardware vsync thread or a PVsync background
  // ipc thread (if it's being delivered to a child).  All observer
  // implementations should make sure to minimize the amount of work
  // done on vsync: they should just post a task on a more appropriate
  // thread and return.
  virtual void NotifyVsync(TimeStamp aVsyncTimestamp) = 0;

  // The source that this observer is observing.  A VsyncObserver can
  // only be attached to one VsyncSource at a time.  This will be
  // non-null after the observer is attached to a VsyncSource, and
  // will become null when it is removed from a source, or when its
  // currently attached source is shut down.  This method is thread
  // safe, but can race -- if this observer is being removed from a
  // source on one thread, another thread asking for the
  // AttachedSource will get unpredictable results (non-null or null).
  VsyncSource *AttachedSource() { return mVsyncSource; }

protected:
  VsyncObserver() : mVsyncSource(nullptr) {}
  virtual ~VsyncObserver() {}

  VsyncSource *mVsyncSource;
}; // VsyncObserver

// A VsyncManager maintains a list of sources, and provides
// global access to them.  It is accessible from the gfxPlatform,
// and lives as long as the gfxPlatform does.
//
// Sources may be registered and unregistered during the lifetime
// of the process (for example, as monitors are connected and
// disconnected).
//
// The Global Display source will always exist, and will generally
// refer to the primary display on the system.
class VsyncManager
{
  friend class VsyncSource;
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VsyncManager)

public:
  // the GUID the global display source must use
  static const nsID kGlobalDisplaySourceID;

  VsyncManager();

  virtual void RegisterSource(VsyncSource* aSource);
  virtual void UnregisterSource(const nsID& aSourceID);
  virtual already_AddRefed<VsyncSource> GetSource(const nsID& aSourceID);
  virtual void GetSources(nsTArray<RefPtr<VsyncSource>>& aSources);

  already_AddRefed<VsyncSource> GetGlobalDisplaySource() {
    return GetSource(kGlobalDisplaySourceID);
  }

  virtual void Shutdown();

protected:
  virtual ~VsyncManager();

  // return index of aSourceID in mSources, or -1 if not found.
  // Assumes mSourcesMutex is locked.
  int32_t GetSourceIndex(const nsID& aSourceID);

  mozilla::Mutex mSourcesMutex;
  nsTArray<RefPtr<VsyncSource>> mSources;
};

//
// A VsyncSource distributes vsync notifications to VsyncObservers.
// There are a few different implementations of VsyncSource, for
// example:
//   - SoftwareVsyncSource: a VsyncSource based on a software timer
//   - DWMVsyncSource (on Windows): a VsyncSource delivering hardware
//     vsync timing
//   - VsyncChild: child half of a PVsync; acts as a VsyncSource
//     that forwards events across process boundaries
//   - VsyncForwardingObserver: in nsBaseWidget; acts as both an
//     Observer and a Source, forwarding events in the same process
//
class VsyncSource {
  // This class can be mixed into classes that are already refcounted;
  // as such, implementors of this class will need to provide their
  // own *thread safe* AddRef and Release.  This can be done with
  // NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VsyncSourceImplClass, override)
  NS_INLINE_DECL_PURE_VIRTUAL_REFCOUNTING

public:
  // return a UUID that can be used to identify this display (during
  // this process lifetime only)
  const nsID& ID() const { return mID; }

  // Add or remove a vsync observer to be notified when vsync
  // happens.  This notification will happen *on the vsync
  // thread*, so if you expect to do any significant work, get off
  // that thread ASAP in the observer callback.  Can be called on
  // any thread.
  void AddVsyncObserver(VsyncObserver *aObserver);
  bool RemoveVsyncObserver(VsyncObserver *aObserver);

  // Notified when this display's vsync occurs, on the vsync thread.
  // Calls all registered observers' NotifyVsync method to notify them of vsync.
  //
  // aVsyncTimestamp is the timestamp of the vsync that just occrred.  Gecko
  // assumes that vsync will always <= Now().
  void OnVsync(TimeStamp aVsyncTimestamp);

  // These can only be called on the main thread.  Subclasses should
  // implement these to enable and disable vsync, as well as report
  // the current status.  You should not need to call these directly;
  // adding or removing a vsync observer will enable or disable vsync
  // as necessary.
  virtual void EnableVsync() = 0;
  virtual void DisableVsync() = 0;
  virtual bool IsVsyncEnabled() = 0;

  // The vsync rate, reported as a duration between successive vsync intervals
  // Unknown if this returns TimeDuration::Forever
  virtual TimeDuration GetVsyncInterval() { return TimeDuration::Forever(); }

  // Shut down this vsync source.  Subclasses can override this,
  // but they should make sure that the base class shutdown was called
  virtual void Shutdown();

protected:
  // constructs a new Display and generates an identifier
  explicit VsyncSource();
  // constructs a new Display using the given identifier
  VsyncSource(const nsID& aID);

  virtual ~VsyncSource();

  // Called when an observer is added/removed, to enable or
  // disable vsync as needed.  Subclasses should not need to
  // override this, but if they do, they should call the base
  // class implementation.
  virtual void UpdateVsyncStatus();

  nsID mID;

  mozilla::Mutex mObserversMutex;
  nsTArray<RefPtr<VsyncObserver>> mVsyncObservers;
};

/**
 * A pure software-timer driven vsync generator
 */
class SoftwareVsyncSource final : public VsyncSource
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SoftwareVsyncSource, override)
public:
  explicit SoftwareVsyncSource(const nsID& aSourceID,
                               double aInterval = 1000.0 / 60.0);
  virtual void EnableVsync() override;
  virtual void DisableVsync() override;
  virtual bool IsVsyncEnabled() override;
  bool IsInSoftwareVsyncThread();
  void Tick(mozilla::TimeStamp aVsyncTimestamp);
  void ScheduleNextVsync(mozilla::TimeStamp aVsyncTimestamp);
  virtual void Shutdown() override;
  virtual TimeDuration GetVsyncInterval() override { return mVsyncInterval; }

protected:
  ~SoftwareVsyncSource();

  void InternalEnableVsync();
  void InternalDisableVsync();

  mozilla::TimeDuration mVsyncInterval;
  // Use a chromium thread because nsITimers* fire on the main thread
  base::Thread* mVsyncThread;
  CancelableTask* mCurrentVsyncTask; // only access on vsync thread
  bool mVsyncEnabled; // Only access on main thread
}; // SoftwareVsyncSource

} // namespace gfx
} // namespace mozilla

#endif /* GFX_VSYNC_H */

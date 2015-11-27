/* vim:set ts=2 sw=2 sts=2 et: */
/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "gfxPlatform.h"
#include "gfxPrefs.h"
#include "MainThreadUtils.h"
#include "nsIThread.h"
#include "mozilla/RefPtr.h"
#include "gfxVsync.h"
#include "mozilla/Monitor.h"
#include "mozilla/TimeStamp.h"

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::layers;
using ::testing::_;

// Timeout for vsync events to occur in milliseconds
const int kVsyncTimeoutMS = 50;

class TestVsyncObserver : public VsyncObserver {
public:
  TestVsyncObserver()
    : mDidGetVsyncNotification(false)
    , mVsyncMonitor("VsyncMonitor")
  {
  }

  virtual void NotifyVsync(TimeStamp aVsyncTimeStamp) override {
    MonitorAutoLock lock(mVsyncMonitor);
    mDidGetVsyncNotification = true;
    mVsyncMonitor.Notify();
  }

  void WaitForVsyncNotification()
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (DidGetVsyncNotification()) {
      return;
    }

    { // scope lock
      MonitorAutoLock lock(mVsyncMonitor);
      PRIntervalTime timeout = PR_MillisecondsToInterval(kVsyncTimeoutMS);
      lock.Wait(timeout);
    }
  }

  bool DidGetVsyncNotification()
  {
    MonitorAutoLock lock(mVsyncMonitor);
    return mDidGetVsyncNotification;
  }

  void ResetVsyncNotification()
  {
    MonitorAutoLock lock(mVsyncMonitor);
    mDidGetVsyncNotification = false;
  }

private:
  bool mDidGetVsyncNotification;

private:
  Monitor mVsyncMonitor;
};

class VsyncTester : public ::testing::Test {
protected:
  explicit VsyncTester()
  {
    gfxPlatform::GetPlatform();
    gfxPrefs::GetSingleton();
    mVsyncManager = gfxPlatform::GetPlatform()->GetHardwareVsync();
    MOZ_RELEASE_ASSERT(mVsyncManager);
  }

  virtual ~VsyncTester()
  {
    mVsyncManager = nullptr;
  }

  RefPtr<VsyncManager> mVsyncManager;
};

// Tests that we can enable/disable vsync notifications
TEST_F(VsyncTester, EnableVsync)
{
  RefPtr<VsyncSource> globalDisplay = mVsyncManager->GetGlobalDisplaySource();
  globalDisplay->DisableVsync();
  ASSERT_FALSE(globalDisplay->IsVsyncEnabled());

  globalDisplay->EnableVsync();
  ASSERT_TRUE(globalDisplay->IsVsyncEnabled());

  globalDisplay->DisableVsync();
  ASSERT_FALSE(globalDisplay->IsVsyncEnabled());
}

#if 0
// Test that if we have vsync enabled, the parent refresh driver should get notifications
TEST_F(VsyncTester, ParentRefreshDriverGetVsyncNotifications)
{
  RefPtr<VsyncSource> globalDisplay = mVsyncManager->GetGlobalDisplaySource();
  globalDisplay->DisableVsync();
  ASSERT_FALSE(globalDisplay->IsVsyncEnabled());

  RefPtr<RefreshTimerVsyncDispatcher> vsyncDispatcher = globalDisplay->GetRefreshTimerVsyncDispatcher();
  ASSERT_TRUE(vsyncDispatcher != nullptr);

  RefPtr<TestVsyncObserver> testVsyncObserver = new TestVsyncObserver();
  vsyncDispatcher->SetParentRefreshTimer(testVsyncObserver);
  ASSERT_TRUE(globalDisplay->IsVsyncEnabled());

  testVsyncObserver->WaitForVsyncNotification();
  ASSERT_TRUE(testVsyncObserver->DidGetVsyncNotification());
  vsyncDispatcher->SetParentRefreshTimer(nullptr);

  testVsyncObserver->ResetVsyncNotification();
  testVsyncObserver->WaitForVsyncNotification();
  ASSERT_FALSE(testVsyncObserver->DidGetVsyncNotification());

  vsyncDispatcher = nullptr;
  testVsyncObserver = nullptr;
}

// Test that child refresh vsync observers get vsync notifications
TEST_F(VsyncTester, ChildRefreshDriverGetVsyncNotifications)
{
  RefPtr<VsyncSource> globalDisplay = mVsyncManager->GetGlobalDisplaySource();
  globalDisplay->DisableVsync();
  ASSERT_FALSE(globalDisplay->IsVsyncEnabled());

  RefPtr<RefreshTimerVsyncDispatcher> vsyncDispatcher = globalDisplay->GetRefreshTimerVsyncDispatcher();
  ASSERT_TRUE(vsyncDispatcher != nullptr);

  RefPtr<TestVsyncObserver> testVsyncObserver = new TestVsyncObserver();
  vsyncDispatcher->AddChildRefreshTimer(testVsyncObserver);
  ASSERT_TRUE(globalDisplay->IsVsyncEnabled());

  testVsyncObserver->WaitForVsyncNotification();
  ASSERT_TRUE(testVsyncObserver->DidGetVsyncNotification());

  vsyncDispatcher->RemoveChildRefreshTimer(testVsyncObserver);
  testVsyncObserver->ResetVsyncNotification();
  testVsyncObserver->WaitForVsyncNotification();
  ASSERT_FALSE(testVsyncObserver->DidGetVsyncNotification());

  vsyncDispatcher = nullptr;
  testVsyncObserver = nullptr;
}
#endif

// Test that we can read the vsync rate
TEST_F(VsyncTester, VsyncSourceHasVsyncRate)
{
  RefPtr<VsyncSource> globalDisplay = mVsyncManager->GetGlobalDisplaySource();
  TimeDuration vsyncRate = globalDisplay->GetVsyncRate();
  ASSERT_NE(vsyncRate, TimeDuration::Forever());
  ASSERT_GT(vsyncRate.ToMilliseconds(), 0);
}

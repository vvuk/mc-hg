/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set shiftwidth=2 tabstop=8 autoindent cindent expandtab: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef RefreshDriverTimerBase_h_
#define RefreshDriverTimerBase_h_

#include "nsISupportsImpl.h"

namespace mozilla {
namespace layout {
// pure base class just for the refcounting, so that we can hold opaque handles to
// RDTs elsewhere
class RefreshDriverTimerBase {
public:
  NS_INLINE_DECL_PURE_VIRTUAL_REFCOUNTING
protected:
  RefreshDriverTimerBase() {}
  virtual ~RefreshDriverTimerBase() {}
};

} // namespace layout
} // namespace mozilla

#endif /* RefreshDriverTimerBase_h_ */

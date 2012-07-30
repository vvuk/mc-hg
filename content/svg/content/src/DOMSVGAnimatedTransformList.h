/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: sw=2 ts=2 et lcs=trail\:.,tab\:>~ :
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZILLA_DOMSVGANIMATEDTRANSFORMLIST_H__
#define MOZILLA_DOMSVGANIMATEDTRANSFORMLIST_H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMSVGAnimTransformList.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

namespace mozilla {

class DOMSVGTransformList;
class SVGAnimatedTransformList;

/**
 * Class DOMSVGAnimatedTransformList
 *
 * This class is used to create the DOM tearoff objects that wrap internal
 * SVGAnimatedTransformList objects.
 *
 * See the architecture comment in DOMSVGAnimatedLengthList.h (that's
 * LENGTH list). The comment for that class largly applies to this one too
 * and will go a long way to helping you understand the architecture here.
 *
 * This class is strongly intertwined with DOMSVGTransformList and
 * DOMSVGTransform.
 * Our DOMSVGTransformList base and anim vals are friends and take care of
 * nulling out our pointers to them when they die (making our pointers to them
 * true weak refs).
 */
class DOMSVGAnimatedTransformList MOZ_FINAL : public nsIDOMSVGAnimatedTransformList
{
  friend class DOMSVGTransformList;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGAnimatedTransformList)
  NS_DECL_NSIDOMSVGANIMATEDTRANSFORMLIST

  /**
   * Factory method to create and return a DOMSVGAnimatedTransformList wrapper
   * for a given internal SVGAnimatedTransformList object. The factory takes
   * care of caching the object that it returns so that the same object can be
   * returned for the given SVGAnimatedTransformList each time it is requested.
   * The cached object is only removed from the cache when it is destroyed due
   * to there being no more references to it or to any of its descendant
   * objects. If that happens, any subsequent call requesting the DOM wrapper
   * for the SVGAnimatedTransformList will naturally result in a new
   * DOMSVGAnimatedTransformList being returned.
   */
  static already_AddRefed<DOMSVGAnimatedTransformList>
    GetDOMWrapper(SVGAnimatedTransformList *aList, nsSVGElement *aElement);

  /**
   * This method returns the DOMSVGAnimatedTransformList wrapper for an internal
   * SVGAnimatedTransformList object if it currently has a wrapper. If it does
   * not, then nullptr is returned.
   */
  static DOMSVGAnimatedTransformList*
    GetDOMWrapperIfExists(SVGAnimatedTransformList *aList);

  /**
   * Called by internal code to notify us when we need to sync the length of
   * our baseVal DOM list with its internal list. This is called just prior to
   * the length of the internal baseVal list being changed so that any DOM list
   * items that need to be removed from the DOM list can first get their values
   * from their internal counterpart.
   *
   * The only time this method could fail is on OOM when trying to increase the
   * length of the DOM list. If that happens then this method simply clears the
   * list and returns. Callers just proceed as normal, and we simply accept
   * that the DOM list will be empty (until successfully set to a new value).
   */
  void InternalBaseValListWillChangeLengthTo(PRUint32 aNewLength);
  void InternalAnimValListWillChangeLengthTo(PRUint32 aNewLength);

  /**
   * Returns true if our attribute is animating (in which case our animVal is
   * not simply a mirror of our baseVal).
   */
  bool IsAnimating() const;

private:

  /**
   * Only our static GetDOMWrapper() factory method may create objects of our
   * type.
   */
  DOMSVGAnimatedTransformList(nsSVGElement *aElement)
    : mBaseVal(nullptr)
    , mAnimVal(nullptr)
    , mElement(aElement)
  {}

  ~DOMSVGAnimatedTransformList();

  /// Get a reference to this DOM wrapper object's internal counterpart.
  SVGAnimatedTransformList& InternalAList();
  const SVGAnimatedTransformList& InternalAList() const;

  // Weak refs to our DOMSVGTransformList baseVal/animVal objects. These objects
  // are friends and take care of clearing these pointers when they die, making
  // these true weak references.
  DOMSVGTransformList *mBaseVal;
  DOMSVGTransformList *mAnimVal;

  // Strong ref to our element to keep it alive. We hold this not only for
  // ourself, but also for our base/animVal and all of their items.
  nsRefPtr<nsSVGElement> mElement;
};

} // namespace mozilla

#endif // MOZILLA_DOMSVGANIMATEDTRANSFORMLIST_H__

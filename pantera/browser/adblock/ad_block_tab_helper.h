// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PANTERA_BROWSER_ADBLOCK_AD_BLOCK_TAB_HELPER_H_
#define PANTERA_BROWSER_ADBLOCK_AD_BLOCK_TAB_HELPER_H_

#include <stddef.h>

#include <vector>

#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

namespace pantera {

// Counts the requests the ad blocker blocked on a tab's current page, for the
// shields panel (TASK-7). Created on the first block.
class AdBlockTabHelper : public content::WebContentsObserver,
                         public content::WebContentsUserData<AdBlockTabHelper> {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // The page changed, or a request was blocked on it.
    virtual void OnBlockedRequestsChanged() = 0;
    // The tab is going away; stop observing.
    virtual void OnAdBlockTabHelperDestroyed() = 0;
  };

  // Most blocked URLs a page keeps; the count goes on.
  static constexpr size_t kMaxBlockedUrls = 500;

  AdBlockTabHelper(const AdBlockTabHelper&) = delete;
  AdBlockTabHelper& operator=(const AdBlockTabHelper&) = delete;
  ~AdBlockTabHelper() override;

  static void RecordBlockedRequest(content::WebContents* web_contents,
                                   const GURL& url);

  size_t blocked_count() const { return blocked_count_; }
  const std::vector<GURL>& blocked_urls() const { return blocked_urls_; }

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;

 private:
  friend class content::WebContentsUserData<AdBlockTabHelper>;

  explicit AdBlockTabHelper(content::WebContents* web_contents);

  size_t blocked_count_ = 0;
  std::vector<GURL> blocked_urls_;
  base::ObserverList<Observer> observers_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace pantera

#endif  // PANTERA_BROWSER_ADBLOCK_AD_BLOCK_TAB_HELPER_H_

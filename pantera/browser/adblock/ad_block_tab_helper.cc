// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/browser/adblock/ad_block_tab_helper.h"

#include "content/public/browser/web_contents.h"

namespace pantera {

AdBlockTabHelper::AdBlockTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AdBlockTabHelper>(*web_contents) {}

AdBlockTabHelper::~AdBlockTabHelper() {
  for (Observer& observer : observers_) {
    observer.OnAdBlockTabHelperDestroyed();
  }
}

void AdBlockTabHelper::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AdBlockTabHelper::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

// static
void AdBlockTabHelper::RecordBlockedRequest(content::WebContents* web_contents,
                                            const GURL& url) {
  if (!web_contents) {
    return;
  }
  AdBlockTabHelper* helper =
      AdBlockTabHelper::GetOrCreateForWebContents(web_contents);
  ++helper->blocked_count_;
  if (helper->blocked_urls_.size() < kMaxBlockedUrls) {
    helper->blocked_urls_.push_back(url);
  }
  for (Observer& observer : helper->observers_) {
    observer.OnBlockedRequestsChanged();
  }
}

void AdBlockTabHelper::PrimaryPageChanged(content::Page& page) {
  blocked_count_ = 0;
  blocked_urls_.clear();
  for (Observer& observer : observers_) {
    observer.OnBlockedRequestsChanged();
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AdBlockTabHelper);

}  // namespace pantera

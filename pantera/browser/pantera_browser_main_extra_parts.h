// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PANTERA_BROWSER_PANTERA_BROWSER_MAIN_EXTRA_PARTS_H_
#define PANTERA_BROWSER_PANTERA_BROWSER_MAIN_EXTRA_PARTS_H_

#include <memory>

#include "chrome/browser/chrome_browser_main_extra_parts.h"

namespace pantera {

class AdBlockService;

// Starts and stops Pantera's browser-wide services.
class PanteraBrowserMainExtraParts : public ChromeBrowserMainExtraParts {
 public:
  PanteraBrowserMainExtraParts();
  PanteraBrowserMainExtraParts(const PanteraBrowserMainExtraParts&) = delete;
  PanteraBrowserMainExtraParts& operator=(const PanteraBrowserMainExtraParts&) =
      delete;
  ~PanteraBrowserMainExtraParts() override;

  // ChromeBrowserMainExtraParts:
  void PreProfileInit() override;
  void PostMainMessageLoopRun() override;

 private:
  std::unique_ptr<AdBlockService> ad_block_service_;
};

// The browser's ad-block service, or null before startup and after shutdown.
AdBlockService* GetAdBlockService();

}  // namespace pantera

#endif  // PANTERA_BROWSER_PANTERA_BROWSER_MAIN_EXTRA_PARTS_H_

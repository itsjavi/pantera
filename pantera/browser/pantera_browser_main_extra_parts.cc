// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/browser/pantera_browser_main_extra_parts.h"

#include <memory>

#include "base/apple/bundle_locations.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_paths.h"
#include "pantera/components/adblock/ad_block_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace pantera {

namespace {

AdBlockService* g_ad_block_service = nullptr;

}  // namespace

PanteraBrowserMainExtraParts::PanteraBrowserMainExtraParts() = default;

PanteraBrowserMainExtraParts::~PanteraBrowserMainExtraParts() = default;

void PanteraBrowserMainExtraParts::PreProfileInit() {
  base::FilePath user_data_dir;
  CHECK(base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir));
  // build.sh bundles the catalog and resources in the framework's
  // Resources/adblock (//pantera/components/adblock:bundle_data).
  const base::FilePath resources_dir =
      base::apple::FrameworkBundlePath().Append("Resources").Append("adblock");

  ad_block_service_ = std::make_unique<AdBlockService>(
      g_browser_process->local_state(),
      g_browser_process->shared_url_loader_factory(),
      user_data_dir.Append("AdBlock"), resources_dir,
      g_browser_process->GetApplicationLocale());
  g_ad_block_service = ad_block_service_.get();
  ad_block_service_->Start();
}

void PanteraBrowserMainExtraParts::PostMainMessageLoopRun() {
  g_ad_block_service = nullptr;
  ad_block_service_.reset();
}

AdBlockService* GetAdBlockService() {
  return g_ad_block_service;
}

}  // namespace pantera

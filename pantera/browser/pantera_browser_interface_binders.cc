// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/browser/pantera_browser_interface_binders.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "pantera/browser/pantera_browser_main_extra_parts.h"
#include "pantera/components/adblock/ad_block_service.h"
#include "pantera/components/cosmetic_filters/browser/cosmetic_filters_resources.h"
#include "pantera/components/cosmetic_filters/common/cosmetic_filters.mojom.h"

namespace pantera {

namespace {

std::unique_ptr<mojom::CosmeticFiltersResources> MakeCosmeticFiltersResources(
    scoped_refptr<HostContentSettingsMap> settings_map,
    AdBlockEngineWrapper* engine_wrapper) {
  return std::make_unique<CosmeticFiltersResources>(engine_wrapper,
                                                    std::move(settings_map));
}

void BindCosmeticFiltersResources(
    content::RenderFrameHost* render_frame_host,
    mojo::PendingReceiver<mojom::CosmeticFiltersResources> receiver) {
  AdBlockService* service = GetAdBlockService();
  if (!service) {
    return;
  }
  // The frame's profile has the shields settings, read with the engines.
  scoped_refptr<HostContentSettingsMap> settings_map =
      HostContentSettingsMapFactory::GetForProfile(
          render_frame_host->GetBrowserContext());
  service->BindCosmeticFiltersResources(
      std::move(receiver),
      base::BindOnce(&MakeCosmeticFiltersResources, std::move(settings_map)));
}

}  // namespace

void PopulateFrameBinders(
    mojo::BinderMapWithContext<content::RenderFrameHost*>* map,
    content::RenderFrameHost* render_frame_host) {
  map->Add<mojom::CosmeticFiltersResources>(
      base::BindRepeating(&BindCosmeticFiltersResources));
}

}  // namespace pantera

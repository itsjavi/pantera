// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PANTERA_BROWSER_UI_VIEWS_SHIELDS_SHIELDS_TOOLBAR_BUTTON_H_
#define PANTERA_BROWSER_UI_VIEWS_SHIELDS_SHIELDS_TOOLBAR_BUTTON_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "pantera/browser/adblock/ad_block_tab_helper.h"
#include "pantera/browser/ui/views/shields/shields_bubble_view.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/widget/widget.h"

class BrowserWindowInterface;

namespace views {
class BubbleDialogDelegate;
}  // namespace views

namespace content {
class WebContents;
}  // namespace content

namespace pantera {

class ShieldsCountBadge;

// The shield right of the address bar (DECISION-19). It shows how many
// requests were blocked on the active tab's page, looks off when shields are
// down for the site, and opens the Shields panel.
class ShieldsToolbarButton : public ToolbarButton,
                             public TabStripModelObserver,
                             public AdBlockTabHelper::Observer {
  METADATA_HEADER(ShieldsToolbarButton, ToolbarButton)

 public:
  explicit ShieldsToolbarButton(BrowserWindowInterface* browser);
  ShieldsToolbarButton(const ShieldsToolbarButton&) = delete;
  ShieldsToolbarButton& operator=(const ShieldsToolbarButton&) = delete;
  ~ShieldsToolbarButton() override;

  // Refreshes the icon and count, for example after the panel changed a
  // setting.
  void UpdateState();

  // ToolbarButton:
  void UpdateIcon() override;
  void Layout(PassKey) override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // AdBlockTabHelper::Observer:
  void OnBlockedRequestsChanged() override;
  void OnAdBlockTabHelperDestroyed() override;

 private:
  void OnPressed();
  void ShowBubble(ShieldsBubbleView::InitialView initial_view);
  void OnBubbleClosed(views::Widget::ClosedReason reason);
  // For the --pantera-show-shields-panel developer switch.
  void ShowBubbleForSwitch();
  void ObserveActiveTab();
  content::WebContents* GetActiveWebContents() const;
  bool AreShieldsUpOnActiveTab() const;

  const raw_ptr<BrowserWindowInterface> browser_;
  bool shields_up_ = true;
  raw_ptr<ShieldsCountBadge> badge_ = nullptr;
  base::ScopedObservation<AdBlockTabHelper, AdBlockTabHelper::Observer>
      tab_helper_observation_{this};
  // The open panel. The delegate outlives its widget.
  std::unique_ptr<views::BubbleDialogDelegate> bubble_delegate_;
  std::unique_ptr<views::Widget> bubble_widget_;

  base::WeakPtrFactory<ShieldsToolbarButton> weak_factory_{this};
};

}  // namespace pantera

#endif  // PANTERA_BROWSER_UI_VIEWS_SHIELDS_SHIELDS_TOOLBAR_BUTTON_H_

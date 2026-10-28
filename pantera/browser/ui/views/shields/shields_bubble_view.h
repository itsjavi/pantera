// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PANTERA_BROWSER_UI_VIEWS_SHIELDS_SHIELDS_BUBBLE_VIEW_H_
#define PANTERA_BROWSER_UI_VIEWS_SHIELDS_SHIELDS_BUBBLE_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/scoped_observation.h"
#include "content/public/browser/web_contents_observer.h"
#include "pantera/browser/adblock/ad_block_tab_helper.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"
#include "url/gurl.h"

class HostContentSettingsMap;

namespace content_settings {
class CookieSettings;
}  // namespace content_settings

namespace views {
class BubbleDialogDelegate;
class Combobox;
class ImageButton;
class ImageView;
class Label;
class ScrollView;
class StyledLabel;
class ToggleButton;
class View;
class Widget;
}  // namespace views

namespace pantera {

class ShieldsToolbarButton;

// The Shields panel (DECISION-19): shields on or off for the site, what was
// blocked on the page, and the site's protection settings. Rows for
// protections Pantera doesn't have yet are shown off and disabled. Changing a
// setting reloads the page.
class ShieldsBubbleView : public views::View,
                          public content::WebContentsObserver,
                          public AdBlockTabHelper::Observer {
  METADATA_HEADER(ShieldsBubbleView, views::View)

 public:
  enum class InitialView {
    kMain,
    // The main page with the advanced options open.
    kAdvanced,
    // The list of blocked requests.
    kBlocked,
  };

  // The bubble that holds the panel, anchored to `anchor` and sized to the
  // panel. The caller owns it and the widget made from it.
  static std::unique_ptr<views::BubbleDialogDelegate> CreateBubbleDelegate(
      ShieldsToolbarButton* anchor,
      content::WebContents* web_contents,
      InitialView initial_view = InitialView::kMain);

  ShieldsBubbleView(ShieldsToolbarButton* button,
                    content::WebContents* web_contents,
                    InitialView initial_view);
  ShieldsBubbleView(const ShieldsBubbleView&) = delete;
  ShieldsBubbleView& operator=(const ShieldsBubbleView&) = delete;
  ~ShieldsBubbleView() override;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

  // AdBlockTabHelper::Observer:
  void OnBlockedRequestsChanged() override;
  void OnAdBlockTabHelperDestroyed() override;

 private:
  void BuildMainPage();
  void BuildHeader(views::View* parent);
  void BuildBlockedCount(views::View* parent);
  void BuildAdvancedOptions(views::View* parent);
  void BuildFooter(views::View* parent);
  void BuildBlockedPage();

  // Shows the page's current site, settings and blocked requests.
  void Refresh();
  void RefreshSite();
  void RefreshBlockedList();
  void ShowBlockedPage(bool show);

  void OnShieldsToggled();
  void OnAdsModeChanged();
  void OnScriptsToggled();
  void OnCookiesModeChanged();
  void OnAdvancedToggled();
  // Settings apply to the next load, so the panel reloads the page.
  void SettingChanged();

  GURL GetSiteUrl() const;
  size_t GetBlockedCount() const;

  const raw_ptr<ShieldsToolbarButton> button_;
  const scoped_refptr<HostContentSettingsMap> settings_map_;
  const scoped_refptr<content_settings::CookieSettings> cookie_settings_;
  base::ScopedObservation<AdBlockTabHelper, AdBlockTabHelper::Observer>
      tab_helper_observation_{this};

  // Main page.
  raw_ptr<views::View> main_page_ = nullptr;
  raw_ptr<views::ImageView> favicon_view_ = nullptr;
  raw_ptr<views::Label> site_label_ = nullptr;
  raw_ptr<views::StyledLabel> status_label_ = nullptr;
  raw_ptr<views::ToggleButton> shields_toggle_ = nullptr;
  raw_ptr<views::View> blocked_count_card_ = nullptr;
  raw_ptr<views::Label> blocked_count_number_ = nullptr;
  raw_ptr<views::Label> blocked_count_text_ = nullptr;
  raw_ptr<views::View> advanced_section_ = nullptr;
  raw_ptr<views::ImageButton> advanced_expand_button_ = nullptr;
  raw_ptr<views::View> advanced_rows_ = nullptr;
  raw_ptr<views::Combobox> ads_combobox_ = nullptr;
  raw_ptr<views::ToggleButton> scripts_toggle_ = nullptr;
  raw_ptr<views::Combobox> cookies_combobox_ = nullptr;

  // List of blocked requests.
  raw_ptr<views::View> blocked_page_ = nullptr;
  raw_ptr<views::Label> blocked_title_ = nullptr;
  raw_ptr<views::ScrollView> blocked_scroll_ = nullptr;
  raw_ptr<views::View> blocked_list_ = nullptr;
};

}  // namespace pantera

#endif  // PANTERA_BROWSER_UI_VIEWS_SHIELDS_SHIELDS_BUBBLE_VIEW_H_

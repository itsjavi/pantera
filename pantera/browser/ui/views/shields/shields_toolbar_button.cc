// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/browser/ui/views/shields/shields_toolbar_button.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/web_contents.h"
#include "pantera/app/grit/pantera_strings.h"
#include "pantera/browser/ui/views/shields/shields_bubble_view.h"
#include "pantera/components/shields/shields_settings.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/text_utils.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/button_controller.h"

namespace pantera {

namespace {

// Developer switch: opens the panel a few seconds after the window appears,
// for screenshots (scripts/capture-app-windows.swift). "advanced" opens the
// advanced options and "blocked" the list of blocked requests.
constexpr char kShowShieldsPanelSwitch[] = "pantera-show-shields-panel";
constexpr base::TimeDelta kShowShieldsPanelDelay = base::Seconds(12);

}  // namespace

// The number of blocked requests, in a pill over the shield's bottom right
// corner, ringed in the toolbar's color so it stands out from the shield.
class ShieldsCountBadge : public views::View {
  METADATA_HEADER(ShieldsCountBadge, views::View)

 public:
  ShieldsCountBadge() { SetCanProcessEventsWithinSubtree(false); }

  void SetCount(size_t count) {
    text_ = count > 99 ? u"99+" : base::NumberToString16(count);
    SetVisible(count > 0);
    PreferredSizeChanged();
    SchedulePaint();
  }

  // views::View:
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override {
    const int height = GetFont().GetHeight() + 2 * kRing;
    return gfx::Size(
        std::max(height, gfx::GetStringWidth(text_, GetFont()) + 6 + 2 * kRing),
        height);
  }

  void OnPaint(gfx::Canvas* canvas) override {
    const ui::ColorProvider* colors = GetColorProvider();
    gfx::RectF bounds(GetLocalBounds());
    cc::PaintFlags flags;
    flags.setAntiAlias(true);
    flags.setColor(colors->GetColor(kColorToolbar));
    canvas->DrawRoundRect(bounds, bounds.height() / 2, flags);
    bounds.Inset(kRing);
    flags.setColor(colors->GetColor(ui::kColorSysPrimary));
    canvas->DrawRoundRect(bounds, bounds.height() / 2, flags);
    canvas->DrawStringRectWithFlags(
        text_, GetFont(), colors->GetColor(ui::kColorSysOnPrimary),
        gfx::ToEnclosingRect(bounds), gfx::Canvas::TEXT_ALIGN_CENTER);
  }

 private:
  static constexpr int kRing = 2;

  static gfx::FontList GetFont() {
    return gfx::FontList().DeriveWithSizeDelta(-4).DeriveWithWeight(
        gfx::Font::Weight::BOLD);
  }

  std::u16string text_;
};

BEGIN_METADATA(ShieldsCountBadge)
END_METADATA

ShieldsToolbarButton::ShieldsToolbarButton(BrowserWindowInterface* browser)
    : ToolbarButton(base::BindRepeating(&ShieldsToolbarButton::OnPressed,
                                        base::Unretained(this))),
      browser_(browser) {
  button_controller()->set_notify_action(
      views::ButtonController::NotifyAction::kOnPress);
  GetViewAccessibility().SetHasPopup(ax::mojom::HasPopup::kDialog);
  badge_ = AddChildView(std::make_unique<ShieldsCountBadge>());
  browser_->GetTabStripModel()->AddObserver(this);
  ObserveActiveTab();
  UpdateState();

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          kShowShieldsPanelSwitch)) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&ShieldsToolbarButton::ShowBubbleForSwitch,
                       weak_factory_.GetWeakPtr()),
        kShowShieldsPanelDelay);
  }
}

ShieldsToolbarButton::~ShieldsToolbarButton() {
  browser_->GetTabStripModel()->RemoveObserver(this);
  bubble_widget_.reset();
  bubble_delegate_.reset();
}

void ShieldsToolbarButton::UpdateState() {
  shields_up_ = AreShieldsUpOnActiveTab();
  // Pages that aren't web pages, such as the New Tab page, have nothing to
  // shield.
  content::WebContents* active = GetActiveWebContents();
  SetEnabled(active && active->GetLastCommittedURL().SchemeIsHTTPOrHTTPS());
  if (!GetEnabled() && bubble_widget_) {
    bubble_widget_->Close();
  }

  size_t count = 0;
  if (content::WebContents* contents = GetActiveWebContents()) {
    if (AdBlockTabHelper* helper =
            AdBlockTabHelper::FromWebContents(contents)) {
      count = helper->blocked_count();
    }
  }
  badge_->SetCount(shields_up_ ? count : 0);
  InvalidateLayout();

  SetTooltipText(l10n_util::GetStringUTF16(
      shields_up_ ? IDS_PANTERA_SHIELDS_BUTTON_TOOLTIP
                  : IDS_PANTERA_SHIELDS_BUTTON_TOOLTIP_DOWN));
  GetViewAccessibility().SetName(
      shields_up_
          ? l10n_util::GetStringFUTF16(IDS_PANTERA_SHIELDS_BUTTON_ACCNAME,
                                       base::NumberToString16(count))
          : l10n_util::GetStringUTF16(IDS_PANTERA_SHIELDS_BUTTON_TOOLTIP_DOWN));
  UpdateIcon();
}

void ShieldsToolbarButton::UpdateIcon() {
  if (!GetColorProvider()) {
    return;
  }
  // The toolbar's own icon colors when shields are up, its disabled color
  // when they're down; the badge carries Pantera's accent.
  if (shields_up_) {
    UpdateIconsWithStandardColors(vector_icons::kShieldIcon);
    return;
  }
  const SkColor color = GetForegroundColor(ButtonState::STATE_DISABLED);
  UpdateIconsWithColors(vector_icons::kShieldIcon, color, color, color, color);
}

void ShieldsToolbarButton::Layout(PassKey) {
  LayoutSuperclass<ToolbarButton>(this);
  // The badge hangs off the shield's bottom right corner, inside the button.
  const gfx::Rect icon = image_container_view()->bounds();
  const gfx::Size size = badge_->GetPreferredSize();
  badge_->SetBounds(std::min(icon.right() + 6, width()) - size.width(),
                    std::min(icon.bottom() + 5, height()) - size.height(),
                    size.width(), size.height());
}

void ShieldsToolbarButton::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    ObserveActiveTab();
    UpdateState();
  }
}

void ShieldsToolbarButton::OnBlockedRequestsChanged() {
  UpdateState();
}

void ShieldsToolbarButton::OnAdBlockTabHelperDestroyed() {
  tab_helper_observation_.Reset();
}

void ShieldsToolbarButton::OnPressed() {
  if (bubble_widget_) {
    bubble_widget_->Close();
    return;
  }
  ShowBubble(ShieldsBubbleView::InitialView::kMain);
}

void ShieldsToolbarButton::ShowBubbleForSwitch() {
  const std::string value =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          kShowShieldsPanelSwitch);
  ShowBubble(value == "advanced"  ? ShieldsBubbleView::InitialView::kAdvanced
             : value == "blocked" ? ShieldsBubbleView::InitialView::kBlocked
                                  : ShieldsBubbleView::InitialView::kMain);
  // Stay open for the screenshot when the page takes the focus back.
  if (bubble_delegate_) {
    bubble_delegate_->set_close_on_deactivate(false);
  }
}

void ShieldsToolbarButton::ShowBubble(
    ShieldsBubbleView::InitialView initial_view) {
  content::WebContents* contents = GetActiveWebContents();
  if (!contents || bubble_widget_) {
    return;
  }
  bubble_delegate_ =
      ShieldsBubbleView::CreateBubbleDelegate(this, contents, initial_view);
  bubble_widget_ = views::BubbleDialogDelegate::CreateBubble(
      bubble_delegate_.get(),
      base::BindOnce(&ShieldsToolbarButton::OnBubbleClosed,
                     base::Unretained(this)));
  bubble_widget_->Show();
}

void ShieldsToolbarButton::OnBubbleClosed(views::Widget::ClosedReason reason) {
  // The widget is still notifying its observers, the delegate among them:
  // delete both after this task, the widget first.
  auto task_runner = base::SequencedTaskRunner::GetCurrentDefault();
  if (bubble_widget_) {
    task_runner->DeleteSoon(FROM_HERE, std::move(bubble_widget_));
  }
  if (bubble_delegate_) {
    task_runner->DeleteSoon(FROM_HERE, std::move(bubble_delegate_));
  }
}

void ShieldsToolbarButton::ObserveActiveTab() {
  tab_helper_observation_.Reset();
  if (content::WebContents* contents = GetActiveWebContents()) {
    tab_helper_observation_.Observe(
        AdBlockTabHelper::GetOrCreateForWebContents(contents));
  }
}

content::WebContents* ShieldsToolbarButton::GetActiveWebContents() const {
  return browser_->GetTabStripModel()->GetActiveWebContents();
}

bool ShieldsToolbarButton::AreShieldsUpOnActiveTab() const {
  content::WebContents* contents = GetActiveWebContents();
  if (!contents) {
    return true;
  }
  const HostContentSettingsMap* map =
      HostContentSettingsMapFactory::GetForProfile(browser_->GetProfile());
  // The button is disabled on pages that aren't web pages, and looks it.
  const GURL& url = contents->GetLastCommittedURL();
  return !url.SchemeIsHTTPOrHTTPS() || AreShieldsUp(*map, url);
}

BEGIN_METADATA(ShieldsToolbarButton)
END_METADATA

}  // namespace pantera

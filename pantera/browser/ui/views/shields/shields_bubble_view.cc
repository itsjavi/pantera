// Copyright 2026 The Pantera Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pantera/browser/ui/views/shields/shields_bubble_view.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tab_ui_helper.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/controls/rich_controls_container_view.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/tabs/public/tab_interface.h"
#include "components/url_formatter/url_formatter.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/web_contents.h"
#include "pantera/app/grit/pantera_strings.h"
#include "pantera/browser/ui/views/shields/shields_toolbar_button.h"
#include "pantera/components/shields/shields_settings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/combobox_model.h"
#include "ui/base/models/image_model.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/color/color_id.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/text_elider.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/separator.h"
#include "ui/views/controls/styled_label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/style/typography.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/widget/widget.h"

namespace pantera {

namespace {

constexpr int kBubbleWidth = 360;
constexpr int kIconSize = 20;
constexpr int kFaviconSize = 24;
constexpr int kBlockedListMaxHeight = 320;

// The panel remembers, for the session, whether the advanced options are
// open.
bool g_advanced_expanded = false;

// Trackers and ads: the order of the combobox items.
enum AdsItem : size_t {
  kAdsAggressive = 0,
  kAdsStandard,
  kAdsAllowAll,  // Not available yet.
};

// Cookies: the order of the combobox items.
enum CookiesItem : size_t {
  kCookiesBlockThirdParty = 0,
  kCookiesBlockAll,
  kCookiesAllowAll,
};

// A combobox model of fixed strings, some of which may be unavailable.
class ItemsModel : public ui::ComboboxModel {
 public:
  ItemsModel(std::vector<int> string_ids, std::vector<bool> enabled)
      : string_ids_(std::move(string_ids)), enabled_(std::move(enabled)) {}

  // ui::ComboboxModel:
  size_t GetItemCount() const override { return string_ids_.size(); }
  std::u16string GetItemAt(size_t index) const override {
    return l10n_util::GetStringUTF16(string_ids_[index]);
  }
  bool IsItemEnabledAt(size_t index) const override { return enabled_[index]; }

 private:
  const std::vector<int> string_ids_;
  const std::vector<bool> enabled_;
};

// Rows start their icon and their text where page info's rows
// (RichControlsContainerView) do, so the panel's icons line up in one column.
int GetHorizontalPadding() {
  return ChromeLayoutProvider::Get()
      ->GetInsetsMetric(INSETS_PAGE_INFO_HOVER_BUTTON)
      .left();
}

int GetIconLabelSpacing() {
  return ChromeLayoutProvider::Get()->GetDistanceMetric(
      DISTANCE_RICH_HOVER_BUTTON_ICON_HORIZONTAL);
}

std::unique_ptr<views::View> MakeSeparator() {
  auto separator = std::make_unique<views::Separator>();
  separator->SetColorId(ui::kColorSysDivider);
  return separator;
}

// A row with an icon and a combobox that takes the rest of the width.
views::Combobox* AddComboboxRow(views::View* parent,
                                const gfx::VectorIcon& icon,
                                std::unique_ptr<ui::ComboboxModel> model,
                                int accessible_name_id) {
  auto* row = parent->AddChildView(std::make_unique<views::BoxLayoutView>());
  row->SetOrientation(views::BoxLayout::Orientation::kHorizontal);
  row->SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kCenter);
  row->SetInsideBorderInsets(gfx::Insets::VH(6, GetHorizontalPadding()));
  row->SetBetweenChildSpacing(GetIconLabelSpacing());
  row->AddChildView(std::make_unique<views::ImageView>(
      ui::ImageModel::FromVectorIcon(icon, ui::kColorIcon, kIconSize)));
  auto* combobox =
      row->AddChildView(std::make_unique<views::Combobox>(std::move(model)));
  row->SetFlexForView(combobox, 1);
  combobox->GetViewAccessibility().SetName(
      l10n_util::GetStringUTF16(accessible_name_id));
  return combobox;
}

// A row with an icon, a title and a switch on the right. Rows for
// protections Pantera doesn't have yet say so and can't be switched.
views::ToggleButton* AddToggleRow(views::View* parent,
                                  const gfx::VectorIcon& icon,
                                  int title_id,
                                  bool available,
                                  views::Button::PressedCallback callback) {
  auto* row =
      parent->AddChildView(std::make_unique<RichControlsContainerView>());
  row->SetIcon(ui::ImageModel::FromVectorIcon(icon, ui::kColorIcon, kIconSize));
  row->SetTitle(l10n_util::GetStringUTF16(title_id));
  if (!available) {
    row->AddSecondaryLabel(
        l10n_util::GetStringUTF16(IDS_PANTERA_SHIELDS_COMING_SOON));
  }
  auto* toggle = row->AddControl(
      std::make_unique<views::ToggleButton>(std::move(callback)));
  toggle->SetPreferredSize(
      gfx::Size(toggle->GetPreferredSize().width(), row->GetFirstLineHeight()));
  toggle->GetViewAccessibility().SetName(l10n_util::GetStringUTF16(title_id));
  toggle->SetEnabled(available);
  return toggle;
}

}  // namespace

// static
std::unique_ptr<views::BubbleDialogDelegate>
ShieldsBubbleView::CreateBubbleDelegate(ShieldsToolbarButton* anchor,
                                        content::WebContents* web_contents,
                                        InitialView initial_view) {
  auto delegate = std::make_unique<views::BubbleDialogDelegate>(
      anchor, views::BubbleBorder::TOP_RIGHT,
      views::BubbleBorder::DIALOG_SHADOW, /*autosize=*/true);
  delegate->SetTitle(
      l10n_util::GetStringUTF16(IDS_PANTERA_SHIELDS_BUTTON_TOOLTIP));
  delegate->SetShowTitle(false);
  delegate->SetShowCloseButton(false);
  delegate->SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
  delegate->set_margins(gfx::Insets());
  delegate->set_fixed_width(kBubbleWidth);
  delegate->SetContentsView(
      std::make_unique<ShieldsBubbleView>(anchor, web_contents, initial_view));
  return delegate;
}

ShieldsBubbleView::ShieldsBubbleView(ShieldsToolbarButton* button,
                                     content::WebContents* web_contents,
                                     InitialView initial_view)
    : content::WebContentsObserver(web_contents),
      button_(button),
      settings_map_(HostContentSettingsMapFactory::GetForProfile(
          Profile::FromBrowserContext(web_contents->GetBrowserContext()))),
      cookie_settings_(CookieSettingsFactory::GetForProfile(
          Profile::FromBrowserContext(web_contents->GetBrowserContext()))) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  tab_helper_observation_.Observe(
      AdBlockTabHelper::GetOrCreateForWebContents(web_contents));

  if (initial_view == InitialView::kAdvanced) {
    g_advanced_expanded = true;
  }
  BuildMainPage();
  BuildBlockedPage();
  Refresh();
  if (initial_view == InitialView::kBlocked) {
    ShowBlockedPage(true);
  }
}

ShieldsBubbleView::~ShieldsBubbleView() = default;

void ShieldsBubbleView::BuildMainPage() {
  main_page_ = AddChildView(std::make_unique<views::BoxLayoutView>());
  static_cast<views::BoxLayoutView*>(main_page_.get())
      ->SetOrientation(views::BoxLayout::Orientation::kVertical);
  BuildHeader(main_page_);
  BuildBlockedCount(main_page_);
  BuildAdvancedOptions(main_page_);
  BuildFooter(main_page_);
}

void ShieldsBubbleView::BuildHeader(views::View* parent) {
  auto* header = parent->AddChildView(std::make_unique<views::BoxLayoutView>());
  header->SetOrientation(views::BoxLayout::Orientation::kHorizontal);
  header->SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kCenter);
  header->SetInsideBorderInsets(gfx::Insets::TLBR(16, GetHorizontalPadding(),
                                                  12, GetHorizontalPadding()));
  header->SetBetweenChildSpacing(12);

  favicon_view_ = header->AddChildView(std::make_unique<views::ImageView>());
  favicon_view_->SetImageSize(gfx::Size(kFaviconSize, kFaviconSize));

  auto* labels = header->AddChildView(std::make_unique<views::BoxLayoutView>());
  labels->SetOrientation(views::BoxLayout::Orientation::kVertical);
  labels->SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kStart);
  header->SetFlexForView(labels, 1);

  site_label_ = labels->AddChildView(std::make_unique<views::Label>(
      std::u16string(), views::style::CONTEXT_DIALOG_TITLE,
      views::style::STYLE_HEADLINE_4));
  site_label_->SetEnabledColor(ui::kColorSysOnSurface);
  site_label_->SetElideBehavior(gfx::ELIDE_HEAD);
  site_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  status_label_ = labels->AddChildView(std::make_unique<views::StyledLabel>());
  status_label_->SetTextContext(views::style::CONTEXT_LABEL);
  status_label_->SetDefaultTextStyle(views::style::STYLE_BODY_4);

  shields_toggle_ = header->AddChildView(
      std::make_unique<views::ToggleButton>(base::BindRepeating(
          &ShieldsBubbleView::OnShieldsToggled, base::Unretained(this))));
  shields_toggle_->GetViewAccessibility().SetName(
      l10n_util::GetStringUTF16(IDS_PANTERA_SHIELDS_TOGGLE_ACCNAME));
}

void ShieldsBubbleView::BuildBlockedCount(views::View* parent) {
  auto* card = parent->AddChildView(std::make_unique<views::BoxLayoutView>());
  blocked_count_card_ = card;
  card->SetOrientation(views::BoxLayout::Orientation::kHorizontal);
  card->SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kCenter);
  card->SetInsideBorderInsets(gfx::Insets::TLBR(10, 16, 10, 8));
  card->SetBetweenChildSpacing(10);
  card->SetProperty(
      views::kMarginsKey,
      gfx::Insets::TLBR(0, GetHorizontalPadding(), 16, GetHorizontalPadding()));
  card->SetBackground(
      views::CreateRoundedRectBackground(ui::kColorSysNeutralContainer, 12));

  blocked_count_number_ = card->AddChildView(std::make_unique<views::Label>());
  blocked_count_number_->SetFontList(
      gfx::FontList().DeriveWithSizeDelta(12).DeriveWithWeight(
          gfx::Font::Weight::BOLD));

  blocked_count_text_ = card->AddChildView(std::make_unique<views::Label>(
      std::u16string(), views::style::CONTEXT_LABEL,
      views::style::STYLE_BODY_3));
  blocked_count_text_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  card->SetFlexForView(blocked_count_text_, 1);

  auto* show_list =
      card->AddChildView(views::CreateVectorImageButtonWithNativeTheme(
          base::BindRepeating(&ShieldsBubbleView::ShowBlockedPage,
                              base::Unretained(this), true),
          vector_icons::kKeyboardArrowRightFlippableIcon, kIconSize));
  const std::u16string show_list_name =
      l10n_util::GetStringUTF16(IDS_PANTERA_SHIELDS_SHOW_BLOCKED);
  show_list->SetTooltipText(show_list_name);
  show_list->GetViewAccessibility().SetName(show_list_name);
}

void ShieldsBubbleView::BuildAdvancedOptions(views::View* parent) {
  advanced_section_ =
      parent->AddChildView(std::make_unique<views::BoxLayoutView>());
  auto* section = static_cast<views::BoxLayoutView*>(advanced_section_.get());
  section->SetOrientation(views::BoxLayout::Orientation::kVertical);
  section->AddChildView(MakeSeparator());

  // "Advanced options", which opens and closes the rows below.
  auto* heading =
      section->AddChildView(std::make_unique<views::BoxLayoutView>());
  heading->SetOrientation(views::BoxLayout::Orientation::kHorizontal);
  heading->SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kCenter);
  auto* label_button =
      heading->AddChildView(std::make_unique<views::LabelButton>(
          base::BindRepeating(&ShieldsBubbleView::OnAdvancedToggled,
                              base::Unretained(this)),
          l10n_util::GetStringUTF16(IDS_PANTERA_SHIELDS_ADVANCED)));
  label_button->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(vector_icons::kSettingsIcon,
                                     ui::kColorSysPrimary, kIconSize));
  label_button->SetEnabledTextColors(ui::kColorSysPrimary);
  label_button->SetImageLabelSpacing(GetIconLabelSpacing());
  // No side padding, so the gear lines up with the rows' icons.
  label_button->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(4, 0)));
  heading->SetFlexForView(label_button, 1);
  advanced_expand_button_ =
      heading->AddChildView(views::CreateVectorImageButtonWithNativeTheme(
          base::BindRepeating(&ShieldsBubbleView::OnAdvancedToggled,
                              base::Unretained(this)),
          vector_icons::kKeyboardArrowDownIcon, kIconSize));
  advanced_expand_button_->GetViewAccessibility().SetName(
      l10n_util::GetStringUTF16(IDS_PANTERA_SHIELDS_ADVANCED));
  // The chevron button's own padding counts towards the row's.
  heading->SetInsideBorderInsets(gfx::Insets::TLBR(
      8, GetHorizontalPadding(), 8,
      GetHorizontalPadding() - advanced_expand_button_->GetInsets().right()));

  advanced_rows_ =
      section->AddChildView(std::make_unique<views::BoxLayoutView>());
  auto* rows = static_cast<views::BoxLayoutView*>(advanced_rows_.get());
  rows->SetOrientation(views::BoxLayout::Orientation::kVertical);
  rows->SetInsideBorderInsets(gfx::Insets::TLBR(0, 0, 8, 0));

  ads_combobox_ =
      AddComboboxRow(rows, vector_icons::kAdOffIcon,
                     std::make_unique<ItemsModel>(
                         std::vector<int>{IDS_PANTERA_SHIELDS_ADS_AGGRESSIVE,
                                          IDS_PANTERA_SHIELDS_ADS_STANDARD,
                                          IDS_PANTERA_SHIELDS_ADS_ALLOW},
                         std::vector<bool>{true, true, false}),
                     IDS_PANTERA_SHIELDS_ADS_ACCNAME);
  ads_combobox_->SetCallback(base::BindRepeating(
      &ShieldsBubbleView::OnAdsModeChanged, base::Unretained(this)));

  // Chromium upgrades connections to HTTPS for every site already; a
  // per-site choice comes later.
  views::ToggleButton* https_toggle =
      AddToggleRow(rows, vector_icons::kLockIcon, IDS_PANTERA_SHIELDS_HTTPS,
                   /*available=*/false, views::Button::PressedCallback());
  https_toggle->SetIsOn(true);

  scripts_toggle_ =
      AddToggleRow(rows, vector_icons::kCodeIcon, IDS_PANTERA_SHIELDS_SCRIPTS,
                   /*available=*/true,
                   base::BindRepeating(&ShieldsBubbleView::OnScriptsToggled,
                                       base::Unretained(this)));

  AddToggleRow(rows, vector_icons::kFingerprintIcon,
               IDS_PANTERA_SHIELDS_FINGERPRINTING, /*available=*/false,
               views::Button::PressedCallback());

  cookies_combobox_ = AddComboboxRow(
      rows, vector_icons::kCookieIcon,
      std::make_unique<ItemsModel>(
          std::vector<int>{IDS_PANTERA_SHIELDS_COOKIES_THIRD_PARTY,
                           IDS_PANTERA_SHIELDS_COOKIES_ALL,
                           IDS_PANTERA_SHIELDS_COOKIES_ALLOW},
          std::vector<bool>{true, true, true}),
      IDS_PANTERA_SHIELDS_COOKIES_ACCNAME);
  cookies_combobox_->SetCallback(base::BindRepeating(
      &ShieldsBubbleView::OnCookiesModeChanged, base::Unretained(this)));

  AddToggleRow(rows, vector_icons::kHistoryIcon, IDS_PANTERA_SHIELDS_FORGET_ME,
               /*available=*/false, views::Button::PressedCallback());
}

void ShieldsBubbleView::BuildFooter(views::View* parent) {
  parent->AddChildView(MakeSeparator());

  // Filter lists and global settings come with the settings page (TASK-34).
  auto* buttons =
      parent->AddChildView(std::make_unique<views::BoxLayoutView>());
  buttons->SetOrientation(views::BoxLayout::Orientation::kHorizontal);
  buttons->SetInsideBorderInsets(gfx::Insets::VH(12, GetHorizontalPadding()));
  buttons->SetBetweenChildSpacing(8);
  for (int string_id : {IDS_PANTERA_SHIELDS_FILTER_LISTS,
                        IDS_PANTERA_SHIELDS_GLOBAL_SETTINGS}) {
    auto* button = buttons->AddChildView(std::make_unique<views::MdTextButton>(
        views::Button::PressedCallback(),
        l10n_util::GetStringUTF16(string_id)));
    button->SetEnabled(false);
    button->SetTooltipText(
        l10n_util::GetStringUTF16(IDS_PANTERA_SHIELDS_COMING_SOON));
    buttons->SetFlexForView(button, 1);
  }

  parent->AddChildView(MakeSeparator());
  auto* footer = parent->AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_PANTERA_SHIELDS_FOOTER),
      views::style::CONTEXT_LABEL, views::style::STYLE_BODY_5));
  footer->SetMultiLine(true);
  footer->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  footer->SetEnabledColor(ui::kColorSysOnSurfaceSubtle);
  footer->SetBorder(
      views::CreateEmptyBorder(gfx::Insets::VH(12, GetHorizontalPadding())));
}

void ShieldsBubbleView::BuildBlockedPage() {
  auto* page = AddChildView(std::make_unique<views::BoxLayoutView>());
  blocked_page_ = page;
  page->SetOrientation(views::BoxLayout::Orientation::kVertical);
  page->SetVisible(false);

  auto* header = page->AddChildView(std::make_unique<views::BoxLayoutView>());
  header->SetOrientation(views::BoxLayout::Orientation::kHorizontal);
  header->SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kCenter);
  header->SetInsideBorderInsets(
      gfx::Insets::TLBR(12, 8, 12, GetHorizontalPadding()));
  header->SetBetweenChildSpacing(8);
  auto* back =
      header->AddChildView(views::CreateVectorImageButtonWithNativeTheme(
          base::BindRepeating(&ShieldsBubbleView::ShowBlockedPage,
                              base::Unretained(this), false),
          vector_icons::kArrowBackIcon, kIconSize));
  const std::u16string back_name =
      l10n_util::GetStringUTF16(IDS_PANTERA_SHIELDS_BACK);
  back->SetTooltipText(back_name);
  back->GetViewAccessibility().SetName(back_name);
  blocked_title_ = header->AddChildView(std::make_unique<views::Label>(
      std::u16string(), views::style::CONTEXT_DIALOG_TITLE,
      views::style::STYLE_HEADLINE_5));
  blocked_title_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  header->SetFlexForView(blocked_title_, 1);

  page->AddChildView(MakeSeparator());
  blocked_scroll_ = page->AddChildView(std::make_unique<views::ScrollView>());
  blocked_scroll_->ClipHeightTo(0, kBlockedListMaxHeight);
  auto list = std::make_unique<views::BoxLayoutView>();
  list->SetOrientation(views::BoxLayout::Orientation::kVertical);
  list->SetInsideBorderInsets(gfx::Insets::VH(8, GetHorizontalPadding()));
  list->SetBetweenChildSpacing(6);
  blocked_list_ = blocked_scroll_->SetContents(std::move(list));
}

void ShieldsBubbleView::RefreshSite() {
  // The tab's favicon, as the tab strip shows it, or a globe.
  ui::ImageModel favicon;
  if (tabs::TabInterface* tab =
          tabs::TabInterface::MaybeGetFromContents(web_contents())) {
    favicon = TabUIHelper::From(tab)->GetFavicon();
  }
  if (favicon.IsEmpty()) {
    favicon = ui::ImageModel::FromVectorIcon(vector_icons::kGlobeIcon,
                                             ui::kColorIcon, kFaviconSize);
  }
  favicon_view_->SetImage(std::move(favicon));
  site_label_->SetText(url_formatter::IDNToUnicode(GetSiteUrl().host()));
}

void ShieldsBubbleView::Refresh() {
  RefreshSite();
  const GURL url = GetSiteUrl();
  const bool shields_up = AreShieldsUp(*settings_map_, url);

  const std::u16string state =
      l10n_util::GetStringUTF16(shields_up ? IDS_PANTERA_SHIELDS_STATUS_UP
                                           : IDS_PANTERA_SHIELDS_STATUS_DOWN);
  size_t offset = 0;
  status_label_->SetText(
      l10n_util::GetStringFUTF16(IDS_PANTERA_SHIELDS_STATUS, state, &offset));
  status_label_->ClearStyleRanges();
  views::StyledLabel::RangeStyleInfo bold;
  bold.text_style = views::style::STYLE_BODY_4_BOLD;
  status_label_->AddStyleRange(gfx::Range(offset, offset + state.size()), bold);
  shields_toggle_->SetIsOn(shields_up);

  // With shields down there's nothing blocked and nothing to tune.
  blocked_count_card_->SetVisible(shields_up);
  advanced_section_->SetVisible(shields_up);
  const size_t count = GetBlockedCount();
  blocked_count_number_->SetText(base::NumberToString16(count));
  blocked_count_text_->SetText(l10n_util::GetPluralStringFUTF16(
      IDS_PANTERA_SHIELDS_BLOCKED_COUNT, static_cast<int>(count)));

  advanced_rows_->SetVisible(g_advanced_expanded);
  views::SetImageFromVectorIconWithColor(
      advanced_expand_button_,
      g_advanced_expanded ? vector_icons::kKeyboardArrowUpIcon
                          : vector_icons::kKeyboardArrowDownIcon,
      kIconSize, views::IconColors(ui::kColorIcon, ui::kColorIconDisabled));
  ads_combobox_->SetSelectedIndex(IsAggressiveBlocking(*settings_map_, url)
                                      ? kAdsAggressive
                                      : kAdsStandard);
  scripts_toggle_->SetIsOn(AreScriptsBlocked(*settings_map_, url));
  switch (GetCookieMode(*settings_map_, *cookie_settings_, url)) {
    case CookieMode::kBlockThirdParty:
      cookies_combobox_->SetSelectedIndex(kCookiesBlockThirdParty);
      break;
    case CookieMode::kBlockAll:
      cookies_combobox_->SetSelectedIndex(kCookiesBlockAll);
      break;
    case CookieMode::kAllowAll:
      cookies_combobox_->SetSelectedIndex(kCookiesAllowAll);
      break;
  }

  RefreshBlockedList();
}

void ShieldsBubbleView::RefreshBlockedList() {
  blocked_title_->SetText(base::StrCat(
      {l10n_util::GetStringUTF16(IDS_PANTERA_SHIELDS_BLOCKED_TITLE), u" (",
       base::NumberToString16(GetBlockedCount()), u")"}));
  blocked_list_->RemoveAllChildViews();
  const AdBlockTabHelper* helper =
      AdBlockTabHelper::FromWebContents(web_contents());
  if (!helper || helper->blocked_urls().empty()) {
    blocked_list_->AddChildView(std::make_unique<views::Label>(
        l10n_util::GetStringUTF16(IDS_PANTERA_SHIELDS_BLOCKED_EMPTY),
        views::style::CONTEXT_LABEL, views::style::STYLE_BODY_4));
    return;
  }
  for (const GURL& url : helper->blocked_urls()) {
    auto* label = blocked_list_->AddChildView(std::make_unique<views::Label>(
        base::UTF8ToUTF16(url.spec()), views::style::CONTEXT_LABEL,
        views::style::STYLE_BODY_5));
    label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    label->SetElideBehavior(gfx::ELIDE_MIDDLE);
    label->SetTooltipText(base::UTF8ToUTF16(url.spec()));
  }
}

void ShieldsBubbleView::ShowBlockedPage(bool show) {
  main_page_->SetVisible(!show);
  blocked_page_->SetVisible(show);
}

void ShieldsBubbleView::OnShieldsToggled() {
  SetShieldsUp(*settings_map_, GetSiteUrl(), shields_toggle_->GetIsOn());
  SettingChanged();
}

void ShieldsBubbleView::OnAdsModeChanged() {
  SetAggressiveBlocking(*settings_map_, GetSiteUrl(),
                        ads_combobox_->GetSelectedIndex() == kAdsAggressive);
  SettingChanged();
}

void ShieldsBubbleView::OnScriptsToggled() {
  SetScriptsBlocked(*settings_map_, GetSiteUrl(), scripts_toggle_->GetIsOn());
  SettingChanged();
}

void ShieldsBubbleView::OnCookiesModeChanged() {
  CookieMode mode = CookieMode::kBlockThirdParty;
  if (cookies_combobox_->GetSelectedIndex() == kCookiesBlockAll) {
    mode = CookieMode::kBlockAll;
  } else if (cookies_combobox_->GetSelectedIndex() == kCookiesAllowAll) {
    mode = CookieMode::kAllowAll;
  }
  SetCookieMode(*settings_map_, *cookie_settings_, GetSiteUrl(), mode);
  SettingChanged();
}

void ShieldsBubbleView::OnAdvancedToggled() {
  g_advanced_expanded = !g_advanced_expanded;
  Refresh();
}

void ShieldsBubbleView::SettingChanged() {
  web_contents()->GetController().Reload(content::ReloadType::NORMAL,
                                         /*check_for_repost=*/true);
  button_->UpdateState();
  Refresh();
}

GURL ShieldsBubbleView::GetSiteUrl() const {
  return web_contents() ? web_contents()->GetLastCommittedURL() : GURL();
}

size_t ShieldsBubbleView::GetBlockedCount() const {
  const AdBlockTabHelper* helper =
      web_contents() ? AdBlockTabHelper::FromWebContents(web_contents())
                     : nullptr;
  return helper ? helper->blocked_count() : 0;
}

void ShieldsBubbleView::WebContentsDestroyed() {
  tab_helper_observation_.Reset();
  if (GetWidget()) {
    GetWidget()->Close();
  }
}

void ShieldsBubbleView::OnBlockedRequestsChanged() {
  Refresh();
}

void ShieldsBubbleView::OnAdBlockTabHelperDestroyed() {
  tab_helper_observation_.Reset();
}

BEGIN_METADATA(ShieldsBubbleView)
END_METADATA

}  // namespace pantera

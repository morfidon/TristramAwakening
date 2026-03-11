/**
 * @file qol/visual_store.cpp
 *
 * Implementation of visual grid-based store UI.
 */
#include "qol/visual_store.h"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string_view>

#include "control/control.hpp"
#include "controls/plrctrls.h"
#include "cursor.h"
#include "engine/clx_sprite.hpp"
#include "engine/load_clx.hpp"
#include "engine/points_in_rectangle_range.hpp"
#include "engine/rectangle.hpp"
#include "engine/render/clx_render.hpp"
#include "engine/render/text_render.hpp"
#include "engine/size.hpp"
#include "game_mode.hpp"
#include "headless_mode.hpp"
#include "inv.h"
#include "items.h"
#include "minitext.h"
#include "options.h"
#include "panels/info_box.hpp"
#include "panels/ui_panels.hpp"
#include "player.h"
#include "qol/stash.h"
#include "spells.h"
#include "stores.h"
#include "utils/format_int.hpp"
#include "utils/language.h"
#include "utils/str_cat.hpp"

namespace devilution {

bool IsVisualStoreOpen;
VisualStoreState VisualStore;
int16_t pcursstoreitem = -1;
int16_t pcursstorebtn = -1;

namespace {

OptionalOwnedClxSpriteList VisualStorePanelArt;
OptionalOwnedClxSpriteList VisualStoreNavButtonArt;
OptionalOwnedClxSpriteList VisualStoreRepairAllButtonArt;
OptionalOwnedClxSpriteList VisualStoreRepairButtonArt;

int VisualStoreButtonPressed = -1;

constexpr Size ButtonSize { 27, 16 };

/** Contains mappings for the buttons in the visual store (page nav, tabs, leave) */
constexpr Rectangle VisualStoreButtonRect[] = {
	// Page navigation buttons
	//{ { 19, 19 }, ButtonSize },  // 10 left
	//{ { 56, 19 }, ButtonSize },  // 1 left
	//{ { 242, 19 }, ButtonSize }, // 1 right
	//{ { 279, 19 }, ButtonSize }, // 10 right
	// Tab buttons
	{ { 14, 21 }, { 72, 22 } },          // Tab 0
	{ { 14 + 73, 21 }, { 72, 22 } },     // Tab 1
	{ { 14 + 73 * 2, 21 }, { 72, 22 } }, // Tab 2
	{ { 233, 315 }, { 48, 24 } },        // Repair All Btn
	{ { 286, 315 }, { 24, 24 } },        // Repair Btn
};

// constexpr int NavButton10Left = 0;
// constexpr int NavButton1Left = 1;
// constexpr int NavButton1Right = 2;
// constexpr int NavButton10Right = 3;
constexpr int TabButton0 = 0;
constexpr int TabButton1 = 1;
constexpr int TabButton2 = 2;
constexpr int RepairAllBtn = 3;
constexpr int RepairBtn = 4;

/** @brief Get the stock items array for a specific vendor/tab combination. */
std::span<Item> GetVendorStockItems(VisualStoreVendor vendor, VisualStoreTab tab)
{
	switch (vendor) {
	case VisualStoreVendor::Smith: {
		if (tab == VisualStoreTab::Premium) {
			return { PremiumItems.data(), static_cast<size_t>(PremiumItems.size()) };
		}
		return { SmithItems.data(), static_cast<size_t>(SmithItems.size()) };
	}
	case VisualStoreVendor::Witch: {
		return { WitchItems.data(), static_cast<size_t>(WitchItems.size()) };
	}
	case VisualStoreVendor::Healer: {
		return { HealerItems.data(), static_cast<size_t>(HealerItems.size()) };
	}
	case VisualStoreVendor::Boy: {
		if (BoyItem.isEmpty()) {
			return {};
		}
		return { &BoyItem, 1 };
	}
	}
	return {};
}

TalkID GetVisualStoreTalkId(VisualStoreVendor vendor)
{
	switch (vendor) {
	case VisualStoreVendor::Smith:
		return TalkID::Smith;
	case VisualStoreVendor::Witch:
		return TalkID::Witch;
	case VisualStoreVendor::Healer:
	case VisualStoreVendor::Boy:
		return TalkID::None;
	}
	return TalkID::None;
}

StaticVector<Item, MaxStoreBuybackItems> *GetVendorBuybackList(VisualStoreVendor vendor)
{
	return GetStoreBuybackItems(GetVisualStoreTalkId(vendor));
}

bool CurrentVendorHasBuybackItems()
{
	const auto *buybackItems = GetStoreBuybackItems(GetVisualStoreTalkId(VisualStore.vendor));
	return buybackItems != nullptr && !buybackItems->empty();
}

int GetVisibleVisualStoreTabCountInternal()
{
	int tabCount = 1; // Basic or Misc
	if (VisualStore.vendor == VisualStoreVendor::Smith)
		tabCount++;
	if (CurrentVendorHasBuybackItems())
		tabCount++;
	return tabCount;
}

bool TryGetTabAtIndex(int tabIndex, VisualStoreTab &tab)
{
	if (tabIndex < 0 || tabIndex >= GetVisibleVisualStoreTabCountInternal())
		return false;

	if (tabIndex == 0) {
		tab = VisualStoreTab::Basic;
		return true;
	}

	if (VisualStore.vendor == VisualStoreVendor::Smith) {
		if (tabIndex == 1) {
			tab = VisualStoreTab::Premium;
			return true;
		}
		if (tabIndex == 2 && CurrentVendorHasBuybackItems()) {
			tab = VisualStoreTab::Buyback;
			return true;
		}
		return false;
	}

	if (tabIndex == 1 && CurrentVendorHasBuybackItems()) {
		tab = VisualStoreTab::Buyback;
		return true;
	}

	return false;
}

int GetTabIndex(VisualStoreTab tab)
{
	for (int tabIndex = 0; tabIndex < GetVisibleVisualStoreTabCountInternal(); ++tabIndex) {
		VisualStoreTab mappedTab;
		if (TryGetTabAtIndex(tabIndex, mappedTab) && mappedTab == tab)
			return tabIndex;
	}

	return 0;
}

std::string_view GetTabLabel(VisualStoreTab tab)
{
	switch (tab) {
	case VisualStoreTab::Basic:
		return VisualStore.vendor == VisualStoreVendor::Smith ? _("Basic") : _("Misc");
	case VisualStoreTab::Premium:
		return _("Premium");
	case VisualStoreTab::Buyback:
		return _("Buyback");
	}
	return "";
}

std::string_view GetTabDescription(VisualStoreTab tab)
{
	switch (tab) {
	case VisualStoreTab::Basic:
		return VisualStore.vendor == VisualStoreVendor::Smith ? _("Basic items") : _("Miscellaneous items");
	case VisualStoreTab::Premium:
		return _("Premium items");
	case VisualStoreTab::Buyback:
		return _("Items sold to this vendor");
	}
	return "";
}

bool IsTabAvailable(VisualStoreTab tab)
{
	switch (tab) {
	case VisualStoreTab::Basic:
		return true;
	case VisualStoreTab::Premium:
		return VisualStore.vendor == VisualStoreVendor::Smith;
	case VisualStoreTab::Buyback:
		return CurrentVendorHasBuybackItems();
	}
	return false;
}

const Item *GetEntryItem(const VisualStoreEntry &entry)
{
	switch (entry.source) {
	case VisualStoreItemSource::VendorStock: {
		const std::span<Item> items = GetVendorStockItems(VisualStore.vendor, VisualStore.activeTab);
		if (entry.sourceIndex >= items.size())
			return nullptr;
		return &items[entry.sourceIndex];
	}
	case VisualStoreItemSource::Buyback: {
		const auto *buybackItems = GetVendorBuybackList(VisualStore.vendor);
		if (buybackItems == nullptr || entry.sourceIndex >= buybackItems->size())
			return nullptr;
		return &(*buybackItems)[entry.sourceIndex];
	}
	}
	return nullptr;
}

Item *GetEntryItemMutable(const VisualStoreEntry &entry)
{
	switch (entry.source) {
	case VisualStoreItemSource::VendorStock: {
		std::span<Item> items = GetVendorStockItems(VisualStore.vendor, VisualStore.activeTab);
		if (entry.sourceIndex >= items.size())
			return nullptr;
		return &items[entry.sourceIndex];
	}
	case VisualStoreItemSource::Buyback: {
		auto *buybackItems = GetVendorBuybackList(VisualStore.vendor);
		if (buybackItems == nullptr || entry.sourceIndex >= buybackItems->size())
			return nullptr;
		return &(*buybackItems)[entry.sourceIndex];
	}
	}
	return nullptr;
}

void BuildVisualStoreEntries()
{
	VisualStore.entries.clear();

	auto addBuybackEntries = [](VisualStoreVendor vendor) {
		auto *buybackItems = GetVendorBuybackList(vendor);
		if (buybackItems == nullptr)
			return;

		for (size_t i = buybackItems->size(); i > 0; --i) {
			const uint16_t sourceIndex = static_cast<uint16_t>(i - 1);
			if ((*buybackItems)[sourceIndex].isEmpty())
				continue;
			VisualStore.entries.push_back({ VisualStoreItemSource::Buyback, sourceIndex });
		}
	};

	auto addVendorStockEntries = [](VisualStoreVendor vendor, VisualStoreTab tab) {
		const std::span<Item> items = GetVendorStockItems(vendor, tab);
		for (uint16_t i = 0; i < items.size(); ++i) {
			if (items[i].isEmpty())
				continue;
			VisualStore.entries.push_back({ VisualStoreItemSource::VendorStock, i });
		}
	};

	switch (VisualStore.vendor) {
	case VisualStoreVendor::Smith:
		if (VisualStore.activeTab == VisualStoreTab::Buyback) {
			addBuybackEntries(VisualStoreVendor::Smith);
		} else {
			addVendorStockEntries(VisualStoreVendor::Smith, VisualStore.activeTab);
		}
		break;
	case VisualStoreVendor::Witch:
		if (VisualStore.activeTab == VisualStoreTab::Buyback) {
			addBuybackEntries(VisualStoreVendor::Witch);
		} else {
			addVendorStockEntries(VisualStoreVendor::Witch, VisualStore.activeTab);
		}
		break;
	case VisualStoreVendor::Healer:
	case VisualStoreVendor::Boy:
		addVendorStockEntries(VisualStore.vendor, VisualStore.activeTab);
		break;
	}
}

void RemoveVisualStoreEntry(const VisualStoreEntry &entry)
{
	switch (entry.source) {
	case VisualStoreItemSource::Buyback: {
		RemoveItemFromVendorBuyback(GetVisualStoreTalkId(VisualStore.vendor), entry.sourceIndex);
		break;
	}
	case VisualStoreItemSource::VendorStock:
		switch (VisualStore.vendor) {
		case VisualStoreVendor::Smith:
			if (VisualStore.activeTab == VisualStoreTab::Premium) {
				PremiumItems[entry.sourceIndex].clear();
				SpawnPremium(*MyPlayer);
			} else {
				SmithItems.erase(SmithItems.begin() + entry.sourceIndex);
			}
			break;
		case VisualStoreVendor::Witch:
			if (entry.sourceIndex >= NumWitchPinnedItems)
				WitchItems.erase(WitchItems.begin() + entry.sourceIndex);
			break;
		case VisualStoreVendor::Healer:
			if (entry.sourceIndex >= static_cast<uint16_t>(gbIsMultiplayer ? NumHealerPinnedItemsMp : NumHealerPinnedItems))
				HealerItems.erase(HealerItems.begin() + entry.sourceIndex);
			break;
		case VisualStoreVendor::Boy:
			BoyItem.clear();
			break;
		}
		break;
	}
}

/** @brief Check if the current vendor has tabs (Smith only). */
bool VendorHasTabs()
{
	return GetVisibleVisualStoreTabCountInternal() > 1;
}

/** @brief Check if the current vendor accepts items for sale. */
bool VendorAcceptsSale()
{
	switch (VisualStore.vendor) {
	case VisualStoreVendor::Smith:
	case VisualStoreVendor::Witch: {
		return true;
	}
	case VisualStoreVendor::Healer:
	case VisualStoreVendor::Boy: {
		return false;
	}
	}
	return false;
}

/** @brief Rebuild the grid layout for the current vendor/tab. */
void RefreshVisualStoreLayout()
{
	if (!IsTabAvailable(VisualStore.activeTab))
		VisualStore.activeTab = VisualStoreTab::Basic;

	BuildVisualStoreEntries();
	for (const VisualStoreEntry &entry : VisualStore.entries) {
		Item *item = GetEntryItemMutable(entry);
		if (item != nullptr)
			item->_iStatFlag = MyPlayer->CanUseItem(*item);
	}

	VisualStore.pages.clear();

	if (VisualStore.entries.empty()) {
		VisualStore.pages.emplace_back();
		VisualStorePage &page = VisualStore.pages.back();
		memset(page.grid, 0, sizeof(page.grid));
		return;
	}

	auto createNewPage = [&]() -> VisualStorePage & {
		VisualStore.pages.emplace_back();
		VisualStorePage &page = VisualStore.pages.back();
		memset(page.grid, 0, sizeof(page.grid));
		return page;
	};

	VisualStorePage *currentPage = &createNewPage();

	for (uint16_t i = 0; i < static_cast<uint16_t>(VisualStore.entries.size()); i++) {
		const Item *item = GetEntryItem(VisualStore.entries[i]);
		if (item == nullptr || item->isEmpty())
			continue;

		const Size itemSize = GetInventorySize(*item);
		bool placed = false;

		// Try to place in current page
		for (auto stashPosition : PointsInRectangle(Rectangle { { 0, 0 }, Size { VisualStoreGridWidth - (itemSize.width - 1), VisualStoreGridHeight - (itemSize.height - 1) } })) {
			bool isSpaceFree = true;
			for (auto itemPoint : PointsInRectangle(Rectangle { stashPosition, itemSize })) {
				if (currentPage->grid[itemPoint.x][itemPoint.y] != 0) {
					isSpaceFree = false;
					break;
				}
			}

			if (isSpaceFree) {
				for (auto itemPoint : PointsInRectangle(Rectangle { stashPosition, itemSize })) {
					currentPage->grid[itemPoint.x][itemPoint.y] = i + 1;
				}
				currentPage->items.push_back({ i, stashPosition + Displacement { 0, itemSize.height - 1 } });
				placed = true;
				break;
			}
		}

		if (!placed) {
			// Start new page
			currentPage = &createNewPage();
			// Try placing again in new page
			for (auto stashPosition : PointsInRectangle(Rectangle { { 0, 0 }, Size { VisualStoreGridWidth - (itemSize.width - 1), VisualStoreGridHeight - (itemSize.height - 1) } })) {
				bool isSpaceFree = true;
				for (auto itemPoint : PointsInRectangle(Rectangle { stashPosition, itemSize })) {
					if (currentPage->grid[itemPoint.x][itemPoint.y] != 0) {
						isSpaceFree = false;
						break;
					}
				}

				if (isSpaceFree) {
					for (auto itemPoint : PointsInRectangle(Rectangle { stashPosition, itemSize })) {
						currentPage->grid[itemPoint.x][itemPoint.y] = i + 1;
					}
					currentPage->items.push_back({ i, stashPosition + Displacement { 0, itemSize.height - 1 } });
					placed = true;
					break;
				}
			}
		}
	}

	if (VisualStore.currentPage >= VisualStore.pages.size())
		VisualStore.currentPage = VisualStore.pages.empty() ? 0 : static_cast<unsigned>(VisualStore.pages.size() - 1);
}

} // namespace

void InitVisualStore()
{
	if (HeadlessMode)
		return;

	// For now, reuse the stash panel art as a placeholder
	// In the future, create dedicated visual store assets
	VisualStorePanelArt = LoadClx("data\\store.clx");
	VisualStoreNavButtonArt = LoadClx("data\\tabBtnUp.clx");
	VisualStoreRepairAllButtonArt = LoadClx("data\\repairAllBtn.clx");
	VisualStoreRepairButtonArt = LoadClx("data\\repairSingleBtn.clx");
}

void FreeVisualStoreGFX()
{
	VisualStoreNavButtonArt = std::nullopt;
	VisualStorePanelArt = std::nullopt;
}

void OpenVisualStore(VisualStoreVendor vendor)
{
	IsVisualStoreOpen = true;
	invflag = true; // Open inventory panel alongside

	VisualStore.vendor = vendor;
	VisualStore.activeTab = VisualStoreTab::Basic;
	VisualStore.currentPage = 0;

	pcursstoreitem = -1;
	pcursstorebtn = -1;

	RefreshVisualStoreLayout();

	// Initialize controller focus to the visual store grid
	FocusOnVisualStore();
}

void CloseVisualStore()
{
	if (IsVisualStoreOpen) {
		IsVisualStoreOpen = false;
		invflag = false;
		pcursstoreitem = -1;
		pcursstorebtn = -1;
		VisualStoreButtonPressed = -1;
		VisualStore.entries.clear();
		VisualStore.pages.clear();
	}
}

void SetVisualStoreTab(VisualStoreTab tab)
{
	/*if (!VendorHasTabs())
	    return;*/

	VisualStore.activeTab = tab;
	VisualStore.currentPage = 0;
	pcursstoreitem = -1;
	pcursstorebtn = -1;

	RefreshVisualStoreLayout();
}

bool CycleVisualStoreTab(int step)
{
	const int tabCount = GetVisibleVisualStoreTabCountInternal();
	if (tabCount <= 1 || step == 0)
		return false;

	const int currentTabIndex = GetTabIndex(VisualStore.activeTab);
	const int normalizedStep = step > 0 ? 1 : -1;
	const int nextTabIndex = (currentTabIndex + normalizedStep + tabCount) % tabCount;

	VisualStoreTab nextTab;
	if (!TryGetTabAtIndex(nextTabIndex, nextTab) || nextTab == VisualStore.activeTab)
		return false;

	SetVisualStoreTab(nextTab);
	return true;
}

void VisualStoreNextPage()
{
	if (VisualStore.currentPage + 1 < VisualStore.pages.size()) {
		VisualStore.currentPage++;
		pcursstoreitem = -1;
		pcursstorebtn = -1;
	}
}

void VisualStorePreviousPage()
{
	if (VisualStore.currentPage > 0) {
		VisualStore.currentPage--;
		pcursstoreitem = -1;
		pcursstorebtn = -1;
	}
}

int GetRepairCost(const Item &item)
{
	if (item.isEmpty() || item._iDurability == item._iMaxDur || item._iMaxDur == DUR_INDESTRUCTIBLE)
		return 0;

	const int due = item._iMaxDur - item._iDurability;
	if (item._iMagical != ITEM_QUALITY_NORMAL && item._iIdentified) {
		return 30 * item._iIvalue * due / (item._iMaxDur * 100 * 2);
	} else {
		return std::max(item._ivalue * due / (item._iMaxDur * 2), 1);
	}
}

void VisualStoreRepairAll()
{
	Player &myPlayer = *MyPlayer;
	int totalCost = 0;

	// Check body items
	for (auto &item : myPlayer.InvBody) {
		totalCost += GetRepairCost(item);
	}

	// Check inventory items
	for (int i = 0; i < myPlayer._pNumInv; i++) {
		totalCost += GetRepairCost(myPlayer.InvList[i]);
	}

	if (totalCost == 0)
		return;

	if (!PlayerCanAfford(totalCost)) {
		// Optional: add a message that player can't afford
		return;
	}

	// Execute repairs
	TakePlrsMoney(totalCost);

	for (auto &item : myPlayer.InvBody) {
		if (!item.isEmpty() && item._iMaxDur != DUR_INDESTRUCTIBLE)
			item._iDurability = item._iMaxDur;
	}

	for (int i = 0; i < myPlayer._pNumInv; i++) {
		Item &item = myPlayer.InvList[i];
		if (!item.isEmpty() && item._iMaxDur != DUR_INDESTRUCTIBLE)
			item._iDurability = item._iMaxDur;
	}

	PlaySFX(SfxID::ItemGold);
	CalcPlrInv(myPlayer, true);
}

void VisualStoreRepair()
{
	NewCursor(CURSOR_REPAIR);
}

void VisualStoreRepairItem(int invIndex)
{
	Player &myPlayer = *MyPlayer;
	Item *item = nullptr;

	if (invIndex < INVITEM_INV_FIRST) {
		item = &myPlayer.InvBody[invIndex];
	} else if (invIndex <= INVITEM_INV_LAST) {
		item = &myPlayer.InvList[invIndex - INVITEM_INV_FIRST];
	} else {
		return; // Belt items don't have durability
	}

	if (item->isEmpty())
		return;

	int cost = GetRepairCost(*item);
	if (cost <= 0)
		return;

	if (!PlayerCanAfford(cost)) {
		// PlaySFX(HeroSpeech::ICantUseThisYet); // Or some other error sound
		return;
	}

	TakePlrsMoney(cost);
	item->_iDurability = item->_iMaxDur;
	PlaySFX(SfxID::ItemGold);
	CalcPlrInv(myPlayer, true);
}

Point GetVisualStoreSlotCoord(Point slot)
{
	constexpr int SlotSpacing = INV_SLOT_SIZE_PX + 1;
	// Grid starts below the header area
	// constexpr Displacement GridOffset { 17, 60 + (INV_SLOT_SIZE_PX/2) };

	return GetPanelPosition(UiPanels::Stash, slot * SlotSpacing + Displacement { 17, 44 });
}

Rectangle GetVisualBtnCoord(int btnId)
{
	const Point panelPos = GetPanelPosition(UiPanels::Stash);
	const Rectangle regBtnPos = { panelPos + (VisualStoreButtonRect[btnId].position - Point { 0, 0 }), VisualStoreButtonRect[btnId].size };

	return regBtnPos;
}

int GetVisualStoreItemCount()
{
	return static_cast<int>(VisualStore.entries.size());
}

const VisualStoreEntry *GetVisualStoreEntry(int16_t entryIndex)
{
	if (entryIndex < 0 || static_cast<size_t>(entryIndex) >= VisualStore.entries.size())
		return nullptr;
	return &VisualStore.entries[entryIndex];
}

const Item *GetVisualStoreItem(int16_t entryIndex)
{
	const VisualStoreEntry *entry = GetVisualStoreEntry(entryIndex);
	if (entry == nullptr)
		return nullptr;
	return GetEntryItem(*entry);
}

Item *GetVisualStoreItemMutable(int16_t entryIndex)
{
	const VisualStoreEntry *entry = GetVisualStoreEntry(entryIndex);
	if (entry == nullptr)
		return nullptr;
	return GetEntryItemMutable(*entry);
}

int GetVisualStorePageCount()
{
	return std::max(1, static_cast<int>(VisualStore.pages.size()));
}

int GetVisibleVisualStoreTabCount()
{
	return GetVisibleVisualStoreTabCountInternal();
}

int GetActiveVisualStoreTabIndex()
{
	return GetTabIndex(VisualStore.activeTab);
}

void DrawVisualStore(const Surface &out)
{
	if (!VisualStorePanelArt)
		return;

	// Draw panel background (reusing stash art for now)
	RenderClxSprite(out, (*VisualStorePanelArt)[0], GetPanelPosition(UiPanels::Stash));

	const Point panelPos = GetPanelPosition(UiPanels::Stash);
	const UiFlags styleWhite = UiFlags::VerticalCenter | UiFlags::ColorWhite;
	const UiFlags styleTabPushed = UiFlags::VerticalCenter | UiFlags::ColorButtonpushed;
	constexpr int TextHeight = 13;

	// Draw store title
	/*DrawString(out, GetVendorName(), { panelPos + Displacement { 0, 2 }, { 320, TextHeight } },
	    { .flags = UiFlags::AlignCenter | styleGold });*/

	// Draw tab buttons
	for (int tabIndex = 0; tabIndex < GetVisibleVisualStoreTabCountInternal(); ++tabIndex) {
		VisualStoreTab tab;
		if (!TryGetTabAtIndex(tabIndex, tab))
			continue;

		const Rectangle tabBtnPos = { panelPos + (VisualStoreButtonRect[tabIndex].position - Point { 0, 0 }), VisualStoreButtonRect[tabIndex].size };
		const UiFlags tabStyle = VisualStore.activeTab == tab ? styleWhite : styleTabPushed;
		RenderClxSprite(out, (*VisualStoreNavButtonArt)[VisualStore.activeTab != tab], tabBtnPos.position);
		DrawString(out, GetTabLabel(tab), tabBtnPos, { .flags = UiFlags::AlignCenter | tabStyle });
	}

	// Draw page number
	/*int pageCount = GetVisualStorePageCount();
	std::string pageText = StrCat(VisualStore.currentPage + 1, "/", pageCount);
	DrawString(out, pageText, { panelPos + Displacement { 132, 40 }, { 57, TextHeight } },
	    { .flags = UiFlags::AlignCenter | styleWhite });*/

	if (VisualStore.currentPage >= VisualStore.pages.size())
		return;

	const VisualStorePage &page = VisualStore.pages[VisualStore.currentPage];

	constexpr Displacement offset { 0, INV_SLOT_SIZE_PX - 1 };

	// First pass: draw item slot backgrounds
	for (int y = 0; y < VisualStoreGridHeight; y++) {
		for (int x = 0; x < VisualStoreGridWidth; x++) {
			const uint16_t itemPlusOne = page.grid[x][y];
			if (itemPlusOne == 0)
				continue;

			const Item *item = GetVisualStoreItem(itemPlusOne - 1);
			if (item == nullptr)
				continue;
			Point position = GetVisualStoreSlotCoord({ x, y }) + offset;
			InvDrawSlotBack(out, position, InventorySlotSizeInPixels, item->_iMagical);
		}
	}

	// Second pass: draw item sprites
	for (const auto &vsItem : page.items) {
		const Item *item = GetVisualStoreItem(vsItem.entryIndex);
		if (item == nullptr)
			continue;
		Point position = GetVisualStoreSlotCoord(vsItem.position) + offset;

		const int frame = item->_iCurs + CURSOR_FIRSTITEM;
		const ClxSprite sprite = GetInvItemSprite(frame);

		// Draw highlight outline if this item is hovered
		if (pcursstoreitem == vsItem.entryIndex) {
			const uint8_t color = GetOutlineColor(*item, true);
			ClxDrawOutline(out, color, position, sprite);
		}

		DrawItem(*item, out, position, sprite);
	}

	// Draw player gold at bottom
	uint32_t totalGold = MyPlayer->_pGold + Stash.gold;
	DrawString(out, StrCat(_("Gold: "), FormatInteger(totalGold)),
	    { panelPos + Displacement { 20, 320 }, { 280, TextHeight } },
	    { .flags = styleWhite });

	// Draw Repair All
	if (VisualStore.vendor == VisualStoreVendor::Smith) {
		const Rectangle repairAllBtnPos = { panelPos + (VisualStoreButtonRect[RepairAllBtn].position - Point { 0, 0 }), VisualStoreButtonRect[RepairAllBtn].size };
		RenderClxSprite(out, (*VisualStoreRepairAllButtonArt)[VisualStoreButtonPressed == RepairAllBtn], repairAllBtnPos.position);
		// DrawString(out, _("Repair All"), repairAllBtnPos, { .flags = UiFlags::AlignCenter | styleWhite

		// Draw Repair
		const Rectangle repairBtnPos = { panelPos + (VisualStoreButtonRect[RepairBtn].position - Point { 0, 0 }), VisualStoreButtonRect[RepairBtn].size };
		RenderClxSprite(out, (*VisualStoreRepairButtonArt)[VisualStoreButtonPressed == RepairBtn], repairBtnPos.position);
		// DrawString(out, _("Repair"), repairBtnPos, { .flags = UiFlags::AlignCenter | styleWhite });
	}
}

int16_t CheckVisualStoreHLight(Point mousePosition)
{
	// Check buttons first
	const Point panelPos = GetPanelPosition(UiPanels::Stash);
	if (MyPlayer->HoldItem.isEmpty()) {
		for (int i = 0; i < 5; i++) {
			if (i <= TabButton2 && i >= GetVisibleVisualStoreTabCountInternal())
				continue;

			Rectangle button = VisualStoreButtonRect[i];
			button.position = panelPos + (button.position - Point { 0, 0 });

			if (button.contains(mousePosition)) {
				if (i <= TabButton2) {
					VisualStoreTab tab;
					if (!TryGetTabAtIndex(i, tab))
						return -1;
					InfoString = GetTabLabel(tab);
					FloatingInfoString = GetTabLabel(tab);
					AddInfoBoxString(GetTabDescription(tab));
					AddInfoBoxString(GetTabDescription(tab), true);
					InfoColor = UiFlags::ColorWhite;
					pcursstorebtn = i;
					return -1;
				} else if (i == RepairAllBtn) {
					int totalCost = 0;
					Player &myPlayer = *MyPlayer;
					for (auto &item : myPlayer.InvBody)
						totalCost += GetRepairCost(item);
					for (int j = 0; j < myPlayer._pNumInv; j++)
						totalCost += GetRepairCost(myPlayer.InvList[j]);

					InfoString = _("Repair All");
					FloatingInfoString = _("Repair All");
					if (totalCost > 0) {
						AddInfoBoxString(StrCat(FormatInteger(totalCost), " Gold"));
						AddInfoBoxString(StrCat(FormatInteger(totalCost), " Gold"), true);
					} else {
						AddInfoBoxString(_("Nothing to repair"));
						AddInfoBoxString(_("Nothing to repair"), true);
					}
					InfoColor = UiFlags::ColorWhite;
					pcursstorebtn = RepairAllBtn;
					return -1;
				} else if (i == RepairBtn) {
					InfoString = _("Repair");
					FloatingInfoString = _("Repair");
					AddInfoBoxString(_("Repair a single item"));
					AddInfoBoxString(_("Repair a single item"), true);
					InfoColor = UiFlags::ColorWhite;
					pcursstorebtn = RepairBtn;
					return -1;
				}
			}
		}
	}

	if (VisualStore.currentPage >= VisualStore.pages.size())
		return -1;

	const VisualStorePage &page = VisualStore.pages[VisualStore.currentPage];

	for (int y = 0; y < VisualStoreGridHeight; y++) {
		for (int x = 0; x < VisualStoreGridWidth; x++) {
			const uint16_t itemPlusOne = page.grid[x][y];
			if (itemPlusOne == 0)
				continue;

			const int itemIndex = itemPlusOne - 1;
			const Item *item = GetVisualStoreItem(itemIndex);
			if (item == nullptr)
				continue;

			const Rectangle cell {
				GetVisualStoreSlotCoord({ x, y }),
				InventorySlotSizeInPixels + 1
			};

			if (cell.contains(mousePosition)) {
				// Set up info display
				InfoColor = item->getTextColor();
				InfoString = item->getName();

				const int price = item->_iIvalue;
				const bool canAfford = PlayerCanAfford(price);

				InfoString = item->getName();
				FloatingInfoString = item->getName();
				InfoColor = canAfford ? item->getTextColor() : UiFlags::ColorRed;

				if (item->_iIdentified) {
					PrintItemDetails(*item);
				} else {
					PrintItemDur(*item);
				}

				AddInfoBoxString(StrCat("", FormatInteger(price), " Gold"));

				return static_cast<int16_t>(itemIndex);
			}
		}
	}

	return -1;
}

void CheckVisualStoreItem(Point mousePosition, bool isCtrlHeld, bool isShiftHeld)
{
	// Check if clicking on an item to buy
	int16_t itemIndex = CheckVisualStoreHLight(mousePosition);
	if (itemIndex < 0)
		return;

	const VisualStoreEntry *entryPtr = GetVisualStoreEntry(itemIndex);
	const Item *itemPtr = GetVisualStoreItem(itemIndex);
	if (entryPtr == nullptr || itemPtr == nullptr || itemPtr->isEmpty())
		return;
	const VisualStoreEntry entry = *entryPtr;
	const Item item = *itemPtr;
	Item placementCheckItem = item;

	// Check if player can afford the item
	const int price = item._iIvalue;
	uint32_t totalGold = MyPlayer->_pGold + Stash.gold;
	if (totalGold < static_cast<uint32_t>(price)) {
		// InitDiabloMsg(EMSG_NOT_ENOUGH_GOLD);
		return;
	}

	// Check if player has room for the item
	if (!StoreAutoPlace(placementCheckItem, false)) {
		// InitDiabloMsg(EMSG_INVENTORY_FULL);
		return;
	}

	// Execute the purchase
	TakePlrsMoney(price);
	Item placedItem = item;
	StoreAutoPlace(placedItem, true);
	PlaySFX(ItemInvSnds[ItemCAnimTbl[item._iCurs]]);
	RemoveVisualStoreEntry(entry);

	pcursstoreitem = -1;
	RefreshVisualStoreLayout();
}

void CheckVisualStorePaste(Point mousePosition)
{
	if (!VendorAcceptsSale())
		return;

	Player &player = *MyPlayer;
	if (player.HoldItem.isEmpty())
		return;

	// Check if the item can be sold to this vendor
	if (!CanSellToCurrentVendor(player.HoldItem)) {
		player.SaySpecific(HeroSpeech::ICantDoThat);
		return;
	}

	const Item soldItem = player.HoldItem;
	// Calculate sell price
	int sellPrice = GetStoreSellPrice(soldItem);

	// Add gold to player
	AddGoldToInventory(player, sellPrice);
	PlaySFX(SfxID::ItemGold);

	// Clear the held item
	player.HoldItem.clear();
	NewCursor(CURSOR_HAND);
	AddItemToVendorBuyback(GetVisualStoreTalkId(VisualStore.vendor), soldItem);
	pcursstoreitem = -1;
	RefreshVisualStoreLayout();
}

bool CanSellToCurrentVendor(const Item &item)
{
	return CanVendorBuyItem(GetVisualStoreTalkId(VisualStore.vendor), item);
}

void SellItemToVisualStore(int invIndex)
{
	if (!VendorAcceptsSale())
		return;

	Player &player = *MyPlayer;
	Item &item = player.InvList[invIndex];

	if (!CanSellToCurrentVendor(item)) {
		player.SaySpecific(HeroSpeech::ICantDoThat);
		return;
	}

	const Item soldItem = item;
	// Calculate sell price
	int sellPrice = GetStoreSellPrice(soldItem);

	// Add gold to player
	AddGoldToInventory(player, sellPrice);
	PlaySFX(SfxID::ItemGold);

	// Remove item from inventory
	player.RemoveInvItem(invIndex);
	AddItemToVendorBuyback(GetVisualStoreTalkId(VisualStore.vendor), soldItem);
	pcursstoreitem = -1;
	RefreshVisualStoreLayout();
}

void CheckVisualStoreButtonPress(Point mousePosition)
{
	if (!MyPlayer->HoldItem.isEmpty())
		return;

	for (int i = 0; i < 5; i++) {
		if (i <= TabButton2 && i >= GetVisibleVisualStoreTabCountInternal())
			continue;

		Rectangle button = VisualStoreButtonRect[i];
		button.position = GetPanelPosition(UiPanels::Stash, button.position);

		if (button.contains(mousePosition)) {
			VisualStoreButtonPressed = i;
			return;
		}
	}

	VisualStoreButtonPressed = -1;
}

void CheckVisualStoreButtonRelease(Point mousePosition)
{
	if (VisualStoreButtonPressed == -1)
		return;

	Rectangle button = VisualStoreButtonRect[VisualStoreButtonPressed];
	button.position = GetPanelPosition(UiPanels::Stash, button.position);

	if (button.contains(mousePosition)) {
		switch (VisualStoreButtonPressed) {
		// case NavButton10Left:
		//	for (int i = 0; i < 10 && VisualStore.currentPage > 0; i++)
		//		VisualStorePreviousPage();
		//	break;
		// case NavButton1Left:
		//	VisualStorePreviousPage();
		//	break;
		// case NavButton1Right:
		//	VisualStoreNextPage();
		//	break;
		// case NavButton10Right:
		//	for (int i = 0; i < 10; i++)
		//		VisualStoreNextPage();
		//	break;
		case TabButton0:
		case TabButton1:
		case TabButton2: {
			VisualStoreTab tab;
			if (TryGetTabAtIndex(VisualStoreButtonPressed, tab))
				SetVisualStoreTab(tab);
			break;
		}
		case RepairAllBtn: {
			VisualStoreRepairAll();
			break;
		}
		case RepairBtn: {
			VisualStoreRepair();
			break;
		}
		}
	}

	VisualStoreButtonPressed = -1;
}

} // namespace devilution

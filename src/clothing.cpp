#include "clothing.h"

#include <algorithm>
#include <iterator>

namespace {
	// The game works out outfit warmth in long_update.c (func_2416, func_2679, func_2680, func_2853):
	// https://github.com/JayKoZa/RDR2-Decompiled-Scripts/blob/master/script_rel/long_update.c
	// Each worn item can carry one warmth tier tag (0-4); tiers of items in the counted categories add up.

	// joaat("tag_item_property"): the tag type the warmth tags belong to.
	const Hash TAG_ITEM_PROPERTY = 0x42D03BDE;
	// Warmth tier tags, tier 0 to 4.
	const Hash WARMTH_TIER_TAGS[] = { 0xA4D7DE35, 0x8C776615, 0xCF0F873A, 0xC8ED2D2B, 0x9BDDBF61 };

	// Items with this tag don't count.
	const Hash WARMTH_SKIP_TAG = 0x0FD143D5;

	// Item categories that count towards warmth.
	const Hash WARMTH_CATEGORIES[] = {
		0x851EBBD3, 0x9989398B, 0x9EB11578, 0xAA25A557, 0xAB219849, 0xCDFBBB30, 0xE2522CFB, 0x04D4C7F4, 0x0A4688FB, 0x1B7C2B66, 0x1DFA352C,
		0x21811425, 0x28C9B4C5, 0x290B0496, 0x29A49A30, 0x4C6BA84F, 0x5813E4A1, 0x6971A6CB, 0x6F58734F, 0x782A9DBA, 0x7A0945B8 };

	// Give everyday cloths that have no warmth have some warmth. 
	// Make being naked impactful if I ever want to go through that again
	// Slot categories from https://github.com/femga/rdr3_discoveries/blob/master/clothes/cloth_hash_names.lua
	struct SlotWarmth {
		Hash slot;
		float warmth;
	};
	const SlotWarmth PLAIN_CLOTHING_WARMTH[] = {
		{ 0x9925C067, 0.5f }, // hats
		{ 0x2026C46D, 0.25f }, // shirts_full
		{ 0x1D4C528A, 0.25f }, // pants
		{ 0x777EC6EF, 0.25f }, // boots
	};

	// ITEMDATABASE_FILLOUT_ITEM_INFO's output. natives.h calls the first field the category, but in game
	// it holds the item's own hash; the second is the category the game checks. Spare room in case the
	// game writes more than these fields.
	struct ItemInfo {
		alignas(8) Hash first;
		alignas(8) Hash category;
		alignas(8) Hash unk2;
		alignas(8) Hash model;
		alignas(8) Hash award;
		alignas(8) Hash spare[16];
	};

	int WarmthTier(Hash item) {
		if (item == 0 || !ITEMDATABASE::_ITEMDATABASE_IS_KEY_VALID(item, 0)) {
			return -1;
		}

		ItemInfo info{};
		if (!ITEMDATABASE::ITEMDATABASE_FILLOUT_ITEM_INFO(item, reinterpret_cast<Any*>(&info))) {
			return -1;
		}

		if (std::find(std::begin(WARMTH_CATEGORIES), std::end(WARMTH_CATEGORIES), info.category) == std::end(WARMTH_CATEGORIES)) {
			return -1;
		}

		if (ITEMDATABASE::_ITEMDATABASE_DOES_ITEM_HAVE_TAG(item, WARMTH_SKIP_TAG, TAG_ITEM_PROPERTY)) {
			return -1;
		}

		for (int tier = 0; tier < (int)std::size(WARMTH_TIER_TAGS); tier++) {
			if (ITEMDATABASE::_ITEMDATABASE_DOES_ITEM_HAVE_TAG(item, WARMTH_TIER_TAGS[tier], TAG_ITEM_PROPERTY)) {
				return tier;
			}
		}
		return -1;
	}
}

float Clothing::WarmthSum(Ped ped) {
	float sum = 0.0f;
	int count = PED::_GET_NUM_COMPONENTS_IN_PED(ped);
	for (int i = 0; i < count; i++) {
		BOOL statusFlag = FALSE;
		Hash wearableState = 0;
		Hash item = PED::_GET_SHOP_ITEM_COMPONENT_AT_INDEX(ped, i, FALSE, &statusFlag, &wearableState);

		int tier = WarmthTier(item);
		if (tier > 0) {
			sum += tier;
		}

		if (item != 0 && tier < 1) {
			Hash slot = PED::_GET_CATEGORY_OF_COMPONENT_AT_INDEX(ped, i, 0);
			for (const SlotWarmth& plain : PLAIN_CLOTHING_WARMTH) {
				if (plain.slot == slot) {
					sum += plain.warmth;
					break;
				}
			}
		}
	}
	return sum;
}
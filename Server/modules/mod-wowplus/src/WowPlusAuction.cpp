/*
 * WowPlus auction house: the staples are always for sale.
 *
 * The auction house is filled with all kinds of things by mod-ah-bot, but that picks at random,
 * so the one bar or glyph a crafter needs may not be there. This file keeps a few auctions up
 * of every crafting material, gem, glyph, and enchant scroll or other item enhancement, and
 * puts one up again when it has been bought. Together with the cities' own vendors it replaces
 * the hub that used to stand on GM Island.
 *
 * The auctions are put up in the name of a character of their own (a setting): not the one
 * mod-ah-bot uses, or it would count them as its own and stop filling the house.
 * Both factions share the neutral auction house (AllowTwoSide.Interaction.Auction).
 */

#include "AuctionHouseMgr.h"
#include "CharacterCache.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"

#include <array>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr uint32 PASS_INTERVAL = 20 * IN_MILLISECONDS;
    constexpr uint32 MAX_NEW_PER_PASS = 300;                // spreads the first filling over a few minutes
    constexpr time_t AUCTION_LASTS = 30 * DAY;              // long, so that unsold ones do not pile up as returned mail
    constexpr uint32 TRADE_GOODS_STACK = 20;
    constexpr uint32 SMALL_STACK = 5;
    constexpr uint32 LOWEST_PRICE = 10;                     // copper

    bool enabled = true;
    uint32 keep = 2;
    uint32 sellerGuid = 0;
    float priceFactor = 1.0f;

    std::vector<ItemTemplate const*> staples;
    uint32 sincePass = 0;
    std::size_t nextStaple = 0;                             // where the next pass goes on

    bool NameLooksUnused(std::string const& name)
    {
        static std::array<char const*, 12> const markers =
        {
            "Test", "TEST", "Deprecated", "DEPRECATED", "DEP ", "OLD", "(old", "Monster -", "[PH]", "Unused", "UNUSED", "NPC Equip"
        };

        for (char const* marker : markers)
            if (name.find(marker) != std::string::npos)
                return true;

        return false;
    }

    bool IsStaple(ItemTemplate const& proto)
    {
        bool const rightKind = proto.Class == ITEM_CLASS_TRADE_GOODS || proto.Class == ITEM_CLASS_GEM || proto.Class == ITEM_CLASS_GLYPH ||
            (proto.Class == ITEM_CLASS_CONSUMABLE && proto.SubClass == ITEM_SUBCLASS_ITEM_ENHANCEMENT);
        if (!rightKind)
            return false;

        // Nothing that binds, nothing temporary, nothing the game never finished
        if (proto.Bonding != NO_BIND || proto.Duration || proto.HasFlag(ITEM_FLAG_CONJURED) || proto.StartQuest)
            return false;

        if (proto.Quality < ITEM_QUALITY_NORMAL || proto.Quality > ITEM_QUALITY_EPIC || !proto.ItemLevel)
            return false;

        return !NameLooksUnused(proto.Name1);
    }

    void BuildStaples()
    {
        for (auto const& [entry, proto] : *sObjectMgr->GetItemTemplateStore())
            if (IsStaple(proto))
                staples.push_back(&proto);

        LOG_INFO("server.loading", ">> WowPlus auction: {} crafting materials, gems, glyphs and enchants are kept for sale ({} of each)",
            staples.size(), keep);
    }

    // The price of one, in copper
    uint32 UnitPrice(ItemTemplate const* proto)
    {
        static std::array<float, 5> const factorByRarity = {5.0f, 10.0f, 40.0f, 150.0f, 600.0f};

        float price = 0.0f;
        if (proto->BuyPrice > 0)
            price = float(proto->BuyPrice) / float(std::max<uint32>(1, proto->BuyCount));
        else if (proto->SellPrice > 0)
            price = float(proto->SellPrice) * 4.0f;
        else
            price = float(proto->ItemLevel) * float(proto->ItemLevel) * factorByRarity[std::min<uint32>(proto->Quality, 4)];

        return std::max<uint32>(LOWEST_PRICE, uint32(price * priceFactor));
    }

    void PutUp(ItemTemplate const* proto, AuctionHouseObject* house, AuctionHouseEntry const* houseEntry, CharacterDatabaseTransaction trans)
    {
        uint32 count = 1;
        if (proto->Class == ITEM_CLASS_TRADE_GOODS)
            count = std::min<uint32>(proto->GetMaxStackSize(), roll_chance_i(50) ? TRADE_GOODS_STACK : SMALL_STACK);

        Item* item = Item::CreateItem(proto->ItemId, count);
        if (!item)
            return;

        uint32 const buyout = UnitPrice(proto) * count;

        AuctionEntry* auction = new AuctionEntry();
        auction->Id = sObjectMgr->GenerateAuctionID();
        auction->houseId = AuctionHouseId::Neutral;
        auction->item_guid = item->GetGUID();
        auction->item_template = item->GetEntry();
        auction->itemCount = item->GetCount();
        auction->owner = ObjectGuid::Create<HighGuid::Player>(sellerGuid);
        auction->startbid = std::max<uint32>(1, buyout * 9 / 10);
        auction->buyout = buyout;
        auction->bid = 0;
        auction->deposit = 0;
        auction->expire_time = time(nullptr) + AUCTION_LASTS;
        auction->auctionHouseEntry = houseEntry;

        item->SaveToDB(trans);
        sAuctionMgr->AddAItem(item);
        house->AddAuction(auction);
        auction->SaveToDB(trans);
    }

    void Pass()
    {
        AuctionHouseObject* house = sAuctionMgr->GetAuctionsMapByHouseId(AuctionHouseId::Neutral);
        AuctionHouseEntry const* houseEntry = AuctionHouseMgr::GetAuctionHouseEntryFromHouse(AuctionHouseId::Neutral);
        if (!house || !houseEntry || staples.empty())
            return;

        // What we have up already
        ObjectGuid const seller = ObjectGuid::Create<HighGuid::Player>(sellerGuid);
        std::unordered_map<uint32, uint32> up;
        for (auto const& [id, auction] : house->GetAuctions())
            if (auction->owner == seller)
                ++up[auction->item_template];

        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        uint32 added = 0;
        for (std::size_t looked = 0; looked < staples.size() && added < MAX_NEW_PER_PASS; ++looked)
        {
            ItemTemplate const* proto = staples[nextStaple];
            nextStaple = (nextStaple + 1) % staples.size();

            for (uint32 have = up[proto->ItemId]; have < keep && added < MAX_NEW_PER_PASS; ++have, ++added)
                PutUp(proto, house, houseEntry, trans);
        }
        CharacterDatabase.CommitTransaction(trans);
    }
}

class WowPlusAuctionWorld : public WorldScript
{
public:
    WowPlusAuctionWorld() : WorldScript("WowPlusAuctionWorld") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        enabled = sConfigMgr->GetOption<bool>("WowPlus.Auction.Staples", true);
        keep = sConfigMgr->GetOption<uint32>("WowPlus.Auction.Staples.Keep", 2);
        sellerGuid = sConfigMgr->GetOption<uint32>("WowPlus.Auction.Staples.SellerGUID", 0);
        priceFactor = sConfigMgr->GetOption<float>("WowPlus.Auction.Staples.PriceFactor", 1.0f);
    }

    void OnStartup() override
    {
        if (!enabled || !keep)
            return;

        if (!sellerGuid || !sCharacterCache->HasCharacterCacheEntry(ObjectGuid::Create<HighGuid::Player>(sellerGuid)))
        {
            LOG_ERROR("server.loading", ">> WowPlus auction: WowPlus.Auction.Staples.SellerGUID is not set to an existing character, "
                "no staples are put up");
            enabled = false;
            return;
        }

        BuildStaples();
    }

    void OnUpdate(uint32 diff) override
    {
        if (!enabled || staples.empty())
            return;

        sincePass += diff;
        if (sincePass < PASS_INTERVAL)
            return;

        sincePass = 0;
        Pass();
    }
};

void AddSC_WowPlusAuction()
{
    new WowPlusAuctionWorld();
}

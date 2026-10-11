-- The auction house, kept stocked by mod-ah-bot (decided 2026-10-11).
--
-- Both factions share one auction house (AllowTwoSide.Interaction.Auction in
-- Settings/worldserver.conf), which the game treats as the neutral one, number 7. So that is
-- the one that gets the items; the Alliance (2) and Horde (6) houses are left empty.
--
-- minitems / maxitems: how many auctions the bot keeps up.
--
-- On the very first start after the module was added, the table does not exist yet (the server
-- creates it while starting), so this file fails once with a warning. Start the server a second
-- time and it works. Safe to run again.
UPDATE mod_auctionhousebot SET minitems = 4000, maxitems = 5000 WHERE auctionhouse = 7;
UPDATE mod_auctionhousebot SET minitems = 0, maxitems = 0 WHERE auctionhouse IN (2, 6);

-- The buyer: every minute it considers this many random auctions (ours are skipped or too dear, so most
-- of them are passes) and bids on those priced within its limits. The limits are the buyerprice* columns:
-- vendor sell price times 3 for white, 5 for green, 12 for blue, 15 for epic (the module's own values).
UPDATE mod_auctionhousebot SET buyerbiddinginterval = 1, buyerbidsperinterval = 40 WHERE auctionhouse = 7;

/*
 * WowPlus: our own changes to the game, each behind a setting in mod_wowplus.conf.
 */

void AddSC_WowPlusLoot();
void AddSC_WowPlusBonuses();
void AddSC_WowPlusProfessions();
void AddSC_WowPlusTravel();
void AddSC_WowPlusVendors();
void AddSC_WowPlusHub();
void AddSC_WowPlusGold();

void Addmod_wowplusScripts()
{
    AddSC_WowPlusLoot();
    AddSC_WowPlusBonuses();
    AddSC_WowPlusProfessions();
    AddSC_WowPlusTravel();
    AddSC_WowPlusVendors();
    AddSC_WowPlusHub();
    AddSC_WowPlusGold();
}

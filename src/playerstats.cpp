
#include "playerstats.h"

#include "caret.h"
#include "Utils/Logger.h"
#include "game.h"
#include "inventory.h"
#include "nx.h"
#include "player.h"
#include "sound/SoundManager.h"
#include "statusbar.h"

void AddHealth(int hp)
{
  player->hp += hp;
  if (player->hp > player->maxHealth)
    player->hp = player->maxHealth;
}

/*
void c------------------------------() {}
*/

// add an item to the inventory list (generates an error msg if inventory is full)
void AddInventory(int item)
{
  if (player->ninventory + 1 >= MAX_INVENTORY)
  {
    LOG_ERROR("<<<AddInventory: inventory is full>>");
    game.running = 0;
    return;
  }

  NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GET_ITEM);
  if (FindInventory(item) != -1)
  {
    return;
  }
  player->inventory[player->ninventory++] = item;
  RefreshInventoryScreen();
}

// remove an item from the inventory list (does nothing if it's not in there)
void DelInventory(int item)
{
  int slot;
  int i;

  for (;;)
  {
    slot = FindInventory(item);
    if (slot == -1)
      break;

    for (i = slot; i < player->ninventory - 1; i++)
    {
      player->inventory[i] = player->inventory[i + 1];
    }
    player->ninventory--;
  }

  RefreshInventoryScreen();
}

// find which slot an item is in (returns -1 if player does not have it)
int FindInventory(int item)
{
  return CheckInventoryList(item, player->inventory, player->ninventory);
}

// checks if the inventory list given contains the given item.
// if so, returns the index of the item. if not, returns -1.
int CheckInventoryList(int item, int *list, int nitems)
{
  int i;

  for (i = 0; i < nitems; i++)
    if (list[i] == item)
      return i;

  return -1;
}

/*
void c------------------------------() {}
*/

// AM+ command.
// adds "ammo" ammo to the specified weapons ammo and maxammo,
// and if you don't have it already, gives it to you.
void GetWeapon(int wpn, int ammo, bool quiet)
{
  Weapon *weapon = player->FindWeapon(wpn);
  if (weapon == NULL)
  {
    weapon = (Weapon*)weaponRegistry.getType(wpn, NULL);
    weapon->ammo      = 0; // will be filled to full by AddAmmo below
    weapon->maxammo   = ammo;
    player->weapons.push_back(weapon);
    if (player->curWeapon == 0) // if player doesn't have any weapons - activate new weapon
      player->curWeapon              = wpn;
  }
  else
  { // missile capacity powerups
    weapon->maxammo += ammo;
  }

  AddAmmo(wpn, ammo);
  if (!quiet) {
    NXE::Sound::SoundManager::getInstance()->playSfx(NXE::Sound::SFX::SND_GET_ITEM);
  }
}

// AM- command. Drops specified weapon.
void LoseWeapon(int wpn)
{
  int weapon = player->FindWeaponSlot(wpn);
  if (weapon == -1) {
    return;
  }
  delete player->weapons[weapon];
  player->weapons.erase(player->weapons.begin() + weapon);
}

// TAM command.
void TradeWeapon(int oldwpn, int newwpn, int ammo, bool quiet)
{
  int oldcurwpn = player->curWeapon;

  // ammo 0 = no change; used when you get missiles are upgraded to Super Missiles
  if (ammo == 0) {
    Weapon* oldWeapon = player->FindWeapon(oldwpn);
    if (oldWeapon != NULL) {
      ammo = oldWeapon->ammo;
    }
  }

  LoseWeapon(oldwpn);

  // ? why was this originally a copy of getweapon

  GetWeapon(newwpn, ammo, quiet);

  // switch to new weapon if the weapon traded was the
  // one we were using. Otherwise, don't change current weapon.
  if (oldwpn == oldcurwpn)
    player->curWeapon = player->FindWeaponSlot(newwpn);
}

// adds "ammo" ammo to the specified weapon, but doesn't go over the limit.
void AddAmmo(int wpn, int ammo)
{
  Weapon *weapon = player->FindWeapon(wpn);
  if (weapon == NULL) {
    return;
  }
  weapon->addAmmo(ammo);
}

// sets all weapons to max ammo. AE+ command.
void RefillAllAmmo(void)
{
  for (int i = 0; i < player->weapons.size(); i++)
  {
    Weapon *weapon = player->weapons[i];
    weapon->addAmmo(weapon->getMaxAmmo());
  }
}

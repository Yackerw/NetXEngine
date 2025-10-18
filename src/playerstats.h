#ifndef __PLAYERSTATS_H_
#define __PLAYERSTATS_H_

void AddHealth(int hp);
void AddInventory(int item);
void DelInventory(int item);
int FindInventory(int item);
int CheckInventoryList(int item, int *list, int nitems);
void GetWeapon(int wpn, int ammo, bool quiet = false);
void LoseWeapon(int wpn);
void TradeWeapon(int oldwpn, int newwpn, int ammo, bool quiet = false);
void AddAmmo(int wpn, int ammo);
void RefillAllAmmo(void);

#endif

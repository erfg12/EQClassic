/*
alter table npc_types add column loottable_id int(11) unsigned not null;

create table loottable (id int(11) unsigned auto_increment primary key, name varchar(255) not null unique, 
mincash int(11) unsigned not null, maxcash int(11) unsigned not null, avgcoin smallint(4) unsigned not null default 0);

create table loottable_entries (loottable_id int(11) unsigned not null, lootdrop_id int(11) unsigned not null, 
multiplier tinyint(2) unsigned default 1 not null, probability tinyint(2) unsigned default 100 not null, 
primary key (loottable_id, lootdrop_id));

create table lootdrop (id int(11) unsigned auto_increment primary key, name varchar(255) not null unique);

create table lootdrop_entries (lootdrop_id int(11) unsigned not null, item_id int(11) not null, 
item_charges tinyint(2) default 1 not null, equip_item tinyint(2) unsigned not null, 
chance tinyint(2) unsigned default 1 not null, primary key (lootdrop_id, item_id));
*/


#ifndef LOOTTABLE_H
#define LOOTTABLE_H
#include "zonedump.h"

typedef LinkedList<ServerLootItem_Struct*> ItemList;

#endif

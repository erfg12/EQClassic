#ifndef MESSAGETYPES_H
#define MESSAGETYPES_H

// msg_type's for custom usercolors
#define MESSAGETYPE_Say					256		// Comment: 
#define MESSAGETYPE_Tell				257		// Comment: 
#define MESSAGETYPE_Group				258		// Comment: 
#define MESSAGETYPE_Guild				259		// Comment: 
#define MESSAGETYPE_OOC					260		// Comment: 
#define MESSAGETYPE_Auction				261		// Comment: 
#define MESSAGETYPE_Shout				262		// Comment: 
#define MESSAGETYPE_Emote				263		// Comment: 
#define MESSAGETYPE_Spells				264		// Comment: 
#define MESSAGETYPE_YouHitOther			265		// Comment: 
#define MESSAGETYPE_OtherHitsYou		266		// Comment: 
#define MESSAGETYPE_YouMissOther		267		// Comment: 
#define MESSAGETYPE_OtherMissesYou		268		// Comment: 
#define MESSAGETYPE_Broadcasts			269		// Comment: 
#define MESSAGETYPE_Skills				270		// Comment: 
#define MESSAGETYPE_Disciplines			271		// Comment: 


enum MessageChannel
{
	MessageChannel_GUILDSAY = 0,
	MessageChannel_UNKNOWN1 = 1,
	MessageChannel_GSAY = 2,
	MessageChannel_SHOUT = 3,
	MessageChannel_AUCTION = 4,
	MessageChannel_OCC = 5,
	MessageChannel_BROADCAST = 6,
	MessageChannel_TELL = 7,
	MessageChannel_SAY = 8,
	MessageChannel_UNKNOWN9 = 9,
	MessageChannel_UNKNOWN10 = 10,
	MessageChannel_GMSAY = 11,
	MessageChannel_UNKNOWN12 = 12,
	MessageChannel_UNKNOWN13 = 13,
	MessageChannel_UNKNOWN14 = 14
};
#endif
#ifndef __QUEST_H__
#define __QUEST_H__

#include "database.h"

typedef struct _tag_quest_entry{
	char	*m_pQuestName;
	char	*m_pQuestText;
	char	*m_pQuestEnd;
	int		m_iNpcId;
	int		m_iQuestObject;
	int		m_iQuestPrice;
	int		m_iQuestCash;
	int		m_iQuestExp;
}quest_entry,*pquest_entry;


class Quest{
public:	
	Quest();
	~Quest();
	static pquest_entry	Test(int NpcId, int QuestObject);

	static pquest_entry	m_pQuests;
	static int			m_nQuests;
};

#endif


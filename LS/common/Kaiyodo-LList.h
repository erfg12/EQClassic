#ifndef __INCLUDED_MYLIST__
#define __INCLUDED_MYLIST__

#ifdef OFFROAD_ACTIVE
#define IO_ENABLED
#endif

// Class def for the actual object to be stored in the list
template <class ClassName> class MyListItem
{
	public:
		ClassName	*Data;
		MyListItem <ClassName> *Prev;
		MyListItem <ClassName> *Next;

		MyListItem(void) : Prev(NULL), Next(NULL) {}
		MyListItem(ClassName *fromData) : Prev(NULL), Next(NULL), Data(fromData) {}
};

// Class def for the handler

template <class ClassName> class MyList : public MyListItem <ClassName>
{
	public:
		MyListItem <ClassName>	*First;
		MyListItem <ClassName>	*Last;
		int Count;

		MyList(void) : First(NULL), Last(NULL), Count (0) {}
		~MyList();

		MyListItem <ClassName> *AddItem(ClassName *Obj);
		MyListItem <ClassName> *PrependItem(ClassName *Obj);

		bool DeleteItem(ClassName *Obj);
		bool DeleteItemAndData(ClassName *Obj);

		void DeleteItem(MyListItem <ClassName> *Obj);
		void DeleteItemAndData(MyListItem <ClassName> *Obj);

		void ClearList(void);
		void ClearListAndData(void);

		MyListItem <ClassName> *AddAfter(MyListItem <ClassName> *Pos, ClassName *New);
		MyListItem <ClassName> *AddBefore(MyListItem <ClassName> *Pos, ClassName *New);

		MyListItem <ClassName> *AddAfter(ClassName *Pos, ClassName *New);
		MyListItem <ClassName> *AddBefore(ClassName *Pos, ClassName *New);

		int	ItemCount(void) const;

		MyListItem <ClassName> *GetItem(ClassName *Obj);

#ifdef IO_ENABLED
//		void SaveList(const int iFileHandle) const;
//		void LoadList(const int iFileHandle);

		void SaveRecursive(const int iFileHandle) const;
		void LoadRecursive(const int iFileHandle);
#endif
};

#ifdef IO_ENABLED
template <class ClassName> void MyList<ClassName>::SaveRecursive(const int iFileHandle) const
{
	MyListItem <ClassName> * Current = First;
	int Size = sizeof(ClassName);

	MyWrite(iFileHandle, (void*)&Count, sizeof(int));
	while (Current)
	{
		Current->Data->Save(iFileHandle);
		Current = Current->Next;
	}
}

template <class ClassName> void MyList<ClassName>::LoadRecursive(const int iFileHandle)
{
	int t, TempCount;
	ClassName* pItem;

	ClearListAndData();

	MyRead(iFileHandle, &TempCount, sizeof(int));
	for (t = 0 ; t < TempCount ; t++)
	{
		pItem = new ClassName;

		pItem->Load(iFileHandle);
		
		AddItem(pItem);
	}
}
/*
template <class ClassName> void MyList<ClassName>::LoadList(const int iFileHandle)
{
	int t, TempCount;
	ClassName* pItem;

	ClearListAndData();
	MyRead(iFileHandle, &TempCount, sizeof(int));

	for (t = 0 ; t < TempCount ; t++)
	{
		pItem = new ClassName;

		pItem->Load(iFileHandle);
		
		AddItem(pItem);
	}
}


template <class ClassName> void MyList<ClassName>::SaveList(const int iFileHandle) const
{
	MyListItem <ClassName> * Current = First;
	int Size = sizeof(ClassName);

	MyWrite(iFileHandle, (void*)&Count, sizeof(int));
	while (Current)
	{
		MyWrite(iFileHandle, (void*) Current->Data, Size);
		Current = Current->Next;
	}
}
*/


#endif

// Destructor
template <class ClassName> MyList<ClassName>::~MyList()
{
	ClearList();
}

// Add Item
template <class ClassName> MyListItem <ClassName> *MyList<ClassName>::AddItem(ClassName *Obj)
{
	MyListItem <ClassName> *Ptr = new MyListItem<ClassName>;

	if(!Ptr)
	{
		//LogFile("Allocation error during linked list");
		return(NULL);
	}

	Ptr->Data = Obj;

	if(First == NULL)
	{
		First = Last = Ptr;
	}
	else
	{
		Ptr->Prev = Last;
		Last->Next = Ptr;
		Last = Ptr;
	}

	Count++;

	return(Ptr);
}

// Add Item
template <class ClassName> MyListItem <ClassName> *MyList<ClassName>::PrependItem(ClassName *Obj)
{
	MyListItem <ClassName> *Ptr = new MyListItem<ClassName>;

	if(!Ptr)
	{
		//LogFile("Allocation error during linked list");
		return(NULL);
	}

	Ptr->Data = Obj;

	if(First == NULL)
	{
		First = Last = Ptr;
	}
	else
	{
		Ptr->Next = First;
		First->Prev = Ptr;
		First = Ptr;
	}

	Count++;

	return(Ptr);
}


// Remove Item and free the Data pointer if vaild
template <class ClassName> void MyList<ClassName>::DeleteItemAndData(MyListItem <ClassName> *Obj)
{
	if(Obj->Prev)
	{
		// Not deleting first element
		Obj->Prev->Next = Obj->Next;

		if(Obj->Next)
		{
			// Not deleting last element
			Obj->Next->Prev = Obj->Prev;
		}
		else
		{
			// Last Element
			Last = Obj->Prev;
		}
	}
	else
	{
		// Deleting first element
		if(Obj->Next)
		{
			// List not empty
			Obj->Next->Prev = NULL;
			First = Obj->Next;
		}
		else
		{
			// List now empty
			First = Last = NULL;
		}
	}

	if(Obj->Data)
		delete Obj->Data;

	Count--;

	delete Obj;
}


// Delete item - leave the data alone
template <class ClassName> void MyList<ClassName>::DeleteItem(MyListItem <ClassName> *Obj)
{
	if(Obj->Prev)
	{
		// Not deleting first element
		Obj->Prev->Next = Obj->Next;

		if(Obj->Next)
		{
			// Not deleting last element
			Obj->Next->Prev = Obj->Prev;
		}
		else
		{
			// Last Element
			Last = Obj->Prev;
		}
	}
	else
	{
		// Deleting first element
		if(Obj->Next)
		{
			// List not empty
			Obj->Next->Prev = NULL;
			First = Obj->Next;
		}
		else
		{
			// List now empty
			First = Last = NULL;
		}
	}

	Count--;

	delete Obj;
}


template <class ClassName> void MyList<ClassName>::ClearList(void)
{
	MyListItem <ClassName> *Ptr, *Next;

	Ptr = First;

	while(Ptr)
	{
		Next = Ptr->Next;
		DeleteItem(Ptr);
		Ptr = Next;
	}

	Count = 0;
}


template <class ClassName> void MyList<ClassName>::ClearListAndData(void)
{
	MyListItem <ClassName> *Ptr, *Next;

	Ptr = First;

	while(Ptr)
	{
		Next = Ptr->Next;
		DeleteItemAndData(Ptr);
		Ptr = Next;
	}

	Count = 0;
}

template <class ClassName> MyListItem <ClassName> *MyList<ClassName>::AddAfter(MyListItem <ClassName> *Pos, ClassName *New)
{
	MyListItem <ClassName> *Ptr = new MyListItem<ClassName>;
	Ptr->Data = New;

	if(Pos->Next)
	{
		// Must be in the middle of the list
		Ptr->Next = Pos->Next;
		Ptr->Prev = Pos;
		Pos->Next = Pos->Next->Prev = Ptr;
	}
	else
	{
		// We're at the end of the list
		Pos->Next = Last = Ptr;
		Ptr->Next = NULL;
		Ptr->Prev = Pos;
	}

	Count++;

	return(Ptr);
}

template <class ClassName> MyListItem <ClassName> *MyList<ClassName>::AddBefore(MyListItem <ClassName> *Pos, ClassName *New)
{
	MyListItem <ClassName> *Ptr = new MyListItem<ClassName>;
	Ptr->Data = New;

	if(Pos->Prev)
	{
		// Must be in the middle of the list
		Ptr->Next = Pos;
		Ptr->Prev = Pos->Prev;
		Pos->Prev = Pos->Prev->Next = Ptr;
	}
	else
	{
		// We're at the start of the list
		Pos->Prev = First = Ptr;
		Ptr->Next = Pos;
		Ptr->Prev = NULL;
	}

	Count++;

	return(Ptr);
}

template <class ClassName> MyListItem <ClassName> *MyList<ClassName>::AddAfter(ClassName *Pos, ClassName *New)
{
	// Find the passed-in position and add after that, add to end if not found
	MyListItem <ClassName> *Search = First;
	while(Search)
	{
		if(Search->Data == Pos)
			break;
		Search = Search->Next;
	}

	if(!Search)			// Add to end of list
		return(AddItem(New));

	MyListItem <ClassName> *Ptr = new MyListItem<ClassName>;
	Ptr->Data = New;

	if(Search->Next)
	{
		// Must be in the middle of the list
		Ptr->Next = Search->Next;
		Ptr->Prev = Search;
		Search->Next = Search->Next->Prev = Ptr;
	}
	else
	{
		// We're at the end of the list
		Search->Next = Last = Ptr;
		Ptr->Next = NULL;
		Ptr->Prev = Search;
	}

	Count++;

	return(Ptr);
}


template <class ClassName> MyListItem <ClassName> *MyList<ClassName>::AddBefore(ClassName *Pos, ClassName *New)
{
	// Find the passed-in position and add before that, add to end if not found
	MyListItem <ClassName> *Search = First;
	while(Search)
	{
		if(Search->Data == Pos)
			break;
		Search = Search->Next;
	}

	if(!Search)			// Add to end of list
		return(AddItem(New));

	MyListItem <ClassName> *Ptr = new MyListItem<ClassName>;
	Ptr->Data = New;

	if(Search->Prev)
	{
		// Must be in the middle of the list
		Ptr->Next = Search;
		Ptr->Prev = Search->Prev;
		Search->Prev = Search->Prev->Next = Ptr;
	}
	else
	{
		// We're at the start of the list
		Search->Prev = First = Ptr;
		Ptr->Next = Search;
		Ptr->Prev = NULL;
	}

	Count++;

	return(Ptr);
}

template <class ClassName> int MyList<ClassName>::ItemCount(void) const
{
	return Count;
}

template <class ClassName> bool MyList<ClassName>::DeleteItemAndData(ClassName *Obj)
{
	MyListItem <ClassName> *Curr = GetItem(Obj);
	bool RetVal = false;

	if (Curr)
	{
		RetVal = true;
		DeleteItemAndData(Curr);
	}

	return RetVal;
}

template <class ClassName> bool MyList<ClassName>::DeleteItem(ClassName *Obj)
{
	MyListItem <ClassName> *Curr = GetItem(Obj);
	bool RetVal = false;

	if (Curr)
	{
		RetVal = true;
		DeleteItem(Curr);
	}

	return RetVal;
}

template <class ClassName> MyListItem <ClassName> *MyList<ClassName>::GetItem(ClassName *Obj)
{
	MyListItem <ClassName> *Curr = First;
	MyListItem <ClassName> *found = NULL;

	while(Curr && !found)
	{
		if (Curr->Data == Obj)
			found = Curr;

		Curr = Curr->Next;
	}

	return found;
}

#endif // __INCLUDED_MYLIST__


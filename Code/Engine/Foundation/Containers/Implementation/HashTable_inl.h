
/// \brief Value used by containers for indices to indicate an invalid index.
#ifndef ezInvalidIndex
#  define ezInvalidIndex 0xFFFFFFFF
#endif

// ***** Const Iterator *****

template <typename K, typename V, typename H>
ezHashTableBase<K, V, H>::ConstIterator::ConstIterator(const ezHashTableBase<K, V, H>& hashTable)
  : m_pHashTable(&hashTable)
{
}

template <typename K, typename V, typename H>
void ezHashTableBase<K, V, H>::ConstIterator::SetToBegin()
{
  if (m_pHashTable->IsEmpty())
  {
    m_uiCurrentIndex = m_pHashTable->m_uiCapacity;
    return;
  }
  while (!m_pHashTable->IsValidEntry(m_uiCurrentIndex))
  {
    ++m_uiCurrentIndex;
  }
}

template <typename K, typename V, typename H>
inline void ezHashTableBase<K, V, H>::ConstIterator::SetToEnd()
{
  m_uiCurrentCount = m_pHashTable->m_uiCount;
  m_uiCurrentIndex = m_pHashTable->m_uiCapacity;
}


template <typename K, typename V, typename H>
EZ_FORCE_INLINE bool ezHashTableBase<K, V, H>::ConstIterator::IsValid() const
{
  return m_uiCurrentCount < m_pHashTable->m_uiCount;
}

template <typename K, typename V, typename H>
EZ_FORCE_INLINE bool ezHashTableBase<K, V, H>::ConstIterator::operator==(const typename ezHashTableBase<K, V, H>::ConstIterator& rhs) const
{
  return m_uiCurrentIndex == rhs.m_uiCurrentIndex && m_pHashTable->m_pEntries == rhs.m_pHashTable->m_pEntries;
}

template <typename K, typename V, typename H>
EZ_ALWAYS_INLINE const K& ezHashTableBase<K, V, H>::ConstIterator::Key() const
{
  return m_pHashTable->m_pEntries[m_uiCurrentIndex].key;
}

template <typename K, typename V, typename H>
EZ_ALWAYS_INLINE const V& ezHashTableBase<K, V, H>::ConstIterator::Value() const
{
  return m_pHashTable->m_pEntries[m_uiCurrentIndex].value;
}

template <typename K, typename V, typename H>
void ezHashTableBase<K, V, H>::ConstIterator::Next()
{
  // if we already iterated over the amount of valid elements that the hash-table stores, early out
  if (m_uiCurrentCount >= m_pHashTable->m_uiCount)
    return;

  // increase the counter of how many elements we have seen
  ++m_uiCurrentCount;
  // increase the index of the element to look at
  ++m_uiCurrentIndex;

  // check that we don't leave the valid range of element indices
  while (m_uiCurrentIndex < m_pHashTable->m_uiCapacity)
  {
    if (m_pHashTable->IsValidEntry(m_uiCurrentIndex))
      return;

    ++m_uiCurrentIndex;
  }

  // if we fell through this loop, we reached the end of all elements in the container
  // set the m_uiCurrentCount to maximum, to enable early-out in the future and to make 'IsValid' return 'false'
  m_uiCurrentCount = m_pHashTable->m_uiCount;
}

template <typename K, typename V, typename H>
EZ_ALWAYS_INLINE void ezHashTableBase<K, V, H>::ConstIterator::operator++()
{
  Next();
}


// ***** Iterator *****

template <typename K, typename V, typename H>
ezHashTableBase<K, V, H>::Iterator::Iterator(const ezHashTableBase<K, V, H>& hashTable)
  : ConstIterator(hashTable)
{
}

template <typename K, typename V, typename H>
ezHashTableBase<K, V, H>::Iterator::Iterator(const typename ezHashTableBase<K, V, H>::Iterator& rhs)
  : ConstIterator(*rhs.m_pHashTable)
{
  this->m_uiCurrentIndex = rhs.m_uiCurrentIndex;
  this->m_uiCurrentCount = rhs.m_uiCurrentCount;
}

template <typename K, typename V, typename H>
EZ_ALWAYS_INLINE void ezHashTableBase<K, V, H>::Iterator::operator=(const Iterator& rhs) // [tested]
{
  this->m_pHashTable = rhs.m_pHashTable;
  this->m_uiCurrentIndex = rhs.m_uiCurrentIndex;
  this->m_uiCurrentCount = rhs.m_uiCurrentCount;
}

template <typename K, typename V, typename H>
EZ_FORCE_INLINE V& ezHashTableBase<K, V, H>::Iterator::Value()
{
  return this->m_pHashTable->m_pEntries[this->m_uiCurrentIndex].value;
}


// ***** ezHashTableBase *****

template <typename K, typename V, typename H>
ezHashTableBase<K, V, H>::ezHashTableBase(ezAllocatorBase* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;
}

template <typename K, typename V, typename H>
ezHashTableBase<K, V, H>::ezHashTableBase(const ezHashTableBase<K, V, H>& other, ezAllocatorBase* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  *this = other;
}

template <typename K, typename V, typename H>
ezHashTableBase<K, V, H>::ezHashTableBase(ezHashTableBase<K, V, H>&& other, ezAllocatorBase* pAllocator)
{
  m_pEntries = nullptr;
  m_pEntryFlags = nullptr;
  m_uiCount = 0;
  m_uiCapacity = 0;
  m_pAllocator = pAllocator;

  *this = std::move(other);
}

template <typename K, typename V, typename H>
ezHashTableBase<K, V, H>::~ezHashTableBase()
{
  Clear();
  EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
  EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);
  m_uiCapacity = 0;
}

template <typename K, typename V, typename H>
void ezHashTableBase<K, V, H>::operator=(const ezHashTableBase<K, V, H>& rhs)
{
  Clear();
  Reserve(rhs.GetCount());

  ezUInt32 uiCopied = 0;
  for (ezUInt32 i = 0; uiCopied < rhs.GetCount(); ++i)
  {
    if (rhs.IsValidEntry(i))
    {
      Insert(rhs.m_pEntries[i].key, rhs.m_pEntries[i].value);
      ++uiCopied;
    }
  }
}

template <typename K, typename V, typename H>
void ezHashTableBase<K, V, H>::operator=(ezHashTableBase<K, V, H>&& rhs)
{
  // Clear any existing data (calls destructors if necessary)
  Clear();

  if (m_pAllocator != rhs.m_pAllocator)
  {
    Reserve(rhs.m_uiCapacity);

    ezUInt32 uiCopied = 0;
    for (ezUInt32 i = 0; uiCopied < rhs.GetCount(); ++i)
    {
      if (rhs.IsValidEntry(i))
      {
        Insert(std::move(rhs.m_pEntries[i].key), std::move(rhs.m_pEntries[i].value));
        ++uiCopied;
      }
    }

    rhs.Clear();
  }
  else
  {
    EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
    EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);

    // Move all data over.
    m_pEntries = rhs.m_pEntries;
    m_pEntryFlags = rhs.m_pEntryFlags;
    m_uiCount = rhs.m_uiCount;
    m_uiCapacity = rhs.m_uiCapacity;

    // Temp copy forgets all its state.
    rhs.m_pEntries = nullptr;
    rhs.m_pEntryFlags = nullptr;
    rhs.m_uiCount = 0;
    rhs.m_uiCapacity = 0;
  }
}

template <typename K, typename V, typename H>
bool ezHashTableBase<K, V, H>::operator==(const ezHashTableBase<K, V, H>& rhs) const
{
  if (m_uiCount != rhs.m_uiCount)
    return false;

  ezUInt32 uiCompared = 0;
  for (ezUInt32 i = 0; uiCompared < m_uiCount; ++i)
  {
    if (IsValidEntry(i))
    {
      const V* pRhsValue = nullptr;
      if (!rhs.TryGetValue(m_pEntries[i].key, pRhsValue))
        return false;

      if (m_pEntries[i].value != *pRhsValue)
        return false;

      ++uiCompared;
    }
  }

  return true;
}

template <typename K, typename V, typename H>
void ezHashTableBase<K, V, H>::Reserve(ezUInt32 uiCapacity)
{
  const ezUInt64 uiCap64 = static_cast<ezUInt64>(uiCapacity);
  ezUInt64 uiNewCapacity64 = uiCap64 + (uiCap64 * 2 / 3); // ensure a maximum load of 60%

  uiNewCapacity64 = ezMath::Min<ezUInt64>(uiNewCapacity64, 0x80000000llu); // the largest power-of-two in 32 bit

  ezUInt32 uiNewCapacity32 = static_cast<ezUInt32>(uiNewCapacity64 & 0xFFFFFFFF);
  EZ_ASSERT_DEBUG(uiCapacity <= uiNewCapacity32, "ezHashSet/Map do not support more than 2 billion entries.");

  if (m_uiCapacity >= uiNewCapacity32)
    return;

  uiNewCapacity32 = ezMath::Max<ezUInt32>(ezMath::PowerOfTwo_Ceil(uiNewCapacity32), CAPACITY_ALIGNMENT);
  SetCapacity(uiNewCapacity32);
}

template <typename K, typename V, typename H>
void ezHashTableBase<K, V, H>::Compact()
{
  if (IsEmpty())
  {
    // completely deallocate all data, if the table is empty.
    EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntries);
    EZ_DELETE_RAW_BUFFER(m_pAllocator, m_pEntryFlags);
    m_uiCapacity = 0;
  }
  else
  {
    const ezUInt32 uiNewCapacity = ezMath::PowerOfTwo_Ceil(m_uiCount + (CAPACITY_ALIGNMENT - 1)) & ~(CAPACITY_ALIGNMENT - 1);
    if (m_uiCapacity != uiNewCapacity)
      SetCapacity(uiNewCapacity);
  }
}

template <typename K, typename V, typename H>
EZ_ALWAYS_INLINE ezUInt32 ezHashTableBase<K, V, H>::GetCount() const
{
  return m_uiCount;
}

template <typename K, typename V, typename H>
EZ_ALWAYS_INLINE bool ezHashTableBase<K, V, H>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename K, typename V, typename H>
void ezHashTableBase<K, V, H>::Clear()
{
  for (ezUInt32 i = 0; i < m_uiCapacity; ++i)
  {
    if (IsValidEntry(i))
    {
      ezMemoryUtils::Destruct(&m_pEntries[i].key, 1);
      ezMemoryUtils::Destruct(&m_pEntries[i].value, 1);
    }
  }

  ezMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());
  m_uiCount = 0;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType, typename CompatibleValueType>
bool ezHashTableBase<K, V, H>::Insert(CompatibleKeyType&& key, CompatibleValueType&& value, V* out_pOldValue /*= nullptr*/)
{
  Reserve(m_uiCount + 1);

  ezUInt32 uiIndex = H::Hash(key) & (m_uiCapacity - 1);
  ezUInt32 uiDeletedIndex = ezInvalidIndex;

  ezUInt32 uiCounter = 0;
  while (!IsFreeEntry(uiIndex) && uiCounter < m_uiCapacity)
  {
    if (IsDeletedEntry(uiIndex))
    {
      if (uiDeletedIndex == ezInvalidIndex)
        uiDeletedIndex = uiIndex;
    }
    else if (H::Equal(m_pEntries[uiIndex].key, key))
    {
      if (out_pOldValue != nullptr)
        *out_pOldValue = std::move(m_pEntries[uiIndex].value);

      m_pEntries[uiIndex].value = std::forward<CompatibleValueType>(value); // Either move or copy assignment.
      return true;
    }
    ++uiIndex;
    if (uiIndex == m_uiCapacity)
      uiIndex = 0;

    ++uiCounter;
  }

  // new entry
  uiIndex = uiDeletedIndex != ezInvalidIndex ? uiDeletedIndex : uiIndex;

  // Both constructions might either be a move or a copy.
  ezMemoryUtils::CopyOrMoveConstruct(&m_pEntries[uiIndex].key, std::forward<CompatibleKeyType>(key));
  ezMemoryUtils::CopyOrMoveConstruct(&m_pEntries[uiIndex].value, std::forward<CompatibleValueType>(value));

  MarkEntryAsValid(uiIndex);
  ++m_uiCount;

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
bool ezHashTableBase<K, V, H>::Remove(const CompatibleKeyType& key, V* out_pOldValue /*= nullptr*/)
{
  ezUInt32 uiIndex = FindEntry(key);
  if (uiIndex != ezInvalidIndex)
  {
    if (out_pOldValue != nullptr)
      *out_pOldValue = std::move(m_pEntries[uiIndex].value);

    RemoveInternal(uiIndex);
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
typename ezHashTableBase<K, V, H>::Iterator ezHashTableBase<K, V, H>::Remove(const typename ezHashTableBase<K, V, H>::Iterator& pos)
{
  EZ_ASSERT_DEBUG(pos.m_pHashTable == this, "Iterator from wrong hashtable");
  Iterator it = pos;
  ezUInt32 uiIndex = pos.m_uiCurrentIndex;
  ++it;
  --it.m_uiCurrentCount;
  RemoveInternal(uiIndex);
  return it;
}

template <typename K, typename V, typename H>
void ezHashTableBase<K, V, H>::RemoveInternal(ezUInt32 uiIndex)
{
  ezMemoryUtils::Destruct(&m_pEntries[uiIndex].key, 1);
  ezMemoryUtils::Destruct(&m_pEntries[uiIndex].value, 1);

  ezUInt32 uiNextIndex = uiIndex + 1;
  if (uiNextIndex == m_uiCapacity)
    uiNextIndex = 0;

  // if the next entry is free we are at the end of a chain and
  // can immediately mark this entry as free as well
  if (IsFreeEntry(uiNextIndex))
  {
    MarkEntryAsFree(uiIndex);

    // run backwards and free all deleted entries in this chain
    ezUInt32 uiPrevIndex = (uiIndex != 0) ? uiIndex : m_uiCapacity;
    --uiPrevIndex;

    while (IsDeletedEntry(uiPrevIndex))
    {
      MarkEntryAsFree(uiPrevIndex);

      if (uiPrevIndex == 0)
        uiPrevIndex = m_uiCapacity;
      --uiPrevIndex;
    }
  }
  else
  {
    MarkEntryAsDeleted(uiIndex);
  }

  --m_uiCount;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline bool ezHashTableBase<K, V, H>::TryGetValue(const CompatibleKeyType& key, V& out_value) const
{
  ezUInt32 uiIndex = FindEntry(key);
  if (uiIndex != ezInvalidIndex)
  {
    EZ_ASSERT_DEBUG(m_pEntries != nullptr, "No entries present"); // To fix static analysis
    out_value = m_pEntries[uiIndex].value;
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline bool ezHashTableBase<K, V, H>::TryGetValue(const CompatibleKeyType& key, const V*& out_pValue) const
{
  ezUInt32 uiIndex = FindEntry(key);
  if (uiIndex != ezInvalidIndex)
  {
    out_pValue = &m_pEntries[uiIndex].value;
    EZ_ANALYSIS_ASSUME(out_pValue != nullptr);
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline bool ezHashTableBase<K, V, H>::TryGetValue(const CompatibleKeyType& key, V*& out_pValue) const
{
  ezUInt32 uiIndex = FindEntry(key);
  if (uiIndex != ezInvalidIndex)
  {
    out_pValue = &m_pEntries[uiIndex].value;
    EZ_ANALYSIS_ASSUME(out_pValue != nullptr);
    return true;
  }

  return false;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline typename ezHashTableBase<K, V, H>::ConstIterator ezHashTableBase<K, V, H>::Find(const CompatibleKeyType& key) const
{
  ezUInt32 uiIndex = FindEntry(key);
  if (uiIndex == ezInvalidIndex)
  {
    return GetEndIterator();
  }

  ConstIterator it(*this);
  it.m_uiCurrentIndex = uiIndex;
  it.m_uiCurrentCount = 0; // we do not know the 'count' (which is used as an optimization), so we just use 0

  return it;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline typename ezHashTableBase<K, V, H>::Iterator ezHashTableBase<K, V, H>::Find(const CompatibleKeyType& key)
{
  ezUInt32 uiIndex = FindEntry(key);
  if (uiIndex == ezInvalidIndex)
  {
    return GetEndIterator();
  }

  Iterator it(*this);
  it.m_uiCurrentIndex = uiIndex;
  it.m_uiCurrentCount = 0; // we do not know the 'count' (which is used as an optimization), so we just use 0
  return it;
}


template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline const V* ezHashTableBase<K, V, H>::GetValue(const CompatibleKeyType& key) const
{
  ezUInt32 uiIndex = FindEntry(key);
  return (uiIndex != ezInvalidIndex) ? &m_pEntries[uiIndex].value : nullptr;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline V* ezHashTableBase<K, V, H>::GetValue(const CompatibleKeyType& key)
{
  ezUInt32 uiIndex = FindEntry(key);
  return (uiIndex != ezInvalidIndex) ? &m_pEntries[uiIndex].value : nullptr;
}

template <typename K, typename V, typename H>
inline V& ezHashTableBase<K, V, H>::operator[](const K& key)
{
  return FindOrAdd(key, nullptr);
}

template <typename K, typename V, typename H>
V& ezHashTableBase<K, V, H>::FindOrAdd(const K& key, bool* out_pExisted)
{
  const ezUInt32 uiHash = H::Hash(key);
  ezUInt32 uiIndex = FindEntry(uiHash, key);

  if (out_pExisted)
  {
    *out_pExisted = uiIndex != ezInvalidIndex;
  }

  if (uiIndex == ezInvalidIndex)
  {
    Reserve(m_uiCount + 1);

    // search for suitable insertion index again, table might have been resized
    uiIndex = uiHash & (m_uiCapacity - 1);
    while (IsValidEntry(uiIndex))
    {
      ++uiIndex;
      if (uiIndex == m_uiCapacity)
        uiIndex = 0;
    }

    // new entry
    ezMemoryUtils::CopyConstruct(&m_pEntries[uiIndex].key, key, 1);
    ezMemoryUtils::DefaultConstruct(&m_pEntries[uiIndex].value, 1);
    MarkEntryAsValid(uiIndex);
    ++m_uiCount;
  }

  EZ_ASSERT_DEBUG(m_pEntries != nullptr, "Entries should be present");
  return m_pEntries[uiIndex].value;
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
EZ_FORCE_INLINE bool ezHashTableBase<K, V, H>::Contains(const CompatibleKeyType& key) const
{
  return FindEntry(key) != ezInvalidIndex;
}

template <typename K, typename V, typename H>
EZ_ALWAYS_INLINE typename ezHashTableBase<K, V, H>::Iterator ezHashTableBase<K, V, H>::GetIterator()
{
  Iterator iterator(*this);
  iterator.SetToBegin();
  return iterator;
}

template <typename K, typename V, typename H>
EZ_ALWAYS_INLINE typename ezHashTableBase<K, V, H>::Iterator ezHashTableBase<K, V, H>::GetEndIterator()
{
  Iterator iterator(*this);
  iterator.SetToEnd();
  return iterator;
}

template <typename K, typename V, typename H>
EZ_ALWAYS_INLINE typename ezHashTableBase<K, V, H>::ConstIterator ezHashTableBase<K, V, H>::GetIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToBegin();
  return iterator;
}

template <typename K, typename V, typename H>
EZ_ALWAYS_INLINE typename ezHashTableBase<K, V, H>::ConstIterator ezHashTableBase<K, V, H>::GetEndIterator() const
{
  ConstIterator iterator(*this);
  iterator.SetToEnd();
  return iterator;
}

template <typename K, typename V, typename H>
EZ_ALWAYS_INLINE ezAllocatorBase* ezHashTableBase<K, V, H>::GetAllocator() const
{
  return m_pAllocator;
}

template <typename K, typename V, typename H>
ezUInt64 ezHashTableBase<K, V, H>::GetHeapMemoryUsage() const
{
  return ((ezUInt64)m_uiCapacity * sizeof(Entry)) + (sizeof(ezUInt32) * (ezUInt64)GetFlagsCapacity());
}

// private methods
template <typename K, typename V, typename H>
void ezHashTableBase<K, V, H>::SetCapacity(ezUInt32 uiCapacity)
{
  EZ_ASSERT_DEBUG(ezMath::IsPowerOf2(uiCapacity), "uiCapacity must be a power of two to avoid modulo during lookup.");
  const ezUInt32 uiOldCapacity = m_uiCapacity;
  m_uiCapacity = uiCapacity;

  Entry* pOldEntries = m_pEntries;
  ezUInt32* pOldEntryFlags = m_pEntryFlags;

  m_pEntries = EZ_NEW_RAW_BUFFER(m_pAllocator, Entry, m_uiCapacity);
  m_pEntryFlags = EZ_NEW_RAW_BUFFER(m_pAllocator, ezUInt32, GetFlagsCapacity());
  ezMemoryUtils::ZeroFill(m_pEntryFlags, GetFlagsCapacity());

  m_uiCount = 0;
  for (ezUInt32 i = 0; i < uiOldCapacity; ++i)
  {
    if (GetFlags(pOldEntryFlags, i) == VALID_ENTRY)
    {
      EZ_VERIFY(!Insert(std::move(pOldEntries[i].key), std::move(pOldEntries[i].value)), "Implementation error");

      ezMemoryUtils::Destruct(&pOldEntries[i].key, 1);
      ezMemoryUtils::Destruct(&pOldEntries[i].value, 1);
    }
  }

  EZ_DELETE_RAW_BUFFER(m_pAllocator, pOldEntries);
  EZ_DELETE_RAW_BUFFER(m_pAllocator, pOldEntryFlags);
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
EZ_ALWAYS_INLINE ezUInt32 ezHashTableBase<K, V, H>::FindEntry(const CompatibleKeyType& key) const
{
  return FindEntry(H::Hash(key), key);
}

template <typename K, typename V, typename H>
template <typename CompatibleKeyType>
inline ezUInt32 ezHashTableBase<K, V, H>::FindEntry(ezUInt32 uiHash, const CompatibleKeyType& key) const
{
  if (m_uiCapacity > 0)
  {
    ezUInt32 uiIndex = uiHash & (m_uiCapacity - 1);
    ezUInt32 uiCounter = 0;
    while (!IsFreeEntry(uiIndex) && uiCounter < m_uiCapacity)
    {
      if (IsValidEntry(uiIndex) && H::Equal(m_pEntries[uiIndex].key, key))
        return uiIndex;

      ++uiIndex;
      if (uiIndex == m_uiCapacity)
        uiIndex = 0;

      ++uiCounter;
    }
  }
  // not found
  return ezInvalidIndex;
}

#define EZ_HASHTABLE_USE_BITFLAGS EZ_ON

template <typename K, typename V, typename H>
EZ_FORCE_INLINE ezUInt32 ezHashTableBase<K, V, H>::GetFlagsCapacity() const
{
#if EZ_ENABLED(EZ_HASHTABLE_USE_BITFLAGS)
  return (m_uiCapacity + 15) / 16;
#else
  return m_uiCapacity;
#endif
}

template <typename K, typename V, typename H>
EZ_ALWAYS_INLINE ezUInt32 ezHashTableBase<K, V, H>::GetFlags(ezUInt32* pFlags, ezUInt32 uiEntryIndex) const
{
#if EZ_ENABLED(EZ_HASHTABLE_USE_BITFLAGS)
  const ezUInt32 uiIndex = uiEntryIndex / 16;
  const ezUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
  return (pFlags[uiIndex] >> uiSubIndex) & FLAGS_MASK;
#else
  return pFlags[uiEntryIndex] & FLAGS_MASK;
#endif
}

template <typename K, typename V, typename H>
void ezHashTableBase<K, V, H>::SetFlags(ezUInt32 uiEntryIndex, ezUInt32 uiFlags)
{
#if EZ_ENABLED(EZ_HASHTABLE_USE_BITFLAGS)
  const ezUInt32 uiIndex = uiEntryIndex / 16;
  const ezUInt32 uiSubIndex = (uiEntryIndex & 15) * 2;
  EZ_ASSERT_DEBUG(uiIndex < GetFlagsCapacity(), "Out of bounds access");
  m_pEntryFlags[uiIndex] &= ~(FLAGS_MASK << uiSubIndex);
  m_pEntryFlags[uiIndex] |= (uiFlags << uiSubIndex);
#else
  EZ_ASSERT_DEBUG(uiEntryIndex < GetFlagsCapacity(), "Out of bounds access");
  m_pEntryFlags[uiEntryIndex] = uiFlags;
#endif
}

template <typename K, typename V, typename H>
EZ_FORCE_INLINE bool ezHashTableBase<K, V, H>::IsFreeEntry(ezUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == FREE_ENTRY;
}

template <typename K, typename V, typename H>
EZ_FORCE_INLINE bool ezHashTableBase<K, V, H>::IsValidEntry(ezUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == VALID_ENTRY;
}

template <typename K, typename V, typename H>
EZ_FORCE_INLINE bool ezHashTableBase<K, V, H>::IsDeletedEntry(ezUInt32 uiEntryIndex) const
{
  return GetFlags(m_pEntryFlags, uiEntryIndex) == DELETED_ENTRY;
}

template <typename K, typename V, typename H>
EZ_FORCE_INLINE void ezHashTableBase<K, V, H>::MarkEntryAsFree(ezUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, FREE_ENTRY);
}

template <typename K, typename V, typename H>
EZ_FORCE_INLINE void ezHashTableBase<K, V, H>::MarkEntryAsValid(ezUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, VALID_ENTRY);
}

template <typename K, typename V, typename H>
EZ_FORCE_INLINE void ezHashTableBase<K, V, H>::MarkEntryAsDeleted(ezUInt32 uiEntryIndex)
{
  SetFlags(uiEntryIndex, DELETED_ENTRY);
}


template <typename K, typename V, typename H, typename A>
ezHashTable<K, V, H, A>::ezHashTable()
  : ezHashTableBase<K, V, H>(A::GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
ezHashTable<K, V, H, A>::ezHashTable(ezAllocatorBase* pAllocator)
  : ezHashTableBase<K, V, H>(pAllocator)
{
}

template <typename K, typename V, typename H, typename A>
ezHashTable<K, V, H, A>::ezHashTable(const ezHashTable<K, V, H, A>& other)
  : ezHashTableBase<K, V, H>(other, A::GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
ezHashTable<K, V, H, A>::ezHashTable(const ezHashTableBase<K, V, H>& other)
  : ezHashTableBase<K, V, H>(other, A::GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
ezHashTable<K, V, H, A>::ezHashTable(ezHashTable<K, V, H, A>&& other)
  : ezHashTableBase<K, V, H>(std::move(other), other.GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
ezHashTable<K, V, H, A>::ezHashTable(ezHashTableBase<K, V, H>&& other)
  : ezHashTableBase<K, V, H>(std::move(other), other.GetAllocator())
{
}

template <typename K, typename V, typename H, typename A>
void ezHashTable<K, V, H, A>::operator=(const ezHashTable<K, V, H, A>& rhs)
{
  ezHashTableBase<K, V, H>::operator=(rhs);
}

template <typename K, typename V, typename H, typename A>
void ezHashTable<K, V, H, A>::operator=(const ezHashTableBase<K, V, H>& rhs)
{
  ezHashTableBase<K, V, H>::operator=(rhs);
}

template <typename K, typename V, typename H, typename A>
void ezHashTable<K, V, H, A>::operator=(ezHashTable<K, V, H, A>&& rhs)
{
  ezHashTableBase<K, V, H>::operator=(std::move(rhs));
}

template <typename K, typename V, typename H, typename A>
void ezHashTable<K, V, H, A>::operator=(ezHashTableBase<K, V, H>&& rhs)
{
  ezHashTableBase<K, V, H>::operator=(std::move(rhs));
}

template <typename KeyType, typename ValueType, typename Hasher>
void ezHashTableBase<KeyType, ValueType, Hasher>::Swap(ezHashTableBase<KeyType, ValueType, Hasher>& other)
{
  ezMath::Swap(this->m_pEntries, other.m_pEntries);
  ezMath::Swap(this->m_pEntryFlags, other.m_pEntryFlags);
  ezMath::Swap(this->m_uiCount, other.m_uiCount);
  ezMath::Swap(this->m_uiCapacity, other.m_uiCapacity);
  ezMath::Swap(this->m_pAllocator, other.m_pAllocator);
}

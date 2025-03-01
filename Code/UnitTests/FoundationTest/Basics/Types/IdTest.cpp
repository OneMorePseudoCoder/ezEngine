#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Types/Id.h>

EZ_WARNING_PUSH()
EZ_WARNING_DISABLE_MSVC(4463)
EZ_WARNING_DISABLE_MSVC(4068)
EZ_WARNING_DISABLE_GCC("-Wbitfield-constant-conversion")
EZ_WARNING_DISABLE_GCC("-Woverflow")
EZ_WARNING_DISABLE_CLANG("-Wbitfield-constant-conversion")

struct TestId
{
  using StorageType = ezUInt32;

  EZ_DECLARE_ID_TYPE(TestId, 20, 6);

  EZ_ALWAYS_INLINE TestId(StorageType instanceIndex, StorageType generation, StorageType systemIndex = 0)
  {
    m_Data = 0;
    m_InstanceIndex = instanceIndex;
    m_Generation = generation;
    m_SystemIndex = systemIndex;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 20;
      StorageType m_Generation : 6;
      StorageType m_SystemIndex : 6;
    };
  };
};

using LargeTestId = ezGenericId<32, 10>;

EZ_CREATE_SIMPLE_TEST(Basics, Id)
{
  TestId id1;
  EZ_TEST_INT(id1.m_InstanceIndex, TestId::INVALID_INSTANCE_INDEX);
  EZ_TEST_INT(id1.m_Generation, 0);
  EZ_TEST_INT(id1.m_SystemIndex, 0);

  TestId id2(1, 20, 15);
  TestId id3(1, 84, 79); // overflow
  EZ_TEST_INT(id2.m_InstanceIndex, 1);
  EZ_TEST_INT(id2.m_Generation, 20);
  EZ_TEST_INT(id2.m_SystemIndex, 15);
  EZ_TEST_BOOL(id2 == id3);

  id2.m_InstanceIndex = 2;
  EZ_TEST_INT(id2.m_InstanceIndex, 2);
  EZ_TEST_BOOL(id2 != id3);
  EZ_TEST_BOOL(!id2.IsIndexAndGenerationEqual(id3));

  id2.m_InstanceIndex = 1;
  id2.m_SystemIndex = 16;
  EZ_TEST_BOOL(id2 != id3);
  EZ_TEST_BOOL(id2.IsIndexAndGenerationEqual(id3));

  id2.m_Generation = 94; // overflow
  EZ_TEST_INT(id2.m_Generation, 30);

  id2.m_SystemIndex = 94; // overflow
  EZ_TEST_INT(id2.m_SystemIndex, 30);

  LargeTestId id4(1, 1224); // overflow
  EZ_TEST_INT(id4.m_InstanceIndex, 1);
  EZ_TEST_INT(id4.m_Generation, 200);
}

EZ_WARNING_POP()

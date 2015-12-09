/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */

#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include "cluster-controller/access-control/LocalDomainAccessStore.h"
#include "joynr/TypeUtil.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::infrastructure::DacTypes;

class LocalDomainAccessStoreTest : public ::testing::Test {
public:
    LocalDomainAccessStoreTest()
    {

    }

    ~LocalDomainAccessStoreTest() {

    }

    void SetUp(){
        bool startWithCleanDatabase = true;
        localDomainAccessStore = new LocalDomainAccessStore(startWithCleanDatabase);
        expectedDomainRoleEntry = DomainRoleEntry(TEST_USER1.toStdString(), TypeUtil::toStd(DOMAINS), Role::OWNER);
        expectedMasterAccessControlEntry = MasterAccessControlEntry(TEST_USER1.toStdString(),
                                                                    TEST_DOMAIN1.toStdString(),
                                                                    TEST_INTERFACE1.toStdString(),
                                                                    TrustLevel::LOW,
                                                                    TypeUtil::toStd(TRUST_LEVELS),
                                                                    TrustLevel::LOW,
                                                                    TypeUtil::toStd(TRUST_LEVELS),
                                                                    TEST_OPERATION1.toStdString(),
                                                                    Permission::NO,
                                                                    TypeUtil::toStd(PERMISSIONS));
        expectedOwnerAccessControlEntry = OwnerAccessControlEntry(TEST_USER1.toStdString(),
                                                                  TEST_DOMAIN1.toStdString(),
                                                                  TEST_INTERFACE1.toStdString(),
                                                                  TrustLevel::LOW,
                                                                  TrustLevel::LOW,
                                                                  TEST_OPERATION1.toStdString(),
                                                                  Permission::NO);
    }

    void TearDown(){
        delete localDomainAccessStore;
    }
protected:
    LocalDomainAccessStore* localDomainAccessStore;
    DomainRoleEntry expectedDomainRoleEntry;
    MasterAccessControlEntry expectedMasterAccessControlEntry;
    OwnerAccessControlEntry expectedOwnerAccessControlEntry;

    static const QString TEST_USER1;
    static const QString TEST_USER2;
    static const QString TEST_DOMAIN1;
    static const QString TEST_INTERFACE1;
    static const QString TEST_INTERFACE2;
    static const QString TEST_OPERATION1;
    static const QString TEST_OPERATION2;
    static const QList<QString> DOMAINS;
    static const QList<Permission::Enum> PERMISSIONS;
    static const QList<TrustLevel::Enum> TRUST_LEVELS;
private:
    DISALLOW_COPY_AND_ASSIGN(LocalDomainAccessStoreTest);
};

const QString LocalDomainAccessStoreTest::TEST_USER1("testUser1");
const QString LocalDomainAccessStoreTest::TEST_USER2("testUser2");
const QString LocalDomainAccessStoreTest::TEST_DOMAIN1("domain1");
const QString LocalDomainAccessStoreTest::TEST_INTERFACE1("interface1");
const QString LocalDomainAccessStoreTest::TEST_INTERFACE2("interface2");
const QString LocalDomainAccessStoreTest::TEST_OPERATION1("READ");
const QString LocalDomainAccessStoreTest::TEST_OPERATION2("WRITE");
const QList<QString> LocalDomainAccessStoreTest::DOMAINS = QList<QString>() << TEST_DOMAIN1;
const QList<Permission::Enum> LocalDomainAccessStoreTest::PERMISSIONS = QList<Permission::Enum>() << Permission::NO << Permission::ASK;
const QList<TrustLevel::Enum> LocalDomainAccessStoreTest::TRUST_LEVELS = QList<TrustLevel::Enum>() << TrustLevel::LOW << TrustLevel::MID;

//----- Tests ------------------------------------------------------------------

TEST_F(LocalDomainAccessStoreTest, getDomainRoles) {
    localDomainAccessStore->updateDomainRole(expectedDomainRoleEntry);

    QList<DomainRoleEntry> domainRoles = localDomainAccessStore->getDomainRoles(QString::fromStdString(expectedDomainRoleEntry.getUid()));
    EXPECT_EQ(expectedDomainRoleEntry, domainRoles.first());

    Optional<DomainRoleEntry> domainRole = localDomainAccessStore->getDomainRole(QString::fromStdString(expectedDomainRoleEntry.getUid()),
                                                                                 expectedDomainRoleEntry.getRole());
    EXPECT_TRUE(bool(domainRole));
    EXPECT_EQ(expectedDomainRoleEntry, domainRole.getValue());
}

TEST_F(LocalDomainAccessStoreTest, updateDomainRole) {
    EXPECT_TRUE(localDomainAccessStore->updateDomainRole(expectedDomainRoleEntry));

    // Check that an entry was added
    QList<DomainRoleEntry> dres = localDomainAccessStore->getDomainRoles(QString::fromStdString(expectedDomainRoleEntry.getUid()));
    EXPECT_FALSE(dres.isEmpty());
    Optional<DomainRoleEntry> dreFromDb = localDomainAccessStore->getDomainRole(QString::fromStdString(expectedDomainRoleEntry.getUid()),
                                                                                expectedDomainRoleEntry.getRole());

    EXPECT_EQ(expectedDomainRoleEntry, dreFromDb.getValue());
}

TEST_F(LocalDomainAccessStoreTest, removeDomainRole) {
    localDomainAccessStore->updateDomainRole(expectedDomainRoleEntry);

    EXPECT_TRUE(localDomainAccessStore->removeDomainRole(QString::fromStdString(expectedDomainRoleEntry.getUid()), expectedDomainRoleEntry.getRole()));
    Optional<DomainRoleEntry> dreFromDb = localDomainAccessStore->getDomainRole(QString::fromStdString(expectedDomainRoleEntry.getUid()),
                                                                                expectedDomainRoleEntry.getRole());
    EXPECT_FALSE(bool(dreFromDb));
}

TEST_F(LocalDomainAccessStoreTest, getMasterAces) {
    localDomainAccessStore->updateMasterAccessControlEntry(expectedMasterAccessControlEntry);
    EXPECT_EQ(expectedMasterAccessControlEntry, localDomainAccessStore->getMasterAccessControlEntries(QString::fromStdString(expectedMasterAccessControlEntry.getUid())).first());
    EXPECT_EQ(expectedMasterAccessControlEntry, localDomainAccessStore->getMasterAccessControlEntries(QString::fromStdString(expectedMasterAccessControlEntry.getDomain()),
                                                                                                     QString::fromStdString(expectedMasterAccessControlEntry.getInterfaceName())).first());
    EXPECT_EQ(expectedMasterAccessControlEntry, localDomainAccessStore->getMasterAccessControlEntries(QString::fromStdString(expectedMasterAccessControlEntry.getUid()),
                                                                                                     QString::fromStdString(expectedMasterAccessControlEntry.getDomain()),
                                                                                                     QString::fromStdString(expectedMasterAccessControlEntry.getInterfaceName())).first());
    EXPECT_EQ(expectedMasterAccessControlEntry, localDomainAccessStore->getMasterAccessControlEntry(QString::fromStdString(expectedMasterAccessControlEntry.getUid()),
                                                                                                    QString::fromStdString(expectedMasterAccessControlEntry.getDomain()),
                                                                                                    QString::fromStdString(expectedMasterAccessControlEntry.getInterfaceName()),
                                                                                                    QString::fromStdString(expectedMasterAccessControlEntry.getOperation())).getValue());
    MasterAccessControlEntry masterAceWildcardUser(expectedMasterAccessControlEntry);
    masterAceWildcardUser.setUid(LocalDomainAccessStore::WILDCARD);
    localDomainAccessStore->updateMasterAccessControlEntry(masterAceWildcardUser);

    QList<MasterAccessControlEntry> masterAces = localDomainAccessStore->getMasterAccessControlEntries(TEST_DOMAIN1, TEST_INTERFACE1);
    int expectedNumberOfMasterAces = 2;
    EXPECT_EQ(expectedNumberOfMasterAces, masterAces.length());
    EXPECT_TRUE(masterAces.contains(expectedMasterAccessControlEntry));
    EXPECT_TRUE(masterAces.contains(masterAceWildcardUser));
    EXPECT_EQ(masterAceWildcardUser, localDomainAccessStore->getMasterAccessControlEntry(TEST_USER2,
                                                                                         QString::fromStdString(masterAceWildcardUser.getDomain()),
                                                                                         QString::fromStdString(masterAceWildcardUser.getInterfaceName()),
                                                                                         QString::fromStdString(masterAceWildcardUser.getOperation())).getValue());
    EXPECT_EQ(masterAceWildcardUser, (localDomainAccessStore->getMasterAccessControlEntries(TEST_USER2)).first());
    EXPECT_EQ(masterAceWildcardUser, (localDomainAccessStore->getMasterAccessControlEntries(TEST_USER2,
                                                                                            QString::fromStdString(masterAceWildcardUser.getDomain()),
                                                                                            QString::fromStdString(masterAceWildcardUser.getInterfaceName())).first()));
}

TEST_F(LocalDomainAccessStoreTest, getMasterAceWithWildcardOperation) {
    expectedMasterAccessControlEntry.setOperation(LocalDomainAccessStore::WILDCARD);
    localDomainAccessStore->updateMasterAccessControlEntry(expectedMasterAccessControlEntry);

    EXPECT_EQ(expectedMasterAccessControlEntry, localDomainAccessStore->getMasterAccessControlEntry(QString::fromStdString(expectedMasterAccessControlEntry.getUid()),
                                                                                                    QString::fromStdString(expectedMasterAccessControlEntry.getDomain()),
                                                                                                    QString::fromStdString(expectedMasterAccessControlEntry.getInterfaceName()),
                                                                                                    TEST_OPERATION1).getValue());
}

TEST_F(LocalDomainAccessStoreTest, editableMasterAces) {
    expectedDomainRoleEntry.setRole(Role::MASTER);
    localDomainAccessStore->updateDomainRole(expectedDomainRoleEntry);
    localDomainAccessStore->updateMasterAccessControlEntry(expectedMasterAccessControlEntry);

    QList<MasterAccessControlEntry> editableMasterAces = localDomainAccessStore->getEditableMasterAccessControlEntries(TEST_USER1);
    int expectedNumberOfMasterAces = 1;
    EXPECT_EQ(expectedNumberOfMasterAces, editableMasterAces.length());
    EXPECT_EQ(expectedMasterAccessControlEntry, editableMasterAces.first());
}

TEST_F(LocalDomainAccessStoreTest, editableMasterAccessControlEntryNoMatchingDre) {
    expectedMasterAccessControlEntry.setUid(TEST_USER2.toStdString());
    localDomainAccessStore->updateMasterAccessControlEntry(expectedMasterAccessControlEntry);

    QList<MasterAccessControlEntry> editableMasterAces = localDomainAccessStore->getEditableMasterAccessControlEntries(TEST_USER1);
    EXPECT_TRUE(editableMasterAces.isEmpty());
}

TEST_F(LocalDomainAccessStoreTest, updateMasterAce) {
    EXPECT_TRUE(localDomainAccessStore->updateMasterAccessControlEntry(expectedMasterAccessControlEntry));

    MasterAccessControlEntry masterAceFromDb =
            localDomainAccessStore->getMasterAccessControlEntry(QString::fromStdString(expectedMasterAccessControlEntry.getUid()),
                                                 QString::fromStdString(expectedMasterAccessControlEntry.getDomain()),
                                                 QString::fromStdString(expectedMasterAccessControlEntry.getInterfaceName()),
                                                 QString::fromStdString(expectedMasterAccessControlEntry.getOperation())).getValue();
    EXPECT_EQ(expectedMasterAccessControlEntry, masterAceFromDb);
}

TEST_F(LocalDomainAccessStoreTest, removeMasterAce) {
    localDomainAccessStore->updateMasterAccessControlEntry(expectedMasterAccessControlEntry);

    EXPECT_TRUE(localDomainAccessStore->removeMasterAccessControlEntry(QString::fromStdString(expectedMasterAccessControlEntry.getUid()),
                                                         QString::fromStdString(expectedMasterAccessControlEntry.getDomain()),
                                                         QString::fromStdString(expectedMasterAccessControlEntry.getInterfaceName()),
                                                         QString::fromStdString(expectedMasterAccessControlEntry.getOperation())));

    // Check the ACE does not exist
    QList<MasterAccessControlEntry> masterAces =
            localDomainAccessStore->getMasterAccessControlEntries(QString::fromStdString(expectedMasterAccessControlEntry.getUid()),
                                                  QString::fromStdString(expectedMasterAccessControlEntry.getDomain()),
                                                  QString::fromStdString(expectedMasterAccessControlEntry.getInterfaceName()));

    EXPECT_TRUE(masterAces.isEmpty());
}

TEST_F(LocalDomainAccessStoreTest, getOwnerAccessControlEntry) {
    localDomainAccessStore->updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);

    EXPECT_EQ(expectedOwnerAccessControlEntry, localDomainAccessStore->getOwnerAccessControlEntries(QString::fromStdString(expectedOwnerAccessControlEntry.getUid())).first());
    EXPECT_EQ(expectedOwnerAccessControlEntry, localDomainAccessStore->getOwnerAccessControlEntries(QString::fromStdString(expectedOwnerAccessControlEntry.getDomain()),
                                                                                                    QString::fromStdString(expectedOwnerAccessControlEntry.getInterfaceName())).first());
    EXPECT_EQ(expectedOwnerAccessControlEntry, localDomainAccessStore->getOwnerAccessControlEntries(QString::fromStdString(expectedOwnerAccessControlEntry.getUid()),
                                                                                                    QString::fromStdString(expectedOwnerAccessControlEntry.getDomain()),
                                                                                                    QString::fromStdString(expectedOwnerAccessControlEntry.getInterfaceName())).first());
    EXPECT_EQ(expectedOwnerAccessControlEntry, localDomainAccessStore->getOwnerAccessControlEntry(QString::fromStdString(expectedOwnerAccessControlEntry.getUid()),
                                                                                                    QString::fromStdString(expectedOwnerAccessControlEntry.getDomain()),
                                                                                                    QString::fromStdString(expectedOwnerAccessControlEntry.getInterfaceName()),
                                                                                                    QString::fromStdString(expectedOwnerAccessControlEntry.getOperation())).getValue());
    OwnerAccessControlEntry ownerAceWildcardUser(expectedOwnerAccessControlEntry);
    ownerAceWildcardUser.setUid(LocalDomainAccessStore::WILDCARD);
    EXPECT_TRUE(localDomainAccessStore->updateOwnerAccessControlEntry(ownerAceWildcardUser));

    QList<OwnerAccessControlEntry> ownerAces = localDomainAccessStore->getOwnerAccessControlEntries(TEST_DOMAIN1, TEST_INTERFACE1);
    int expectedNumberOfOwnerAces = 2;
    EXPECT_EQ(expectedNumberOfOwnerAces, ownerAces.length());
    EXPECT_TRUE(ownerAces.contains(expectedOwnerAccessControlEntry));
    EXPECT_TRUE(ownerAces.contains(ownerAceWildcardUser));
    EXPECT_EQ(ownerAceWildcardUser, localDomainAccessStore->getOwnerAccessControlEntry(TEST_USER2,
                                                                                         QString::fromStdString(ownerAceWildcardUser.getDomain()),
                                                                                         QString::fromStdString(ownerAceWildcardUser.getInterfaceName()),
                                                                                         QString::fromStdString(ownerAceWildcardUser.getOperation())).getValue());
    EXPECT_EQ(ownerAceWildcardUser, (localDomainAccessStore->getOwnerAccessControlEntries(TEST_USER2)).first());
    EXPECT_EQ(ownerAceWildcardUser, (localDomainAccessStore->getOwnerAccessControlEntries(TEST_USER2,
                                                                                            QString::fromStdString(ownerAceWildcardUser.getDomain()),
                                                                                            QString::fromStdString(ownerAceWildcardUser.getInterfaceName()))).first());
}

TEST_F(LocalDomainAccessStoreTest, getEditableOwnerAces) {
    localDomainAccessStore->updateDomainRole(expectedDomainRoleEntry);
    localDomainAccessStore->updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);

    // Check that the correct editable domain is returned
    QList<OwnerAccessControlEntry> editableOwnerAces =
            localDomainAccessStore->getEditableOwnerAccessControlEntries(TEST_USER1);

    int expectedEditableOwnerAces = 1;
    EXPECT_EQ(expectedEditableOwnerAces, editableOwnerAces.length());
    EXPECT_EQ(expectedOwnerAccessControlEntry, editableOwnerAces.first());
}

TEST_F(LocalDomainAccessStoreTest, editableOwnerAccessControlEntryNoMatchingDre) {
    localDomainAccessStore->updateDomainRole(expectedDomainRoleEntry);
    expectedOwnerAccessControlEntry.setUid(TEST_USER2.toStdString());
    localDomainAccessStore->updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);

    // Check that the correct editable domain is returned
    QList<OwnerAccessControlEntry> editableOwnerAces =
            localDomainAccessStore->getEditableOwnerAccessControlEntries(TEST_USER2);

    EXPECT_TRUE(editableOwnerAces.isEmpty());
}

TEST_F(LocalDomainAccessStoreTest, updateOwnerAccessControlEntry) {
    EXPECT_TRUE(localDomainAccessStore->updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry));
}

TEST_F(LocalDomainAccessStoreTest, updateOwnerAce) {
    EXPECT_TRUE(localDomainAccessStore->updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry));

    // Check that the entry was added
    OwnerAccessControlEntry ownerAceFromDb =
            localDomainAccessStore->getOwnerAccessControlEntry(QString::fromStdString(expectedOwnerAccessControlEntry.getUid()),
                                                QString::fromStdString(expectedOwnerAccessControlEntry.getDomain()),
                                                QString::fromStdString(expectedOwnerAccessControlEntry.getInterfaceName()),
                                                QString::fromStdString(expectedOwnerAccessControlEntry.getOperation())).getValue();
    EXPECT_EQ(expectedOwnerAccessControlEntry, ownerAceFromDb);
}

TEST_F(LocalDomainAccessStoreTest, removeOwnerAce) {
    localDomainAccessStore->updateOwnerAccessControlEntry(expectedOwnerAccessControlEntry);
    // Remove the ACE
    EXPECT_TRUE( localDomainAccessStore->removeOwnerAccessControlEntry(QString::fromStdString(expectedOwnerAccessControlEntry.getUid()),
                                                        QString::fromStdString(expectedOwnerAccessControlEntry.getDomain()),
                                                        QString::fromStdString(expectedOwnerAccessControlEntry.getInterfaceName()),
                                                        QString::fromStdString(expectedOwnerAccessControlEntry.getOperation())));

    // Check the ACE does not exist
    QList<OwnerAccessControlEntry> ownerAces =
            localDomainAccessStore->getOwnerAccessControlEntries(QString::fromStdString(expectedOwnerAccessControlEntry.getUid()),
                                                  QString::fromStdString(expectedOwnerAccessControlEntry.getDomain()),
                                                  QString::fromStdString(expectedOwnerAccessControlEntry.getInterfaceName()));
    EXPECT_TRUE(ownerAces.empty());
}


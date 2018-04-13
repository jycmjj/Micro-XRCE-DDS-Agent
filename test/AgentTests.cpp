// Copyright 2017 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Common.h"

#include <agent/Root.h>
#include <agent/client/ProxyClient.h>

#include <MessageHeader.h>
#include <SubMessageHeader.h>

#include <gtest/gtest.h>

namespace eprosima {
namespace micrortps {
namespace testing {

class AgentTests : public CommonData, public ::testing::Test
{
  protected:
    AgentTests() = default;

    virtual ~AgentTests() = default;

    eprosima::micrortps::Agent agent_;
};

TEST_F(AgentTests, CreateClientOk)
{
    dds::xrce::AGENT_Representation agent_representation;
    dds::xrce::ResultStatus response = agent_.create_client(generate_create_client_payload().client_representation(),
                                                            agent_representation, 0, 0);
    ASSERT_EQ(dds::xrce::STATUS_OK, response.status());
}

TEST_F(AgentTests, CreateClientBadCookie)
{
    dds::xrce::CREATE_CLIENT_Payload create_data = generate_create_client_payload();
    dds::xrce::AGENT_Representation agent_representation;
    create_data.client_representation().xrce_cookie({0x00, 0x00});
    dds::xrce::ResultStatus response = agent_.create_client(create_data.client_representation(),
                                                            agent_representation, 0, 0);
    ASSERT_EQ(dds::xrce::STATUS_ERR_INVALID_DATA, response.status());
}

TEST_F(AgentTests, CreateClientCompatibleVersion)
{
    dds::xrce::CREATE_CLIENT_Payload create_data = generate_create_client_payload();
    create_data.client_representation().xrce_version({{XRCE_VERSION_MAJOR, 0x20}});
    dds::xrce::AGENT_Representation agent_representation;
    dds::xrce::ResultStatus response = agent_.create_client(create_data.client_representation(),
                                                            agent_representation, 0, 0);
    ASSERT_EQ(dds::xrce::STATUS_OK, response.status());
}

TEST_F(AgentTests, CreateClientIncompatibleVersion)
{
    dds::xrce::CREATE_CLIENT_Payload create_data = generate_create_client_payload();
    create_data.client_representation().xrce_version({{0x02, XRCE_VERSION_MINOR}});
    dds::xrce::AGENT_Representation agent_representation;
    dds::xrce::ResultStatus response = agent_.create_client(create_data.client_representation(),
                                                            agent_representation, 0, 0);
    ASSERT_EQ(dds::xrce::STATUS_ERR_INCOMPATIBLE, response.status());
}

TEST_F(AgentTests, DeleteExistingClient)
{
    dds::xrce::CREATE_CLIENT_Payload create_data = generate_create_client_payload();
    dds::xrce::AGENT_Representation agent_representation;
    dds::xrce::ResultStatus response      = agent_.create_client(create_data.client_representation(),
                                                                 agent_representation, 0, 0);
    ASSERT_EQ(dds::xrce::STATUS_OK, response.status());

    response = agent_.delete_client(client_key);
    ASSERT_EQ(dds::xrce::STATUS_OK, response.status());
}

TEST_F(AgentTests, DeleteOnEmptyAgent)
{
    dds::xrce::ResultStatus response = agent_.delete_client(client_key);
    ASSERT_EQ(dds::xrce::STATUS_ERR_UNKNOWN_REFERENCE, response.status());
}

TEST_F(AgentTests, DeleteNoExistingClient)
{
    const dds::xrce::ClientKey fake_client_key = {{0xFA, 0xFB, 0xFC, 0xFD}};

    dds::xrce::CREATE_CLIENT_Payload create_data = generate_create_client_payload();
    dds::xrce::AGENT_Representation agent_representation;
    dds::xrce::ResultStatus response      = agent_.create_client(create_data.client_representation(),
                                                                 agent_representation, 0, 0);
    ASSERT_EQ(dds::xrce::STATUS_OK, response.status());

    response = agent_.delete_client(fake_client_key);
    ASSERT_EQ(dds::xrce::STATUS_ERR_UNKNOWN_REFERENCE, response.status());
}

class ProxyClientTests : public CommonData, public ::testing::Test
{
  protected:
    ProxyClientTests()          = default;
    virtual ~ProxyClientTests() = default;

    ProxyClient client_;
};

/* TODO (Julian): participant is need for creating a  subscriber. */
//TEST_F(ProxyClientTests, CreateSubscriberOK)
//{
//    dds::xrce::ResultStatus result = client_.create(dds::xrce::CreationMode{}, generate_create_payload(dds::xrce::OBJK_SUBSCRIBER));
//    ASSERT_EQ(dds::xrce::STATUS_OK, result.status());
//}

/* TODO (Julian): participant is need for creating a  subscriber. */
//TEST_F(ProxyClientTests, CreateSubscriberDuplicated)
//{
//    dds::xrce::ResultStatus result = client_.create(dds::xrce::CreationMode{}, generate_create_payload(dds::xrce::OBJK_SUBSCRIBER));
//    ASSERT_EQ(dds::xrce::STATUS_OK, result.status());
//
//    result = client_.create(dds::xrce::CreationMode{}, generate_create_payload(dds::xrce::OBJK_SUBSCRIBER));
//    ASSERT_EQ(dds::xrce::STATUS_ERR_ALREADY_EXISTS, result.status());
//}

/* TODO (Julian): participant is need for creating a  subscriber. */
//TEST_F(ProxyClientTests, CreateSubscriberDuplicatedReplaced)
//{
//    dds::xrce::ResultStatus result = client_.create(dds::xrce::CreationMode{}, generate_create_payload(dds::xrce::OBJK_SUBSCRIBER));
//    ASSERT_EQ(dds::xrce::STATUS_OK, result.status());
//
//    dds::xrce::CreationMode creation_mode;
//    creation_mode.reuse(false);
//    creation_mode.replace(true);
//    result = client_.create(creation_mode, generate_create_payload(dds::xrce::OBJK_SUBSCRIBER));
//    ASSERT_EQ(dds::xrce::STATUS_OK, result.status());
//}

TEST_F(ProxyClientTests, DeleteOnEmpty)
{
    dds::xrce::ResultStatus result_status = client_.delete_object(object_id);
    ASSERT_EQ(dds::xrce::STATUS_ERR_UNKNOWN_REFERENCE, result_status.status());
}

/* TODO (Julian): participant is need for creating a  subscriber. */
//TEST_F(ProxyClientTests, DeleteWrongId)
//{
//    dds::xrce::ResultStatus result = client_.create(dds::xrce::CreationMode{}, generate_create_payload(dds::xrce::OBJK_SUBSCRIBER));
//    ASSERT_EQ(dds::xrce::STATUS_OK, result.status());
//
//    const dds::xrce::ObjectId fake_object_id = {{0xFA, 0xFA}};
//    ASSERT_NE(object_id, fake_object_id);
//
//    result = client_.delete_object(generate_delete_resource_payload(fake_object_id));
//    ASSERT_EQ(dds::xrce::STATUS_ERR_UNKNOWN_REFERENCE, result.status());
//}

/* TODO (Julian): participant is need for creating a  subscriber. */
//TEST_F(ProxyClientTests, DeleteOK)
//{
//    dds::xrce::CREATE_Payload create_data = generate_create_payload(dds::xrce::OBJK_SUBSCRIBER);
//    dds::xrce::ResultStatus result        = client_.create(dds::xrce::CreationMode{}, create_data);
//    ASSERT_EQ(dds::xrce::STATUS_OK, result.status());
//
//    result = client_.delete_object(generate_delete_resource_payload(create_data.object_id()));
//    ASSERT_EQ(dds::xrce::STATUS_OK, result.implementation_status());
//}
} // namespace testing
} // namespace micrortps
} // namespace eprosima

int main(int args, char** argv)
{
    ::testing::InitGoogleTest(&args, argv);
    return RUN_ALL_TESTS();
}

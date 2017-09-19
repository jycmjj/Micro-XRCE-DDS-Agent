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

#include "agent/XRCEParser.h"

#include "agent/ObjectVariant.h"
#include "agent/Payloads.h"
#include "agent/MessageHeader.h"
#include "agent/SubMessageHeader.h"

#include <iostream>

namespace eprosima {
namespace micrortps {

// TODO cxx style
void XRCEParser::print(const MessageHeader& message_header)
{
    printf("<Header> \n");
    printf("  - client_key: 0x%08X\n", message_header.client_key());
    printf("  - session_id: 0x%02X\n", message_header.session_id());
    printf("  - stream_id: 0x%02X\n", message_header.stream_id());
    printf("  - sequence_nr: %u\n", message_header.sequence_nr());
    printf("\n\n");
}

// TODO cxx style
void XRCEParser::print(const SubmessageHeader& submessage_header)
{
    switch(submessage_header.submessage_id())
    {
        case CREATE:
            printf("<Submessage> [CREATE] \n");
        break;
        case DELETE:
            printf("<Submessage> [DELETE]\n");
        break;
        case WRITE_DATA:
            printf("<Submessage> [WRITE_DATA]\n");
        break;
        case READ_DATA:
            printf("<Submessage> [READ_DATA]\n");
        break;
        case DATA:
            printf("<Submessage> [DATA]\n");
        break;
    }

    printf("  <Submessage header> \n");
    printf("  - id: 0x%02X\n", submessage_header.submessage_id());
    printf("  - flags: 0x%02X\n", submessage_header.flags());
    printf("  - length: %u\n", submessage_header.submessage_length());
    printf("\n");
}

// TODO cxx style
void XRCEParser::print(const CREATE_PAYLOAD& create_message)
{
    printf("  <Payload>\n");
    printf("  - request_id: 0x%08X\n", create_message.request_id());
    printf("  - object_id: 0x%06X\n", create_message.object_id());
    printf("  - kind: 0x%02X\n", create_message.object_representation().discriminator());

    switch(create_message.object_representation().discriminator())
    {
        case OBJK_DATAWRITER:
            printf("  - string_size: 0x%08X\n", create_message.object_representation().data_writer().as_string().size());
            printf("  - string: %s\n", create_message.object_representation().data_writer().as_string().data());
            printf("    <Data writer>\n");
            printf("    - participan_id: 0x%06X\n", create_message.object_representation().data_writer().participant_id());
            printf("    - publisher_id: 0x%06X\n", create_message.object_representation().data_writer().publisher_id());
        break;

        case OBJK_DATAREADER:
            printf("  - string_size: 0x%08X\n", create_message.object_representation().data_reader().as_string().size());
            printf("  - string: %s\n", create_message.object_representation().data_reader().as_string().data());
            printf("    <Data reader>\n");
            printf("    - participan_id: 0x%06X\n", create_message.object_representation().data_reader().participant_id());
            printf("    - subscriber_id: 0x%06X\n", create_message.object_representation().data_reader().subscriber_id());
        break;

        case OBJK_SUBSCRIBER:
            printf("  - string_size: 0x%08X\n", create_message.object_representation().subscriber().as_string().size());
            printf("  - string: %s\n", create_message.object_representation().subscriber().as_string().data());
            printf("    <Data subscriber>\n");
            printf("    - participan_id: 0x%06X\n", create_message.object_representation().subscriber().participant_id());
        break;

        case OBJK_PUBLISHER:
            printf("  - string_size: 0x%08X\n", create_message.object_representation().publisher().as_string().size());
            printf("  - string: %s\n", create_message.object_representation().publisher().as_string().data());
            printf("    <Data publisher>\n");
            printf("    - participan_id: 0x%06X\n", create_message.object_representation().publisher().participant_id());
        break;
    }
    printf("\n\n");
}

bool XRCEParser::parse()
{
    MessageHeader message_header;
    if (deserializer_.deserialize(message_header))
    {
        print(message_header);
        SubmessageHeader submessage_header;
        bool valid_submessage = false;
        do
        {
            if (valid_submessage = deserializer_.deserialize(submessage_header))
            {
                print(submessage_header);
                switch (submessage_header.submessage_id())
                {
                case CREATE:
                    if (!process_create(message_header, submessage_header))
                        std::cerr << "Error processing create" << std::endl;
                    break;
                case WRITE_DATA:
                    if (!process_write_data(message_header, submessage_header))
                        std::cerr << "Error processing write" << std::endl;
                    break;
                case READ_DATA:
                    if (!process_read_data(message_header, submessage_header))
                        std::cerr << "Error processing read" << std::endl;
                    break;
                case GET_INFO:
                case DELETE:
                case STATUS:
                case INFO:
                case DATA:
                case ACKNACK:
                case HEARTBEAT:
                case RESET:
                case FRAGMENT:
                case FRAGMENT_END:
                default:
                    std::cerr << "Error submessage ID not recognized" << std::endl;
                    return false;
                    break;
                }
            }
            else
            {
                std::cerr << "Error reading submessage header" << std::endl;
                return false;
            }
        } while (valid_submessage && !deserializer_.bufferEnd());
    }
    else
    {
        std::cerr << "Error reading message header" << std::endl;
        return false;
    }
    return true;
}

bool XRCEParser::process_create(const MessageHeader& header, const SubmessageHeader& sub_header)
{
    CREATE_PAYLOAD create_payload;
    if (deserializer_.deserialize(create_payload))
    {
        print(create_payload);
        listener_->on_message(header, sub_header, create_payload);
        return true;
    }
    return false;
}

bool XRCEParser::process_delete(const MessageHeader& header, const SubmessageHeader& sub_header)
{
    DELETE_PAYLOAD delete_payload;
    if (deserializer_.deserialize(delete_payload))
    {
        listener_->on_message(header, sub_header, delete_payload);
        return true;
    }
    return false;
}

bool XRCEParser::process_read_data(const MessageHeader& header, const SubmessageHeader& sub_header)
{
    READ_DATA_PAYLOAD read_data_payload;
    if (deserializer_.deserialize(read_data_payload))
    {
        listener_->on_message(header, sub_header, read_data_payload);
        return true;
    }
    return false;
}

bool XRCEParser::process_write_data(const MessageHeader& header, const SubmessageHeader& sub_header)
{
    WRITE_DATA_PAYLOAD write_data_payload;
    if (deserializer_.deserialize(write_data_payload))
    {
        listener_->on_message(header, sub_header, write_data_payload);
        return true;
    }
    return false;
}



} /* namespace micrortps */
} /* namespace eprosima */
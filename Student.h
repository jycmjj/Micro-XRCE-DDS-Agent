
#include <string>
#include <array>
// #include <ucdr/microcdr.h>
// #include <uxr/agent/middleware/fastdds/FastDDSMiddleware.hpp>
// #include <uxr/agent/utils/Conversion.hpp>
// #include <uxr/agent/logger/Logger.hpp>
// #include <ucdr/microcdr.h>
// #include <fastrtps/xmlparser/XMLProfileManager.h>
// #include <fastdds/dds/subscriber/SampleInfo.hpp>

// #include <uxr/agent/middleware/utils/Callbacks.hpp>

struct Student{
    std::string name;
    long number;
    long grade;
    std::string hobby[3];
    // void deserialize(ucdrBuffer &udr){
    //     char temp[255];
    //     if(ucdr_deserialize_string(&udr,temp,255)){
    //         name = std::string(temp);
    //     }
    //     ucdr_deserialize_int32_t(&udr,&number);
    //     ucdr_deserialize_int32_t(&udr,&grade);
    //     for(int i =0;i<3;++i){
    //         if(ucdr_deserialize_string(&udr,temp,255)){
    //             hobby[i]=std::string(temp);
    //         }else{
    //             throw std::runtime_error("failed to deserialize["+std::to_string(i)+"]");
    //         }
            
    //     }
    // }
};
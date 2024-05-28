// serializedPayload.h
#ifndef SERIALIZED_PAYLOAD_H
#define SERIALIZED_PAYLOAD_H

#include <iostream>
#include <cstring>
#include <w2rp/log.hpp>


namespace w2rp {

class SerializedPayload
{
  public:
    unsigned char* data;
    uint32_t length;
    uint32_t max_size;

    //!Default constructor
    SerializedPayload(): data(nullptr),
                            length(0),
                            max_size(0)
    {
    }

    SerializedPayload(unsigned char* binaryData, uint32_t length): 
                            data(binaryData),
                            length(length),
                            max_size(length)
    {
    }

    // /**
    //  * @brief copy constructor //TODO leads to "error: binding reference of type ‘w2rp::SerializedPayload&’ to ‘const w2rp::SerializedPayload’ discards qualifiers"
    //  *
    //  * @param sp reference to object to be copied
    //  */
    // SerializedPayload(SerializedPayload &sp): 
    //                         length(sp.length),
    //                         max_size(sp.length)
    // {
    //     this->data = new unsigned char[length];
    //     memset(this->data, 0, length * sizeof(unsigned char));
    //     memcpy(this->data, sp.data, this->length);
    // }

    ~SerializedPayload()
    {
        logInfo("[SerializedPayload] delete")
        //  TODO buggy, leads to double free or data corruption ...
        // this->empty();
    }

    bool operator == (
            const SerializedPayload& other) const
    {
        return ((length == other.length) &&
               (0 == memcmp(data, other.data, length)));
    }

    // bool copy(
    //         const SerializedPayload* serData,
    //         bool with_limit = true)
    // {
    //     logInfo("[SerializedPayload] copy")
    //     length = serData->length;
        
    //     this->reserve(serData->length);
    
    //     memcpy(data, serData->data, length);
    //     return true;
    // }

    // void empty()
    // {
    //     length = 0;
    //     if (data != nullptr)
    //     {
    //         free(data);
    //     }
    //     data = nullptr;
    // }

    // void reserve(
    //         uint32_t new_size)
    // {
    //     if (data == nullptr)
    //     {
    //         data = (unsigned char*)calloc(new_size, sizeof(unsigned char));
    //         if (!data)
    //         {
    //             throw std::bad_alloc();
    //         }
    //     }
    //     else
    //     {
    //         void* old_data = data;
    //         data = (unsigned char*)realloc(data, new_size);
    //         if (!data)
    //         {
    //             free(old_data);
    //             throw std::bad_alloc();
    //         }
    //         memset(data + max_size, 0, (new_size - max_size) * sizeof(unsigned char));
    //     }
    //     max_size = new_size;
    // }
};

}; // end namespace

#endif // SERIALIZED_PAYLOAD_H
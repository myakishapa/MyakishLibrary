#pragma once
#include <utility>
#include <map>

#include <MyakishLibrary/Enum/BitwiseOperators.hpp>

namespace myakish::enums::dynamic
{
    struct Untyped {};

    template<typename Underlying, typename TypeDescriptor = Untyped>
    struct Descriptor
    {
        Underlying value;

        [[no_unique_address]] 
        TypeDescriptor type;


    };
    
    template<typename Underlying, typename TypeDescriptor = Untyped>
    struct Table
    {

    };
}
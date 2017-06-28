#pragma once

#include <type_traits>

namespace com_port_api
{

    /**
     *	The utility class that simplifies a work with bit
     *	fields.
     */
    template <class T,
              typename = std::enable_if_t<std::is_integral<T>::value>
    >
    class bit_field
    {
    
    public:
    
        using value_t = T;
    
    public:
    
        value_t value;
    
    public:
    
        bit_field() : value(static_cast<value_t>(0))
        {
        }
    
        template <class O,
                  typename = std::enable_if_t<std::is_integral<T>::value>
        >
        bit_field(const O &other) : value(static_cast<value_t>(other))
        {
        }
    
        template <class O,
                  typename = std::enable_if_t<std::is_integral<O>::value>
        >
        bit_field(const bit_field<O> &other) : value(static_cast<value_t>(other))
        {
        }
        
        template <class O,
                  typename = std::enable_if_t<std::is_constructible<bit_field<T>, O>::value>
        >
        bit_field<T> & operator = (const O &other)
        {
            this->value = static_cast<value_t>(other);
            return *this;
        }
        
        template <class O,
                  typename = std::enable_if_t<std::is_constructible<bit_field<T>, O>::value>
        >
        bool operator == (const O &other) const
        {
            return (this->value == static_cast<value_t>(other));
        }
        
        template <class O,
                  typename = std::enable_if_t<std::is_constructible<bit_field<T>, O>::value>
        >
        bool operator != (const O &other) const
        {
            return (this->value != static_cast<value_t>(other));
        }
        
        /**
         *	Sets the given bits.
         */
        template <class O,
                  typename = std::enable_if_t<std::is_constructible<bit_field<T>, O>::value>
        >
        bit_field<T> & operator += (const O &other)
        {
            this->value |= static_cast<value_t>(other);
            return *this;
        }
        
        /**
         *	Returns bit field having the given bits set.
         */
        template <class O,
                  typename = std::enable_if_t<std::is_constructible<bit_field<T>, O>::value>
        >
        bit_field<T> operator + (const O &other) const
        {
            return bit_field<T>(this->value | static_cast<value_t>(other));
        }
        
        /**
         *	Clears the given bits.
         */
        template <class O,
                  typename = std::enable_if_t<std::is_constructible<bit_field<T>, O>::value>
        >
        bit_field<T> & operator -= (const O &other)
        {
            this->value &= (~(static_cast<value_t>(other)));
            return *this;
        }
        
        /**
         *	Returns bit field having the given bits clear.
         */
        template <class O,
                  typename = std::enable_if_t<std::is_constructible<bit_field<T>, O>::value>
        >
        bit_field<T> operator - (const O &other) const
        {
            return bit_field<T>(this->value & (~(static_cast<value_t>(other))));
        }
        
        /**
         *	Tests if all the given bits set.
         */
        template <class O,
                  typename = std::enable_if_t<std::is_constructible<bit_field<T>, O>::value>
        >
        bool operator & (const O &other) const
        {
            return ((this->value & (static_cast<value_t>(other))) == (static_cast<value_t>(other)));
        }
        
        /**
         *	Tests if any of the given bits set.
         */
        template <class O,
                  typename = std::enable_if_t<std::is_constructible<bit_field<T>, O>::value>
        >
        bool operator | (const O &other) const
        {
            return ((this->value & (static_cast<value_t>(other))) != 0);
        }
    
        explicit operator value_t () const
        {
            return this->value;
        }
    
    };

}
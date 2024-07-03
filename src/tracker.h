#pragma once

#include <iostream>

struct tracker
{
    tracker() :
        mValue(nullptr)
    {
        std::cout << "Default constructed\n";
    }
    tracker(const tracker& other) :
        mValue(other.mValue ? new int(*other.mValue) : nullptr)
    {
        std::cout << "Copy constructed: " << *this << '\n';
    }
    tracker(tracker&& other) noexcept :
        mValue(other.mValue)
    {
        other.mValue = nullptr;

        std::cout << "Move constructed: " << *this << '\n';
    }
    tracker(int value) :
        mValue(new int(value))
    {
        std::cout << "Value constructed: " << *this << '\n';
    }

    ~tracker()
    {
        std::cout << "Destructed: " << *this << '\n';
        
        delete mValue;
        mValue = nullptr;
    }

    tracker& operator=(const tracker& other)
    {
        if (this == &other)
        {
            std::cout << "Copy assigned: " << *this << '\n';
            return *this;
        }

        delete mValue;
        mValue = (other.mValue ? new int(*other.mValue) : nullptr);

        std::cout << "Copy assigned: " << *this << '\n';
        return *this;
    }
    tracker& operator=(tracker&& other) noexcept
    {
        if (this == &other)
        {
            std::cout << "Move assigned: " << *this << '\n';
            return *this;
        }

        delete mValue;
        mValue = other.mValue;
        other.mValue = nullptr;

        std::cout << "Move assigned: " << *this << '\n';
        return *this;
    }
    tracker& operator=(int value)
    {
        delete mValue;
        mValue = new int(value);

        std::cout << "Value assigned: " << *this << '\n';
        return *this;
    }

    auto operator<=>(const tracker& other) const
    {
        if (mValue)
        {
            if (other.mValue)
            {
                return *mValue <=> *other.mValue;
            }
            return std::strong_ordering::greater;
        }
        return std::strong_ordering::less;
    }
    bool operator==(const tracker&) const = default;

    friend std::istream& operator<<(std::istream& in, tracker& instance)
    {
        int value;
        in >> value;
        instance = value;
        return in;
    }
    friend std::ostream& operator<<(std::ostream& out, const tracker& instance)
    {
        if (instance.mValue)
        {
            out << *instance.mValue;
        }
        else
        {
            out << "nullptr";
        }
        return out;
    }
    
private:
    int* mValue;
};